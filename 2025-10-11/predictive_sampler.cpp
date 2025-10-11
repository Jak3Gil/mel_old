#include "predictive_sampler.h"
#include "melvin_leap_nodes.h"
#include "src/embeddings/embedding_bridge.h"
#include "melvin_types.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <iomanip>
#include <fstream>
#include <sstream>

namespace melvin {

// Global random number generator
static std::mt19937 rng(std::random_device{}());

// Compute context embedding as mean of recent node embeddings
std::vector<float> compute_context_embedding(const std::vector<uint64_t>& recent_nodes) {
    if (recent_nodes.empty()) {
        return std::vector<float>(128, 0.0f); // Default embedding size
    }
    
    std::vector<float> context_emb(128, 0.0f);
    int valid_nodes = 0;
    
    for (uint64_t node_id : recent_nodes) {
        // Get node embedding from G_nodes
        if (G_nodes.find(node_id) != G_nodes.end()) {
            const auto& node = G_nodes[node_id];
            if (!node.emb.empty()) {
                for (size_t i = 0; i < std::min(context_emb.size(), node.emb.size()); ++i) {
                    context_emb[i] += node.emb[i];
                }
                valid_nodes++;
            }
        }
    }
    
    // Normalize by number of valid nodes
    if (valid_nodes > 0) {
        for (float& val : context_emb) {
            val /= valid_nodes;
        }
    }
    
    return context_emb;
}

// Cosine similarity between embeddings
float similarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.empty() || b.empty()) return 0.0f;
    
    float dot_product = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;
    
    size_t min_size = std::min(a.size(), b.size());
    for (size_t i = 0; i < min_size; ++i) {
        dot_product += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    if (norm_a == 0.0f || norm_b == 0.0f) return 0.0f;
    
    return dot_product / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

// Score all neighbors using LLM-style attention
std::vector<Candidate> score_neighbors(
    uint64_t current_node,
    const std::vector<uint64_t>& context,
    const PredictiveConfig& cfg) {
    
    std::vector<Candidate> candidates;
    
    if (G_adj.find(current_node) == G_adj.end()) {
        return candidates; // No outgoing edges
    }
    
    // Compute context embedding
    std::vector<float> ctx_emb = compute_context_embedding(context);
    
    // Score each outgoing edge
    for (size_t edge_idx : G_adj[current_node]) {
        if (edge_idx >= G_edges.size()) continue;
        
        const auto& edge = G_edges[edge_idx];
        uint64_t neighbor_id = edge.locB;
        
        // Get neighbor embedding
        std::vector<float> neighbor_emb;
        if (G_nodes.find(neighbor_id) != G_nodes.end()) {
            neighbor_emb = G_nodes[neighbor_id].emb;
        }
        
        // Core scoring formula: β(α·freq + (1-α)·sim) + γ·rel_bias
        float freq_score = cfg.alpha * (edge.freq_ratio + cfg.epsilon);
        float sim_score = (1.0f - cfg.alpha) * similarity(ctx_emb, neighbor_emb);
        float rel_score = cfg.gamma * cfg.rel_bias[static_cast<int>(edge.rel)];
        
        float total_score = cfg.beta * (freq_score + sim_score) + rel_score;
        
        candidates.emplace_back(neighbor_id, total_score, 
                               static_cast<Rel>(edge.rel), edge.freq_ratio);
    }
    
    return candidates;
}

// Apply n-gram continuation bonus using Thought nodes
void apply_ngram_bonus(
    const std::vector<uint64_t>& context,
    std::vector<Candidate>& candidates,
    const PredictiveConfig& cfg) {
    
    if (context.size() < 2) return; // Need at least 2 nodes for n-gram
    
    // Try different n-gram sizes (K, K-1, ..., 2)
    for (int n = std::min(static_cast<int>(context.size()), cfg.ctx_K); n >= 2; --n) {
        // Create n-gram from recent context
        std::vector<uint64_t> ngram(context.end() - n, context.end());
        
        // Look for Thought nodes that continue this n-gram
        for (auto& candidate : candidates) {
            // Check if this candidate continues the n-gram pattern
            // This is a simplified check - in practice, you'd look for actual Thought nodes
            // that represent this n-gram continuation
            
            // For now, give a small bonus to candidates that appear frequently
            // in similar contexts (this would be enhanced with actual Thought node lookup)
            if (candidate.freq_ratio > 0.1f) { // Threshold for "frequent" continuation
                candidate.score += cfg.ngram_bonus;
            }
        }
        
        // If we found some high-scoring continuations, stop here
        bool found_continuation = false;
        for (const auto& candidate : candidates) {
            if (candidate.score > 1.0f) { // Threshold for "good" continuation
                found_continuation = true;
                break;
            }
        }
        
        if (found_continuation) break;
    }
}

// Convert scores to probabilities using softmax
void softmax_inplace(std::vector<Candidate>& candidates) {
    if (candidates.empty()) return;
    
    // Find maximum score for numerical stability
    float max_score = candidates[0].score;
    for (const auto& candidate : candidates) {
        max_score = std::max(max_score, candidate.score);
    }
    
    // Compute exponentials and sum
    float sum_exp = 0.0f;
    for (auto& candidate : candidates) {
        candidate.score = std::exp(candidate.score - max_score);
        sum_exp += candidate.score;
    }
    
    // Normalize to probabilities
    if (sum_exp > 0.0f) {
        for (auto& candidate : candidates) {
            candidate.score /= sum_exp;
        }
    }
}

// Beam search generation with thinking log and adaptive tuning
std::vector<uint64_t> beam_search(
    const std::vector<uint64_t>& seed_context,
    PredictiveConfig& cfg,
    LeapController* leap_controller,
    embeddings::EmbeddingBridge* embedding_bridge) {  // Note: cfg is now non-const for adaptive tuning
    
    struct BeamItem {
        std::vector<uint64_t> path;
        float score;
        
        BeamItem(const std::vector<uint64_t>& p, float s) : path(p), score(s) {}
    };
    
    std::vector<BeamItem> beam;
    beam.emplace_back(seed_context, 0.0f);
    
    // Track active edges for reinforcement
    std::vector<uint64_t> active_edges;
    
    // Leap system tracking
    bool leap_triggered = false;
    float initial_entropy = 0.0f;
    
    for (int hop = 0; hop < cfg.max_hops && !beam.empty(); ++hop) {
        std::vector<BeamItem> next_beam;
        
        for (const auto& item : beam) {
            if (item.path.empty()) continue;
            
            uint64_t current = item.path.back();
            std::vector<Candidate> candidates = score_neighbors(current, item.path, cfg);
            
            if (candidates.empty()) continue;
            
            apply_ngram_bonus(item.path, candidates, cfg);
            
        // 🧠 LEAP SYSTEM INTEGRATION - Apply before softmax
        if (leap_controller && cfg.enable_leap_system) {
            initial_entropy = compute_entropy(candidates);
            LeapIntegrator::apply_leap_to_generation(
                candidates, item.path, *leap_controller, leap_triggered);
        }
        
        // 🌉 EMBEDDING BRIDGE INTEGRATION - Apply semantic bias
        if (embedding_bridge && cfg.enable_embedding_bridge) {
            // Compute activation vector from context nodes
            std::vector<float> activations(item.path.size(), 1.0f / item.path.size());
            
            auto activation_vec = embedding_bridge->compute_activation_vector(
                item.path, activations);
            
            // Get candidate tokens
            std::vector<std::string> candidate_tokens;
            for (const auto& cand : candidates) {
                // Get node text for this candidate
                if (G_nodes.count(cand.node_id) > 0) {
                    candidate_tokens.push_back(
                        embeddings::node_to_token(G_nodes[cand.node_id].text));
                }
            }
            
            // Compute token bias from activation
            auto bias_field = embedding_bridge->compute_token_bias(
                activation_vec, candidate_tokens);
            
            // Apply bias to candidates
            for (size_t i = 0; i < candidates.size() && i < candidate_tokens.size(); ++i) {
                float bias = bias_field.get_bias(candidate_tokens[i]);
                candidates[i].score += bias;
            }
        }
        
        softmax_inplace(candidates);
            
            // 🧠 Thinking log: dump top candidates and probabilities
            log_sampler_step(current, candidates, hop);
            
            // Add top candidates to next beam
            std::sort(candidates.begin(), candidates.end(), 
                     [](const Candidate& a, const Candidate& b) {
                         return a.score > b.score;
                     });
            
            for (int i = 0; i < std::min(cfg.beam_width, static_cast<int>(candidates.size())); ++i) {
                std::vector<uint64_t> new_path = item.path;
                new_path.push_back(candidates[i].node_id);
                
                float new_score = item.score + std::log(candidates[i].score + cfg.epsilon);
                next_beam.emplace_back(new_path, new_score);
                
                // Track active edges for reinforcement
                // Find edge index for this transition
                if (G_adj.find(current) != G_adj.end()) {
                    for (size_t edge_idx : G_adj[current]) {
                        if (edge_idx < G_edges.size() && G_edges[edge_idx].locB == candidates[i].node_id) {
                            active_edges.push_back(edge_idx);
                            break;
                        }
                    }
                }
            }
            
            // 🧩 Adaptive tuning: adjust config based on entropy
            adaptive_tune_config(cfg, candidates, true); // Assume success for now
        }
        
        beam = next_beam;
        
        // Check for termination conditions
        bool should_terminate = false;
        for (const auto& item : beam) {
            if (item.path.size() >= cfg.max_hops) {
                should_terminate = true;
                break;
            }
            
            // Check for terminal relations (ISA, CONSUMES completion)
            if (!item.path.empty()) {
                uint64_t last_node = item.path.back();
                // Simple termination: if we've reached a "complete" concept
                if (G_nodes.find(last_node) != G_nodes.end()) {
                    const auto& node = G_nodes[last_node];
                    if (node.text.find('.') != std::string::npos || 
                        node.text.find('!') != std::string::npos ||
                        node.text.find('?') != std::string::npos) {
                        should_terminate = true;
                        break;
                    }
                }
            }
        }
        
        if (should_terminate) break;
    }
    
    // 🧩 Self-reinforcement: strengthen co-activated edges
    if (!active_edges.empty()) {
        reinforce_active_edges(active_edges, 0.002f); // Small learning rate
    }
    
    // Return best path
    if (!beam.empty()) {
        auto best = std::max_element(beam.begin(), beam.end(),
                                   [](const BeamItem& a, const BeamItem& b) {
                                       return a.score < b.score;
                                   });
        
        // 💭 Store successful path as Thought node
        if (best->path.size() > 1 && best->score > 0.5f) { // Only save good paths
            save_thought_node(best->path);
        }
        
        // 📊 Optional: dump subgraph visualization
        static int viz_counter = 0;
        if (++viz_counter % 5 == 0) { // Every 5th generation
            std::string filename = "melvin_step_" + std::to_string(viz_counter) + ".dot";
            dump_subgraph_to_dot(best->path, filename);
        }
        
        // 🧠 LEAP SYSTEM FEEDBACK - Record results if leap was used
        if (leap_controller && leap_triggered && cfg.enable_leap_system) {
            LeapIntegrator::record_generation_feedback(*leap_controller, best->path, initial_entropy);
        }
        
        return best->path;
    }
    
    return seed_context; // Fallback
}

// Top-p sampling generation
std::vector<uint64_t> top_p_sample(
    const std::vector<uint64_t>& seed_context,
    const PredictiveConfig& cfg,
    LeapController* leap_controller,
    embeddings::EmbeddingBridge* embedding_bridge) {
    
    std::vector<uint64_t> path = seed_context;
    bool leap_triggered = false;
    float initial_entropy = 0.0f;
    
    for (int hop = 0; hop < cfg.max_hops; ++hop) {
        if (path.empty()) break;
        
        uint64_t current = path.back();
        std::vector<Candidate> candidates = score_neighbors(current, path, cfg);
        
        if (candidates.empty()) break;
        
        apply_ngram_bonus(path, candidates, cfg);
        
        // 🧠 LEAP SYSTEM INTEGRATION - Apply before softmax  
        if (leap_controller && cfg.enable_leap_system) {
            initial_entropy = compute_entropy(candidates);
            LeapIntegrator::apply_leap_to_generation(
                candidates, path, *leap_controller, leap_triggered);
        }
        
        // 🌉 EMBEDDING BRIDGE INTEGRATION - Apply semantic bias
        if (embedding_bridge && cfg.enable_embedding_bridge) {
            // Compute activation vector from context
            std::vector<float> activations(path.size(), 1.0f / path.size());
            
            auto activation_vec = embedding_bridge->compute_activation_vector(
                path, activations);
            
            // Get candidate tokens
            std::vector<std::string> candidate_tokens;
            for (const auto& cand : candidates) {
                if (G_nodes.count(cand.node_id) > 0) {
                    candidate_tokens.push_back(
                        embeddings::node_to_token(G_nodes[cand.node_id].text));
                }
            }
            
            // Compute and apply token bias
            auto bias_field = embedding_bridge->compute_token_bias(
                activation_vec, candidate_tokens);
            
            for (size_t i = 0; i < candidates.size() && i < candidate_tokens.size(); ++i) {
                float bias = bias_field.get_bias(candidate_tokens[i]);
                candidates[i].score += bias;
            }
        }
        
        softmax_inplace(candidates);
        
        // Sort by probability (descending)
        std::sort(candidates.begin(), candidates.end(),
                 [](const Candidate& a, const Candidate& b) {
                     return a.score > b.score;
                 });
        
        // Top-p sampling
        float cumulative_prob = 0.0f;
        std::vector<Candidate> top_p_candidates;
        
        for (const auto& candidate : candidates) {
            cumulative_prob += candidate.score;
            top_p_candidates.push_back(candidate);
            
            if (cumulative_prob >= cfg.top_p) break;
        }
        
        if (top_p_candidates.empty()) break;
        
        // Sample from top-p candidates
        std::uniform_real_distribution<float> dist(0.0f, cumulative_prob);
        float random_val = dist(rng);
        
        float running_sum = 0.0f;
        for (const auto& candidate : top_p_candidates) {
            running_sum += candidate.score;
            if (random_val <= running_sum) {
                path.push_back(candidate.node_id);
                break;
            }
        }
        
        // Check for termination
        if (!path.empty()) {
            uint64_t last_node = path.back();
            if (G_nodes.find(last_node) != G_nodes.end()) {
                const auto& node = G_nodes[last_node];
                if (node.text.find('.') != std::string::npos || 
                    node.text.find('!') != std::string::npos ||
                    node.text.find('?') != std::string::npos) {
                    break;
                }
            }
        }
    }
    
    // 🧠 LEAP SYSTEM FEEDBACK - Record results if leap was used
    if (leap_controller && leap_triggered && cfg.enable_leap_system) {
        LeapIntegrator::record_generation_feedback(*leap_controller, path, initial_entropy);
    }
    
    return path;
}

// Main generation function
std::vector<uint64_t> generate_path(
    const std::vector<uint64_t>& seed_context,
    PredictiveConfig& cfg,
    LeapController* leap_controller,
    embeddings::EmbeddingBridge* embedding_bridge) {  // Note: cfg is now non-const for adaptive tuning
    
    if (cfg.use_beam) {
        return beam_search(seed_context, cfg, leap_controller, embedding_bridge);
    } else {
        return top_p_sample(seed_context, cfg, leap_controller, embedding_bridge);
    }
}

// Online learning: update frequencies after generation
void update_frequencies_from_path(const std::vector<uint64_t>& path) {
    for (size_t i = 0; i < path.size() - 1; ++i) {
        uint64_t src = path[i];
        uint64_t dst = path[i + 1];
        
        // Find and update the edge
        for (size_t edge_idx : G_adj[src]) {
            if (edge_idx < G_edges.size() && G_edges[edge_idx].locB == dst) {
                G_edges[edge_idx].update_frequency(G_total_nodes);
                break;
            }
        }
    }
}

// Debug: print probability distribution at each hop
void debug_generation_step(
    uint64_t current_node,
    const std::vector<Candidate>& candidates,
    const std::string& context_text) {
    
    std::cout << "🔍 Debug Step - Node: " << current_node;
    if (!context_text.empty()) {
        std::cout << " (" << context_text << ")";
    }
    std::cout << "\n";
    
    std::cout << "📊 Top candidates:\n";
    for (size_t i = 0; i < std::min(size_t(5), candidates.size()); ++i) {
        const auto& candidate = candidates[i];
        std::string node_text = "";
        if (G_nodes.find(candidate.node_id) != G_nodes.end()) {
            node_text = G_nodes[candidate.node_id].text;
        }
        
        std::cout << "  " << (i+1) << ". " << node_text 
                  << " (score: " << candidate.score 
                  << ", freq: " << candidate.freq_ratio
                  << ", rel: " << static_cast<int>(candidate.rel) << ")\n";
    }
    std::cout << "\n";
}

// Thinking log: dump sampler's top-k neighbors and probabilities
void log_sampler_step(
    uint64_t from_node,
    const std::vector<Candidate>& candidates,
    int step) {
    
    std::cout << "🧠 Step " << step << " from " << node_text(from_node) << "\n";
    
    for (size_t i = 0; i < std::min(size_t(5), candidates.size()); ++i) {
        const auto& c = candidates[i];
        std::cout << "   → " << node_text(c.node_id)
                  << "  P=" << std::fixed << std::setprecision(3) << c.score
                  << "  rel=" << rel_to_string(c.rel) 
                  << "  freq=" << std::fixed << std::setprecision(3) << c.freq_ratio << "\n";
    }
    std::cout << "\n";
}

// Compute entropy of candidate distribution
float compute_entropy(const std::vector<Candidate>& candidates) {
    if (candidates.empty()) return 0.0f;
    
    float entropy = 0.0f;
    for (const auto& c : candidates) {
        if (c.score > 0.0f) {
            entropy -= c.score * std::log2(c.score);
        }
    }
    return entropy;
}

// Adaptive tuning: adjust config based on entropy and success
void adaptive_tune_config(PredictiveConfig& cfg, 
                         const std::vector<Candidate>& candidates,
                         bool success) {
    
    float entropy = compute_entropy(candidates);
    
    // Entropy-based beta adjustment (exploration vs exploitation)
    if (entropy < 0.5f) {
        cfg.beta -= 0.2f;  // too certain → explore more
    } else if (entropy > 1.5f) {
        cfg.beta += 0.2f;  // too random → sharpen
    }
    
    // Clamp beta to reasonable range
    cfg.beta = std::max(1.0f, std::min(15.0f, cfg.beta));
    
    // Alpha adjustment based on success (freq vs similarity balance)
    float alpha_delta = 0.01f * (success ? 1.0f : -1.0f);
    cfg.alpha = std::max(0.5f, std::min(0.9f, cfg.alpha + alpha_delta));
    
    // Optional: adjust relation biases based on success
    if (success) {
        // Boost successful relation types
        for (const auto& c : candidates) {
            if (c.score > 0.3f) { // High probability candidates
                cfg.rel_bias[static_cast<int>(c.rel)] += 0.01f;
                cfg.rel_bias[static_cast<int>(c.rel)] = std::min(1.0f, cfg.rel_bias[static_cast<int>(c.rel)]);
            }
        }
    }
}

// Self-reinforcement: strengthen co-activated edges
void reinforce_active_edges(const std::vector<uint64_t>& active_edges,
                           float learning_rate) {
    
    for (uint64_t edge_idx : active_edges) {
        if (edge_idx < G_edges.size()) {
            auto& edge = G_edges[edge_idx];
            // Update frequency ratio with small learning rate
            edge.freq_ratio = (1.0f - learning_rate) * edge.freq_ratio + 
                             learning_rate * (1.0f / G_total_nodes);
        }
    }
}

// Store successful paths as Thought nodes
void save_thought_node(const std::vector<uint64_t>& path) {
    if (path.size() < 2) return;
    
    // Create a Thought node representing this successful path
    // Format: "1110 [node1→node2→node3] 1110"
    std::string thought_text = "1110 [";
    for (size_t i = 0; i < path.size(); ++i) {
        if (i > 0) thought_text += "→";
        thought_text += node_text(path[i]);
    }
    thought_text += "] 1110";
    
    // Add as a new node (simplified - in practice you'd use proper node creation)
    uint64_t thought_id = G_total_nodes++;
    G_nodes[thought_id] = melvin::Node();
    G_nodes[thought_id].id = thought_id;
    G_nodes[thought_id].text = thought_text;
    G_nodes[thought_id].type = 1110; // Thought node type
    G_nodes[thought_id].freq = 1;
    
    // Create edges from path nodes to this Thought node
    for (uint64_t path_node : path) {
        // Add bidirectional connections to the Thought node
        size_t edge_idx = G_edges.size();
        G_edges.emplace_back();
        auto& edge = G_edges.back();
        edge.locA = path_node;
        edge.locB = thought_id;
        edge.rel = static_cast<uint8_t>(Rel::LEAP); // Use LEAP relation for Thought connections
        edge.freq_ratio = 0.1f; // Initial frequency
        edge.count = 1;
        
        // Add to adjacency list
        G_adj[path_node].push_back(edge_idx);
    }
    
    std::cout << "💭 Saved thought: " << thought_text << "\n";
}

// Convert relation enum to string for logging
std::string rel_to_string(Rel rel) {
    switch (rel) {
        case Rel::EXACT: return "EXACT";
        case Rel::TEMPORAL: return "TEMPORAL";
        case Rel::LEAP: return "LEAP";
        case Rel::GENERALIZATION: return "GEN";
        case Rel::ISA: return "ISA";
        case Rel::CONSUMES: return "CONSUMES";
        case Rel::HAS: return "HAS";
        case Rel::CAN: return "CAN";
        default: return "UNKNOWN";
    }
}

// Get node text for logging
std::string node_text(uint64_t node_id) {
    if (G_nodes.find(node_id) != G_nodes.end()) {
        return G_nodes[node_id].text;
    }
    return "[" + std::to_string(node_id) + "]";
}

// Visualization: dump subgraph around context to .dot file
void dump_subgraph_to_dot(const std::vector<uint64_t>& context, 
                         const std::string& filename) {
    
    std::ofstream dot_file(filename);
    if (!dot_file.is_open()) {
        std::cerr << "Failed to open " << filename << " for writing\n";
        return;
    }
    
    // Collect all nodes in the subgraph (context + neighbors)
    std::unordered_set<uint64_t> subgraph_nodes;
    for (uint64_t node_id : context) {
        subgraph_nodes.insert(node_id);
        
        // Add neighbors
        if (G_adj.find(node_id) != G_adj.end()) {
            for (size_t edge_idx : G_adj[node_id]) {
                if (edge_idx < G_edges.size()) {
                    subgraph_nodes.insert(G_edges[edge_idx].locB);
                }
            }
        }
    }
    
    // Write DOT header
    dot_file << "digraph MelvinGraph {\n";
    dot_file << "  rankdir=LR;\n";
    dot_file << "  node [shape=ellipse, style=filled];\n";
    dot_file << "  edge [fontsize=10];\n\n";
    
    // Write nodes
    for (uint64_t node_id : subgraph_nodes) {
        std::string node_text_str = node_text(node_id);
        
        // Escape quotes and special characters for DOT
        std::string escaped_text = node_text_str;
        size_t pos = 0;
        while ((pos = escaped_text.find("\"", pos)) != std::string::npos) {
            escaped_text.replace(pos, 1, "\\\"");
            pos += 2;
        }
        
        // Color nodes based on type
        std::string color = "lightblue";
        if (G_nodes.find(node_id) != G_nodes.end()) {
            if (G_nodes[node_id].type == 1110) {
                color = "lightcoral"; // Thought nodes
            } else if (G_nodes[node_id].freq > 5) {
                color = "lightgreen"; // High frequency nodes
            }
        }
        
        // Highlight context nodes
        bool is_context = std::find(context.begin(), context.end(), node_id) != context.end();
        if (is_context) {
            color = "gold";
        }
        
        dot_file << "  n" << node_id << " [label=\"" << escaped_text 
                 << "\", fillcolor=\"" << color << "\"];\n";
    }
    
    dot_file << "\n";
    
    // Write edges
    for (uint64_t node_id : subgraph_nodes) {
        if (G_adj.find(node_id) != G_adj.end()) {
            for (size_t edge_idx : G_adj[node_id]) {
                if (edge_idx < G_edges.size()) {
                    const auto& edge = G_edges[edge_idx];
                    
                    // Only include edges within our subgraph
                    if (subgraph_nodes.find(edge.locB) != subgraph_nodes.end()) {
                        std::string rel_label = rel_to_string(static_cast<Rel>(edge.rel));
                        std::string edge_label = rel_label + " " + 
                                               std::to_string(static_cast<int>(edge.freq_ratio * 100));
                        
                        dot_file << "  n" << node_id << " -> n" << edge.locB 
                                 << " [label=\"" << edge_label << "\"];\n";
                    }
                }
            }
        }
    }
    
    dot_file << "}\n";
    dot_file.close();
    
    std::cout << "📊 Graph visualization saved to " << filename << "\n";
    std::cout << "   Render with: dot -Tpng " << filename << " -o melvin_graph.png\n\n";
}

} // namespace melvin
