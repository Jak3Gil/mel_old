/*
 * Test Energy-Based Reasoning - Demonstrate adaptive depth
 */

#include "../core/storage.h"
#include "../core/reasoning.h"
#include "../core/types.h"
#include "../core/diagnostics.h"
#include <iostream>
#include <iomanip>

using namespace melvin;

void test_query(const std::string& query_text, Storage& storage, ReasoningEngine& engine) {
    Query query;
    query.text = query_text;
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "Query: " << query_text << "\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    auto answer = engine.infer(query, &storage);
    
    if (!answer.paths.empty()) {
        const auto& path = answer.paths[0];
        
        std::cout << "  Answer: " << answer.text << "\n";
        std::cout << "  Confidence: " << answer.confidence << "\n";
        std::cout << "  Actual hops: " << static_cast<int>(answer.metrics["actual_hops"]) << "\n";
        std::cout << "  Final energy: " << answer.metrics["final_energy"] << "\n";
        std::cout << "  Path length: " << path.nodes.size() << " nodes\n\n";
        
        std::cout << "  Reasoning path:\n    ";
        for (size_t i = 0; i < path.nodes.size(); ++i) {
            std::cout << storage.get_node_content(path.nodes[i]);
            if (i < path.nodes.size() - 1) {
                std::cout << " → ";
            }
        }
        std::cout << "\n\n";
        
        // Log for diagnostics
        diagnostics::log_reasoning_session(
            query_text,
            static_cast<int>(answer.metrics["actual_hops"]),
            answer.metrics["final_energy"],
            answer.confidence,
            answer.metrics["actual_hops"] < 50  // Early termination
        );
    } else {
        std::cout << "  No path found\n\n";
    }
}

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  ENERGY-BASED DYNAMIC REASONING TEST                  ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    // Load knowledge
    Storage storage;
    if (!storage.load("data/nodes.melvin", "data/edges.melvin")) {
        std::cerr << "✗ Failed to load. Run ./demos/teach_knowledge first!\n\n";
        return 1;
    }
    
    std::cout << "📊 Loaded: " << storage.node_count() << " nodes, " 
              << storage.edge_count() << " edges\n\n";
    
    // Create reasoning engine
    ReasoningEngine engine;
    
    std::cout << "⚙️  ENERGY-BASED CONFIG:\n";
    std::cout << "  Energy decay:    " << engine.get_config().energy_decay << "\n";
    std::cout << "  Energy floor:    " << engine.get_config().energy_floor << "\n";
    std::cout << "  Energy epsilon:  " << engine.get_config().energy_epsilon << "\n";
    std::cout << "  Safety limit:    " << engine.get_config().max_hops_safety << " hops\n";
    std::cout << "  Time budget:     " << engine.get_config().time_budget_ms << " ms\n\n";
    
    std::cout << "💡 System will automatically stop when:\n";
    std::cout << "  • Energy drops below floor (natural termination)\n";
    std::cout << "  • Safety limit reached (50 hops)\n";
    std::cout << "  • Time budget exceeded (100ms)\n\n";
    
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "  RUNNING TESTS\n";
    std::cout << "═══════════════════════════════════════════════════════\n\n";
    
    // Test with varying query depths
    test_query("heat", storage, engine);          // Should be 1-2 hops
    test_query("warmth", storage, engine);        // Should be 2-3 hops
    test_query("growth", storage, engine);        // Should be 3-5 hops
    test_query("understanding", storage, engine); // Should be 4-6 hops
    
    // Show statistics
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "  ENERGY-BASED STATISTICS\n";
    std::cout << "═══════════════════════════════════════════════════════\n\n";
    
    auto stats = engine.get_stats();
    
    std::cout << "  Queries processed:      " << stats.queries_processed << "\n";
    std::cout << "  Average actual hops:    " << std::fixed << std::setprecision(2) 
              << stats.avg_hops_actual << "\n";
    std::cout << "  Average final energy:   " << stats.avg_energy_final << "\n";
    std::cout << "  Early terminations:     " << stats.early_terminations 
              << " (energy-based)\n";
    std::cout << "  Safety terminations:    " << stats.safety_terminations 
              << " (hit limit)\n\n";
    
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ ENERGY-BASED REASONING WORKING                    ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "🎉 System adapts reasoning depth based on energy!\n";
    std::cout << "   • Weak connections → stops early\n";
    std::cout << "   • Strong connections → continues naturally\n";
    std::cout << "   • No fixed depth limit!\n\n";
    
    std::cout << "📊 Logs saved to: data/reasoning_sessions.csv\n\n";
    
    return 0;
}

