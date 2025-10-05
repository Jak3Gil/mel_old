#include "beam.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>

namespace melvin {

// BeamPath implementation
void BeamPath::add_edge(const EdgeRec& edge, const NodeID& target_node, double edge_score) {
    edges.push_back(edge);
    nodes.push_back(target_node);
    score += edge_score;
    
    // Update visited sets
    NodeID src_node;
    std::copy(edge.src, edge.src + 32, src_node.begin());
    visited_nodes.insert(src_node);
    visited_nodes.insert(target_node);
    
    EdgeID edge_id;
    std::hash<std::string> hasher;
    std::string edge_str;
    edge_str.reserve(65);
    for (uint8_t byte : edge.src) edge_str.push_back(static_cast<char>(byte));
    for (uint8_t byte : edge.dst) edge_str.push_back(static_cast<char>(byte));
    edge_str.push_back(static_cast<char>(edge.rel));
    
    size_t hash = hasher(edge_str);
    for (size_t i = 0; i < 8; ++i) {
        edge_id[i*4] = static_cast<uint8_t>((hash >> (i*8)) & 0xFF);
        edge_id[i*4+1] = static_cast<uint8_t>((hash >> (i*8+8)) & 0xFF);
        edge_id[i*4+2] = static_cast<uint8_t>((hash >> (i*8+16)) & 0xFF);
        edge_id[i*4+3] = static_cast<uint8_t>((hash >> (i*8+24)) & 0xFF);
    }
    
    visited_edges.insert(edge_id);
    
    timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool BeamPath::has_loop() const {
    // Check for repeated nodes
    if (nodes.size() != visited_nodes.size()) return true;
    
    // Check for repeated edges
    if (edges.size() != visited_edges.size()) return true;
    
    return false;
}

NodeID BeamPath::get_current_node() const {
    if (nodes.empty()) {
        // Return zero node if path is empty
        return NodeID{};
    }
    return nodes.back();
}

bool BeamPath::is_at_max_depth(size_t max_depth) const {
    return edges.size() >= max_depth;
}

// BeamSearchState implementation
void BeamSearchState::add_path(const BeamPath& path) {
    if (path.is_complete) {
        completed_paths_.push_back(path);
        return;
    }
    
    // Check for duplicates
    std::string signature = generate_path_signature(path);
    if (path_signatures_.find(signature) != path_signatures_.end()) {
        return;
    }
    
    path_signatures_.insert(signature);
    beam_.push(path);
}

std::vector<BeamPath> BeamSearchState::get_best_paths(size_t k) const {
    std::vector<BeamPath> result;
    std::priority_queue<BeamPath> temp_beam = beam_;
    
    size_t count = 0;
    while (!temp_beam.empty() && count < k) {
        result.push_back(temp_beam.top());
        temp_beam.pop();
        count++;
    }
    
    return result;
}

void BeamSearchState::expand_beam(const std::vector<BeamPath>& new_paths) {
    current_depth_++;
    
    // Clear current beam
    while (!beam_.empty()) beam_.pop();
    path_signatures_.clear();
    
    // Add new paths
    for (const auto& path : new_paths) {
        add_path(path);
    }
}

std::string BeamSearchState::generate_path_signature(const BeamPath& path) const {
    std::ostringstream oss;
    for (const auto& edge : path.edges) {
        for (size_t i = 0; i < 32; ++i) {
            oss << std::hex << static_cast<int>(edge.src[i]);
        }
        for (size_t i = 0; i < 32; ++i) {
            oss << std::hex << static_cast<int>(edge.dst[i]);
        }
        oss << edge.rel;
    }
    return oss.str();
}

// StochasticExpander implementation
std::vector<EdgeRec> StochasticExpander::select_edges_top_p(
    const std::vector<std::pair<EdgeRec, double>>& scored_edges, 
    double top_p) const {
    
    if (scored_edges.empty()) return {};
    
    // Sort by score (descending)
    std::vector<std::pair<EdgeRec, double>> sorted_edges = scored_edges;
    std::sort(sorted_edges.begin(), sorted_edges.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Convert scores to probabilities (softmax)
    std::vector<double> scores;
    for (const auto& pair : sorted_edges) {
        scores.push_back(pair.second);
    }
    
    std::vector<double> probs = compute_cumulative_probs(scores);
    
    // Find cutoff point for top-p
    double cumulative_prob = 0.0;
    size_t cutoff = 0;
    
    for (size_t i = 0; i < probs.size(); ++i) {
        cumulative_prob += probs[i];
        if (cumulative_prob >= top_p) {
            cutoff = i + 1;
            break;
        }
    }
    
    if (cutoff == 0) cutoff = 1; // Always select at least one edge
    
    // Select edges up to cutoff
    std::vector<EdgeRec> selected;
    for (size_t i = 0; i < cutoff; ++i) {
        selected.push_back(sorted_edges[i].first);
    }
    
    return selected;
}

std::vector<EdgeRec> StochasticExpander::select_edges_top_k(
    const std::vector<std::pair<EdgeRec, double>>& scored_edges, 
    size_t top_k) const {
    
    if (scored_edges.empty()) return {};
    
    // Sort by score (descending)
    std::vector<std::pair<EdgeRec, double>> sorted_edges = scored_edges;
    std::sort(sorted_edges.begin(), sorted_edges.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Select top-k
    std::vector<EdgeRec> selected;
    size_t count = std::min(top_k, sorted_edges.size());
    
    for (size_t i = 0; i < count; ++i) {
        selected.push_back(sorted_edges[i].first);
    }
    
    return selected;
}

std::vector<double> StochasticExpander::compute_cumulative_probs(const std::vector<double>& scores) const {
    if (scores.empty()) return {};
    
    // Softmax normalization
    std::vector<double> exp_scores;
    double max_score = *std::max_element(scores.begin(), scores.end());
    double sum_exp = 0.0;
    
    for (double score : scores) {
        double exp_score = std::exp(score - max_score);
        exp_scores.push_back(exp_score);
        sum_exp += exp_score;
    }
    
    // Normalize to probabilities
    std::vector<double> probs;
    for (double exp_score : exp_scores) {
        probs.push_back(exp_score / sum_exp);
    }
    
    return probs;
}

size_t StochasticExpander::sample_categorical(const std::vector<double>& probs) const {
    if (probs.empty()) return 0;
    
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double random_val = dist(rng_);
    
    double cumulative = 0.0;
    for (size_t i = 0; i < probs.size(); ++i) {
        cumulative += probs[i];
        if (random_val <= cumulative) {
            return i;
        }
    }
    
    return probs.size() - 1; // Fallback to last element
}

// BeamSearchEngine implementation
std::vector<BeamPath> BeamSearchEngine::search(const Query& query, 
                                              const NodeID& start_node,
                                              Store* store) {
    return search_with_params(query, start_node, store, params_);
}

std::vector<BeamPath> BeamSearchEngine::search_with_params(const Query& query,
                                                          const NodeID& start_node,
                                                          Store* store,
                                                          const BeamParams& custom_params) {
    if (is_zero_id(start_node) || !store) return {};
    
    BeamSearchState state(custom_params);
    ScoringContext context;
    context.query = &query;
    context.store = store;
    context.current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    // Initialize with start node
    BeamPath initial_path;
    initial_path.nodes.push_back(start_node);
    initial_path.visited_nodes.insert(start_node);
    initial_path.score = scorer_.score_node_prior(start_node, query, store);
    
    state.add_path(initial_path);
    
    double best_score = initial_path.score;
    
    // Main search loop
    while (!state.is_empty() && state.get_current_depth() < custom_params.max_depth) {
        std::vector<BeamPath> current_paths = state.get_best_paths(custom_params.beam_width);
        std::vector<BeamPath> expanded_paths;
        
        for (const auto& path : current_paths) {
            if (path.is_complete) continue;
            
            std::vector<BeamPath> new_paths = expand_path(path, context, store);
            expanded_paths.insert(expanded_paths.end(), new_paths.begin(), new_paths.end());
        }
        
        if (expanded_paths.empty()) break;
        
        // Check stopping condition
        double current_best = 0.0;
        for (const auto& path : expanded_paths) {
            current_best = std::max(current_best, path.score);
        }
        
        if (current_best - best_score < custom_params.stop_threshold) {
            break;
        }
        
        best_score = current_best;
        state.expand_beam(expanded_paths);
    }
    
    // Return completed paths
    std::vector<BeamPath> results = state.get_completed_paths();
    
    // Sort by score (descending)
    std::sort(results.begin(), results.end(),
              [](const BeamPath& a, const BeamPath& b) { return a.score > b.score; });
    
    return results;
}

std::vector<BeamPath> BeamSearchEngine::expand_path(const BeamPath& path,
                                                   const ScoringContext& context,
                                                   Store* store) {
    std::vector<BeamPath> expanded_paths;
    
    if (path.is_at_max_depth(params_.max_depth)) {
        // Mark as complete
        BeamPath complete_path = path;
        complete_path.is_complete = true;
        expanded_paths.push_back(complete_path);
        return expanded_paths;
    }
    
    // Score outgoing edges
    std::vector<std::pair<EdgeRec, double>> scored_edges = score_outgoing_edges(path, context, store);
    
    if (scored_edges.empty()) {
        // No more edges to explore, mark as complete
        BeamPath complete_path = path;
        complete_path.is_complete = true;
        expanded_paths.push_back(complete_path);
        return expanded_paths;
    }
    
    // Select edges for expansion
    std::vector<EdgeRec> selected_edges;
    if (params_.enable_stochastic_expansion) {
        selected_edges = expander_.select_edges_top_p(scored_edges, params_.top_p);
    } else {
        selected_edges = expander_.select_edges_top_k(scored_edges, params_.top_k);
    }
    
    // Create new paths
    for (const auto& edge : selected_edges) {
        if (!is_valid_expansion(path, edge)) continue;
        
        NodeID target_node;
        std::copy(edge.dst, edge.dst + 32, target_node.begin());
        
        BeamPath new_path = create_new_path(path, edge, scored_edges[0].second);
        expanded_paths.push_back(new_path);
    }
    
    return expanded_paths;
}

bool BeamSearchEngine::should_stop_search(const BeamSearchState& state, double best_score) const {
    // Check if we have enough good paths
    if (state.get_completed_paths().size() >= params_.beam_width) {
        return true;
    }
    
    // Check depth limit
    if (state.get_current_depth() >= params_.max_depth) {
        return true;
    }
    
    return false;
}

std::vector<std::pair<EdgeRec, double>> BeamSearchEngine::score_outgoing_edges(
    const BeamPath& path,
    const ScoringContext& context,
    Store* store) const {
    
    std::vector<std::pair<EdgeRec, double>> scored_edges;
    NodeID current_node = path.get_current_node();
    
    if (is_zero_id(current_node)) return scored_edges;
    
    // Get outgoing edges
    RelMask all_relations;
    all_relations.set_all();
    
    AdjView view;
    if (!store->get_out_edges(current_node, all_relations, view)) {
        return scored_edges;
    }
    
    // Score each edge
    for (size_t i = 0; i < view.count; ++i) {
        const EdgeRec& edge = view.edges[i];
        double score = scorer_.score_edge_step(edge, context);
        
        scored_edges.emplace_back(edge, score);
    }
    
    return scored_edges;
}

BeamPath BeamSearchEngine::create_new_path(const BeamPath& parent_path, 
                                          const EdgeRec& edge,
                                          double edge_score) const {
    BeamPath new_path = parent_path;
    
    NodeID target_node;
    std::copy(edge.dst, edge.dst + 32, target_node.begin());
    
    new_path.add_edge(edge, target_node, edge_score);
    
    // Update confidence
    new_path.confidence = scorer_.score_path(new_path.edges, parent_path.nodes[0], 
                                           ScoringContext{});
    
    return new_path;
}

bool BeamSearchEngine::is_valid_expansion(const BeamPath& path, const EdgeRec& edge) const {
    if (!params_.enable_loop_detection) return true;
    
    // Check if this would create a loop
    NodeID target_node;
    std::copy(edge.dst, edge.dst + 32, target_node.begin());
    
    if (path.visited_nodes.find(target_node) != path.visited_nodes.end()) {
        return false; // Would create a loop
    }
    
    // Check if edge was already used
    EdgeID edge_id;
    std::hash<std::string> hasher;
    std::string edge_str;
    edge_str.reserve(65);
    for (uint8_t byte : edge.src) edge_str.push_back(static_cast<char>(byte));
    for (uint8_t byte : edge.dst) edge_str.push_back(static_cast<char>(byte));
    edge_str.push_back(static_cast<char>(edge.rel));
    
    size_t hash = hasher(edge_str);
    for (size_t i = 0; i < 8; ++i) {
        edge_id[i*4] = static_cast<uint8_t>((hash >> (i*8)) & 0xFF);
        edge_id[i*4+1] = static_cast<uint8_t>((hash >> (i*8+8)) & 0xFF);
        edge_id[i*4+2] = static_cast<uint8_t>((hash >> (i*8+16)) & 0xFF);
        edge_id[i*4+3] = static_cast<uint8_t>((hash >> (i*8+24)) & 0xFF);
    }
    
    return path.visited_edges.find(edge_id) == path.visited_edges.end();
}

// MultiStartBeamSearch implementation
std::vector<BeamPath> MultiStartBeamSearch::search_multiple_starts(
    const Query& query,
    const std::vector<NodeID>& start_nodes,
    Store* store) {
    
    std::vector<std::vector<BeamPath>> all_results;
    
    for (const auto& start_node : start_nodes) {
        std::vector<BeamPath> results = engine_.search(query, start_node, store);
        all_results.push_back(results);
    }
    
    return merge_search_results(all_results);
}

std::vector<NodeID> MultiStartBeamSearch::find_best_start_nodes(
    const Query& query,
    Store* store,
    size_t num_starts) const {
    
    std::vector<std::pair<NodeID, double>> scored_nodes;
    
    // This is a simplified implementation
    // In practice, you'd want to use the NodePriorScorer to find good starting nodes
    
    RelMask all_relations;
    all_relations.set_all();
    
    // For now, just return some arbitrary nodes
    // In a real implementation, you'd score all nodes and pick the best ones
    std::vector<NodeID> start_nodes;
    
    // This is a placeholder - you'd implement proper node scoring here
    return start_nodes;
}

std::vector<BeamPath> MultiStartBeamSearch::merge_search_results(
    const std::vector<std::vector<BeamPath>>& results,
    size_t max_results) const {
    
    std::vector<BeamPath> merged;
    
    for (const auto& result_set : results) {
        merged.insert(merged.end(), result_set.begin(), result_set.end());
    }
    
    // Sort by score and take top results
    std::sort(merged.begin(), merged.end(),
              [](const BeamPath& a, const BeamPath& b) { return a.score > b.score; });
    
    if (merged.size() > max_results) {
        merged.resize(max_results);
    }
    
    return merged;
}

// CachedBeamSearch implementation
std::vector<BeamPath> CachedBeamSearch::search_cached(const Query& query,
                                                     const NodeID& start_node,
                                                     Store* store) {
    std::string cache_key = generate_cache_key(query, start_node);
    
    auto cache_it = cache_.find(cache_key);
    if (cache_it != cache_.end()) {
        return cache_it->second;
    }
    
    // Perform search
    std::vector<BeamPath> results = engine_.search(query, start_node, store);
    
    // Cache results
    if (cache_.size() >= max_cache_size_) {
        evict_old_entries();
    }
    
    cache_[cache_key] = results;
    return results;
}

std::string CachedBeamSearch::generate_cache_key(const Query& query, const NodeID& start_node) const {
    std::ostringstream oss;
    oss << query.original_text << "_";
    for (uint8_t byte : start_node) {
        oss << std::hex << static_cast<int>(byte);
    }
    return oss.str();
}

void CachedBeamSearch::evict_old_entries() {
    // Simple LRU eviction - remove oldest entries
    // In practice, you'd want a more sophisticated cache management system
    size_t to_remove = max_cache_size_ / 4; // Remove 25% of entries
    
    auto it = cache_.begin();
    for (size_t i = 0; i < to_remove && it != cache_.end(); ++i) {
        it = cache_.erase(it);
    }
}

size_t CachedBeamSearch::get_cache_hits() const {
    // This would need to be tracked separately
    return 0;
}

size_t CachedBeamSearch::get_cache_misses() const {
    // This would need to be tracked separately
    return 0;
}

// BeamSearchUtils implementation
namespace BeamSearchUtils {
    double compute_path_diversity(const std::vector<BeamPath>& paths) {
        if (paths.size() <= 1) return 1.0;
        
        std::unordered_set<std::string> unique_signatures;
        for (const auto& path : paths) {
            unique_signatures.insert(compute_path_signature(path));
        }
        
        return static_cast<double>(unique_signatures.size()) / paths.size();
    }
    
    std::vector<BeamPath> filter_by_confidence(const std::vector<BeamPath>& paths, 
                                              double min_confidence) {
        std::vector<BeamPath> filtered;
        
        for (const auto& path : paths) {
            if (path.confidence >= min_confidence) {
                filtered.push_back(path);
            }
        }
        
        return filtered;
    }
    
    void sort_by_score(std::vector<BeamPath>& paths) {
        std::sort(paths.begin(), paths.end(),
                  [](const BeamPath& a, const BeamPath& b) { return a.score > b.score; });
    }
    
    std::string compute_path_signature(const BeamPath& path) {
        std::ostringstream oss;
        for (const auto& node : path.nodes) {
            for (uint8_t byte : node) {
                oss << std::hex << static_cast<int>(byte);
            }
        }
        return oss.str();
    }
    
    bool paths_are_similar(const BeamPath& path1, const BeamPath& path2, double threshold) {
        // Simple similarity based on common nodes
        std::unordered_set<NodeID, NodeIDHash> nodes1(path1.nodes.begin(), path1.nodes.end());
        std::unordered_set<NodeID, NodeIDHash> nodes2(path2.nodes.begin(), path2.nodes.end());
        
        std::unordered_set<NodeID, NodeIDHash> intersection;
        for (const auto& node : nodes1) {
            if (nodes2.find(node) != nodes2.end()) {
                intersection.insert(node);
            }
        }
        
        std::unordered_set<NodeID, NodeIDHash> union_set = nodes1;
        for (const auto& node : nodes2) {
            union_set.insert(node);
        }
        
        if (union_set.empty()) return false;
        
        double similarity = static_cast<double>(intersection.size()) / union_set.size();
        return similarity >= threshold;
    }
    
    std::vector<BeamPath> deduplicate_paths(const std::vector<BeamPath>& paths, 
                                          double similarity_threshold) {
        std::vector<BeamPath> deduplicated;
        
        for (const auto& path : paths) {
            bool is_duplicate = false;
            
            for (const auto& existing : deduplicated) {
                if (paths_are_similar(path, existing, similarity_threshold)) {
                    is_duplicate = true;
                    break;
                }
            }
            
            if (!is_duplicate) {
                deduplicated.push_back(path);
            }
        }
        
        return deduplicated;
    }
}

} // namespace melvin
