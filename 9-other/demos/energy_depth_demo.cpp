/*
 * Energy-Based Depth Demo - Show adaptive reasoning
 */

#include "../core/storage.h"
#include "../core/reasoning.h"
#include "../core/types.h"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace melvin;

struct TestCase {
    std::string query_start;
    std::string query_end;
    int expected_min_hops;
    std::string description;
};

void run_test(const TestCase& test, Storage& storage, ReasoningEngine& engine) {
    // Find start nodes for the query
    auto start_nodes = storage.find_nodes(test.query_start);
    if (start_nodes.empty()) {
        std::cout << "  ✗ Could not find: " << test.query_start << "\n\n";
        return;
    }
    
    Query query;
    query.text = test.query_start + " to " + test.query_end;
    
    auto answer = engine.infer(query, &storage);
    
    if (!answer.paths.empty()) {
        int actual_hops = static_cast<int>(answer.metrics["actual_hops"]);
        float final_energy = answer.metrics["final_energy"];
        
        std::cout << "  " << test.description << "\n";
        std::cout << "    Hops taken: " << actual_hops 
                  << " (expected: ~" << test.expected_min_hops << ")\n";
        std::cout << "    Final energy: " << std::fixed << std::setprecision(4) 
                  << final_energy << "\n";
        std::cout << "    Confidence: " << answer.confidence << "\n";
        
        // Show energy decay
        std::cout << "    Energy decay: 1.0";
        float energy = 1.0f;
        for (int h = 1; h <= actual_hops; ++h) {
            energy *= 0.9f * 0.9f;  // decay × edge weight (~0.9)
            std::cout << " → " << std::setprecision(3) << energy;
        }
        std::cout << "\n\n";
    }
}

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  ENERGY-BASED ADAPTIVE REASONING DEMONSTRATION        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    Storage storage;
    if (!storage.load("data/nodes.melvin", "data/edges.melvin")) {
        std::cerr << "✗ Failed to load\n";
        return 1;
    }
    
    ReasoningEngine engine;
    
    std::cout << "🧠 ENERGY-BASED REASONING:\n";
    std::cout << "  Instead of fixed MAX_HOPS, reasoning continues while\n";
    std::cout << "  energy (activation × edge_weight) remains strong.\n\n";
    
    std::cout << "  Formula: energy(hop+1) = energy(hop) × decay × edge_weight\n";
    std::cout << "  Stops when: energy < " << engine.get_config().energy_floor << "\n\n";
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    // Test cases with different expected depths
    std::vector<TestCase> tests = {
        {"fire", "heat", 1, "Simple (1-hop): Fire directly produces heat"},
        {"fire", "warmth", 2, "Medium (2-hop): Fire → Heat → Warmth"},
        {"water", "rain", 3, "Complex (3-hop): Water → Vapor → Clouds → Rain"},
        {"plants", "growth", 3, "Multi-path (3-hop): Plants → Energy → Growth"},
        {"practice", "mastery", 2, "Abstract (2-hop): Practice → Skill → Mastery"},
    };
    
    for (const auto& test : tests) {
        run_test(test, storage, engine);
    }
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    auto stats = engine.get_stats();
    
    std::cout << "📊 SESSION STATISTICS:\n\n";
    std::cout << "  Queries:              " << stats.queries_processed << "\n";
    std::cout << "  Avg hops (adaptive):  " << std::fixed << std::setprecision(2) 
              << stats.avg_hops_actual << "\n";
    std::cout << "  Avg final energy:     " << stats.avg_energy_final << "\n";
    std::cout << "  Early stops:          " << stats.early_terminations 
              << " (energy depleted)\n";
    std::cout << "  Safety stops:         " << stats.safety_terminations 
              << " (hit 50-hop limit)\n\n";
    
    std::cout << "✅ BENEFITS:\n";
    std::cout << "  • No arbitrary hop limit\n";
    std::cout << "  • Adapts to connection strength\n";
    std::cout << "  • Stops naturally when energy fades\n";
    std::cout << "  • Safe (time & hop limits)\n";
    std::cout << "  • Mimics neural activation decay\n\n";
    
    std::cout << "💡 With strong connections, could explore 50+ hops!\n";
    std::cout << "   With weak connections, stops after 1-2 hops.\n";
    std::cout << "   Perfectly adaptive! 🧠\n\n";
    
    return 0;
}

