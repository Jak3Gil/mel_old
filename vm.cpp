#include "vm.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <sstream>
#include <cstring>
#include <iostream>

namespace melvin {

// VMContext implementation
VMContext::VMContext(Store* store, uint64_t seed) 
    : store_(store), seed_(seed), rng_(seed), beam_size_(8), max_depth_(3),
      steps_executed_(0), memory_usage_(0), tau_exp_(1.0f), lambda_mix_(0.7f),
      tau_mid_(0.01f), tau_high_(0.05f), tau_rec_(3600.0f), gamma_c_(0.1f),
      gamma_v_(0.05f), anti_repeat_window_(10) {
}

void VMContext::push_node(const NodeID& node) {
    node_stack_.push(node);
    memory_usage_ += sizeof(NodeID);
}

NodeID VMContext::pop_node() {
    if (node_stack_.empty()) return NodeID{};
    NodeID node = node_stack_.top();
    node_stack_.pop();
    memory_usage_ -= sizeof(NodeID);
    return node;
}

void VMContext::push_edge(const EdgeID& edge) {
    edge_stack_.push(edge);
    memory_usage_ += sizeof(EdgeID);
}

EdgeID VMContext::pop_edge() {
    if (edge_stack_.empty()) return EdgeID{};
    EdgeID edge = edge_stack_.top();
    edge_stack_.pop();
    memory_usage_ -= sizeof(EdgeID);
    return edge;
}

void VMContext::push_float(float value) {
    float_stack_.push(value);
    memory_usage_ += sizeof(float);
}

float VMContext::pop_float() {
    if (float_stack_.empty()) return 0.0f;
    float value = float_stack_.top();
    float_stack_.pop();
    memory_usage_ -= sizeof(float);
    return value;
}

void VMContext::push_path(const Path& path) {
    path_stack_.push(path);
    memory_usage_ += sizeof(Path) + path.edges.size() * sizeof(EdgeID);
}

Path VMContext::pop_path() {
    if (path_stack_.empty()) return Path{};
    Path path = path_stack_.top();
    path_stack_.pop();
    memory_usage_ -= sizeof(Path) + path.edges.size() * sizeof(EdgeID);
    return path;
}

void VMContext::init_beam(size_t size, size_t max_depth) {
    beam_size_ = size;
    max_depth_ = max_depth;
    clear_beam();
}

void VMContext::add_to_beam(const Path& path) {
    beam_.push(path);
    if (beam_.size() > beam_size_) {
        beam_.pop();
    }
}

Path VMContext::get_best_path() {
    if (beam_.empty()) return Path{};
    return beam_.top();
}

std::vector<Path> VMContext::get_top_paths(size_t k) {
    std::vector<Path> result;
    auto temp_beam = beam_;
    
    while (!temp_beam.empty() && result.size() < k) {
        result.push_back(temp_beam.top());
        temp_beam.pop();
    }
    
    return result;
}

void VMContext::clear_beam() {
    while (!beam_.empty()) {
        beam_.pop();
    }
}

// Calculate size-relative weight scaling factor
float VMContext::get_size_scaling_factor() const {
    size_t node_count = store_->node_count();
    size_t edge_count = store_->edge_count();
    
    // Use geometric mean of node and edge counts for scaling
    float db_size = std::sqrt(static_cast<float>(node_count) * static_cast<float>(edge_count));
    
    // Scale factor: smaller databases get higher weights
    // For a database with 100 nodes/edges, scaling = 1.0
    // For a database with 10,000 nodes/edges, scaling = 0.1
    float scaling = 10.0f / std::max(1.0f, std::sqrt(db_size));
    
    return std::clamp(scaling, 0.01f, 10.0f); // Clamp between 0.01 and 10.0
}

float VMContext::score_edge(const EdgeRec& edge) const {
    // Context/core mix with proper weighting
    float m = lambda_mix_ * edge.w_ctx + (1.0f - lambda_mix_) * edge.w_core;
    
    // Apply size-relative scaling - facts are more important in smaller databases
    float size_scaling = get_size_scaling_factor();
    m *= size_scaling;
    
    // Recency factor (more important for CTX track)
    uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    float dt = static_cast<float>(now - edge.ts_last) / 1e9f; // Convert to seconds
    float rho = std::exp(-dt / tau_rec_);
    
    // Provenance trust (based on count and flags)
    float pi = std::min(1.0f, static_cast<float>(edge.count) / 10.0f);
    if (edge.flags & EDGE_DEPRECATED) {
        pi *= 0.1f;
    }
    if (edge.flags & EDGE_GENERALIZED) {
        pi *= 1.2f; // Boost generalized edges
    }
    
    // Curiosity-driven exploration bonus
    float curiosity_bonus = 1.0f + (drivers_.curiosity - 0.5f) * 0.2f;
    
    return m * rho * pi * curiosity_bonus;
}

float VMContext::score_path(const Path& path) const {
    if (path.edges.empty()) return 0.0f;
    
    // Get edge scores
    std::vector<float> edge_scores;
    for (const EdgeID& edge_id : path.edges) {
        EdgeRec edge;
        if (store_->get_edge(edge_id, edge)) {
            edge_scores.push_back(score_edge(edge));
        } else {
            edge_scores.push_back(0.0f);
        }
    }
    
    // Geometric mean of per-edge scores
    float product = 1.0f;
    for (float score : edge_scores) {
        product *= std::max(score, 0.001f); // Avoid zero
    }
    float G = std::pow(product, 1.0f / edge_scores.size());
    
    // Continuity bonus (γ_c = 1.1)
    float C = 0.0f;
    for (size_t i = 1; i < path.edges.size(); ++i) {
        EdgeRec prev_edge, curr_edge;
        if (store_->get_edge(path.edges[i-1], prev_edge) &&
            store_->get_edge(path.edges[i], curr_edge)) {
            if (prev_edge.rel == curr_edge.rel) {
                C += 1.0f;
            }
        }
    }
    float continuity_bonus = 1.0f + gamma_c_ * C;
    
    // Coverage bonus (γ_v = 1.1) - reward diverse subgraphs
    float coverage_bonus = 1.0f + gamma_v_ * std::log(1.0f + path.edges.size());
    
    // Exploration bonus - reward paths with inferred edges
    float exploration_bonus = 1.0f;
    int inferred_count = 0;
    for (const EdgeID& edge_id : path.edges) {
        EdgeRec edge;
        if (store_->get_edge(edge_id, edge)) {
            if (edge.flags & static_cast<uint32_t>(EdgeFlags::EDGE_INFERRED)) {
                inferred_count++;
            }
        }
    }
    if (inferred_count > 0) {
        exploration_bonus = 1.0f + (drivers_.curiosity - 0.5f) * 0.2f * inferred_count;
    }
    
    // Confidence-based scoring - higher confidence paths get bonus
    float confidence_bonus = 1.0f + path.confidence * 0.1f;
    
    // Apply all bonuses
    return G * continuity_bonus * coverage_bonus * exploration_bonus * confidence_bonus;
}

float VMContext::compute_confidence(float path_score) const {
    // Logistic function for confidence calibration
    float a = 5.0f; // Steepness
    float b = -3.0f; // Offset
    return 1.0f / (1.0f + std::exp(-(a * path_score + b)));
}

std::vector<Path> VMContext::beam_search(const NodeID& start, const RelMask& mask, 
                                        size_t depth, size_t beam_size) {
    std::vector<Path> result;
    
    // Initialize beam with start node
    std::vector<Path> beam;
    Path initial_path;
    initial_path.score = 1.0f;
    initial_path.confidence = 1.0f;
    initial_path.is_durable = false;
    beam.push_back(initial_path);
    
    std::cout << "🔍 Beam search starting from node, depth=" << depth << ", beam_size=" << beam_size << "\n";
    
    for (size_t d = 0; d < depth; ++d) {
        std::vector<Path> next_beam;
        
        for (const Path& path : beam) {
            // Determine current node
            NodeID current_node = start;
            if (!path.edges.empty()) {
                // Get the last edge's destination
                EdgeRec last_edge;
                if (store_->get_edge(path.edges.back(), last_edge)) {
                    std::copy(last_edge.dst, last_edge.dst + 32, current_node.begin());
                }
            }
            
            std::cout << "  🔍 Expanding from depth " << d << ", path has " << path.edges.size() << " edges\n";
            
            // Expand with outgoing edges
            AdjView view;
            if (store_->get_out_edges(current_node, mask, view) && view.edges != nullptr) {
                std::cout << "    📡 Found " << view.count << " outgoing edges\n";
                
                for (size_t i = 0; i < view.count; ++i) {
                    Path new_path = path;
                    NodeID dst;
                    std::copy(view.edges[i].dst, view.edges[i].dst + 32, dst.begin());
                    
                    // Create edge ID
                    EdgeID edge_id = {};  // Simplified for now
                    new_path.edges.push_back(edge_id);
                    
                    // Score the new path
                    new_path.score = score_path(new_path);
                    new_path.confidence = compute_confidence(new_path.score);
                    new_path.is_durable = new_path.score > tau_mid_;
                    
                    // Debug: show relation type
                    std::string rel_name = "UNKNOWN";
                    switch (static_cast<Rel>(view.edges[i].rel)) {
                        case Rel::ISA: rel_name = "ISA"; break;
                        case Rel::SIMILAR_TO: rel_name = "SIMILAR_TO"; break;
                        case Rel::HAS_PROPERTY: rel_name = "HAS_PROPERTY"; break;
                        case Rel::PART_OF: rel_name = "PART_OF"; break;
                        case Rel::LOCATED_IN: rel_name = "LOCATED_IN"; break;
                        case Rel::CAUSES: rel_name = "CAUSES"; break;
                        case Rel::EFFECT: rel_name = "EFFECT"; break;
                        default: rel_name = "OTHER"; break;
                    }
                    
                    std::cout << "      ➡️  New path: " << new_path.edges.size() << " edges, rel=" << rel_name 
                              << ", score=" << new_path.score << ", confidence=" << new_path.confidence << "\n";
                    
                    next_beam.push_back(new_path);
                }
            } else {
                std::cout << "    ❌ No outgoing edges found\n";
            }
        }
        
        // Sort by score and keep top beam_size
        std::sort(next_beam.begin(), next_beam.end(), 
                 [](const Path& a, const Path& b) { return a.score > b.score; });
        
        if (next_beam.size() > beam_size) {
            next_beam.resize(beam_size);
        }
        
        beam = next_beam;
        
        std::cout << "  📊 Depth " << d << " complete: " << beam.size() << " paths in beam\n";
        
        // Early exit if we have a durable path
        if (!beam.empty() && beam[0].is_durable && d >= 1) {
            std::cout << "  ✅ Found durable path, stopping early\n";
            break;
        }
    }
    
    std::cout << "🎯 Beam search complete: " << beam.size() << " final paths\n";
    return beam;
}

std::vector<Path> VMContext::explore_paths(const NodeID& start, const RelMask& mask, 
                                          size_t depth, size_t num_samples) {
    std::vector<Path> result;
    
    for (size_t sample = 0; sample < num_samples; ++sample) {
        Path path;
        NodeID current_node = start;
        
        for (size_t d = 0; d < depth; ++d) {
            AdjView view;
            if (!store_->get_out_edges(current_node, mask, view) || view.count == 0) {
                break;
            }
            
            // Stochastic sampling with temperature and exploration bonus
            std::vector<float> weights;
            for (size_t i = 0; i < view.count; ++i) {
                float score = score_edge(view.edges[i]);
                
                // Apply exploration bonus for weaker edges
                EdgeRec edge;
                EdgeID edge_id = {};  // Simplified for now
                if (store_->get_edge(edge_id, edge)) {
                    if (edge.flags & static_cast<uint32_t>(EdgeFlags::EDGE_INFERRED)) {
                        score *= (1.0f + drivers_.curiosity * 0.3f); // Boost inferred edges
                    }
                }
                
                weights.push_back(std::pow(score, 1.0f / tau_exp_));
            }
            
            // Select edge with exploration vs exploitation balance
            size_t selected = 0;
            if (drivers_.curiosity > 0.7f) {
                // High curiosity: more exploration, less exploitation
                std::uniform_real_distribution<float> dist(0.0f, 1.0f);
                float random_val = dist(rng_);
                
                if (random_val < 0.3f) {
                    // 30% chance to pick highest weight (exploitation)
                    float highest_weight = weights[0];
                    for (size_t i = 1; i < weights.size(); ++i) {
                        if (weights[i] > highest_weight) {
                            highest_weight = weights[i];
                            selected = i;
                        }
                    }
                } else {
                    // 70% chance to pick randomly (exploration)
                    std::uniform_int_distribution<size_t> idx_dist(0, weights.size() - 1);
                    selected = idx_dist(rng_);
                }
            } else {
                // Low curiosity: more exploitation, less exploration
                float highest_weight = weights[0];
                for (size_t i = 1; i < weights.size(); ++i) {
                    if (weights[i] > highest_weight) {
                        highest_weight = weights[i];
                        selected = i;
                    }
                }
            }
            
            NodeID dst;
            std::copy(view.edges[selected].dst, view.edges[selected].dst + 32, dst.begin());
            EdgeID edge_id = {};  // Simplified for now
            path.edges.push_back(edge_id);
            
            // Move to next node
            std::copy(view.edges[selected].dst, view.edges[selected].dst + 32, current_node.begin());
        }
        
        if (!path.edges.empty()) {
            path.score = score_path(path);
            path.confidence = compute_confidence(path.score);
            path.is_durable = true; // All paths are considered valid since we pick highest weight
            result.push_back(path);
        }
    }
    
    return result;
}

void VMContext::reinforce_edge(const EdgeID& edge, float alpha_ctx, float alpha_core) {
    EdgeRec edge_rec;
    if (store_->get_edge(edge, edge_rec)) {
        // Apply size-relative scaling to reinforcement increments
        float size_scaling = get_size_scaling_factor();
        float scaled_alpha_ctx = alpha_ctx * size_scaling;
        float scaled_alpha_core = alpha_core * size_scaling;
        
        edge_rec.w_ctx = std::clamp(edge_rec.w_ctx + scaled_alpha_ctx, 0.0f, 1.0f);
        edge_rec.w_core = std::clamp(edge_rec.w_core + scaled_alpha_core, 0.0f, 1.0f);
        edge_rec.count++;
        edge_rec.ts_last = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        // Update effective weight
        edge_rec.w = lambda_mix_ * edge_rec.w_ctx + (1.0f - lambda_mix_) * edge_rec.w_core;
        
        store_->upsert_edge(edge_rec);
    }
}

void VMContext::decay_edge(const EdgeID& edge, float beta_ctx, float beta_core) {
    EdgeRec edge_rec;
    if (store_->get_edge(edge, edge_rec)) {
        edge_rec.w_ctx = std::max(0.0f, edge_rec.w_ctx * (1.0f - beta_ctx));
        edge_rec.w_core = std::max(0.0f, edge_rec.w_core * (1.0f - beta_core));
        edge_rec.w = lambda_mix_ * edge_rec.w_ctx + (1.0f - lambda_mix_) * edge_rec.w_core;
        
        store_->upsert_edge(edge_rec);
    }
}

// Dynamic connection creation during reasoning
EdgeID VMContext::create_inferred_edge(const NodeID& src, const NodeID& dst, Rel relation, float confidence) {
    if (!enable_dynamic_inference_ || confidence < min_inference_confidence_) {
        return EdgeID{}; // Return empty edge ID if not creating
    }
    
    // Check if edge already exists
    EdgeID existing_edge = {};  // Simplified for now
    EdgeRec existing_rec;
    if (store_->get_edge(existing_edge, existing_rec)) {
        // Edge exists, reinforce it
        reinforce_edge(existing_edge, confidence * 0.1f, confidence * 0.05f);
        return existing_edge;
    }
    
    // Create new inferred edge
    EdgeRec edge;
    std::copy(src.begin(), src.end(), edge.src);
    edge.rel = static_cast<uint32_t>(relation);
    std::copy(dst.begin(), dst.end(), edge.dst);
    edge.layer = 0;
    
    // Apply size-relative scaling
    float size_scaling = get_size_scaling_factor();
    float base_weight = confidence * size_scaling;
    
    edge.w = std::clamp(base_weight, 0.0f, 1.0f);
    edge.w_core = std::clamp(base_weight * 0.3f, 0.0f, 1.0f); // Lower core weight for inferred edges
    edge.w_ctx = std::clamp(base_weight * 0.7f, 0.0f, 1.0f); // Higher context weight for inferred edges
    edge.count = 1;
    edge.ts_last = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    edge.flags = static_cast<uint32_t>(EdgeFlags::EDGE_INFERRED); // Mark as inferred
    edge.pad = 0;
    
    EdgeID edge_id = store_->upsert_edge(edge);
    
    std::cout << "🔗 Created inferred edge: " << static_cast<int>(relation) 
              << " (confidence=" << confidence << ")\n";
    
    return edge_id;
}

std::vector<EdgeID> VMContext::infer_missing_connections(const Path& path, float min_confidence) {
    std::vector<EdgeID> created_edges;
    
    if (path.edges.size() < 2) {
        return created_edges; // Need at least 2 edges to infer connections
    }
    
    // Get the first and last nodes in the path
    EdgeRec first_edge, last_edge;
    if (!store_->get_edge(path.edges[0], first_edge) || 
        !store_->get_edge(path.edges.back(), last_edge)) {
        return created_edges;
    }
    
    NodeID start_node, end_node;
    std::copy(first_edge.src, first_edge.src + 32, start_node.begin());
    std::copy(last_edge.dst, last_edge.dst + 32, end_node.begin());
    
    // Calculate confidence based on path score and length
    float path_confidence = path.confidence;
    float length_penalty = 1.0f / (1.0f + path.edges.size() * 0.1f); // Penalty for longer paths
    float inference_confidence = path_confidence * length_penalty;
    
    if (inference_confidence >= min_confidence) {
        // Try to infer a direct connection
        Rel inferred_relation = static_cast<Rel>(first_edge.rel); // Use first edge's relation type
        
        EdgeID inferred_edge = create_inferred_edge(start_node, end_node, inferred_relation, inference_confidence);
        if (!is_zero_id(inferred_edge)) {
            created_edges.push_back(inferred_edge);
        }
        
        // Also try to infer reverse relationship if it makes sense
        if (inferred_relation == Rel::ISA || inferred_relation == Rel::PART_OF) {
            Rel reverse_relation = (inferred_relation == Rel::ISA) ? Rel::HAS_PROPERTY : Rel::PART_OF;
            EdgeID reverse_edge = create_inferred_edge(end_node, start_node, reverse_relation, inference_confidence * 0.8f);
            if (!is_zero_id(reverse_edge)) {
                created_edges.push_back(reverse_edge);
            }
        }
    }
    
    return created_edges;
}

bool VMContext::should_create_connection(const NodeID& src, const NodeID& dst, Rel relation, float confidence) const {
    if (!enable_dynamic_inference_) {
        return false;
    }
    
    if (confidence < min_inference_confidence_) {
        return false;
    }
    
    // Check if connection already exists
    EdgeID existing_edge = {};  // Simplified for now
    EdgeRec existing_rec;
    if (store_->get_edge(existing_edge, existing_rec)) {
        return false; // Already exists
    }
    
    // Apply exploration bonus based on drivers
    float exploration_factor = 1.0f + (drivers_.curiosity - 0.5f) * exploration_bonus_;
    float adjusted_confidence = confidence * exploration_factor;
    
    return adjusted_confidence >= min_inference_confidence_;
}

bool VMContext::generate_text_output(const NodeID& node, ByteBuf& out) {
    // Simple text generation based on node type and edges
    NodeRecHeader header;
    std::vector<uint8_t> payload;
    if (!store_->get_node(node, header, payload)) {
        return false;
    }
    
    std::string text;
    NodeType type = static_cast<NodeType>(header.type);
    
    if (type == NodeType::SYMBOL) {
        text = std::string(reinterpret_cast<const char*>(payload.data()), payload.size());
    } else if (type == NodeType::CONCEPT) {
        // Generate concept description
        AdjView view;
        RelMask mask;
        mask.set(Rel::ISA);
        mask.set(Rel::HAS_PROPERTY);
        
        if (store_->get_out_edges(node, mask, view)) {
            text = "Concept with " + std::to_string(view.count) + " properties";
        } else {
            text = "Unknown concept";
        }
    } else {
        text = "Complex node";
    }
    
    // Check for repetition
    if (check_repetition(text)) {
        text = "[Variation] " + text;
    }
    
    out.append(text);
    add_to_recent(text);
    return true;
}

bool VMContext::generate_audio_output(const NodeID& node, ByteBuf& out) {
    // Placeholder for audio generation
    out.append("AUDIO_PLACEHOLDER");
    return true;
}

bool VMContext::generate_control_output(const NodeID& node, ByteBuf& out) {
    // Placeholder for control generation
    out.append("CONTROL_PLACEHOLDER");
    return true;
}

bool VMContext::check_repetition(const std::string& text) const {
    // Simple bigram check
    if (text.length() < 4) return false;
    
    for (size_t i = 0; i < text.length() - 1; ++i) {
        std::string bigram = text.substr(i, 2);
        if (recent_bigrams_.find(bigram) != recent_bigrams_.end()) {
            return true;
        }
    }
    
    return false;
}

void VMContext::add_to_recent(const std::string& text) {
    // Add bigrams to recent set
    for (size_t i = 0; i < text.length() - 1; ++i) {
        std::string bigram = text.substr(i, 2);
        recent_bigrams_.insert(bigram);
    }
    
    // Maintain window size
    if (recent_bigrams_.size() > anti_repeat_window_ * 10) {
        recent_bigrams_.clear(); // Simple cleanup
    }
}

// MelvinVM implementation
MelvinVM::MelvinVM(Store* store, uint64_t seed) 
    : context_(std::make_unique<VMContext>(store, seed)) {
}

void MelvinVM::exec(const std::vector<uint8_t>& bytecode) {
    exec(bytecode.data(), bytecode.size());
}

void MelvinVM::exec(const uint8_t* bytecode, size_t len) {
    BytecodeDecoder decoder(bytecode, len);
    
    while (decoder.has_more()) {
        uint8_t opcode = decoder.read_u8();
        context_->increment_steps(); // Track steps
        
        switch (static_cast<Opcode>(opcode)) {
            case Opcode::LINK: {
                NodeID src = decoder.read_node_id();
                RelMask mask = decoder.read_rel_mask();
                NodeID dst = decoder.read_node_id();
                float delta_w = decoder.read_f32();
                
                // Create edge
                EdgeRec edge;
                std::copy(src.begin(), src.end(), edge.src);
                edge.rel = static_cast<uint32_t>(Rel::NEXT); // Default relation
                std::copy(dst.begin(), dst.end(), edge.dst);
                
                // Apply size-relative scaling to initial weights
                float size_scaling = context_->get_size_scaling_factor();
                float scaled_w = delta_w * size_scaling;
                
                edge.w = std::clamp(scaled_w, 0.0f, 1.0f);
                edge.w_core = std::clamp(scaled_w * 0.5f, 0.0f, 1.0f);
                edge.w_ctx = std::clamp(scaled_w * 0.5f, 0.0f, 1.0f);
                edge.count = 1;
                edge.ts_last = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::high_resolution_clock::now().time_since_epoch()).count();
                
                EdgeID edge_id = context_->get_store()->upsert_edge(edge);
                context_->push_edge(edge_id);
                break;
            }
            
            case Opcode::WALK: {
                NodeID start = decoder.read_node_id();
                RelMask mask = decoder.read_rel_mask();
                uint32_t depth = decoder.read_u32();
                uint32_t beam_size = decoder.read_u32();
                
                auto paths = context_->beam_search(start, mask, depth, beam_size);
                for (const Path& path : paths) {
                    context_->push_path(path);
                }
                break;
            }
            
            case Opcode::COMPOSE: {
                uint32_t num_edges = decoder.read_u32();
                std::vector<EdgeID> edges;
                for (uint32_t i = 0; i < num_edges; ++i) {
                    edges.push_back(decoder.read_edge_id());
                }
                
                float score;
                PathID path_id = context_->get_store()->compose_path(edges, score);
                
                Path path;
                path.edges = edges;
                path.score = score;
                path.confidence = context_->compute_confidence(score);
                path.is_durable = true; // All paths valid since we pick highest weight
                
                context_->push_path(path);
                break;
            }
            
            case Opcode::REINFORCE: {
                EdgeID edge = decoder.read_edge_id();
                float alpha = decoder.read_f32();
                
                context_->reinforce_edge(edge, alpha, alpha * 0.5f);
                break;
            }
            
            case Opcode::DECAY: {
                EdgeID edge = decoder.read_edge_id();
                float beta = decoder.read_f32();
                
                context_->decay_edge(edge, beta, beta * 0.1f);
                break;
            }
            
            case Opcode::EMIT: {
                NodeID node = decoder.read_node_id();
                ViewKind kind = static_cast<ViewKind>(decoder.read_u8());
                
                ByteBuf out;
                bool success = false;
                
                switch (kind) {
                    case ViewKind::TEXT_VIEW:
                        success = context_->generate_text_output(node, out);
                        break;
                    case ViewKind::AUDIO_VIEW:
                        success = context_->generate_audio_output(node, out);
                        break;
                    case ViewKind::CONTROL_VIEW:
                        success = context_->generate_control_output(node, out);
                        break;
                    default:
                        break;
                }
                
                if (success) {
                    // Store output in a node
                    NodeRecHeader header;
                    header.type = static_cast<uint32_t>(NodeType::META_COG_STEP);
                    header.flags = 0;
                    header.payload_len = static_cast<uint32_t>(out.size());
                    
                    std::vector<uint8_t> payload(out.data(), out.data() + out.size());
                    NodeID output_id = context_->get_store()->upsert_node(header, payload);
                    context_->push_node(output_id);
                }
                break;
            }
            
            case Opcode::HALT:
                return;
                
            default:
                // Unknown opcode - skip
                break;
        }
    }
}

