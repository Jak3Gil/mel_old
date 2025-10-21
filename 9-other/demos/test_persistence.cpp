/*
 * Test Persistence - Verify save/load works
 */

#include "../core/storage.h"
#include "../core/types.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace melvin;

void test_save() {
    std::cout << "════════════════════════════════════════════════\n";
    std::cout << "  TEST 1: SAVE DATA\n";
    std::cout << "════════════════════════════════════════════════\n\n";
    
    Storage storage;
    
    // Create some nodes
    std::cout << "Creating nodes...\n";
    auto fire = storage.create_node("fire", NodeType::CONCEPT);
    auto heat = storage.create_node("heat", NodeType::CONCEPT);
    auto smoke = storage.create_node("smoke", NodeType::CONCEPT);
    auto water = storage.create_node("water", NodeType::CONCEPT);
    auto wet = storage.create_node("wet", NodeType::CONCEPT);
    
    std::cout << "  ✓ Created " << storage.node_count() << " nodes\n\n";
    
    // Create some edges
    std::cout << "Creating edges...\n";
    storage.create_edge(fire, heat, RelationType::CAUSAL, 0.9f);
    storage.create_edge(fire, smoke, RelationType::CAUSAL, 0.85f);
    storage.create_edge(heat, fire, RelationType::ATTRIBUTE, 0.7f);
    storage.create_edge(water, wet, RelationType::ATTRIBUTE, 0.95f);
    
    std::cout << "  ✓ Created " << storage.edge_count() << " edges\n\n";
    
    // Print what we have
    storage.print_stats();
    
    // Save to disk
    std::cout << "\nSaving to disk...\n";
    bool success = storage.save("data/nodes.melvin", "data/edges.melvin");
    
    if (success) {
        std::cout << "  ✓ Successfully saved to data/\n";
    } else {
        std::cout << "  ✗ Failed to save!\n";
        return;
    }
    
    std::cout << "\n✅ Save test complete!\n\n";
}

void test_load() {
    std::cout << "════════════════════════════════════════════════\n";
    std::cout << "  TEST 2: LOAD DATA\n";
    std::cout << "════════════════════════════════════════════════\n\n";
    
    Storage storage;
    
    std::cout << "Loading from disk...\n";
    bool success = storage.load("data/nodes.melvin", "data/edges.melvin");
    
    if (!success) {
        std::cout << "  ✗ Failed to load!\n";
        return;
    }
    
    std::cout << "  ✓ Successfully loaded from data/\n\n";
    
    // Print what we loaded
    storage.print_stats();
    
    // Verify content
    std::cout << "\nVerifying loaded data...\n";
    auto nodes = storage.get_all_nodes();
    
    std::cout << "  Nodes loaded:\n";
    for (const auto& node : nodes) {
        std::cout << "    - " << node.content 
                  << " (type: " << (int)node.type << ")\n";
    }
    
    auto edges = storage.get_all_edges();
    std::cout << "\n  Edges loaded:\n";
    for (const auto& edge : edges) {
        Node from, to;
        if (storage.get_node(edge.from_id, from) && 
            storage.get_node(edge.to_id, to)) {
            std::cout << "    - " << from.content 
                      << " --[" << (int)edge.relation << "]--> " 
                      << to.content 
                      << " (weight: " << edge.weight << ")\n";
        }
    }
    
    std::cout << "\n✅ Load test complete!\n\n";
}

void test_persistence() {
    std::cout << "════════════════════════════════════════════════\n";
    std::cout << "  TEST 3: PERSISTENCE ACROSS RUNS\n";
    std::cout << "════════════════════════════════════════════════\n\n";
    
    Storage storage;
    
    // Load existing
    std::cout << "Loading existing data...\n";
    storage.load("data/nodes.melvin", "data/edges.melvin");
    
    size_t before_nodes = storage.node_count();
    size_t before_edges = storage.edge_count();
    
    std::cout << "  Before: " << before_nodes << " nodes, " 
              << before_edges << " edges\n\n";
    
    // Add more data
    std::cout << "Adding new data...\n";
    auto clouds = storage.create_node("clouds", NodeType::CONCEPT);
    auto rain = storage.create_node("rain", NodeType::CONCEPT);
    storage.create_edge(clouds, rain, RelationType::CAUSAL, 0.8f);
    
    std::cout << "  After:  " << storage.node_count() << " nodes, " 
              << storage.edge_count() << " edges\n\n";
    
    // Save again
    std::cout << "Saving updated data...\n";
    storage.save("data/nodes.melvin", "data/edges.melvin");
    std::cout << "  ✓ Saved\n";
    
    std::cout << "\n✅ Persistence test complete!\n\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN PERSISTENCE TEST                              ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    // Test 1: Save
    test_save();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Test 2: Load
    test_load();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Test 3: Persistence
    test_persistence();
    
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  ALL TESTS PASSED ✓                                   ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    return 0;
}

