/**
 * @file graph_api.h
 * @brief Simple global API for graph operations
 */

#ifndef MELVIN_GRAPH_API_H
#define MELVIN_GRAPH_API_H

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace melvin {
namespace core {

// Global graph data
extern std::unordered_map<std::string, int> g_token_to_id;
extern std::unordered_map<int, std::string> g_id_to_token;
extern std::unordered_map<int, std::vector<std::pair<int, float>>> g_edges;
extern std::unordered_map<int, std::vector<float>> g_embeddings;
extern std::mutex g_graph_mutex;
extern int g_next_node_id;

// Get or create node ID
inline int get_node_id(const std::string& token) {
    std::lock_guard<std::mutex> lock(g_graph_mutex);
    auto it = g_token_to_id.find(token);
    if (it != g_token_to_id.end()) {
        return it->second;
    }
    int new_id = g_next_node_id++;
    g_token_to_id[token] = new_id;
    g_id_to_token[new_id] = token;
    // Initialize empty embedding
    g_embeddings[new_id] = std::vector<float>(128, 0.0f);
    return new_id;
}

// Add edge
inline void add_edge(const std::string& from_token, const std::string& to_token, float weight) {
    std::lock_guard<std::mutex> lock(g_graph_mutex);
    int from_id = get_node_id(from_token);
    int to_id = get_node_id(to_token);
    
    // Add edge if not exists or update weight if higher
    auto& edges_from = g_edges[from_id];
    bool found = false;
    for (auto& edge : edges_from) {
        if (edge.first == to_id) {
            edge.second = std::max(edge.second, weight);
            found = true;
            break;
        }
    }
    if (!found) {
        edges_from.push_back({to_id, weight});
    }
}

// Get all nodes
inline const std::unordered_map<std::string, int>& get_all_nodes() {
    return g_token_to_id;
}

// Get all edges
inline const std::unordered_map<int, std::vector<std::pair<int, float>>>& get_all_edges() {
    return g_edges;
}

// Get all embeddings
inline const std::unordered_map<int, std::vector<float>>& get_all_embeddings() {
    return g_embeddings;
}

// Get token to ID map
inline const std::unordered_map<std::string, int>& get_token_to_id_map() {
    return g_token_to_id;
}

// Get ID to token map
inline const std::unordered_map<int, std::string>& get_id_to_token_map() {
    return g_id_to_token;
}

// Set graph data (for loading)
inline void set_graph_data(
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& edges,
    const std::unordered_map<int, std::vector<float>>& embeddings,
    const std::unordered_map<std::string, int>& token_to_id,
    const std::unordered_map<int, std::string>& id_to_token
) {
    std::lock_guard<std::mutex> lock(g_graph_mutex);
    g_edges = edges;
    g_embeddings = embeddings;
    g_token_to_id = token_to_id;
    g_id_to_token = id_to_token;
    
    // Update next ID
    g_next_node_id = 0;
    for (const auto& pair : g_id_to_token) {
        g_next_node_id = std::max(g_next_node_id, pair.first + 1);
    }
}

} // namespace core
} // namespace melvin

#endif // MELVIN_GRAPH_API_H

