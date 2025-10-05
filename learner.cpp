#include "learner.hpp"
#include "scoring.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <random>

namespace melvin {

// EdgeLearner implementation
void EdgeLearner::update_edge_on_use(const EdgeID& edge_id, const LearningContext& context) {
    if (!context.store) return;
    
    EdgeRec edge;
    if (!context.store->get_edge(edge_id, edge)) return;
    
    // Check if edge is anchored (skip updates for anchored edges)
    if (LearningUtils::is_edge_anchored(edge)) return;
    
    // Update timestamp
    uint64_t current_time = context.timestamp;
    last_update_times_[edge_id] = current_time;
    
    // Reinforce weights
    reinforce_edge_weight(edge, context);
    
    // Update the edge in store
    context.store->upsert_edge(edge);
}

void EdgeLearner::update_edges_on_path(const std::vector<EdgeID>& edge_ids, 
                                      const LearningContext& context) {
    for (const auto& edge_id : edge_ids) {
        update_edge_on_use(edge_id, context);
    }
}

void EdgeLearner::decay_all_edges(Store* store) {
    // This is a simplified implementation
    // In practice, you'd want to iterate through all edges efficiently
    
    // For now, we'll use the store's built-in decay pass
    store->decay_pass(params_.lambda_ctx, params_.lambda_core);
}

bool EdgeLearner::materialize_inferred_edge(const NodeID& src, const NodeID& dst, 
                                           Rel relation, double confidence, Store* store) {
    if (confidence < params_.epsilon_prune) return false;
    
    // Create new edge
    EdgeRec new_edge;
    std::copy(src.begin(), src.end(), new_edge.src);
    std::copy(dst.begin(), dst.end(), new_edge.dst);
    new_edge.rel = static_cast<uint32_t>(relation);
    new_edge.layer = 1; // Inferred layer
    new_edge.w = static_cast<float>(confidence);
    new_edge.w_core = static_cast<float>(confidence * params_.alpha_infer);
    new_edge.w_ctx = 0.0f;
    new_edge.ts_last = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    new_edge.count = 1;
    new_edge.flags = static_cast<uint32_t>(EDGE_INFERRED);
    
    EdgeID edge_id = store->upsert_edge(new_edge);
    return !is_zero_id(edge_id);
}

double EdgeLearner::compute_context_similarity(const std::vector<NodeID>& query_context,
                                              const std::vector<NodeID>& edge_context) const {
    if (query_context.empty() || edge_context.empty()) return 0.0;
    
    // Simple Jaccard similarity
    std::unordered_set<NodeID, NodeIDHash> query_set(query_context.begin(), query_context.end());
    std::unordered_set<NodeID, NodeIDHash> edge_set(edge_context.begin(), edge_context.end());
    
    std::unordered_set<NodeID, NodeIDHash> intersection;
    for (const auto& node : query_set) {
        if (edge_set.find(node) != edge_set.end()) {
            intersection.insert(node);
        }
    }
    
    std::unordered_set<NodeID, NodeIDHash> union_set = query_set;
    for (const auto& node : edge_set) {
        union_set.insert(node);
    }
    
    if (union_set.empty()) return 0.0;
    
    return static_cast<double>(intersection.size()) / union_set.size();
}

void EdgeLearner::update_context_window(const std::vector<NodeID>& new_nodes) {
    recent_context_window_.insert(recent_context_window_.end(), new_nodes.begin(), new_nodes.end());
    
    // Keep only the most recent nodes
    if (recent_context_window_.size() > params_.context_window_size) {
        recent_context_window_.erase(recent_context_window_.begin(), 
                                   recent_context_window_.end() - params_.context_window_size);
    }
}

void EdgeLearner::reinforce_edge_weight(EdgeRec& edge, const LearningContext& context) {
    double alpha_core = context.is_explicit_observation ? params_.alpha_core : params_.alpha_infer;
    double alpha_ctx = params_.alpha_ctx;
    
    // Compute context similarity
    double context_sim = compute_context_similarity(context.query_nodes, recent_context_window_);
    
    // Update weights
    edge.w_core = std::min(1.0f, edge.w_core + static_cast<float>(alpha_core));
    edge.w_ctx = std::min(1.0f, edge.w_ctx + static_cast<float>(alpha_ctx * context_sim));
    
    // Update effective weight
    edge.w = (edge.w_core + edge.w_ctx) / 2.0f;
    
    // Update timestamp and count
    edge.ts_last = context.timestamp;
    edge.count++;
}

void EdgeLearner::decay_edge_weight(EdgeRec& edge, double time_delta) {
    if (LearningUtils::is_edge_anchored(edge)) return; // Skip decay for anchored edges
    
    // Apply exponential decay
    double decay_factor_core = LearningUtils::compute_decay_factor(time_delta, params_.lambda_core);
    double decay_factor_ctx = LearningUtils::compute_decay_factor(time_delta, params_.lambda_ctx);
    
    edge.w_core *= static_cast<float>(decay_factor_core);
    edge.w_ctx *= static_cast<float>(decay_factor_ctx);
    
    // Update effective weight
    edge.w = (edge.w_core + edge.w_ctx) / 2.0f;
}

bool EdgeLearner::should_prune_edge(const EdgeRec& edge) const {
    if (LearningUtils::is_edge_anchored(edge)) return false; // Never prune anchored edges
    
    return (edge.w_core + edge.w_ctx) < params_.epsilon_prune;
}

// PatternLearner implementation
void PatternLearner::learn_patterns(const std::vector<NodeID>& node_sequence, Store* store) {
    if (node_sequence.size() < 2) return;
    
    // Extract n-grams of different sizes
    for (size_t n = 2; n <= std::min(static_cast<size_t>(4), node_sequence.size()); ++n) {
        std::vector<std::string> ngrams = extract_ngrams(node_sequence, store, n);
        
        for (const auto& ngram : ngrams) {
            ngram_counts_[ngram]++;
            
            // Store context for diversity computation
            std::string context = node_sequence_to_string(node_sequence, store);
            ngram_contexts_[ngram].insert(context);
        }
    }
}

std::vector<NodeID> PatternLearner::create_generalization_nodes(Store* store) {
    std::vector<NodeID> created_nodes;
    
    for (const auto& [pattern, count] : ngram_counts_) {
        if (should_generalize_pattern(pattern)) {
            NodeID gen_node = create_generalization_node(pattern, store);
            if (!is_zero_id(gen_node)) {
                created_nodes.push_back(gen_node);
            }
        }
    }
    
    return created_nodes;
}

bool PatternLearner::should_generalize_pattern(const std::string& pattern) const {
    auto count_it = ngram_counts_.find(pattern);
    if (count_it == ngram_counts_.end()) return false;
    
    if (count_it->second < theta_pat) return false;
    
    double diversity = get_pattern_diversity(pattern);
    return diversity >= theta_div;
}

double PatternLearner::get_pattern_diversity(const std::string& pattern) const {
    auto context_it = ngram_contexts_.find(pattern);
    if (context_it == ngram_contexts_.end()) return 0.0;
    
    return static_cast<double>(context_it->second.size()) / 
           static_cast<double>(ngram_counts_.at(pattern));
}

std::vector<std::string> PatternLearner::extract_ngrams(const std::vector<NodeID>& nodes, 
                                                       Store* store, size_t n) const {
    std::vector<std::string> ngrams;
    
    for (size_t i = 0; i <= nodes.size() - n; ++i) {
        std::vector<NodeID> ngram_nodes(nodes.begin() + i, nodes.begin() + i + n);
        std::string ngram = node_sequence_to_string(ngram_nodes, store);
        ngrams.push_back(ngram);
    }
    
    return ngrams;
}

std::string PatternLearner::node_sequence_to_string(const std::vector<NodeID>& nodes, Store* store) const {
    std::string result;
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (i > 0) result += " ";
        
        NodeRecHeader header;
        std::vector<uint8_t> payload;
        if (store->get_node(nodes[i], header, payload)) {
            std::string content(reinterpret_cast<const char*>(payload.data()), payload.size());
            result += content;
        } else {
            result += "<unknown>";
        }
    }
    