void MelvinVM::set_drivers(const Drivers& drivers) {
    context_->set_drivers(drivers);
}

Drivers MelvinVM::get_drivers() const {
    return context_->get_drivers();
}

void MelvinVM::set_seed(uint64_t seed) {
    context_->set_seed(seed);
}

bool MelvinVM::emit_view(const NodeID& node, ViewKind kind, ByteBuf& out) {
    switch (kind) {
        case ViewKind::TEXT_VIEW:
            return context_->generate_text_output(node, out);
        case ViewKind::AUDIO_VIEW:
            return context_->generate_audio_output(node, out);
        case ViewKind::CONTROL_VIEW:
            return context_->generate_control_output(node, out);
        default:
            return false;
    }
}

size_t MelvinVM::steps_executed() const {
    return context_->get_steps_executed();
}

size_t MelvinVM::memory_usage() const {
    return context_->get_memory_usage();
}

// BytecodeDecoder implementation
uint8_t BytecodeDecoder::read_u8() {
    if (pos_ >= len_) return 0;
    return code_[pos_++];
}

uint16_t BytecodeDecoder::read_u16() {
    if (pos_ + 1 >= len_) return 0;
    uint16_t value = (static_cast<uint16_t>(code_[pos_]) << 8) | code_[pos_ + 1];
    pos_ += 2;
    return value;
}

