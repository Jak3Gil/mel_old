/*
 * MELVIN INTERACTIVE TERMINAL
 * 
 * Ask questions using graph-guided predictive reasoning:
 * - Loads tinychat_brain.bin with 6,869 words and 260,942 connections
 * - Loads pre-trained Markov predictor
 * - Hybrid mode: graph reasoning + predictive generation
 * - Shows node/connection counts from storage.cpp after each query
 */

#include "include/melvin_predictive.h"
#include "src/util/config.h"
#include <iostream>
#include <string>

using namespace melvin_predictive;
using namespace melvin_storage;
using namespace melvin_config;

void show_counts() {
    std::cout << "\n📊 Total Nodes: " << nodes.size() 
              << " | Total Connections: " << edges.size() << "\n\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN INTERACTIVE TERMINAL                                 ║\n";
    std::cout << "║  Graph-Guided Predictive Reasoning                           ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    // Load config
    load_config();
    
    auto cfg = get_config();
    std::cout << "⚙️  Mode: " << (cfg.reasoning_only_mode ? "Reasoning-Only" : "Hybrid Prediction") << "\n";
    std::cout << "⚙️  Graph Bias λ: " << cfg.lambda_graph_bias << "\n\n";
    
    // Load brain with predictor
    std::cout << "🧠 Loading knowledge graph + predictor...\n";
    PredictiveBrain brain;
    
    std::cout << "✅ Loaded successfully\n";
    show_counts();
    
    std::cout << "💡 Type your questions below (or 'quit' to exit)\n";
    std::cout << "   Commands: 'stats' | 'save' | 'quit'\n";
    std::cout << "   Toggle modes: REASONING_ONLY=1 or LAMBDA_GRAPH_BIAS=0.0\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    std::string input;
    int query_count = 0;
    
    while (true) {
        std::cout << "You: ";
        std::getline(std::cin, input);
        
        if (input.empty()) continue;
        
        // Check for exit
        if (input == "quit" || input == "exit" || input == "q") {
            std::cout << "\n👋 Goodbye!\n\n";
            break;
        }
        
        // Special commands
        if (input == "stats") {
            std::cout << "\n";
            brain.stats();
            show_counts();
            continue;
        }
        
        if (input == "save") {
            brain.save();
            std::cout << "✅ Brain saved\n";
            show_counts();
            continue;
        }
        
        // Process query through reasoning pipeline
        query_count++;
        std::cout << "\n";
        brain.think(input);
        
        // Show updated counts
        show_counts();
    }
    
    // Save on exit
    std::cout << "💾 Saving brain state...\n";
    brain.save();
    std::cout << "✅ Processed " << query_count << " queries\n";
    show_counts();
    
    return 0;
}