    return result;
}

NodeID PatternLearner::create_generalization_node(const std::string& pattern, Store* store) const {
    // Create a new generalization node
    NodeRecHeader header;
    header.type = static_cast<uint32_t>(NodeType::ABSTRACTION);
    header.flags = static_cast<uint32_t>(GENERALIZED);
    header.ts_created = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    header.ts_updated = header.ts_created;
    header.payload_len = pattern.size();
    header.degree_hint = 0;
    header.pin_expiry = 0;
    header.confirm_count = 0;
    header.anchor_pad = 0;
    
    std::vector<uint8_t> payload(pattern.begin(), pattern.end());
    
    return store->upsert_node(header, payload);
}

// PMITable implementation
void PMITable::update(const std::vector<NodeID>& node_sequence) {
    if (node_sequence.size() < 2) return;
    
    total_observations_ += node_sequence.size();
    
    // Update node counts
    for (const auto& node : node_sequence) {
        node_counts_[node]++;
    }
    
    // Update pair counts (with temporal adjacency check)
    for (size_t i = 0; i < node_sequence.size() - 1; ++i) {
        for (size_t j = i + 1; j < node_sequence.size(); ++j) {
            if (j - i <= 2) continue; // Skip temporally adjacent pairs
            
            NodeID node_a = node_sequence[i];
            NodeID node_b = node_sequence[j];
            
            if (node_a < node_b) {
                pair_counts_[{node_a, node_b}]++;
            } else {
                pair_counts_[{node_b, node_a}]++;
            }
        }
    }
}

