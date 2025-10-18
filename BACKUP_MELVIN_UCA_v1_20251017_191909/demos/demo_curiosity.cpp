/*
 * Curiosity Loop Demo
 * Shows how Melvin investigates unknown objects
 */

#include "../vision/curiosity_loop.h"
#include "../core/atomic_graph.h"
#include <iostream>

using namespace melvin;
using namespace melvin::vision;

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🔍 CURIOSITY LOOP DEMO                                        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    AtomicGraph graph;
    CuriosityLoop curiosity(&graph);
    
    std::cout << "Scenario: Melvin sees unknown objects over time\n\n";
    
    // Frame 1: See something with low confidence
    std::cout << "Frame 1: Detect object with low confidence (0.3)\n";
    auto obj1 = graph.get_or_create_node("obs:mystery:frame1", 1);
    curiosity.observe("unknown", 0.3f, obj1);
    curiosity.update();
    std::cout << "  → Entered curiosity loop\n\n";
    
    // Frames 2-9: Keep seeing it
    for (int i = 2; i <= 9; ++i) {
        std::cout << "Frame " << i << ": Still observing mystery object\n";
        curiosity.observe("unknown", 0.35f, obj1);
        curiosity.update();
    }
    std::cout << "  → Tracking for 9 frames...\n\n";
    
    // Frame 10: Curiosity threshold reached!
    std::cout << "Frame 10: Curiosity duration reached!\n";
    curiosity.observe("unknown", 0.4f, obj1);
    curiosity.update();
    std::cout << "  → Created 'unknown_object' node in graph\n";
    std::cout << "  → Linked to observations\n\n";
    
    curiosity.print_stats();
    
    std::cout << "AtomicGraph state:\n";
    graph.print_stats();
    
    // Later: High similarity match
    std::cout << "\n╔═══ Later Session ═══╗\n";
    std::cout << "Melvin sees similar object with high confidence\n";
    curiosity.reinforce(obj1, 0.85f);
    std::cout << "  → Mystery resolved! (similarity > 0.8)\n\n";
    
    curiosity.print_stats();
    
    // Decay test
    std::cout << "\n╔═══ Decay Over Time ═══╗\n";
    std::cout << "Objects not re-seen fade from memory...\n";
    
    auto obj2 = graph.get_or_create_node("obs:temporary:frame50", 1);
    curiosity.observe("unknown", 0.4f, obj2);
    
    for (int i = 0; i < 20; ++i) {
        curiosity.decay();
    }
    
    std::cout << "  → Temporary object decayed and forgotten\n\n";
    
    curiosity.print_stats();
    
    // Save
    graph.save("curiosity_demo.bin", "curiosity_edges.bin");
    std::cout << "💾 Saved curiosity graph\n\n";
    
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ CURIOSITY DEMO COMPLETE                                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "✨ Melvin now has biological curiosity!\n";
    std::cout << "   - Investigates low-confidence detections\n";
    std::cout << "   - Creates unknown nodes after sustained observation\n";
    std::cout << "   - Resolves mysteries with new evidence\n";
    std::cout << "   - Forgets unimportant objects (decay)\n\n";
    
    return 0;
}

