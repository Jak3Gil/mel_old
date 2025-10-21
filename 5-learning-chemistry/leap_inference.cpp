/*
 * LEAP Inference Engine Implementation
 * 
 * Creates educated guesses (LEAP connections) from known facts (EXACT connections)
 */

#include "leap_inference.h"
#include <iostream>
#include <set>
#include <map>

namespace melvin {
namespace leap {

LEAPInference::LEAPInference(const Config& config) : config_(config) {}

bool LEAPInference::leap_exists(Storage* storage, NodeID from, NodeID to) {
    auto edges = storage->get_edges(from, to);
    for (const auto& edge : edges) {
        if (edge.relation == RelationType::LEAP) {
            return true;
        }
    }
    return false;
}

int LEAPInference::create_transitive_leaps(Storage* storage) {
    int created = 0;
    
    // For each 2-hop path, create a LEAP shortcut
    auto all_nodes = storage->get_all_nodes();
    
    for (const auto& start_node : all_nodes) {
        NodeID start = start_node.id;
        
        // Get 1-hop neighbors (only EXACT connections)
        auto edges1 = storage->get_edges_from(start);
        
        for (const auto& edge1 : edges1) {
            if (edge1.relation != RelationType::EXACT) continue;
            
            NodeID middle = edge1.to_id;
            
            // Get 2-hop neighbors
            auto edges2 = storage->get_edges_from(middle);
            
            for (const auto& edge2 : edges2) {
                if (edge2.relation != RelationType::EXACT) continue;
                
                NodeID end = edge2.to_id;
                
                // Don't create self-loops
                if (start == end) continue;
                
                // Check if LEAP already exists
                if (leap_exists(storage, start, end)) continue;
                
                // Check if EXACT already exists (don't override)
                auto existing = storage->get_edges(start, end);
                bool has_exact = false;
                for (const auto& e : existing) {
                    if (e.relation == RelationType::EXACT) {
                        has_exact = true;
                        break;
                    }
                }
                if (has_exact) continue;
                
                // Create LEAP shortcut
                storage->create_edge(start, end, RelationType::LEAP, config_.leap_confidence);
                created++;
                
                if (config_.verbose) {
                    std::cout << "  [LEAP] " << storage->get_node_content(start)
                              << " → " << storage->get_node_content(end)
                              << " (via " << storage->get_node_content(middle) << ")\n";
                }
            }
        }
    }
    
    stats_.transitive_leaps = created;
    return created;
}

int LEAPInference::create_similarity_leaps(Storage* storage) {
    int created = 0;
    
    // Find nodes with shared targets
    std::map<NodeID, std::set<NodeID>> targets_map;  // node → set of targets
    
    auto all_nodes = storage->get_all_nodes();
    
    // Build targets map (only EXACT connections)
    for (const auto& node : all_nodes) {
        auto edges = storage->get_edges_from(node.id);
        for (const auto& edge : edges) {
            if (edge.relation == RelationType::EXACT) {
                targets_map[node.id].insert(edge.to_id);
            }
        }
    }
    
    // Find pairs with shared targets
    for (size_t i = 0; i < all_nodes.size(); ++i) {
        for (size_t j = i + 1; j < all_nodes.size(); ++j) {
            NodeID node1 = all_nodes[i].id;
            NodeID node2 = all_nodes[j].id;
            
            // Count shared targets
            std::set<NodeID> shared;
            std::set_intersection(
                targets_map[node1].begin(), targets_map[node1].end(),
                targets_map[node2].begin(), targets_map[node2].end(),
                std::inserter(shared, shared.begin())
            );
            
            // If enough shared targets, create similarity LEAP
            if (shared.size() >= static_cast<size_t>(config_.min_shared_neighbors)) {
                // Create bidirectional LEAPs (similar nodes)
                if (!leap_exists(storage, node1, node2)) {
                    storage->create_edge(node1, node2, RelationType::LEAP, 
                                       config_.leap_confidence * 0.8f);  // Slightly lower for similarity
                    created++;
                    
                    if (config_.verbose) {
                        std::cout << "  [LEAP-SIM] " << storage->get_node_content(node1)
                                  << " ↔ " << storage->get_node_content(node2)
                                  << " (shared: " << shared.size() << " targets)\n";
                    }
                }
                
                if (!leap_exists(storage, node2, node1)) {
                    storage->create_edge(node2, node1, RelationType::LEAP,
                                       config_.leap_confidence * 0.8f);
                    created++;
                }
            }
        }
    }
    
    stats_.similarity_leaps = created;
    return created;
}

int LEAPInference::create_pattern_leaps(Storage* storage) {
    int created = 0;
    
    // For now, pattern matching is future work
    // Could detect:
    // - Analogy patterns (A:B :: C:D)
    // - Common subgraphs
    // - Frequent motifs
    
    stats_.pattern_leaps = created;
    return created;
}

int LEAPInference::create_leap_connections(Storage* storage) {
    if (!storage) return 0;
    
    stats_ = Stats();  // Reset stats
    
    std::cout << "\n🧠 Creating LEAP connections...\n\n";
    
    // Strategy 1: Transitivity
    std::cout << "Strategy 1: Transitive shortcuts (A→B→C ⇒ A--[LEAP]-->C)\n";
    int trans = create_transitive_leaps(storage);
    std::cout << "  ✓ Created " << trans << " transitive LEAPs\n\n";
    
    // Strategy 2: Similarity
    std::cout << "Strategy 2: Similarity from shared targets\n";
    int sim = create_similarity_leaps(storage);
    std::cout << "  ✓ Created " << sim << " similarity LEAPs\n\n";
    
    // Strategy 3: Patterns (future)
    // int pat = create_pattern_leaps(storage);
    
    stats_.total_leaps = trans + sim;
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  TOTAL: " << stats_.total_leaps << " LEAP connections created\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    return stats_.total_leaps;
}

} // namespace leap
} // namespace melvin

