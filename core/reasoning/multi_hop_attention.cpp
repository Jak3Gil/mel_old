#include "multi_hop_attention.h"
#include <cmath>
#include <algorithm>
#include <unordered_set>

namespace melvin {
namespace reasoning {

MultiHopAttention::MultiHopAttention(int embedding_dim, int attention_heads)
    : embedding_dim_(embedding_dim)
    , attention_heads_(attention_heads)
    , head_dim_(embedding_dim / attention_heads)
{
}

float MultiHopAttention::compute_attention(
    const std::vector<float>& query,
    const std::vector<float>& key,
    const std::vector<float>& value
) {
    // Simplified attention: dot product
    float score = 0.0f;
    for (size_t i = 0; i < std::min({query.size(), key.size(), value.size()}); ++i) {
        score += query[i] * key[i];
    }
    return score / std::sqrt(static_cast<float>(head_dim_));
}

std::vector<MultiHopAttention::QueryResult> MultiHopAttention::query(
    const std::vector<float>& query_embedding,
    ActivationField& activation_field,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    const std::unordered_map<int, std::vector<float>>& embeddings,
    int max_hops,
    float frontier_threshold
) {
    std::vector<QueryResult> path;
    std::unordered_set<int> visited;
    
    std::vector<float> current_query = query_embedding;
    
    for (int hop = 0; hop < max_hops; ++hop) {
        // Get active frontier
        auto active_nodes = activation_field.get_active_nodes(frontier_threshold);
        
        if (active_nodes.empty()) {
            break;
        }
        
        // Find best node based on attention
        int best_node = -1;
        float best_attention = -1.0f;
        
        for (const auto& active_pair : active_nodes) {
            int node_id = active_pair.first;
            float activation = active_pair.second;
            
            if (visited.count(node_id) > 0) {
                continue;
            }
            
            auto emb_it = embeddings.find(node_id);
            if (emb_it == embeddings.end()) {
                continue;
            }
            
            float attention = compute_attention(current_query, emb_it->second, emb_it->second);
            attention *= activation;  // Weight by activation
            
            if (attention > best_attention) {
                best_attention = attention;
                best_node = node_id;
            }
        }
        
        if (best_node == -1) {
            break;
        }
        
        // Add to path
        QueryResult result;
        result.node_id = best_node;
        result.attention_score = best_attention;
        result.hop_number = hop;
        path.push_back(result);
        
        visited.insert(best_node);
        
        // Spread activation to neighbors
        auto graph_it = graph.find(best_node);
        if (graph_it != graph.end()) {
            for (const auto& edge : graph_it->second) {
                activation_field.activate(edge.first, edge.second * 0.3f);
            }
        }
        
        // Update query embedding (blend with current node)
        auto emb_it = embeddings.find(best_node);
        if (emb_it != embeddings.end()) {
            for (size_t i = 0; i < std::min(current_query.size(), emb_it->second.size()); ++i) {
                current_query[i] = current_query[i] * 0.7f + emb_it->second[i] * 0.3f;
            }
        }
    }
    
    return path;
}

} // namespace reasoning
} // namespace melvin