double PMITable::compute_pmi(const NodeID& node_a, const NodeID& node_b) const {
    double prob_a = compute_probability(node_a);
    double prob_b = compute_probability(node_b);
    double prob_ab = compute_joint_probability(node_a, node_b);
    
    if (prob_a <= 0.0 || prob_b <= 0.0 || prob_ab <= 0.0) return 0.0;
    
    return std::log(prob_ab / (prob_a * prob_b));
}

std::vector<std::pair<NodeID, NodeID>> PMITable::get_leap_candidates() const {
    std::vector<std::pair<NodeID, NodeID>> candidates;
    
    for (const auto& [pair, count] : pair_counts_) {
        if (should_create_leap_edge(pair.first, pair.second)) {
            candidates.push_back(pair);
        }
    }
    
    return candidates;
}

bool PMITable::should_create_leap_edge(const NodeID& node_a, const NodeID& node_b) const {
    if (are_temporally_adjacent(node_a, node_b)) return false;
    
    double pmi = compute_pmi(node_a, node_b);
    return pmi >= theta_pmi_;
}

double PMITable::compute_probability(const NodeID& node) const {
    auto it = node_counts_.find(node);
    if (it == node_counts_.end() || total_observations_ == 0) return 0.0;
    
    return static_cast<double>(it->second) / total_observations_;
}

double PMITable::compute_joint_probability(const NodeID& node_a, const NodeID& node_b) const {
    auto it = pair_counts_.find({node_a, node_b});
    if (it == pair_counts_.end()) {
        it = pair_counts_.find({node_b, node_a});
        if (it == pair_counts_.end()) return 0.0;
    }
    
    return static_cast<double>(it->second) / total_observations_;
}

bool PMITable::are_temporally_adjacent(const NodeID& node_a, const NodeID& node_b) const {
    // This is a simplified check - in practice, you'd want to track temporal adjacency
    // based on the cooccurrence contexts
    return false;
}

// CausalLearner implementation
void CausalLearner::learn_causal_patterns(const std::vector<NodeID>& temporal_sequence) {
    if (temporal_sequence.size() < 2) return;
    
    for (size_t i = 0; i < temporal_sequence.size() - 1; ++i) {
        for (size_t j = i + 1; j < temporal_sequence.size(); ++j) {
            update_lead_lag_count(temporal_sequence[i], temporal_sequence[j]);
        }
    }
}

double CausalLearner::get_causal_strength(const NodeID& cause, const NodeID& effect) const {
    auto it = causal_strengths_.find({cause, effect});
    if (it != causal_strengths_.end()) {
        return it->second;
    }
    
    // Compute from lead-lag counts if not cached
    auto count_it = lead_lag_counts_.find({cause, effect});
    if (count_it != lead_lag_counts_.end()) {
        return compute_causal_strength_from_counts(count_it->second, 0);
    }
    
    return 0.0;
}

bool CausalLearner::is_causal(const NodeID& cause, const NodeID& effect) const {
    return get_causal_strength(cause, effect) >= min_causal_threshold_;
}

std::vector<std::pair<NodeID, NodeID>> CausalLearner::get_causal_relationships() const {
    std::vector<std::pair<NodeID, NodeID>> relationships;
    
    for (const auto& [pair, strength] : causal_strengths_) {
        if (strength >= min_causal_threshold_) {
            relationships.push_back(pair);
        }
    }
    
    return relationships;
}

void CausalLearner::update_edge_causal_annotations(Store* store) {
    // This would update edge flags to indicate causal relationships
    // Implementation depends on the specific edge annotation system
}

void CausalLearner::update_lead_lag_count(const NodeID& earlier, const NodeID& later) {
    lead_lag_counts_[{earlier, later}]++;
    
    // Update causal strength
    auto it = lead_lag_counts_.find({earlier, later});
    auto reverse_it = lead_lag_counts_.find({later, earlier});
    
    uint32_t lead_count = it->second;
    uint32_t lag_count = (reverse_it != lead_lag_counts_.end()) ? reverse_it->second : 0;
    
    causal_strengths_[{earlier, later}] = compute_causal_strength_from_counts(lead_count, lag_count);
}

