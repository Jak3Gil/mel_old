#include "melvin.h"
#include <iostream>

int main() {
    std::cout << "🧠 Testing Melvin Weights & Associations System\n";
    std::cout << "===============================================\n\n";
    
    // Initialize Melvin
    melvin_t* melvin = melvin_create(nullptr);
    
    std::cout << "✅ Initial state: " << melvin_node_count(melvin) << " nodes, " 
              << melvin_edge_count(melvin) << " edges\n\n";
    
    // Learn some knowledge with repeated patterns
    std::cout << "📚 Learning knowledge with repeated patterns...\n";
    
    // Teach "cats are mammals" multiple times to build weight
    for (int i = 0; i < 5; ++i) {
        melvin_learn(melvin, "cats are mammals");
    }
    
    // Teach "cats have tails" fewer times
    for (int i = 0; i < 2; ++i) {
        melvin_learn(melvin, "cats have tails");
    }
    
    // Teach "cats drink milk" once
    melvin_learn(melvin, "cats drink milk");
    
    std::cout << "✅ After learning: " << melvin_node_count(melvin) << " nodes, " 
              << melvin_edge_count(melvin) << " edges\n\n";
    
    // Debug the "cats" node connections
    std::cout << "🔍 Debugging 'cats' node connections:\n";
    melvin_debug_node(melvin, "cats");
    std::cout << "\n";
    
    // Ask questions to test reasoning
    std::cout << "❓ Testing reasoning with weighted connections:\n";
    
    const char* queries[] = {
        "What are cats?",
        "What do cats have?", 
        "What do cats drink?"
    };
    
    for (const auto& query : queries) {
        std::cout << "Q: " << query << "\n";
        const char* answer = melvin_reason(melvin, query);
        std::cout << "A: " << answer << "\n\n";
    }
    
    // Run a decay pass to test decay functionality
    std::cout << "⏰ Running decay pass...\n";
    melvin_decay_pass(melvin);
    
    std::cout << "🔍 Debugging 'cats' node connections after decay:\n";
    melvin_debug_node(melvin, "cats");
    std::cout << "\n";
    
    // Cleanup
    melvin_destroy(melvin);
    
    std::cout << "✅ Test complete!\n";
    
    return 0;
}
