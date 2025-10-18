/*
 * Quick LEAP quality spot-check (20 random samples)
 */

#include "melvin/core/storage.h"
#include <iostream>
#include <iomanip>
#include <random>
#include <set>

using namespace melvin;

bool has_exact_path(Storage* storage, NodeID from, NodeID to, int max_hops = 5) {
    std::set<NodeID> visited;
    std::vector<NodeID> current = {from};
    
    for (int hop = 0; hop < max_hops && !current.empty(); ++hop) {
        std::vector<NodeID> next;
        for (NodeID node : current) {
            if (visited.count(node)) continue;
            visited.insert(node);
            if (node == to) return true;
            
            auto edges = storage->get_edges_from(node);
            for (const auto& e : edges) {
                if (e.relation == RelationType::EXACT) {
                    next.push_back(e.to_id);
                }
            }
        }
        current = next;
    }
    return false;
}

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🔍 QUICK LEAP QUALITY CHECK (20 Random Samples)              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    auto storage = std::make_unique<Storage>();
    std::cout << "Loading database...\n";
    
    if (!storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin")) {
        std::cerr << "❌ Failed to load\n";
        return 1;
    }
    std::cout << "✅ Loaded!\n\n";
    
    // Get LEAP edges
    auto all_edges = storage->get_all_edges();
    std::vector<Edge> leaps;
    for (const auto& e : all_edges) {
        if (e.relation == RelationType::LEAP) leaps.push_back(e);
    }
    
    std::cout << "Found " << leaps.size() << " LEAP edges\n";
    std::cout << "Checking 20 random samples...\n\n";
    
    // Random sample
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, leaps.size() - 1);
    
    int valid = 0, invalid = 0, self_loops = 0, duplicates = 0;
    
    std::cout << "═══════════════════════════════════════════════════════════════\n";
    
    for (int i = 0; i < 20; ++i) {
        const auto& leap = leaps[dis(gen)];
        
        std::string from = storage->get_node_content(leap.from_id);
        std::string to = storage->get_node_content(leap.to_id);
        if (from.length() > 25) from = from.substr(0, 22) + "...";
        if (to.length() > 25) to = to.substr(0, 22) + "...";
        
        std::cout << std::setw(2) << (i+1) << ". \"" << from << "\" → \"" << to << "\"\n";
        
        // Check
        if (leap.from_id == leap.to_id) {
            std::cout << "    ❌ Self-loop (A→A)\n";
            self_loops++;
            invalid++;
        } else {
            // Check for duplicate EXACT
            auto direct = storage->get_edges(leap.from_id, leap.to_id);
            bool has_exact = false;
            for (const auto& e : direct) {
                if (e.relation == RelationType::EXACT) has_exact = true;
            }
            
            if (has_exact) {
                std::cout << "    ⚠️  Redundant (EXACT exists)\n";
                duplicates++;
                invalid++;
            } else if (has_exact_path(storage.get(), leap.from_id, leap.to_id, 5)) {
                std::cout << "    ✅ Valid (EXACT path exists)\n";
                valid++;
            } else {
                std::cout << "    ❌ No path (no EXACT backup)\n";
                invalid++;
            }
        }
    }
    
    std::cout << "═══════════════════════════════════════════════════════════════\n\n";
    
    // Results
    std::cout << "RESULTS:\n";
    std::cout << "  Valid LEAPs:       " << valid << "/20 (" << (valid*5) << "%)\n";
    std::cout << "  Invalid LEAPs:     " << invalid << "/20 (" << (invalid*5) << "%)\n";
    std::cout << "    - Self-loops:    " << self_loops << "\n";
    std::cout << "    - Redundant:     " << duplicates << "\n";
    std::cout << "    - No path:       " << (invalid - self_loops - duplicates) << "\n\n";
    
    // Assessment
    float quality = valid * 5.0f;
    std::cout << "Quality Score: " << quality << "%\n\n";
    
    if (quality >= 85) {
        std::cout << "✅ EXCELLENT - LEAPs are high quality!\n";
    } else if (quality >= 70) {
        std::cout << "✓ GOOD - LEAPs are mostly valid\n";
    } else if (quality >= 50) {
        std::cout << "⚠️  MODERATE - Mixed quality\n";
        std::cout << "   Consider cleanup\n";
    } else {
        std::cout << "❌ POOR - Many invalid LEAPs\n";
        std::cout << "   Cleanup recommended!\n";
    }
    
    std::cout << "\n";
    return 0;
}

