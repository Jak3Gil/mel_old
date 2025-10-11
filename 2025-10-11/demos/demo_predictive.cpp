/*
 * MELVIN PREDICTIVE REASONING DEMO
 * 
 * Demonstrates graph-guided prediction:
 * - Markov-only (no graph bias)
 * - Graph-only (reasoning-only mode)
 * - Hybrid (Markov + graph clusters)
 * - Incremental learning comparison
 */

#include "../include/melvin_predictive.h"
#include "../src/util/config.h"
#include <iostream>

using namespace melvin_predictive;
using namespace melvin_config;

void test_mode(const std::string& mode_name, const std::string& query, 
               float lambda, bool reasoning_only) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "MODE: " << mode_name << "\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    
    // Set config
    auto cfg = get_config();
    global_config.lambda_graph_bias = lambda;
    global_config.reasoning_only_mode = reasoning_only;
    global_config.log_predictions = true;
    
    // Create brain
    PredictiveBrain brain;
    
    // Ask question
    brain.think(query);
}

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN PREDICTIVE REASONING DEMO                            ║\n";
    std::cout << "║  Compare: Markov-only | Graph-only | Hybrid                 ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    
    // Load config
    load_config();
    
    std::string test_query = "how do i feel better";
    
    if (argc > 1) {
        test_query = argv[1];
    }
    
    std::cout << "\n📝 Test Query: \"" << test_query << "\"\n";
    
    // Test 1: Markov-only (λ = 0, no graph bias)
    test_mode("Markov-only (λ=0, pure fluency)", test_query, 0.0f, false);
    
    // Test 2: Graph-only (reasoning-only mode)
    test_mode("Graph-only (reasoning-only, no prediction)", test_query, 0.5f, true);
    
    // Test 3: Hybrid λ=0.3 (light graph bias)
    test_mode("Hybrid λ=0.3 (light graph bias)", test_query, 0.3f, false);
    
    // Test 4: Hybrid λ=0.5 (balanced)
    test_mode("Hybrid λ=0.5 (balanced)", test_query, 0.5f, false);
    
    // Test 5: Hybrid λ=0.8 (heavy graph bias)
    test_mode("Hybrid λ=0.8 (heavy graph bias)", test_query, 0.8f, false);
    
    // Summary
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  DEMO COMPLETE                                               ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Key Observations:\n";
    std::cout << "  • λ=0.0: Pure Markov, most fluent but may drift from meaning\n";
    std::cout << "  • λ=0.3-0.5: Balanced, fluent + grounded\n";
    std::cout << "  • λ=0.8: Graph-heavy, most grounded but may be less fluent\n";
    std::cout << "  • Reasoning-only: Original system, concatenates path\n\n";
    
    std::cout << "Tune λ at runtime:\n";
    std::cout << "  LAMBDA_GRAPH_BIAS=0.3 ./melvin_interactive\n";
    std::cout << "  LAMBDA_GRAPH_BIAS=0.8 LOG_PREDICTIONS=1 ./melvin_interactive\n\n";
    
    return 0;
}

