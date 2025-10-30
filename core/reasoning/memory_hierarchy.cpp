#include "memory_hierarchy.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace melvin {
namespace reasoning {

MemoryHierarchy::MemoryHierarchy() {
}

void MemoryHierarchy::add_to_working_memory(const std::vector<int>& sequence) {
    if (!sequence.empty()) {
        working_memory_.push_back(sequence);
        if (working_memory_.size() > 10) {
            working_memory_.pop_front();
        }
    }
}

void MemoryHierarchy::record_episode(const std::vector<int>& activation_sequence) {
    if (!activation_sequence.empty()) {
        episodic_traces_.push_back(activation_sequence);
        if (episodic_traces_.size() > 100) {
            episodic_traces_.pop_front();
        }
    }
}

std::unordered_map<int, std::vector<std::pair<int, float>>> MemoryHierarchy::build_context_subgraph(
    const std::vector<float>& query_embedding,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    const std::unordered_map<int, std::vector<float>>& embeddings,
    int top_k
) {
    std::unordered_set<int> context_nodes;
    
    // 1. Retrieve semantically similar nodes
    if (!query_embedding.empty() && !embeddings.empty()) {
        std::vector<std::pair<int, float>> similarities;
        
        for (const auto& emb_pair : embeddings) {
            int node_id = emb_pair.first;
            const auto& node_emb = emb_pair.second;
            
            if (node_emb.empty()) continue;
            
            // Cosine similarity
            float dot = 0.0f, norm1 = 0.0f, norm2 = 0.0f;
            for (size_t i = 0; i < std::min(query_embedding.size(), node_emb.size()); ++i) {
                dot += query_embedding[i] * node_emb[i];
                norm1 += query_embedding[i] * query_embedding[i];
                norm2 += node_emb[i] * node_emb[i];
            }
            
            float similarity = dot / (std::sqrt(norm1) * std::sqrt(norm2) + 1e-8f);
            similarities.emplace_back(node_id, similarity);
        }
        
        // Sort and take top-k
        std::sort(similarities.begin(), similarities.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        for (size_t i = 0; i < std::min(similarities.size(), static_cast<size_t>(top_k)); ++i) {
            context_nodes.insert(similarities[i].first);
        }
    }
    
    // 2. Add nodes from working memory
    for (const auto& seq : working_memory_) {
        for (int node_id : seq) {
            context_nodes.insert(node_id);
        }
    }
    
    // 3. Materialize subgraph
    std::unordered_map<int, std::vector<std::pair<int, float>>> context_graph;
    
    for (int node_id : context_nodes) {
        auto it = graph.find(node_id);
        if (it != graph.end()) {
            // Filter edges to only include context nodes
            std::vector<std::pair<int, float>> filtered_edges;
            for (const auto& edge : it->second) {
                if (context_nodes.count(edge.first) > 0) {
                    filtered_edges.push_back(edge);
                }
            }
            context_graph[node_id] = filtered_edges;
        }
    }
    
    // 4. Bias activation field towards context
    for (int node_id : context_nodes) {
        activation_field_.activate(node_id, 0.5f);
    }
    
    return context_graph;
}

} // namespace reasoning
} // namespace melvin
