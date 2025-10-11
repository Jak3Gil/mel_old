#include "multihop_engine.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cctype>
#include <cmath>

// Stub Node/Edge definitions for compilation
// (Will link with actual definitions from melvin core)
struct Node {
    uint64_t id = 0;
    std::string text;
    std::vector<float> embedding;
    int freq = 0;
};

struct Edge {
    uint64_t u, v;
    float weight = 1.0f;
    std::string rel;
    uint32_t count = 0;
};

namespace melvin {
namespace reasoning {

// ReasoningPath methods
std::string ReasoningPath::to_string(const std::unordered_map<uint64_t, Node>& nodes) const {
    std::ostringstream oss;
    for (size_t i = 0; i < hops.size(); i++) {
        const auto& hop = hops[i];
        auto from_it = nodes.find(hop.from_node);
        auto to_it = nodes.find(hop.to_node);
        
        if (from_it != nodes.end()) {
            oss << from_it->second.text;
        }
        oss << " -" << hop.relation << "→ ";
        if (to_it != nodes.end() && i == hops.size() - 1) {
            oss << to_it->second.text;
        }
    }
    return oss.str();
}

// MultihopEngine implementation
MultihopEngine::MultihopEngine(const MultihopConfig& config)
    : config_(config) {
}

MultihopResult MultihopEngine::search(
    const std::string& query,
    const std::unordered_map<uint64_t, Node>& nodes,
    const std::vector<Edge>& edges,
    const std::unordered_map<uint64_t, std::vector<size_t>>& adjacency
) {
    MultihopResult result;
    result.nodes_explored = 0;
    result.paths_generated = 0;
    
    // Step 1: Find starting nodes from query
    auto start_nodes = find_start_nodes(query, nodes);
    if (start_nodes.empty()) {
        result.success = false;
        result.error_message = "No starting nodes found for query";
        return result;
    }
    
    // Step 2: Run beam search to find paths
    auto paths = beam_search(start_nodes, nodes, edges, adjacency, config_.max_hops);
    
    if (paths.empty()) {
        result.success = false;
        result.error_message = "No paths found";
        return result;
    }
    
    // Step 3: Sort paths by score (highest first)
    std::sort(paths.begin(), paths.end(),
              [](const ReasoningPath& a, const ReasoningPath& b) {
                  return a.total_score > b.total_score;
              });
    
    // Step 4: Select best path
    result.success = true;
    result.best_path = paths[0];
    result.answer = paths[0].answer;
    result.all_paths = paths;  // Keep top paths for inspection
    result.paths_generated = static_cast<int>(paths.size());
    
    return result;
}

std::vector<uint64_t> MultihopEngine::find_start_nodes(
    const std::string& query,
    const std::unordered_map<uint64_t, Node>& nodes
) {
    std::vector<uint64_t> start_nodes;
    
    // Parse query into tokens (simple whitespace split + lowercase)
    std::vector<std::string> query_tokens;
    std::istringstream iss(query);
    std::string token;
    while (iss >> token) {
        // Lowercase and remove punctuation
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        token.erase(std::remove_if(token.begin(), token.end(), ::ispunct), token.end());
        if (!token.empty() && token.length() > 2) {  // Ignore short words (is, do, etc.)
            query_tokens.push_back(token);
        }
    }
    
    if (query_tokens.empty()) {
        return start_nodes;
    }
    
    // Find nodes that match query tokens
    std::vector<std::pair<uint64_t, float>> candidates;  // (node_id, match_score)
    
    for (const auto& [node_id, node] : nodes) {
        float match_score = 0.0f;
        std::string node_text_lower = node.text;
        std::transform(node_text_lower.begin(), node_text_lower.end(), 
                      node_text_lower.begin(), ::tolower);
        
        // Check for token matches
        for (const auto& qt : query_tokens) {
            if (node_text_lower.find(qt) != std::string::npos ||
                qt.find(node_text_lower) != std::string::npos) {
                match_score += 1.0f;
            }
        }
        
        // Boost by frequency (popular nodes more likely to be starting points)
        if (node.freq > 0) {
            match_score += std::log(node.freq + 1) * 0.1f;
        }
        
        if (match_score > 0.0f) {
            candidates.push_back({node_id, match_score});
        }
    }
    
    // Sort by match score (highest first)
    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });
    
    // Return top-k candidates (up to beam_width)
    int top_k = std::min(config_.beam_width, static_cast<int>(candidates.size()));
    for (int i = 0; i < top_k; i++) {
        start_nodes.push_back(candidates[i].first);
    }
    
    return start_nodes;
}

