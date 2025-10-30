/**
 * @file graph_api.cpp
 * @brief Implementation of global graph data
 */

#include "graph_api.h"

namespace melvin {
namespace core {

// Define global graph data
std::unordered_map<std::string, int> g_token_to_id;
std::unordered_map<int, std::string> g_id_to_token;
std::unordered_map<int, std::vector<std::pair<int, float>>> g_edges;
std::unordered_map<int, std::vector<float>> g_embeddings;
std::mutex g_graph_mutex;
int g_next_node_id = 1;

} // namespace core
} // namespace melvin

