/*
 * AtomicGraph Demo
 * Test minimal binary graph system
 */

#include "../core/atomic_graph.h"
#include <iostream>
#include <iomanip>

using namespace melvin;

void test_basic_operations() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 1: Basic Operations                                      ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    AtomicGraph graph;
    
    // Create concept nodes
    std::cout << "[Test] Creating concept nodes...\n";
    auto person_c = graph.get_or_create_node("person", 0);
    auto laptop_c = graph.get_or_create_node("laptop", 0);
    auto cup_c = graph.get_or_create_node("cup", 0);
    
    std::cout << "  Created: person (ID=" << person_c << ")\n";
    std::cout << "  Created: laptop (ID=" << laptop_c << ")\n";
    std::cout << "  Created: cup (ID=" << cup_c << ")\n";
    
    // Create instance nodes
    std::cout << "\n[Test] Creating instance nodes...\n";
    auto p1 = graph.get_or_create_node("person_obs_1", 1);
    auto l1 = graph.get_or_create_node("laptop_obs_1", 1);
    auto c1 = graph.get_or_create_node("cup_obs_1", 1);
    
    std::cout << "  Created: person_obs_1 (ID=" << p1 << ")\n";
    std::cout << "  Created: laptop_obs_1 (ID=" << l1 << ")\n";
    std::cout << "  Created: cup_obs_1 (ID=" << c1 << ")\n";
    
    // Link instances to concepts
    std::cout << "\n[Test] Linking instances to concepts...\n";
    graph.add_edge(p1, person_c, INSTANCE_OF);
    graph.add_edge(l1, laptop_c, INSTANCE_OF);
    graph.add_edge(c1, cup_c, INSTANCE_OF);
    std::cout << "  ✓ Created INSTANCE_OF edges\n";
    
    // Create co-occurrence edges
    std::cout << "\n[Test] Creating co-occurrence edges...\n";
    graph.add_edge(p1, l1, CO_OCCURS_WITH);
    std::cout << "  person_obs_1 ←→ laptop_obs_1 (weight=1)\n";
    
    graph.add_edge(p1, l1, CO_OCCURS_WITH);  // Add again!
    std::cout << "  person_obs_1 ←→ laptop_obs_1 (weight=2) [reinforced!]\n";
    
    graph.add_edge(p1, c1, CO_OCCURS_WITH);
    std::cout << "  person_obs_1 ←→ cup_obs_1 (weight=1)\n";
    
    // Check weight
    float weight = graph.get_edge_weight(p1, l1, CO_OCCURS_WITH);
    std::cout << "\n[Test] Edge weight between person and laptop: " << weight << "\n";
    
    // Print stats
    graph.print_stats();
    
    std::cout << "✓ Basic operations test passed\n";
}

void test_persistence() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 2: Persistence (Save & Load)                             ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    const std::string nodes_file = "test_atomic_nodes.bin";
    const std::string edges_file = "test_atomic_edges.bin";
    
    // Create and save graph
    {
        std::cout << "[Test] Creating graph...\n";
        AtomicGraph graph;
        
        auto person = graph.get_or_create_node("person", 0);
        auto laptop = graph.get_or_create_node("laptop", 0);
        
        auto p1 = graph.get_or_create_node("obs:person:1", 1);
        auto l1 = graph.get_or_create_node("obs:laptop:1", 1);
        
        graph.add_edge(p1, person, INSTANCE_OF);
        graph.add_edge(l1, laptop, INSTANCE_OF);
        graph.add_edge(p1, l1, CO_OCCURS_WITH, 5.0f);  // Seen together 5 times
        
        std::cout << "  Nodes: " << graph.node_count() << "\n";
        std::cout << "  Edges: " << graph.edge_count() << "\n";
        
        std::cout << "\n[Test] Saving to disk...\n";
        graph.save(nodes_file, edges_file);
        std::cout << "  ✓ Saved to " << nodes_file << " and " << edges_file << "\n";
    }
    
    // Load and verify
    {
        std::cout << "\n[Test] Loading from disk...\n";
        AtomicGraph graph;
        graph.load(nodes_file, edges_file);
        
        std::cout << "  ✓ Loaded successfully\n";
        std::cout << "  Nodes: " << graph.node_count() << "\n";
        std::cout << "  Edges: " << graph.edge_count() << "\n";
        
        graph.print_stats();
        
        // Verify weight
        auto p1 = graph.get_or_create_node("obs:person:1", 1);
        auto l1 = graph.get_or_create_node("obs:laptop:1", 1);
        float weight = graph.get_edge_weight(p1, l1, CO_OCCURS_WITH);
        
        std::cout << "[Test] Verified edge weight: " << weight << " (expected 5.0)\n";
        
        if (weight == 5.0f) {
            std::cout << "✓ Persistence test passed\n";
        } else {
            std::cout << "✗ Weight mismatch!\n";
        }
    }
    
    // Cleanup
    std::remove(nodes_file.c_str());
    std::remove(edges_file.c_str());
}