uint32_t BytecodeDecoder::read_u32() {
    if (pos_ + 3 >= len_) return 0;
    uint32_t value = (static_cast<uint32_t>(code_[pos_]) << 24) |
                    (static_cast<uint32_t>(code_[pos_ + 1]) << 16) |
                    (static_cast<uint32_t>(code_[pos_ + 2]) << 8) |
                    code_[pos_ + 3];
    pos_ += 4;
    return value;
}

uint64_t BytecodeDecoder::read_u64() {
    if (pos_ + 7 >= len_) return 0;
    uint64_t value = (static_cast<uint64_t>(code_[pos_]) << 56) |
                    (static_cast<uint64_t>(code_[pos_ + 1]) << 48) |
                    (static_cast<uint64_t>(code_[pos_ + 2]) << 40) |
                    (static_cast<uint64_t>(code_[pos_ + 3]) << 32) |
                    (static_cast<uint64_t>(code_[pos_ + 4]) << 24) |
                    (static_cast<uint64_t>(code_[pos_ + 5]) << 16) |
                    (static_cast<uint64_t>(code_[pos_ + 6]) << 8) |
                    code_[pos_ + 7];
    pos_ += 8;
    return value;
}

float BytecodeDecoder::read_f32() {
    uint32_t bits = read_u32();
    return *reinterpret_cast<float*>(&bits);
}

