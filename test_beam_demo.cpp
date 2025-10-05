#include "melvin.h"
#include <iostream>

int main() {
    std::cout << "🧠 BEAM SEARCH DEMONSTRATION\n";
    std::cout << "============================\n\n";
    
    melvin_t* melvin = melvin_create(nullptr);
    
    // Teach some knowledge
    std::cout << "📚 Teaching knowledge...\n";
    for (int i = 0; i < 5; ++i) {
        melvin_learn(melvin, "cats are mammals");
    }
    for (int i = 0; i < 3; ++i) {
        melvin_learn(melvin, "cats are animals");
    }
    for (int i = 0; i < 2; ++i) {
        melvin_learn(melvin, "cats are cute");
    }
    
    std::cout << "✅ Knowledge learned. Nodes: " << melvin_node_count(melvin) 
              << ", Edges: " << melvin_edge_count(melvin) << "\n\n";
    
    // Test 1: Basic beam search
    std::cout << "🔍 Test 1: Basic beam search (k=3, M=4)\n";
    const char* result1 = melvin_generate_phrase_beam_c(melvin, "cats", 5, 3, 4, "balanced", 0, 0.05f, "", "what is a cat", 1);
    std::cout << "Result: \"" << result1 << "\"\n\n";
    
    // Test 2: Lower threshold
    std::cout << "🔍 Test 2: Lower threshold (0.01)\n";
    const char* result2 = melvin_generate_phrase_beam_c(melvin, "cats", 5, 3, 4, "balanced", 0, 0.01f, "", "what is a cat", 1);
    std::cout << "Result: \"" << result2 << "\"\n\n";
    
    // Test 3: Greedy comparison
    std::cout << "🔍 Test 3: Greedy comparison\n";
    const char* result3 = melvin_generate_phrase_c(melvin, "cats", 5, "balanced", "deterministic", "", "what is a cat", 1);
    std::cout << "Result: \"" << result3 << "\"\n\n";
    
    // Test 4: Probabilistic beam search
    std::cout << "🔍 Test 4: Probabilistic beam search\n";
    const char* result4 = melvin_generate_phrase_beam_c(melvin, "cats", 5, 3, 4, "balanced", 1, 0.01f, "", "what is a cat", 1);
    std::cout << "Result: \"" << result4 << "\"\n\n";
    
    // Test 5: Trace generation
    std::cout << "🔍 Test 5: Trace generation\n";
    melvin_trace_generation(melvin, "cats", "what is a cat", "balanced");
    
    melvin_destroy(melvin);
    
    return 0;
}
