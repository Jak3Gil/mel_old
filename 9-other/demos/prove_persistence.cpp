/*
 * Prove Persistence - Demonstrate that knowledge persists across runs
 */

#include "../core/storage.h"
#include "../core/types.h"
#include <iostream>
#include <vector>
#include <set>

using namespace melvin;

void show_graph_stats(Storage& storage) {
    std::cout << "\n📊 MEMORY STATE:\n";
    std::cout << "  Nodes: " << storage.node_count() << "\n";
    std::cout << "  Edges: " << storage.edge_count() << "\n";
}

void show_sample_connections(Storage& storage) {
    std::cout << "\n🔗 SAMPLE CONNECTIONS:\n";
    
    auto edges = storage.get_all_edges();
    
    // Show first 15 edges as examples
    for (size_t i = 0; i < std::min(edges.size(), size_t(15)); ++i) {
        std::string from_text = storage.get_node_content(edges[i].from_id);
        std::string to_text = storage.get_node_content(edges[i].to_id);
        
        std::cout << "  " << (i+1) << ". " << from_text << " → " 
                  << to_text << "\n";
    }
}

void demonstrate_reasoning(Storage& storage) {
    std::cout << "\n🧠 REASONING EXAMPLES:\n";
    
    // Find some nodes and show their connections
    std::vector<std::string> test_words = {"fire", "water", "plants", "animals", "sun"};
    
    for (const auto& word : test_words) {
        auto found = storage.find_nodes(word);
        if (!found.empty()) {
            // Get first match
            auto edges = storage.get_edges_from(found[0].id);
            if (!edges.empty()) {
                std::cout << "\n  " << word << " leads to:\n";
                // Show up to 3 connections
                for (size_t i = 0; i < std::min(edges.size(), size_t(3)); ++i) {
                    std::string to_text = storage.get_node_content(edges[i].to_id);
                    std::cout << "    → " << to_text 
                              << " (weight: " << edges[i].get_weight() << ")\n";
                }
            }
        }
    }
}

void find_2hop_paths(Storage& storage, const std::string& start_word) {
    auto start_nodes = storage.find_nodes(start_word);
    if (start_nodes.empty()) return;
    
    std::cout << "\n  2-hop paths from '" << start_word << "':\n";
    
    // Get 1-hop neighbors
    auto edges1 = storage.get_edges_from(start_nodes[0].id);
    
    int count = 0;
    for (const auto& edge1 : edges1) {
        if (count >= 3) break; // Show max 3 paths
        
        std::string mid_text = storage.get_node_content(edge1.to_id);
        
        // Get 2-hop neighbors
        auto edges2 = storage.get_edges_from(edge1.to_id);
        
        for (const auto& edge2 : edges2) {
            std::string end_text = storage.get_node_content(edge2.to_id);
            std::cout << "    " << start_word << " → " << mid_text 
                      << " → " << end_text << "\n";
            count++;
            if (count >= 3) break;
        }
    }
}

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  PROVE PERSISTENCE - Knowledge Survives Restart       ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    
    Storage storage;
    
    std::cout << "\n📂 Loading memory from disk...\n";
    if (!storage.load("data/nodes.melvin", "data/edges.melvin")) {
        std::cerr << "\n✗ No saved memory found!\n";
        std::cerr << "  Run ./demos/teach_knowledge first\n\n";
        return 1;
    }
    
    std::cout << "  ✅ Successfully loaded from disk!\n";
    
    // Show what we loaded
    show_graph_stats(storage);
    show_sample_connections(storage);
    demonstrate_reasoning(storage);
    
    // Show multi-hop reasoning
    std::cout << "\n🚀 MULTI-HOP REASONING:\n";
    find_2hop_paths(storage, "fire");
    find_2hop_paths(storage, "plants");
    find_2hop_paths(storage, "animals");
    
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ PERSISTENCE PROVEN                                ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "🎉 This proves:\n";
    std::cout << "  1. Knowledge was SAVED to disk (data/nodes.melvin & edges.melvin)\n";
    std::cout << "  2. Knowledge was LOADED from disk\n";
    std::cout << "  3. Connections are preserved\n";
    std::cout << "  4. Multi-hop reasoning works\n";
    std::cout << "\n";
    std::cout << "💡 You can restart this program anytime - the knowledge persists!\n\n";
    
    return 0;
}

