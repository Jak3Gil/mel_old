/*
 * Reasoning Demo - Test multi-hop reasoning
 */

#include "../core/melvin.h"
#include "../UCAConfig.h"
#include <iostream>
#include <vector>

using namespace melvin;

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN REASONING DEMO                                ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    // Create Melvin instance
    MelvinCore melvin;
    melvin.load_memory(config::STORAGE_PATH_NODES, config::STORAGE_PATH_EDGES);
    
    // Test queries
    std::vector<std::string> test_queries = {
        "What is fire?",
        "What causes smoke?",
        "What produces heat?",
        "What is water?",
        "How do plants grow?"
    };
    
    std::cout << "Running reasoning tests...\n\n";
    
    for (const auto& query : test_queries) {
        std::cout << "Q: " << query << "\n";
        auto answer = melvin.query(query);
        std::cout << "A: " << answer.text << "\n";
        std::cout << "   (confidence: " << answer.confidence << ")\n\n";
    }
    
    // Print statistics
    melvin.print_stats();
    
    return 0;
}

