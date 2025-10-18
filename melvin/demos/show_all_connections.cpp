/*
 * Show All Connections - Display complete knowledge graph
 */

#include "../core/storage.h"
#include "../core/types.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <set>

using namespace melvin;

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  COMPLETE CONNECTION LIST                             ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    Storage storage;
    
    if (!storage.load("data/nodes.melvin", "data/edges.melvin")) {
        std::cerr << "✗ Failed to load\n";
        return 1;
    }
    
    std::cout << "📊 GRAPH STATISTICS:\n";
    std::cout << "  Nodes (concepts):    " << storage.node_count() << "\n";
    std::cout << "  Edges (connections): " << storage.edge_count() << "\n\n";
    
    // Get all edges and organize by relation type
    auto edges = storage.get_all_edges();
    
    std::map<RelationType, std::vector<Edge>> by_relation;
    for (const auto& edge : edges) {
        by_relation[edge.relation].push_back(edge);
    }
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  ALL " << edges.size() << " CONNECTIONS:\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    // Show all connections
    for (size_t i = 0; i < edges.size(); ++i) {
        std::string from = storage.get_node_content(edges[i].from_id);
        std::string to = storage.get_node_content(edges[i].to_id);
        float weight = edges[i].get_weight();
        
        std::cout << "  " << std::setw(3) << (i+1) << ". "
                  << std::setw(15) << from << " → "
                  << std::setw(15) << to 
                  << " (weight: " << std::fixed << std::setprecision(2) << weight << ")\n";
    }
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  BREAKDOWN BY RELATION TYPE:\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    const char* relation_names[] = {
        "SEMANTIC", "CAUSAL", "ATTRIBUTE", "SEQUENCE", 
        "ABSTRACTION", "REINFORCEMENT", "MULTIMODAL"
    };
    
    for (const auto& [rel, rel_edges] : by_relation) {
        std::cout << "  " << relation_names[static_cast<int>(rel)] 
                  << ": " << rel_edges.size() << " connections\n";
    }
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  CONNECTIVITY ANALYSIS:\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    // Count outgoing edges per node
    std::map<NodeID, int> out_degree;
    std::map<NodeID, int> in_degree;
    
    for (const auto& edge : edges) {
        out_degree[edge.from_id]++;
        in_degree[edge.to_id]++;
    }
    
    // Find nodes with most connections
    std::vector<std::pair<int, NodeID>> by_out_degree;
    for (const auto& [node_id, degree] : out_degree) {
        by_out_degree.push_back({degree, node_id});
    }
    std::sort(by_out_degree.begin(), by_out_degree.end(), std::greater<>());
    
    std::cout << "  Most connected nodes (outgoing):\n";
    for (size_t i = 0; i < std::min(by_out_degree.size(), size_t(5)); ++i) {
        std::string name = storage.get_node_content(by_out_degree[i].second);
        std::cout << "    " << std::setw(15) << name 
                  << ": " << by_out_degree[i].first << " connections\n";
    }
    
    std::cout << "\n  Average connections per node: " 
              << std::fixed << std::setprecision(1)
              << (float)edges.size() / storage.node_count() << "\n";
    
    std::cout << "\n✅ COMPLETE!\n\n";
    
    return 0;
}

