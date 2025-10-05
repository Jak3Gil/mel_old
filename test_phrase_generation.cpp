#include "melvin.h"
#include <iostream>

int main() {
    std::cout << "🧠 Testing Complete Phrase Generation Loop\n";
    std::cout << "==========================================\n\n";
    
    // Initialize Melvin
    melvin_t* melvin = melvin_create(nullptr);
    
    std::cout << "✅ Initial state: " << melvin_node_count(melvin) << " nodes, " 
              << melvin_edge_count(melvin) << " edges\n\n";
    
    // Learn comprehensive knowledge for phrase generation
    std::cout << "📚 Learning comprehensive knowledge...\n";
    
    // Teach animal facts
    for (int i = 0; i < 10; ++i) {
        melvin_learn(melvin, "cats are mammals");
    }
    for (int i = 0; i < 5; ++i) {
        melvin_learn(melvin, "cats are animals");
    }
    for (int i = 0; i < 3; ++i) {
        melvin_learn(melvin, "cats are pets");
    }
    
    // Teach mammal properties
    for (int i = 0; i < 8; ++i) {
        melvin_learn(melvin, "mammals have fur");
    }
    for (int i = 0; i < 6; ++i) {
        melvin_learn(melvin, "mammals have warm blood");
    }
    for (int i = 0; i < 4; ++i) {
        melvin_learn(melvin, "mammals give birth");
    }
    
    // Teach some ending patterns
    melvin_learn(melvin, "cats are cute");
    melvin_learn(melvin, "cats are smart");
    melvin_learn(melvin, "cats are independent");
    
    std::cout << "✅ After learning: " << melvin_node_count(melvin) << " nodes, " 
              << melvin_edge_count(melvin) << " edges\n\n";
    
    // Test phrase generation with debug enabled
    std::cout << "🔄 Testing phrase generation with debug:\n\n";
    
    const char* phrase1 = melvin_generate_phrase_c(melvin, "cats", 6, "balanced", "deterministic", "", "", 1);
    std::cout << "Generated phrase 1: \"" << phrase1 << "\"\n\n";
    
    // Test different driver modes
    std::cout << "🎛️ Testing different driver modes:\n";
    
    const char* phrase2 = melvin_generate_phrase_c(melvin, "cats", 5, "exploration", "deterministic", "", "", 0);
    std::cout << "Exploration mode: \"" << phrase2 << "\"\n";
    
    const char* phrase3 = melvin_generate_phrase_c(melvin, "cats", 5, "accuracy", "deterministic", "", "", 0);
    std::cout << "Accuracy mode: \"" << phrase3 << "\"\n";
    
    const char* phrase4 = melvin_generate_phrase_c(melvin, "cats", 5, "exploitation", "deterministic", "", "", 0);
    std::cout << "Exploitation mode: \"" << phrase4 << "\"\n\n";
    
    // Test with context and query
    std::cout << "📝 Testing with context and query:\n";
    
    const char* phrase5 = melvin_generate_phrase_c(melvin, "cats", 6, "balanced", "deterministic", "cats are", "what are cats", 0);
    std::cout << "With context 'cats are' and query 'what are cats': \"" << phrase5 << "\"\n";
    
    const char* phrase6 = melvin_generate_phrase_c(melvin, "mammals", 5, "balanced", "deterministic", "", "what do mammals have", 0);
    std::cout << "Starting from 'mammals' with query 'what do mammals have': \"" << phrase6 << "\"\n\n";
    
    // Test the answer generation function
    std::cout << "💡 Testing answer generation from queries:\n";
    
    const char* answer1 = melvin_generate_answer(melvin, "what are cats", "balanced");
    std::cout << "Answer to 'what are cats': \"" << answer1 << "\"\n";
    
    const char* answer2 = melvin_generate_answer(melvin, "what do mammals have", "accuracy");
    std::cout << "Answer to 'what do mammals have': \"" << answer2 << "\"\n";
    
    const char* answer3 = melvin_generate_answer(melvin, "describe cats", "exploration");
    std::cout << "Answer to 'describe cats': \"" << answer3 << "\"\n\n";
    
    // Test probabilistic vs deterministic selection
    std::cout << "🎲 Testing probabilistic vs deterministic selection:\n";
    
    std::cout << "Deterministic selection (3 runs):\n";
    for (int i = 0; i < 3; ++i) {
        const char* phrase = melvin_generate_phrase_c(melvin, "cats", 4, "balanced", "deterministic", "", "", 0);
        std::cout << "  Run " << (i+1) << ": \"" << phrase << "\"\n";
    }
    
    std::cout << "\nProbabilistic selection (3 runs):\n";
    for (int i = 0; i < 3; ++i) {
        const char* phrase = melvin_generate_phrase_c(melvin, "cats", 4, "balanced", "probabilistic", "", "", 0);
        std::cout << "  Run " << (i+1) << ": \"" << phrase << "\"\n";
    }
    
    std::cout << "\n";
    
    // Show debug output for a specific node after generation
    std::cout << "🔍 Debug output for 'cats' node after generation:\n";
    melvin_debug_node_with_context(melvin, "cats", "", "", "balanced");
    
    std::cout << "\n";
    
    // Cleanup
    melvin_destroy(melvin);
    
    std::cout << "✅ Complete phrase generation loop test finished!\n";
    
    return 0;
}