double CausalLearner::compute_causal_strength_from_counts(uint32_t lead_count, uint32_t lag_count) const {
    if (lead_count + lag_count == 0) return 0.0;
    
    return static_cast<double>(lead_count) / (lead_count + lag_count);
}

// GraphLearner implementation
void GraphLearner::learn_from_query(const std::vector<NodeID>& query_nodes, 
                                   const std::vector<EdgeID>& used_edges,
                                   Store* store, bool is_explicit) {
    LearningContext context;
    context.store = store;
    context.query_nodes = query_nodes;
    context.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    context.is_explicit_observation = is_explicit;
    
    // Update edges used in the query
    edge_learner_.update_edges_on_path(used_edges, context);
    
    // Update context window
    edge_learner_.update_context_window(query_nodes);
}

void GraphLearner::learn_from_sequence(const std::vector<NodeID>& node_sequence, Store* store) {
    if (node_sequence.empty()) return;
    
    // Learn patterns
    pattern_learner_.learn_patterns(node_sequence, store);
    
    // Update PMI table
    pmi_table_.update(node_sequence);
    
    // Learn causal patterns
    causal_learner_.learn_causal_patterns(node_sequence);
    
    // Update context window
    edge_learner_.update_context_window(node_sequence);
}

void GraphLearner::run_maintenance_pass(Store* store) {
    // Decay all edges
    edge_learner_.decay_all_edges(store);
    
    // Create generalization nodes
    std::vector<NodeID> new_generalizations = pattern_learner_.create_generalization_nodes(store);
    
    // Create LEAP edges from PMI candidates
    std::vector<std::pair<NodeID, NodeID>> leap_candidates = pmi_table_.get_leap_candidates();
    for (const auto& [src, dst] : leap_candidates) {
        edge_learner_.materialize_inferred_edge(src, dst, Rel::LEAP, 0.5, store);
    }
    
    // Update causal annotations
    causal_learner_.update_edge_causal_annotations(store);
}

// LearningUtils implementation
namespace LearningUtils {
    double compute_decay_factor(double time_delta, double lambda) {
        return std::exp(-lambda * time_delta);
    }
    
    EdgeID compute_edge_id(const NodeID& src, const NodeID& dst, Rel relation) {
        // Simple hash-based edge ID computation
        EdgeID edge_id;
        std::hash<std::string> hasher;
        
        std::string combined;
        combined.reserve(65); // 32 + 32 + 1 bytes
        
        for (uint8_t byte : src) {
            combined.push_back(static_cast<char>(byte));
        }
        for (uint8_t byte : dst) {
            combined.push_back(static_cast<char>(byte));
        }
        combined.push_back(static_cast<char>(static_cast<uint32_t>(relation)));
        
        size_t hash = hasher(combined);
        
        // Convert hash to EdgeID
        for (size_t i = 0; i < 8; ++i) {
            edge_id[i*4] = static_cast<uint8_t>((hash >> (i*8)) & 0xFF);
            edge_id[i*4+1] = static_cast<uint8_t>((hash >> (i*8+8)) & 0xFF);
            edge_id[i*4+2] = static_cast<uint8_t>((hash >> (i*8+16)) & 0xFF);
            edge_id[i*4+3] = static_cast<uint8_t>((hash >> (i*8+24)) & 0xFF);
        }
        
        return edge_id;
    }
    
    bool is_edge_anchored(const EdgeRec& edge) {
        return (edge.flags & static_cast<uint32_t>(EDGE_ANCHOR)) != 0;
    }
    
    float normalize_weight(float weight) {
        return std::max(0.0f, std::min(1.0f, weight));
    }
    
    double compute_node_similarity(const NodeID& a, const NodeID& b, Store* store) {
        // Simple content-based similarity
        NodeRecHeader header_a, header_b;
        std::vector<uint8_t> payload_a, payload_b;
        
        if (!store->get_node(a, header_a, payload_a) || 
            !store->get_node(b, header_b, payload_b)) {
            return 0.0;
        }
        
        std::string content_a(reinterpret_cast<const char*>(payload_a.data()), payload_a.size());
        std::string content_b(reinterpret_cast<const char*>(payload_b.data()), payload_b.size());
        
        if (content_a == content_b) return 1.0;
        
        // Simple string similarity (in practice, use proper embeddings)
        size_t common_chars = 0;
        size_t min_len = std::min(content_a.length(), content_b.length());
        
        for (size_t i = 0; i < min_len; ++i) {
            if (content_a[i] == content_b[i]) common_chars++;
        }
        
        if (min_len == 0) return 0.0;
        
        return static_cast<double>(common_chars) / min_len;
    }
}

} // namespace melvin
