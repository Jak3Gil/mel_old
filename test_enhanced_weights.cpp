#include "melvin.h"
#include <iostream>

int main() {
    std::cout << "🧠 Testing Enhanced Melvin Weights & Constraint System\n";
    std::cout << "=====================================================\n\n";
    
    // Initialize Melvin
    melvin_t* melvin = melvin_create(nullptr);
    
    std::cout << "✅ Initial state: " << melvin_node_count(melvin) << " nodes, " 
              << melvin_edge_count(melvin) << " edges\n\n";
    
    // Learn knowledge with different patterns to test scoring
    std::cout << "📚 Learning knowledge with varied patterns...\n";
    
    // Teach "cats are mammals" multiple times (high frequency)
    for (int i = 0; i < 8; ++i) {
        melvin_learn(melvin, "cats are mammals");
    }
    
    // Teach "cats are animals" fewer times (medium frequency)
    for (int i = 0; i < 3; ++i) {
        melvin_learn(melvin, "cats are animals");
    }
    
    // Teach "cats are cute" once (low frequency)
    melvin_learn(melvin, "cats are cute");
    
    // Teach some "what" question patterns
    melvin_learn(melvin, "what are cats");
    melvin_learn(melvin, "what do cats eat");
    
    std::cout << "✅ After learning: " << melvin_node_count(melvin) << " nodes, " 
              << melvin_edge_count(melvin) << " edges\n\n";
    
    // Test different driver modes and contexts
    std::cout << "🔍 Testing different driver modes and contexts:\n\n";
    
    // Test with "what" question context (should favor nouns)
    std::cout << "📝 Context: 'what' question (should favor nouns):\n";
    melvin_debug_node_with_context(melvin, "cats", "", "what are cats", "balanced");
    std::cout << "\n";
    
    // Test with "are" context (should favor nouns/adjectives after "are")
    std::cout << "📝 Context: 'are' statement (should favor nouns/adjectives):\n";
    melvin_debug_node_with_context(melvin, "cats", "cats are", "what are cats", "balanced");
    std::cout << "\n";
    
    // Test exploration mode (should boost rare connections)
    std::cout << "🔍 Exploration mode (should boost rare connections):\n";
    melvin_debug_node_with_context(melvin, "cats", "cats are", "what are cats", "exploration");
    std::cout << "\n";
    
    // Test accuracy mode (should strongly favor frequent connections)
    std::cout << "🎯 Accuracy mode (should strongly favor frequent connections):\n";
    melvin_debug_node_with_context(melvin, "cats", "cats are", "what are cats", "accuracy");
    std::cout << "\n";
    
    // Test next word suggestion
    std::cout << "💡 Testing next word suggestions:\n";
    
    const char* suggestions[] = {
        melvin_suggest_next_word(melvin, "cats", "cats are", "what are cats"),
        melvin_suggest_next_word(melvin, "cats", "", "what are cats"),
        melvin_suggest_next_word(melvin, "cats", "", "")
    };
    
    const char* contexts[] = {"cats are", "no context", "no context"};
    const char* queries[] = {"what are cats", "what are cats", "no query"};
    
    for (int i = 0; i < 3; ++i) {
        std::cout << "Context: '" << contexts[i] << "', Query: '" << queries[i] << "'\n";
        std::cout << "Next word: " << (suggestions[i] ? suggestions[i] : "none") << "\n\n";
    }
    
    // Test different driver modes
    std::cout << "🎛️ Testing driver mode switching:\n";
    
    melvin_set_driver_mode(melvin, "exploration");
    std::cout << "Exploration mode - next word: " 
              << (melvin_suggest_next_word(melvin, "cats", "cats are", "") ? melvin_suggest_next_word(melvin, "cats", "cats are", "") : "none") << "\n";
    
    melvin_set_driver_mode(melvin, "accuracy");
    std::cout << "Accuracy mode - next word: " 
              << (melvin_suggest_next_word(melvin, "cats", "cats are", "") ? melvin_suggest_next_word(melvin, "cats", "cats are", "") : "none") << "\n";
    
    melvin_set_driver_mode(melvin, "exploitation");
    std::cout << "Exploitation mode - next word: " 
              << (melvin_suggest_next_word(melvin, "cats", "cats are", "") ? melvin_suggest_next_word(melvin, "cats", "cats are", "") : "none") << "\n\n";
    
    // Test decay
    std::cout << "⏰ Running decay pass...\n";
    melvin_decay_pass(melvin);
    
    std::cout << "🔍 After decay (counts should be slightly reduced):\n";
    melvin_debug_node_with_context(melvin, "cats", "cats are", "what are cats", "balanced");
    std::cout << "\n";
    
    // Cleanup
    melvin_destroy(melvin);
    
    std::cout << "✅ Enhanced weights and constraint system test complete!\n";
    
    return 0;
}
