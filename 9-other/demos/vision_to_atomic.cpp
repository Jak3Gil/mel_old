/*
 * Example: How to add vision data to AtomicGraph
 * Shows practical usage for real vision system
 */

#include "../core/atomic_graph.h"
#include <iostream>

using namespace melvin;

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Practical Example: Vision Data → AtomicGraph                  ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    // Create or load existing graph
    AtomicGraph graph;
    graph.load("melvin/data/atomic_nodes.bin", "melvin/data/atomic_edges.bin");
    
    std::cout << "Starting graph state:\n";
    std::cout << "  Nodes: " << graph.node_count() << "\n";
    std::cout << "  Edges: " << graph.edge_count() << "\n\n";
    
    // Simulate vision frame detection
    std::cout << "╔═══ FRAME 1 ═══╗\n";
    std::cout << "Detected: person, laptop\n\n";
    
    // Create concept nodes (or reuse if exist)
    auto person_c = graph.get_or_create_node("person", 0);
    auto laptop_c = graph.get_or_create_node("laptop", 0);
    
    // Create instance nodes (specific observations)
    auto p1 = graph.get_or_create_node("obs:person:frame1", 1);
    auto l1 = graph.get_or_create_node("obs:laptop:frame1", 1);
    
    // Link instances to concepts (IS-A relationships)
    graph.add_edge(p1, person_c, INSTANCE_OF);
    graph.add_edge(l1, laptop_c, INSTANCE_OF);
    
    // Co-occurrence (they appeared together!)
    graph.add_edge(p1, l1, CO_OCCURS_WITH, 1.0f);
    
    std::cout << "Added to graph:\n";
    std::cout << "  person (concept) ← obs:person:frame1 (instance)\n";
    std::cout << "  laptop (concept) ← obs:laptop:frame1 (instance)\n";
    std::cout << "  obs:person ←[1]→ obs:laptop (co-occurred)\n\n";
    
    // Next frame
    std::cout << "╔═══ FRAME 2 ═══╗\n";
    std::cout << "Detected: person, laptop (same objects!)\n\n";
    
    auto p2 = graph.get_or_create_node("obs:person:frame2", 1);
    auto l2 = graph.get_or_create_node("obs:laptop:frame2", 1);
    
    graph.add_edge(p2, person_c, INSTANCE_OF);
    graph.add_edge(l2, laptop_c, INSTANCE_OF);
    graph.add_edge(p2, l2, CO_OCCURS_WITH, 1.0f);
    
    // Temporal edges (frame1 → frame2)
    graph.add_edge(p1, p2, TEMPORAL_NEXT, 0.5f);
    graph.add_edge(l1, l2, TEMPORAL_NEXT, 0.5f);
    
    std::cout << "Added:\n";
    std::cout << "  New instances for frame 2\n";
    std::cout << "  Temporal edges: frame1 → frame2\n\n";
    
    // Another frame with cup
    std::cout << "╔═══ FRAME 3 ═══╗\n";
    std::cout << "Detected: person, laptop, cup (new!)\n\n";
    
    auto p3 = graph.get_or_create_node("obs:person:frame3", 1);
    auto l3 = graph.get_or_create_node("obs:laptop:frame3", 1);
    auto cup_c = graph.get_or_create_node("cup", 0);  // New concept!
    auto c3 = graph.get_or_create_node("obs:cup:frame3", 1);
    
    graph.add_edge(p3, person_c, INSTANCE_OF);
    graph.add_edge(l3, laptop_c, INSTANCE_OF);
    graph.add_edge(c3, cup_c, INSTANCE_OF);
    
    // Co-occurrences
    graph.add_edge(p3, l3, CO_OCCURS_WITH, 1.0f);
    graph.add_edge(p3, c3, CO_OCCURS_WITH, 1.0f);
    graph.add_edge(l3, c3, CO_OCCURS_WITH, 1.0f);
    
    std::cout << "Added:\n";
    std::cout << "  New concept: cup\n";
    std::cout << "  3-way co-occurrence: person, laptop, cup\n\n";
    
    // Show final state
    std::cout << "╔═══════════════════════════════════════════╗\n";
    std::cout << "║  Final Graph State                        ║\n";
    std::cout << "╚═══════════════════════════════════════════╝\n\n";
    
    graph.print_stats();
    
    // Query: What appears with person?
    std::cout << "Query: What objects appear with 'person'?\n\n";
    
    auto person_neighbors = graph.neighbors(person_c, 255);  // All relations
    std::cout << "Direct connections to 'person' concept: " << person_neighbors.size() << "\n";
    
    // Count co-occurrences through instances
    std::cout << "\nCo-occurrence analysis:\n";
    std::cout << "  person ←→ laptop: 3 frames\n";
    std::cout << "  person ←→ cup: 1 frame\n\n";
    
    // Save
    std::cout << "Saving graph...\n";
    graph.save("example_vision_nodes.bin", "example_vision_edges.bin");
    std::cout << "  ✓ Saved to example_vision_*.bin\n\n";
    
    std::cout << "✨ This is how vision data gets added to Melvin's brain!\n";
    std::cout << "   - Concepts reused across frames\n";
    std::cout << "   - Instances track specific observations\n";
    std::cout << "   - Edges accumulate weights (association strength)\n";
    std::cout << "   - Graph grows continuously!\n\n";
    
    return 0;
}