NodeID BytecodeDecoder::read_node_id() {
    NodeID id;
    for (size_t i = 0; i < 32 && pos_ < len_; ++i) {
        id[i] = code_[pos_++];
    }
    return id;
}

EdgeID BytecodeDecoder::read_edge_id() {
    EdgeID id;
    for (size_t i = 0; i < 32 && pos_ < len_; ++i) {
        id[i] = code_[pos_++];
    }
    return id;
}

RelMask BytecodeDecoder::read_rel_mask() {
    RelMask mask;
    mask.mask_low = read_u64();
    mask.mask_high = read_u64();
    return mask;
}

// CogStepRecorder implementation
void CogStepRecorder::record_step(CogOp op, float conf, const Drivers& drivers,
                                 const std::vector<NodeID>& inputs,
                                 const std::vector<NodeID>& outputs,
                                 uint64_t latency_us, ErrorFlags err_flags) {
    CogStep step;
    step.op = static_cast<uint8_t>(op);
    step.conf = conf;
    step.drivers[0] = drivers.curiosity;
    step.drivers[1] = drivers.efficiency;
    step.drivers[2] = drivers.consistency;
    step.drivers[3] = drivers.social;
    step.drivers[4] = drivers.survival;
    step.num_inputs = static_cast<uint32_t>(inputs.size());
    step.num_outputs = static_cast<uint32_t>(outputs.size());
    step.latency_us = latency_us;
    step.err_flags = static_cast<uint32_t>(err_flags);
    
    steps_.push_back(step);
}

} // namespace melvin
