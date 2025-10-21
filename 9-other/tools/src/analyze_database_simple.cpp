/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  DATABASE ANALYZER - Show what's really in Melvin's brain                ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "melvin/core/storage.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <vector>

using namespace melvin;

void print_header(const std::string& title) {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  " << std::left << std::setw(60) << title << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  📊 MELVIN DATABASE ANALYZER                                   ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    
    // Load storage
    auto storage = std::make_unique<Storage>();
    std::cout << "\nLoading database from melvin/data/...\n";
    bool loaded = storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    
    if (!loaded) {
        std::cerr << "❌ Failed to load database\n";
        return 1;
    }
    
    std::cout << "✅ Loaded successfully!\n";
    
    // ========================================================================
    // BASIC STATISTICS
    // ========================================================================
    
    print_header("BASIC STATISTICS");
    
    size_t total_nodes = storage->node_count();
    size_t total_edges = storage->edge_count();
    size_t exact_edges = storage->edge_count_by_type(RelationType::EXACT);
    size_t leap_edges = storage->edge_count_by_type(RelationType::LEAP);
    
    std::cout << "Nodes:         " << total_nodes << "\n";
    std::cout << "Edges:         " << total_edges << "\n";
    std::cout << "  EXACT:       " << exact_edges << " (taught facts)\n";
    std::cout << "  LEAP:        " << leap_edges << " (inferred shortcuts)\n";
    std::cout << "LEAP ratio:    " << (exact_edges > 0 ? (float)leap_edges / exact_edges : 0.0f) 
              << " LEAPs per EXACT\n";
    std::cout << "Edges/node:    " << (total_nodes > 0 ? (float)total_edges / total_nodes : 0.0f) << "\n";
    
    // ========================================================================
    // NODE CONNECTIVITY ANALYSIS
    // ========================================================================
    
    print_header("NODE CONNECTIVITY ANALYSIS");
    
    auto all_nodes = storage->get_all_nodes();
    
    size_t nodes_with_incoming = 0;
    size_t nodes_with_outgoing = 0;
    size_t isolated_nodes = 0;
    std::vector<std::pair<std::string, size_t>> highly_connected;
    
    for (const auto& node : all_nodes) {
        auto in_edges = storage->get_edges_to(node.id);
        auto out_edges = storage->get_edges_from(node.id);
        
        size_t total_connections = in_edges.size() + out_edges.size();
        
        if (!in_edges.empty()) nodes_with_incoming++;
        if (!out_edges.empty()) nodes_with_outgoing++;
        if (in_edges.empty() && out_edges.empty()) isolated_nodes++;
        
        if (total_connections > 50) {
            std::string content = storage->get_node_content(node.id);
            highly_connected.push_back({content, total_connections});
        }
    }
    
    std::cout << "Nodes with incoming edges: " << nodes_with_incoming 
              << " (" << (total_nodes > 0 ? nodes_with_incoming * 100.0f / total_nodes : 0.0f) << "%)\n";
    std::cout << "Nodes with outgoing edges: " << nodes_with_outgoing 
              << " (" << (total_nodes > 0 ? nodes_with_outgoing * 100.0f / total_nodes : 0.0f) << "%)\n";
    std::cout << "Isolated nodes (no edges): " << isolated_nodes 
              << " (" << (total_nodes > 0 ? isolated_nodes * 100.0f / total_nodes : 0.0f) << "%)\n";
    
    float connectivity_ratio = total_nodes > 0 ? 
        (float)(nodes_with_incoming + nodes_with_outgoing) / (2.0f * total_nodes) : 0.0f;
    std::cout << "\nOverall connectivity:      " << (connectivity_ratio * 100) << "%\n";
    
    // ========================================================================
    // SAMPLE NODES
    // ========================================================================
    
    print_header("SAMPLE NODES (First 30)");
    
    int shown = 0;
    for (const auto& node : all_nodes) {
        if (shown >= 30) break;
        
        std::string content = storage->get_node_content(node.id);
        auto out_edges = storage->get_edges_from(node.id);
        auto in_edges = storage->get_edges_to(node.id);
        
        if (content.length() > 60) {
            content = content.substr(0, 57) + "...";
        }
        
        std::cout << std::setw(3) << (shown + 1) << ". \"" << content << "\"\n";
        std::cout << "     Edges: " << in_edges.size() << " in, " << out_edges.size() << " out\n";
        
        shown++;
    }
    
    // ========================================================================
    // HIGHLY CONNECTED NODES
    // ========================================================================
    
    if (!highly_connected.empty()) {
        print_header("HIGHLY CONNECTED NODES (>50 connections)");
        
        std::sort(highly_connected.begin(), highly_connected.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        for (size_t i = 0; i < std::min(size_t(20), highly_connected.size()); ++i) {
            std::string content = highly_connected[i].first;
            if (content.length() > 50) {
                content = content.substr(0, 47) + "...";
            }
            std::cout << std::setw(3) << (i + 1) << ". \"" << content 
                      << "\" - " << highly_connected[i].second << " connections\n";
        }
    }
    
    // ========================================================================
    // SAMPLE EDGES
    // ========================================================================
    
    print_header("SAMPLE EDGES (First 30)");
    
    auto all_edges = storage->get_all_edges();
    
    for (size_t i = 0; i < std::min(size_t(30), all_edges.size()); ++i) {
        const auto& edge = all_edges[i];
        
        std::string from_content = storage->get_node_content(edge.from_id);
        std::string to_content = storage->get_node_content(edge.to_id);
        
        if (from_content.length() > 25) from_content = from_content.substr(0, 22) + "...";
        if (to_content.length() > 25) to_content = to_content.substr(0, 22) + "...";
        
        std::cout << std::setw(3) << (i + 1) << ". \"" << from_content << "\" ";
        
        if (edge.relation == RelationType::EXACT) {
            std::cout << "─[EXACT]→ ";
        } else {
            std::cout << "─[LEAP]─→ ";
        }
        
        std::cout << "\"" << to_content << "\"\n";
    }
    
    // ========================================================================
    // CONTENT LENGTH ANALYSIS
    // ========================================================================
    
    print_header("CONTENT LENGTH ANALYSIS");
    
    std::map<int, int> length_distribution;
    int total_chars = 0;
    int min_len = INT_MAX;
    int max_len = 0;
    
    for (const auto& node : all_nodes) {
        std::string content = storage->get_node_content(node.id);
        int len = content.length();
        
        total_chars += len;
        min_len = std::min(min_len, len);
        max_len = std::max(max_len, len);
        
        // Bucket by length
        int bucket = (len / 10) * 10;
        length_distribution[bucket]++;
    }
    
    std::cout << "Total characters: " << total_chars << "\n";
    std::cout << "Average length:   " << (total_nodes > 0 ? total_chars / total_nodes : 0) << " chars\n";
    std::cout << "Min length:       " << min_len << " chars\n";
    std::cout << "Max length:       " << max_len << " chars\n";
    
    std::cout << "\nLength distribution:\n";
    for (const auto& [bucket, count] : length_distribution) {
        if (count > 0) {
            std::cout << "  " << std::setw(4) << bucket << "-" << std::setw(3) << (bucket + 9) 
                      << " chars: " << count << " nodes\n";
        }
    }
    
    // ========================================================================
    // SUMMARY
    // ========================================================================
    
    print_header("SUMMARY");
    
    std::cout << "Database health:\n";
    std::cout << "  Total storage:      ~" << (total_chars / 1024.0f / 1024.0f) << " MB of text content\n";
    std::cout << "  Connectivity ratio: " << (connectivity_ratio * 100) << "%\n";
    std::cout << "  Isolated nodes:     " << (isolated_nodes * 100.0f / total_nodes) << "%\n";
    std::cout << "  LEAP efficiency:    " << (exact_edges > 0 ? (float)leap_edges / exact_edges : 0) << "x\n";
    
    if (connectivity_ratio > 0.8f) {
        std::cout << "\n✅ Database is WELL CONNECTED!\n";
    } else if (connectivity_ratio > 0.5f) {
        std::cout << "\n⚠️  Database is MODERATELY connected\n";
    } else {
        std::cout << "\n❌ Database has LOW connectivity\n";
    }
    
    std::cout << "\n";
    
    // Call the built-in stats printer for additional info
    std::cout << "════════════════════════════════════════════════════════════════\n";
    std::cout << "Built-in storage stats:\n";
    std::cout << "════════════════════════════════════════════════════════════════\n";
    storage->print_stats();
    
    return 0;
}

