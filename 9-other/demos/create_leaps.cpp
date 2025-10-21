/*
 * Create LEAP Connections - Infer educated guesses from EXACT facts
 */

#include "../core/storage.h"
#include "../core/leap_inference.h"
#include "../core/types.h"
#include <iostream>

using namespace melvin;

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  CREATE LEAP CONNECTIONS                              ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    // Load existing knowledge
    Storage storage;
    
    std::cout << "📂 Loading existing knowledge...\n";
    if (!storage.load("data/nodes.melvin", "data/edges.melvin")) {
        std::cerr << "✗ Failed to load. Run ./demos/teach_knowledge first!\n\n";
        return 1;
    }
    
    std::cout << "  ✓ Loaded " << storage.node_count() << " nodes, " 
              << storage.edge_count() << " edges\n\n";
    
    // Count EXACT vs LEAP before
    auto edges_before = storage.get_all_edges();
    int exact_count = 0;
    int leap_count = 0;
    
    for (const auto& edge : edges_before) {
        if (edge.relation == RelationType::EXACT) exact_count++;
        else if (edge.relation == RelationType::LEAP) leap_count++;
    }
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  BEFORE:\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    std::cout << "  EXACT connections:  " << exact_count << " (taught facts)\n";
    std::cout << "  LEAP connections:   " << leap_count << " (inferred)\n";
    std::cout << "  Total:              " << storage.edge_count() << "\n\n";
    
    // Create LEAP inference engine
    leap::LEAPInference::Config config;
    config.max_transitive_hops = 3;
    config.min_shared_neighbors = 2;
    config.leap_confidence = 0.7f;
    config.verbose = true;
    
    leap::LEAPInference inference(config);
    
    // Create LEAP connections
    int leaps_created = inference.create_leap_connections(&storage);
    
    // Count after
    auto edges_after = storage.get_all_edges();
    exact_count = 0;
    leap_count = 0;
    
    for (const auto& edge : edges_after) {
        if (edge.relation == RelationType::EXACT) exact_count++;
        else if (edge.relation == RelationType::LEAP) leap_count++;
    }
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  AFTER:\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    std::cout << "  EXACT connections:  " << exact_count << " (unchanged)\n";
    std::cout << "  LEAP connections:   " << leap_count << " (NEW!)\n";
    std::cout << "  Total:              " << storage.edge_count() << "\n\n";
    
    std::cout << "  Increase: +" << leaps_created << " connections (+";
    std::cout << (int)((float)leaps_created / exact_count * 100) << "%)\n\n";
    
    // Save enhanced knowledge
    std::cout << "💾 Saving enhanced knowledge (EXACT + LEAP)...\n";
    if (storage.save("data/nodes.melvin", "data/edges.melvin")) {
        std::cout << "  ✓ Saved successfully\n\n";
    }
    
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ LEAP CREATION COMPLETE                            ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    auto stats = inference.get_stats();
    
    std::cout << "📊 Statistics:\n";
    std::cout << "  Transitive LEAPs:  " << stats.transitive_leaps << "\n";
    std::cout << "  Similarity LEAPs:  " << stats.similarity_leaps << "\n";
    std::cout << "  Pattern LEAPs:     " << stats.pattern_leaps << "\n";
    std::cout << "  Total LEAPs:       " << stats.total_leaps << "\n\n";
    
    std::cout << "✨ Benefits:\n";
    std::cout << "  • Faster reasoning (shortcuts available)\n";
    std::cout << "  • More inference paths\n";
    std::cout << "  • Can answer questions with fewer hops\n";
    std::cout << "  • Still distinguishes facts (EXACT) from guesses (LEAP)\n\n";
    
    std::cout << "💡 Try: ./demos/prove_persistence to see LEAP in action!\n\n";
    
    return 0;
}

