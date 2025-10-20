/*
 * Diagnose graph structure to see why LEAPs aren't forming
 */

#include "melvin/core/storage.h"
#include <iostream>
#include <map>

using namespace melvin;

int main() {
    auto storage = std::make_unique<Storage>();
    storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    
    std::cout << "Graph Structure Analysis:\n\n";
    
    auto all_edges = storage->get_all_edges();
    auto all_nodes = storage->get_all_nodes();
    
    std::cout << "Total nodes: " << all_nodes.size() << "\n";
    std::cout << "Total edges: " << all_edges.size() << "\n\n";
    
    // Show first 20 edges to see structure
    std::cout << "Sample edges:\n";
    for (size_t i = 0; i < std::min(size_t(20), all_edges.size()); i++) {
        const auto& edge = all_edges[i];
        std::cout << "  " << storage->get_node_content(edge.from_id)
                  << " → " << storage->get_node_content(edge.to_id)
                  << " (" << (edge.relation == RelationType::EXACT ? "EXACT" : "LEAP") << ")\n";
    }
    
    // Check for 2-hop chains
    std::cout << "\nChecking for 2-hop chains (A→B→C):\n";
    int chains_found = 0;
    
    for (const auto& edge1 : all_edges) {
        if (edge1.relation != RelationType::EXACT) continue;
        
        NodeID middle = edge1.to_id;
        auto edges2 = storage->get_edges_from(middle);
        
        if (!edges2.empty()) {
            for (const auto& edge2 : edges2) {
                if (edge2.relation == RelationType::EXACT) {
                    std::cout << "  " << storage->get_node_content(edge1.from_id)
                              << " → " << storage->get_node_content(middle)
                              << " → " << storage->get_node_content(edge2.to_id) << "\n";
                    chains_found++;
                    if (chains_found >= 10) break;
                }
            }
        }
        if (chains_found >= 10) break;
    }
    
    if (chains_found == 0) {
        std::cout << "  ❌ No 2-hop chains found!\n";
        std::cout << "\nDiagnosing why...\n";
        
        // Check node connectivity
        std::map<int, int> out_degree;
        std::map<int, int> in_degree;
        
        for (const auto& edge : all_edges) {
            out_degree[edge.from_id]++;
            in_degree[edge.to_id]++;
        }
        
        int nodes_with_out = 0, nodes_with_in = 0;
        for (const auto& node : all_nodes) {
            if (out_degree[node.id] > 0) nodes_with_out++;
            if (in_degree[node.id] > 0) nodes_with_in++;
        }
        
        std::cout << "  Nodes with outgoing edges: " << nodes_with_out << "\n";
        std::cout << "  Nodes with incoming edges: " << nodes_with_in << "\n";
        std::cout << "  Nodes with both: " << (nodes_with_out + nodes_with_in - all_nodes.size()) << "\n";
        
        // Show nodes that could be middle nodes (have both in and out)
        std::cout << "\nNodes that could form chains (have both in & out):\n";
        int count = 0;
        for (const auto& node : all_nodes) {
            if (out_degree[node.id] > 0 && in_degree[node.id] > 0) {
                std::cout << "  " << storage->get_node_content(node.id)
                          << " (in: " << in_degree[node.id]
                          << ", out: " << out_degree[node.id] << ")\n";
                count++;
                if (count >= 10) break;
            }
        }
        
        if (count == 0) {
            std::cout << "  ❌ No nodes have both incoming AND outgoing edges!\n";
            std::cout << "  This means the graph is structured as disconnected pairs, not chains.\n";
        }
    } else {
        std::cout << "\n✅ Found " << chains_found << " 2-hop chains!\n";
    }
    
    return 0;
}