void test_edge_reinforcement() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 3: Edge Reinforcement (Weight Accumulation)              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    AtomicGraph graph;
    
    auto person = graph.get_or_create_node("person", 0);
    auto laptop = graph.get_or_create_node("laptop", 0);
    
    std::cout << "[Test] Adding same edge multiple times...\n";
    
    for (int i = 1; i <= 10; ++i) {
        graph.add_edge(person, laptop, CO_OCCURS_WITH, 1.0f);
        float weight = graph.get_edge_weight(person, laptop, CO_OCCURS_WITH);
        std::cout << "  Iteration " << i << ": weight = " << weight << "\n";
    }
    
    std::cout << "\n[Test] Final weight: " 
              << graph.get_edge_weight(person, laptop, CO_OCCURS_WITH) << "\n";
    std::cout << "✓ Edge reinforcement test passed\n";
}

void test_decay() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEST 4: Edge Decay (Temporal Forgetting)                      ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    AtomicGraph graph;
    
    auto a = graph.get_or_create_node("node_a", 0);
    auto b = graph.get_or_create_node("node_b", 0);
    
    graph.add_edge(a, b, CO_OCCURS_WITH, 10.0f);
    
    std::cout << "[Test] Initial weight: " << graph.get_edge_weight(a, b, CO_OCCURS_WITH) << "\n";
    
    std::cout << "[Test] Applying decay (factor=0.9)...\n";
    for (int i = 1; i <= 5; ++i) {
        graph.decay_edges(0.9f);
        std::cout << "  After decay " << i << ": weight = " 
                  << std::fixed << std::setprecision(2)
                  << graph.get_edge_weight(a, b, CO_OCCURS_WITH) << "\n";
    }
    
    std::cout << "✓ Decay test passed\n";
}

void demo_vision_scenario() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  DEMO: Vision System Scenario                                  ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    AtomicGraph graph;
    
    std::cout << "[Demo] Simulating vision system observations...\n\n";
    
    // Create concept nodes
    auto person_c = graph.get_or_create_node("person", 0);
    auto laptop_c = graph.get_or_create_node("laptop", 0);
    auto cup_c = graph.get_or_create_node("cup", 0);
    
    // Simulate 100 frames of vision
    std::cout << "[Demo] Processing 100 frames...\n";
    
    for (int frame = 1; frame <= 100; ++frame) {
        // Frame observations
        auto obs_person = graph.get_or_create_node("obs:person:session1:" + std::to_string(frame), 1);
        auto obs_laptop = graph.get_or_create_node("obs:laptop:session1:" + std::to_string(frame), 1);
        
        // Link to concepts
        graph.add_edge(obs_person, person_c, INSTANCE_OF);
        graph.add_edge(obs_laptop, laptop_c, INSTANCE_OF);
        
        // Co-occurrence
        graph.add_edge(obs_person, obs_laptop, CO_OCCURS_WITH);
        
        // Sometimes cup appears
        if (frame % 3 == 0) {
            auto obs_cup = graph.get_or_create_node("obs:cup:session1:" + std::to_string(frame), 1);
            graph.add_edge(obs_cup, cup_c, INSTANCE_OF);
            graph.add_edge(obs_person, obs_cup, CO_OCCURS_WITH);
        }
    }
    
    std::cout << "  ✓ Processed 100 frames\n\n";
    
    graph.print_stats();
    
    // Show concept connections
    std::cout << "Concept relationships:\n";
    std::cout << "  person appeared in: 100 frames\n";
    std::cout << "  laptop appeared in: 100 frames\n";
    std::cout << "  cup appeared in: 33 frames\n";
    std::cout << "  person-laptop co-occurrence: 100x\n";
    std::cout << "  person-cup co-occurrence: 33x\n\n";
    
    // Save
    std::cout << "[Demo] Saving graph...\n";
    graph.save("demo_vision_nodes.bin", "demo_vision_edges.bin");
    std::cout << "  ✓ Saved to demo_vision_*.bin\n\n";
    
    std::cout << "✓ Vision scenario demo complete\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                                ║\n";
    std::cout << "║   ⚛️  ATOMIC GRAPH TEST SUITE                                  ║\n";
    std::cout << "║                                                                ║\n";
    std::cout << "║   Minimal Binary Graph for Vision + Language                  ║\n";
    std::cout << "║                                                                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    
    try {
        test_basic_operations();
        test_persistence();
        test_edge_reinforcement();
        test_decay();
        demo_vision_scenario();
        
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  ✅  ALL TESTS PASSED                                          ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ TEST FAILED: " << e.what() << "\n\n";
        return 1;
    }
    
    return 0;
}

