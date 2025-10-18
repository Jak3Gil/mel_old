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

struct AnalysisStats {
    size_t total_nodes = 0;
    size_t total_edges = 0;
    size_t exact_edges = 0;
    size_t leap_edges = 0;
    
    std::map<NodeType, size_t> nodes_by_type;
    std::map<std::string, size_t> node_content_lengths;
    
    size_t nodes_with_incoming = 0;
    size_t nodes_with_outgoing = 0;
    size_t isolated_nodes = 0;
    
    std::vector<std::pair<std::string, size_t>> most_connected_nodes;
};

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
    
    AnalysisStats stats;
    
    // ========================================================================
    // BASIC STATISTICS
    // ========================================================================
    
    print_header("BASIC STATISTICS");
    
    stats.total_nodes = storage->node_count();
    stats.total_edges = storage->edge_count();
    stats.exact_edges = storage->edge_count_by_type(RelationType::EXACT);
    stats.leap_edges = storage->edge_count_by_type(RelationType::LEAP);
    
    std::cout << "Nodes:         " << stats.total_nodes << "\n";
    std::cout << "Edges:         " << stats.total_edges << "\n";
    std::cout << "  EXACT:       " << stats.exact_edges << " (taught facts)\n";
    std::cout << "  LEAP:        " << stats.leap_edges << " (inferred shortcuts)\n";
    std::cout << "LEAP ratio:    " << (stats.exact_edges > 0 ? (float)stats.leap_edges / stats.exact_edges : 0.0f) 
              << " LEAPs per EXACT\n";
    
    // ========================================================================
    // NODE ANALYSIS
    // ========================================================================
    
    print_header("NODE ANALYSIS");
    
    auto all_nodes = storage->get_all_nodes();
    
    std::map<size_t, size_t> content_length_distribution;
    std::vector<std::pair<std::string, size_t>> nodes_with_edges;
    
    for (const auto& node : all_nodes) {
        // Track by type
        stats.nodes_by_type[node.type]++;
        
        // Track content length
        size_t content_len = node.content.length();
        content_length_distribution[content_len / 10 * 10]++; // Bucket by 10s
        
        // Check connectivity
        auto in_edges = storage->get_edges_to(node.id);
        auto out_edges = storage->get_edges_from(node.id);
        
        size_t total_connections = in_edges.size() + out_edges.size();
        
        if (!in_edges.empty()) stats.nodes_with_incoming++;
        if (!out_edges.empty()) stats.nodes_with_outgoing++;
        if (in_edges.empty() && out_edges.empty()) stats.isolated_nodes++;
        
        // Track for most connected
        if (total_connections > 0) {
            nodes_with_edges.push_back({node.content, total_connections});
        }
    }
    
    std::cout << "Node Types:\n";
    for (const auto& [type, count] : stats.nodes_by_type) {
        std::cout << "  ";
        switch(type) {
            case NodeType::CONCEPT: std::cout << "CONCEPT:  "; break;
            case NodeType::PHRASE: std::cout << "PHRASE:   "; break;
            case NodeType::SENSORY: std::cout << "SENSORY:  "; break;
            case NodeType::ABSTRACT: std::cout << "ABSTRACT: "; break;
            case NodeType::EMERGENT_DIM: std::cout << "EMERGENT: "; break;
        }
        std::cout << count << "\n";
    }
    
    std::cout << "\nConnectivity:\n";
    std::cout << "  Nodes with incoming edges: " << stats.nodes_with_incoming << "\n";
    std::cout << "  Nodes with outgoing edges: " << stats.nodes_with_outgoing << "\n";
    std::cout << "  Isolated nodes (no edges): " << stats.isolated_nodes << "\n";
    std::cout << "  Average edges per node:    " << (stats.total_nodes > 0 ? (float)stats.total_edges / stats.total_nodes : 0.0f) << "\n";
    
    // ========================================================================
    // SAMPLE NODES
    // ========================================================================
    
    print_header("SAMPLE NODES (First 20)");
    
    int shown = 0;
    for (const auto& node : all_nodes) {
        if (shown >= 20) break;
        
        auto out_edges = storage->get_edges_from(node.id);
        auto in_edges = storage->get_edges_to(node.id);
        
        std::cout << std::setw(3) << (shown + 1) << ". \"" << node.content << "\"\n";
        std::cout << "     Type: ";
        switch(node.type) {
            case NodeType::CONCEPT: std::cout << "CONCEPT"; break;
            case NodeType::PHRASE: std::cout << "PHRASE"; break;
            case NodeType::SENSORY: std::cout << "SENSORY"; break;
            case NodeType::ABSTRACT: std::cout << "ABSTRACT"; break;
            case NodeType::EMERGENT_DIM: std::cout << "EMERGENT"; break;
        }
        std::cout << " | Edges: " << in_edges.size() << " in, " << out_edges.size() << " out\n";
        
        shown++;
    }
    
    // ========================================================================
    // MOST CONNECTED NODES
    // ========================================================================
    
    print_header("TOP 20 MOST CONNECTED NODES");
    
    // Sort by connection count
    std::sort(nodes_with_edges.begin(), nodes_with_edges.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (size_t i = 0; i < std::min(size_t(20), nodes_with_edges.size()); ++i) {
        std::cout << std::setw(3) << (i + 1) << ". \"" << nodes_with_edges[i].first 
                  << "\" - " << nodes_with_edges[i].second << " connections\n";
    }
    
    // ========================================================================
    // SAMPLE EDGES
    // ========================================================================
    
    print_header("SAMPLE EDGES (First 20)");
    
    auto all_edges = storage->get_all_edges();
    
    for (size_t i = 0; i < std::min(size_t(20), all_edges.size()); ++i) {
        const auto& edge = all_edges[i];
        
        Node from_node, to_node;
        storage->get_node(edge.from_id, from_node);
        storage->get_node(edge.to_id, to_node);
        
        std::cout << std::setw(3) << (i + 1) << ". \"" << from_node.content << "\" ";
        
        if (edge.type == RelationType::EXACT) {
            std::cout << "─[EXACT]→ ";
        } else {
            std::cout << "─[LEAP]→ ";
        }
        
        std::cout << "\"" << to_node.content << "\"\n";
        std::cout << "     Confidence: " << edge.confidence << "\n";
    }
    
    // ========================================================================
    // EDGE PATTERNS
    // ========================================================================
    
    print_header("EDGE PATTERNS");
    
    std::map<std::pair<NodeType, NodeType>, size_t> edge_type_patterns;
    
    for (const auto& edge : all_edges) {
        Node from_node, to_node;
        if (storage->get_node(edge.from_id, from_node) && 
            storage->get_node(edge.to_id, to_node)) {
            edge_type_patterns[{from_node.type, to_node.type}]++;
        }
    }
    
    std::cout << "Edge patterns by node types:\n";
    for (const auto& [pattern, count] : edge_type_patterns) {
        std::cout << "  ";
        
        auto print_type = [](NodeType t) {
            switch(t) {
                case NodeType::CONCEPT: return "CONCEPT";
                case NodeType::PHRASE: return "PHRASE";
                case NodeType::SENSORY: return "SENSORY";
                case NodeType::ABSTRACT: return "ABSTRACT";
                case NodeType::EMERGENT_DIM: return "EMERGENT";
                default: return "UNKNOWN";
            }
        };
        
        std::cout << print_type(pattern.first) << " → " << print_type(pattern.second) 
                  << ": " << count << "\n";
    }
    
    // ========================================================================
    // SUMMARY
    // ========================================================================
    
    print_header("SUMMARY");
    
    std::cout << "Database health:\n";
    
    float connectivity_ratio = stats.total_nodes > 0 ? 
        (float)(stats.nodes_with_incoming + stats.nodes_with_outgoing) / (2.0f * stats.total_nodes) : 0.0f;
    
    std::cout << "  Connectivity ratio: " << (connectivity_ratio * 100) << "%\n";
    std::cout << "  Isolated nodes:     " << (stats.isolated_nodes * 100.0f / stats.total_nodes) << "%\n";
    std::cout << "  LEAP efficiency:    " << (stats.exact_edges > 0 ? stats.leap_edges / stats.exact_edges : 0) << "x\n";
    
    if (connectivity_ratio > 0.8f) {
        std::cout << "\n✅ Database is WELL CONNECTED!\n";
    } else if (connectivity_ratio > 0.5f) {
        std::cout << "\n⚠️  Database is MODERATELY connected\n";
    } else {
        std::cout << "\n❌ Database has LOW connectivity\n";
    }
    
    std::cout << "\n";
    
    return 0;
}