std::vector<ReasoningPath> MultihopEngine::beam_search(
    const std::vector<uint64_t>& start_nodes,
    const std::unordered_map<uint64_t, Node>& nodes,
    const std::vector<Edge>& edges,
    const std::unordered_map<uint64_t, std::vector<size_t>>& adjacency,
    int max_hops
) {
    std::vector<ReasoningPath> beam;
    
    // Initialize beam with start nodes (0-hop paths)
    for (uint64_t start_id : start_nodes) {
        ReasoningPath path;
        path.hops.clear();
        path.total_score = 1.0f;  // Initial score
        
        // Set answer to start node text
        auto it = nodes.find(start_id);
        if (it != nodes.end()) {
            path.answer = it->second.text;
        }
        
        beam.push_back(path);
    }
    
    // Iteratively expand paths up to max_hops
    for (int hop = 0; hop < max_hops; hop++) {
        std::vector<ReasoningPath> expanded_paths;
        
        // Expand each path in current beam
        for (const auto& path : beam) {
            auto new_paths = expand_path(path, nodes, edges, adjacency);
            expanded_paths.insert(expanded_paths.end(), new_paths.begin(), new_paths.end());
        }
        
        // If no expansions possible, return current beam
        if (expanded_paths.empty()) {
            break;
        }
        
        // Score all expanded paths
        for (auto& path : expanded_paths) {
            path.total_score = score_path(path);
        }
        
        // Sort by score (highest first)
        std::sort(expanded_paths.begin(), expanded_paths.end(),
                  [](const ReasoningPath& a, const ReasoningPath& b) {
                      return a.total_score > b.total_score;
                  });
        
        // Prune to beam_width
        int keep = std::min(config_.beam_width, static_cast<int>(expanded_paths.size()));
        beam.clear();
        for (int i = 0; i < keep; i++) {
            beam.push_back(expanded_paths[i]);
        }
        
        // Also keep original unexpanded paths (allow variable-length paths)
        // This means we can return 1-hop, 2-hop, or 3-hop paths
        for (const auto& old_path : beam) {
            if (!old_path.hops.empty()) {
                expanded_paths.push_back(old_path);
            }
        }
    }
    
    // Return final beam (all path lengths up to max_hops)
    // Remove duplicates and re-sort
    std::sort(beam.begin(), beam.end(),
              [](const ReasoningPath& a, const ReasoningPath& b) {
                  return a.total_score > b.total_score;
              });
    
    return beam;
}

std::vector<ReasoningPath> MultihopEngine::expand_path(
    const ReasoningPath& path,
    const std::unordered_map<uint64_t, Node>& nodes,
    const std::vector<Edge>& edges,
    const std::unordered_map<uint64_t, std::vector<size_t>>& adjacency
) {
    std::vector<ReasoningPath> expanded;
    
    // Determine the "from" node for expansion
    uint64_t from_node;
    if (path.hops.empty()) {
        // Path is just a start node, stored in answer
        // Find node by text (hackish but works for scaffold)
        from_node = 0;
        for (const auto& [id, node] : nodes) {
            if (node.text == path.answer) {
                from_node = id;
                break;
            }
        }
        if (from_node == 0) {
            return expanded;  // Can't expand
        }
    } else {
        // Get last node from last hop
        from_node = path.hops.back().to_node;
    }
    
    // Find all outgoing edges from from_node
    auto adj_it = adjacency.find(from_node);
    if (adj_it == adjacency.end()) {
        return expanded;  // No outgoing edges
    }
    
    // Expand along each outgoing edge
    for (size_t edge_idx : adj_it->second) {
        if (edge_idx >= edges.size()) continue;
        
        const Edge& edge = edges[edge_idx];
        
        // Create new path with this hop added
        ReasoningPath new_path = path;
        
        ReasoningHop hop;
        hop.from_node = edge.u;
        hop.to_node = edge.v;
        hop.edge_id = edge_idx;
        hop.relation = edge.rel;
        hop.weight = edge.weight;
        
        new_path.hops.push_back(hop);
        
        // Update answer to destination node text
        auto to_it = nodes.find(edge.v);
        if (to_it != nodes.end()) {
            new_path.answer = to_it->second.text;
        }
        
        // Avoid cycles (don't revisit nodes)
        bool is_cycle = false;
        for (const auto& prev_hop : path.hops) {
            if (prev_hop.to_node == edge.v) {
                is_cycle = true;
                break;
            }
        }
        
        if (!is_cycle) {
            expanded.push_back(new_path);
        }
    }
    
    return expanded;
}

float MultihopEngine::score_path(const ReasoningPath& path) const {
    if (path.hops.empty()) {
        return 1.0f;  // Start node has base score
    }
    
    float score = 1.0f;
    
    // Product of edge weights and relation priors
    for (const auto& hop : path.hops) {
        // Edge weight contribution
        score *= hop.weight;
        
        // Relation prior contribution
        float prior = get_relation_prior(hop.relation);
        score *= prior;
    }
    
    // Apply length penalty (prefer shorter paths)
    // length_penalty^num_hops
    // e.g., if length_penalty = 0.9 and 2 hops: score *= 0.9^2 = 0.81
    int num_hops = static_cast<int>(path.hops.size());
    float penalty = std::pow(config_.length_penalty, static_cast<float>(num_hops));
    score *= penalty;
    
    return score;
}

float MultihopEngine::get_relation_prior(const std::string& relation) const {
    auto it = config_.relation_priors.find(relation);
    if (it != config_.relation_priors.end()) {
        return it->second;
    }
    return 0.5f;  // Default prior
}

bool MultihopEngine::node_matches_query(
    uint64_t node_id,
    const std::string& query,
    const std::unordered_map<uint64_t, Node>& nodes
) {
    auto it = nodes.find(node_id);
    if (it == nodes.end()) {
        return false;
    }
    
    const Node& node = it->second;
    
    // Simple text matching (case-insensitive substring)
    std::string query_lower = query;
    std::string node_text_lower = node.text;
    
    std::transform(query_lower.begin(), query_lower.end(), 
                   query_lower.begin(), ::tolower);
    std::transform(node_text_lower.begin(), node_text_lower.end(), 
                   node_text_lower.begin(), ::tolower);
    
    // Check if node text appears in query or vice versa
    if (query_lower.find(node_text_lower) != std::string::npos ||
        node_text_lower.find(query_lower) != std::string::npos) {
        return true;
    }
    
    // TODO: In future, also check embedding similarity
    // For now, text matching is sufficient
    
    return false;
}

// Utility functions
std::string format_path(
    const ReasoningPath& path,
    const std::unordered_map<uint64_t, Node>& nodes
) {
    return path.to_string(nodes);
}

} // namespace reasoning
} // namespace melvin

