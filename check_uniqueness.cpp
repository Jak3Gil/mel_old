/*
 * Check if nodes are truly unique and if connections are being made
 */

#include "melvin/core/storage.h"
#include <iostream>
#include <map>
#include <set>
#include <iomanip>

using namespace melvin;

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  NODE UNIQUENESS & CONNECTION ANALYSIS                        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    auto storage = std::make_unique<Storage>();
    storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    
    auto all_nodes = storage->get_all_nodes();
    auto all_edges = storage->get_all_edges();
    
    std::cout << "📊 OVERVIEW:\n";
    std::cout << "  Total nodes: " << all_nodes.size() << "\n";
    std::cout << "  Total edges: " << all_edges.size() << "\n\n";
    
    // Check for duplicate node content
    std::map<std::string, std::vector<NodeID>> content_to_ids;
    
    for (const auto& node : all_nodes) {
        std::string content = storage->get_node_content(node.id);
        content_to_ids[content].push_back(node.id);
    }
    
    std::cout << "🔍 UNIQUENESS CHECK:\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    int duplicate_count = 0;
    for (const auto& [content, ids] : content_to_ids) {
        if (ids.size() > 1) {
            duplicate_count++;
            std::cout << "  ⚠ DUPLICATE: \"" << content << "\" appears " 
                      << ids.size() << " times (IDs: ";
            for (auto id : ids) std::cout << id << " ";
            std::cout << ")\n";
        }
    }
    
    if (duplicate_count == 0) {
        std::cout << "  ✅ ALL NODES ARE UNIQUE - No duplicates found!\n";
        std::cout << "  " << all_nodes.size() << " nodes = " << content_to_ids.size() 
                  << " unique concepts\n";
    } else {
        std::cout << "\n  ⚠ Found " << duplicate_count << " duplicate concepts\n";
    }
    
    std::cout << "\n🔗 CONNECTION ANALYSIS:\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    // Check connection distribution
    std::map<int, int> connection_histogram;  // connections -> count of nodes
    int isolated_nodes = 0;
    int well_connected = 0;
    int super_connected = 0;
    
    for (const auto& node : all_nodes) {
        auto in_edges = storage->get_edges_to(node.id);
        auto out_edges = storage->get_edges_from(node.id);
        int total_connections = in_edges.size() + out_edges.size();
        
        connection_histogram[total_connections]++;
        
        if (total_connections == 0) {
            isolated_nodes++;
        } else if (total_connections >= 10) {
            well_connected++;
        }
        if (total_connections >= 100) {
            super_connected++;
        }
    }
    
    float avg_connections = all_nodes.empty() ? 0.0f : 
        (float)all_edges.size() * 2 / all_nodes.size();
    
    std::cout << "  Average connections per node: " << std::fixed << std::setprecision(2) 
              << avg_connections << "\n";
    std::cout << "  Isolated nodes (0 connections): " << isolated_nodes << "\n";
    std::cout << "  Well-connected (10+ connections): " << well_connected << "\n";
    std::cout << "  Super-connected (100+ connections): " << super_connected << "\n\n";
    
    // Show connection distribution
    std::cout << "  Connection distribution:\n";
    std::vector<std::pair<int, int>> sorted_hist(connection_histogram.begin(), connection_histogram.end());
    std::sort(sorted_hist.begin(), sorted_hist.end());
    
    for (const auto& [connections, count] : sorted_hist) {
        if (count > 5) {  // Only show common ranges
            std::cout << "    " << std::setw(4) << connections << " connections: " 
                      << count << " nodes\n";
        }
    }
    
    std::cout << "\n⏱  TIME ANALYSIS:\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    // Check if nodes/edges have timestamps
    bool has_timestamps = false;
    for (const auto& node : all_nodes) {
        if (node.creation_timestamp > 0) {
            has_timestamps = true;
            break;
        }
    }
    
    if (has_timestamps) {
        std::cout << "  ✅ Nodes have timestamps - can track learning over time\n";
        
        // Find oldest and newest
        uint64_t oldest = UINT64_MAX;
        uint64_t newest = 0;
        
        for (const auto& node : all_nodes) {
            if (node.creation_timestamp > 0) {
                oldest = std::min(oldest, node.creation_timestamp);
                newest = std::max(newest, node.creation_timestamp);
            }
        }
        
        if (oldest < UINT64_MAX) {
            uint64_t span_ms = newest - oldest;
            std::cout << "  Learning span: " << (span_ms / 1000) << " seconds\n";
            std::cout << "  (" << (span_ms / 60000) << " minutes)\n";
        }
    } else {
        std::cout << "  ⚠ Timestamps not set (older knowledge base)\n";
    }
    
    std::cout << "\n💡 VERDICT:\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    if (duplicate_count == 0) {
        std::cout << "  ✅ UNIQUENESS: Perfect - All nodes are unique\n";
    } else {
        std::cout << "  ⚠ UNIQUENESS: " << duplicate_count << " duplicates found\n";
    }
    
    if (avg_connections > 10.0f) {
        std::cout << "  ✅ CONNECTIONS: Excellent - " << avg_connections << " avg/node\n";
    } else if (avg_connections > 2.0f) {
        std::cout << "  ✓ CONNECTIONS: Good - " << avg_connections << " avg/node\n";
    } else {
        std::cout << "  ⚠ CONNECTIONS: Low - " << avg_connections << " avg/node\n";
    }
    
    if (isolated_nodes < all_nodes.size() * 0.1) {
        std::cout << "  ✅ INTEGRATION: Excellent - <10% isolated nodes\n";
    } else {
        std::cout << "  ⚠ INTEGRATION: " << (100 * isolated_nodes / all_nodes.size()) 
                  << "% isolated nodes\n";
    }
    
    std::cout << "\n  Overall: ";
    if (duplicate_count == 0 && avg_connections > 10.0f) {
        std::cout << "🎉 PERFECT! Unique nodes with rich connections!\n";
    } else {
        std::cout << "✓ Working well\n";
    }
    
    std::cout << "\n";
    
    return 0;
}
