/*
 * Inspect Memory - Show what's actually stored
 */

#include "../core/storage.h"
#include "../core/types.h"
#include <iostream>
#include <algorithm>

using namespace melvin;

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  MEMORY INSPECTOR                                     ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    Storage storage;
    
    std::cout << "Loading memory...\n";
    if (!storage.load("data/nodes.melvin", "data/edges.melvin")) {
        std::cerr << "✗ Failed to load\n";
        return 1;
    }
    
    std::cout << "  ✓ Loaded " << storage.node_count() << " nodes, " 
              << storage.edge_count() << " edges\n\n";
    
    // Show all nodes
    std::cout << "═══ NODES ═══\n";
    auto nodes = storage.get_all_nodes();
    std::sort(nodes.begin(), nodes.end(), 
        [](const Node& a, const Node& b) { return a.content < b.content; });
    
    for (const auto& node : nodes) {
        std::cout << "  • " << node.content << "\n";
    }
    
    std::cout << "\n═══ EDGES (first 20) ═══\n";
    auto edges = storage.get_all_edges();
    
    for (size_t i = 0; i < std::min(edges.size(), size_t(20)); ++i) {
        Node from, to;
        if (storage.get_node(edges[i].from_id, from) && 
            storage.get_node(edges[i].to_id, to)) {
            std::cout << "  " << from.content << " → " << to.content 
                      << " (weight: " << edges[i].weight << ")\n";
        }
    }
    
    // Show some specific connections
    std::cout << "\n═══ SPECIFIC PATHS ═══\n";
    
    std::vector<std::string> test_concepts = {"fire", "heat", "warmth", "sunlight", "energy", "growth"};
    
    for (const auto& test_concept : test_concepts) {
        auto found = storage.find_nodes(test_concept);
        if (!found.empty()) {
            auto edges_from = storage.get_edges_from(found[0].id);
            if (!edges_from.empty()) {
                std::cout << "\n" << test_concept << " connects to:\n";
                for (const auto& edge : edges_from) {
                    Node to;
                    if (storage.get_node(edge.to_id, to)) {
                        std::cout << "  → " << to.content << "\n";
                    }
                }
            }
        }
    }
    
    std::cout << "\n✅ Inspection complete\n\n";
    
    return 0;
}

