/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  LEAP QUALITY ANALYZER - Check if LEAP edges are meaningful               ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "melvin/core/storage.h"
#include <iostream>
#include <iomanip>
#include <random>
#include <set>

using namespace melvin;

struct LEAPAnalysis {
    int total_leaps_checked = 0;
    int leaps_with_valid_path = 0;
    int leaps_without_path = 0;
    int leaps_to_self = 0;
    int leaps_duplicate_exact = 0;
    
    std::vector<std::string> good_examples;
    std::vector<std::string> questionable_examples;
};

bool has_exact_path(Storage* storage, NodeID from, NodeID to, int max_hops = 5) {
    // BFS to find if there's a path using only EXACT edges
    std::set<NodeID> visited;
    std::vector<NodeID> current_level = {from};
    
    for (int hop = 0; hop < max_hops; ++hop) {
        std::vector<NodeID> next_level;
        
        for (NodeID node : current_level) {
            if (visited.count(node)) continue;
            visited.insert(node);
            
            if (node == to) return true;
            
            auto edges = storage->get_edges_from(node);
            for (const auto& edge : edges) {
                if (edge.relation == RelationType::EXACT) {
                    next_level.push_back(edge.to_id);
                }
            }
        }
        
        if (next_level.empty()) break;
        current_level = next_level;
    }
    
    return false;
}

int count_exact_path_length(Storage* storage, NodeID from, NodeID to, int max_hops = 5) {
    // BFS to find shortest EXACT path length
    std::set<NodeID> visited;
    std::vector<std::pair<NodeID, int>> current_level = {{from, 0}};
    
    for (int hop = 0; hop < max_hops; ++hop) {
        std::vector<std::pair<NodeID, int>> next_level;
        
        for (auto [node, dist] : current_level) {
            if (visited.count(node)) continue;
            visited.insert(node);
            
            if (node == to) return dist;
            
            auto edges = storage->get_edges_from(node);
            for (const auto& edge : edges) {
                if (edge.relation == RelationType::EXACT) {
                    next_level.push_back({edge.to_id, dist + 1});
                }
            }
        }
        
        if (next_level.empty()) break;
        current_level = next_level;
    }
    
    return -1; // No path found
}

void print_header(const std::string& title) {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  " << std::left << std::setw(60) << title << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🔍 LEAP QUALITY ANALYZER                                      ║\n";
    std::cout << "║     Checking if LEAP edges are meaningful or junk             ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    
    auto storage = std::make_unique<Storage>();
    std::cout << "\nLoading database...\n";
    
    if (!storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin")) {
        std::cerr << "❌ Failed to load database\n";
        return 1;
    }
    
    std::cout << "✅ Loaded!\n";
    
    // Get all edges
    auto all_edges = storage->get_all_edges();
    std::vector<Edge> leap_edges;
    
    for (const auto& edge : all_edges) {
        if (edge.relation == RelationType::LEAP) {
            leap_edges.push_back(edge);
        }
    }
    
    std::cout << "\nFound " << leap_edges.size() << " LEAP edges\n";
    
    // ========================================================================
    // SAMPLE ANALYSIS
    // ========================================================================
    
    print_header("ANALYZING RANDOM SAMPLE OF 100 LEAP EDGES");
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, leap_edges.size() - 1);
    
    LEAPAnalysis analysis;
    
    std::cout << "Checking validity of LEAP shortcuts...\n\n";
    
    for (int i = 0; i < 100 && i < leap_edges.size(); ++i) {
        int idx = dis(gen);
        const auto& leap = leap_edges[idx];
        
        analysis.total_leaps_checked++;
        
        // Check if it's a self-loop
        if (leap.from_id == leap.to_id) {
            analysis.leaps_to_self++;
            continue;
        }
        
        // Check if there's an EXACT edge with same from/to (redundant LEAP)
        auto direct_edges = storage->get_edges(leap.from_id, leap.to_id);
        bool has_direct_exact = false;
        for (const auto& e : direct_edges) {
            if (e.relation == RelationType::EXACT) {
                has_direct_exact = true;
                break;
            }
        }
        
        if (has_direct_exact) {
            analysis.leaps_duplicate_exact++;
            continue;
        }
        
        // Check if there's a valid EXACT path
        int path_length = count_exact_path_length(storage.get(), leap.from_id, leap.to_id, 5);
        
        if (path_length > 0) {
            analysis.leaps_with_valid_path++;
            
            // Save good example
            if (analysis.good_examples.size() < 5) {
                std::string from = storage->get_node_content(leap.from_id);
                std::string to = storage->get_node_content(leap.to_id);
                if (from.length() > 30) from = from.substr(0, 27) + "...";
                if (to.length() > 30) to = to.substr(0, 27) + "...";
                
                std::string example = "\"" + from + "\" ─[LEAP]→ \"" + to + 
                                     "\" (path length: " + std::to_string(path_length) + ")";
                analysis.good_examples.push_back(example);
            }
        } else {
            analysis.leaps_without_path++;
            
            // Save questionable example
            if (analysis.questionable_examples.size() < 5) {
                std::string from = storage->get_node_content(leap.from_id);
                std::string to = storage->get_node_content(leap.to_id);
                if (from.length() > 30) from = from.substr(0, 27) + "...";
                if (to.length() > 30) to = to.substr(0, 27) + "...";
                
                std::string example = "\"" + from + "\" ─[LEAP]→ \"" + to + "\" (NO EXACT PATH FOUND!)";
                analysis.questionable_examples.push_back(example);
            }
        }
        
        if ((i + 1) % 20 == 0) {
            std::cout << "  Checked " << (i + 1) << "/100...\n";
        }
    }
    
    // ========================================================================
    // RESULTS
    // ========================================================================
    
    print_header("LEAP QUALITY RESULTS");
    
    std::cout << "Sample size:           " << analysis.total_leaps_checked << " LEAP edges\n\n";
    
    std::cout << "Valid LEAPs:           " << analysis.leaps_with_valid_path 
              << " (" << (analysis.leaps_with_valid_path * 100.0 / analysis.total_leaps_checked) << "%)\n";
    std::cout << "  → These have actual EXACT paths backing them up\n\n";
    
    std::cout << "Invalid LEAPs:         " << analysis.leaps_without_path 
              << " (" << (analysis.leaps_without_path * 100.0 / analysis.total_leaps_checked) << "%)\n";
    std::cout << "  → No EXACT path found (might be stale or over-inferred)\n\n";
    
    std::cout << "Self-loops:            " << analysis.leaps_to_self 
              << " (" << (analysis.leaps_to_self * 100.0 / analysis.total_leaps_checked) << "%)\n";
    std::cout << "  → LEAPs from node to itself (useless)\n\n";
    
    std::cout << "Duplicate of EXACT:    " << analysis.leaps_duplicate_exact 
              << " (" << (analysis.leaps_duplicate_exact * 100.0 / analysis.total_leaps_checked) << "%)\n";
    std::cout << "  → LEAP where direct EXACT edge exists (redundant)\n\n";
    
    // ========================================================================
    // GOOD EXAMPLES
    // ========================================================================
    
    if (!analysis.good_examples.empty()) {
        print_header("EXAMPLES OF GOOD LEAPs");
        std::cout << "These LEAPs have valid underlying EXACT paths:\n\n";
        for (size_t i = 0; i < analysis.good_examples.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << analysis.good_examples[i] << "\n";
        }
    }
    
    // ========================================================================
    // QUESTIONABLE EXAMPLES
    // ========================================================================
    
    if (!analysis.questionable_examples.empty()) {
        print_header("EXAMPLES OF QUESTIONABLE LEAPs");
        std::cout << "These LEAPs have NO underlying EXACT path within 5 hops:\n\n";
        for (size_t i = 0; i < analysis.questionable_examples.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << analysis.questionable_examples[i] << "\n";
        }
    }
    
    // ========================================================================
    // ASSESSMENT
    // ========================================================================
    
    print_header("OVERALL ASSESSMENT");
    
    float quality_score = (analysis.leaps_with_valid_path * 100.0) / 
                         (analysis.total_leaps_checked > 0 ? analysis.total_leaps_checked : 1);
    
    std::cout << "Quality Score: " << quality_score << "%\n\n";
    
    if (quality_score >= 90) {
        std::cout << "✅ EXCELLENT - LEAPs are high quality!\n";
        std::cout << "   Most LEAPs represent valid transitive shortcuts.\n";
    } else if (quality_score >= 70) {
        std::cout << "✓ GOOD - LEAPs are mostly valid\n";
        std::cout << "   Some cleanup might improve quality.\n";
    } else if (quality_score >= 50) {
        std::cout << "⚠️  MODERATE - Mixed quality\n";
        std::cout << "   Consider running LEAP cleanup or regeneration.\n";
    } else {
        std::cout << "❌ POOR - Many invalid LEAPs\n";
        std::cout << "   Database needs LEAP cleanup!\n";
    }
    
    std::cout << "\n";
    
    // ========================================================================
    // RECOMMENDATIONS
    // ========================================================================
    
    print_header("RECOMMENDATIONS");
    
    if (analysis.leaps_to_self > 0) {
        std::cout << "⚠️  Found " << analysis.leaps_to_self << " self-loop LEAPs\n";
        std::cout << "   → These should be removed (useless)\n\n";
    }
    
    if (analysis.leaps_duplicate_exact > 0) {
        std::cout << "⚠️  Found " << analysis.leaps_duplicate_exact << " redundant LEAPs\n";
        std::cout << "   → These duplicate EXACT edges (can be removed)\n\n";
    }
    
    if (analysis.leaps_without_path > 10) {
        std::cout << "⚠️  Found " << analysis.leaps_without_path << " LEAPs without backing paths\n";
        std::cout << "   → These might be:\n";
        std::cout << "      • Stale (from deleted EXACT edges)\n";
        std::cout << "      • Over-inferred (similarity-based but not transitive)\n";
        std::cout << "      • Valid but >5 hops away\n\n";
    }
    
    float junk_ratio = (analysis.leaps_to_self + analysis.leaps_duplicate_exact + analysis.leaps_without_path) * 100.0 / 
                       analysis.total_leaps_checked;
    
    std::cout << "Estimated junk ratio: " << junk_ratio << "%\n\n";
    
    if (junk_ratio > 30) {
        std::cout << "💡 SUGGESTION: Run LEAP cleanup to improve quality\n";
        std::cout << "   This will remove invalid/redundant LEAPs\n";
    } else if (junk_ratio > 15) {
        std::cout << "💡 SUGGESTION: Consider periodic LEAP maintenance\n";
        std::cout << "   Quality is good but could be improved\n";
    } else {
        std::cout << "✅ LEAPs are in good shape!\n";
        std::cout << "   No immediate cleanup needed\n";
    }
    
    std::cout << "\n";
    
    return 0;
}

