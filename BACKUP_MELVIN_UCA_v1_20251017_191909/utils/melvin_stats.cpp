/*
 * Melvin Stats - Quick Brain Snapshot
 * 
 * Lightweight utility to check Melvin's brain state:
 * - Total nodes
 * - Total connections (edges)
 * - EXACT vs LEAP breakdown
 * - Storage sizes
 * - Runtime state summary
 * 
 * No reasoning, no testing - just a pure snapshot!
 */

#include "../core/storage.h"
#include <iostream>
#include <iomanip>
#include <sys/stat.h>

using namespace melvin;

// Get file size
size_t get_file_size(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return st.st_size;
    }
    return 0;
}

// Format bytes
std::string format_bytes(size_t bytes) {
    if (bytes < 1024) {
        return std::to_string(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return std::to_string(bytes / 1024) + " KB";
    } else {
        return std::to_string(bytes / (1024 * 1024)) + " MB";
    }
}

int main(int argc, char* argv[]) {
    // Parse optional path arguments
    std::string nodes_path = "data/nodes.melvin";
    std::string edges_path = "data/edges.melvin";
    
    if (argc > 1) nodes_path = argv[1];
    if (argc > 2) edges_path = argv[2];
    
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN BRAIN SNAPSHOT                                ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    // Load brain
    Storage storage;
    
    std::cout << "📂 Loading from:\n";
    std::cout << "  " << nodes_path << "\n";
    std::cout << "  " << edges_path << "\n\n";
    
    if (!storage.load(nodes_path, edges_path)) {
        std::cerr << "✗ Failed to load brain!\n";
        std::cerr << "  Files may not exist or are corrupted.\n\n";
        return 1;
    }
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  BRAIN STATISTICS\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    // Get totals
    size_t total_nodes = storage.node_count();
    size_t total_edges = storage.edge_count();
    size_t exact_count = storage.edge_count_by_type(RelationType::EXACT);
    size_t leap_count = storage.edge_count_by_type(RelationType::LEAP);
    
    std::cout << "🧠 Total Nodes:        " << std::setw(10) << total_nodes << "\n";
    std::cout << "🔗 Total Connections:  " << std::setw(10) << total_edges << "\n\n";
    
    std::cout << "Connection Types:\n";
    std::cout << "  EXACT (taught):      " << std::setw(10) << exact_count;
    if (total_edges > 0) {
        std::cout << " (" << std::fixed << std::setprecision(1) 
                  << (exact_count * 100.0f / total_edges) << "%)";
    }
    std::cout << "\n";
    
    std::cout << "  LEAP (inferred):     " << std::setw(10) << leap_count;
    if (total_edges > 0) {
        std::cout << " (" << std::fixed << std::setprecision(1) 
                  << (leap_count * 100.0f / total_edges) << "%)";
    }
    std::cout << "\n\n";
    
    // Runtime state
    auto nodes = storage.get_all_nodes();
    auto edges = storage.get_all_edges();
    
    int active_nodes = 0;
    int active_edges = 0;
    uint64_t total_activations = 0;
    uint64_t total_coactivations = 0;
    float total_node_weight = 0.0f;
    float total_edge_weight = 0.0f;
    
    for (const auto& node : nodes) {
        if (node.activations > 0) {
            active_nodes++;
            total_activations += node.activations;
        }
        total_node_weight += node.weight;
    }
    
    for (const auto& edge : edges) {
        if (edge.coactivations > 0) {
            active_edges++;
            total_coactivations += edge.coactivations;
        }
        total_edge_weight += edge.adaptive_weight;
    }
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  RUNTIME STATE\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    std::cout << "⚡ Activations:\n";
    std::cout << "  Active nodes:        " << std::setw(10) << active_nodes << " / " << total_nodes;
    if (total_nodes > 0) {
        std::cout << " (" << std::fixed << std::setprecision(1) 
                  << (active_nodes * 100.0f / total_nodes) << "%)";
    }
    std::cout << "\n";
    
    std::cout << "  Total activations:   " << std::setw(10) << total_activations << "\n";
    std::cout << "  Active edges:        " << std::setw(10) << active_edges << " / " << total_edges;
    if (total_edges > 0) {
        std::cout << " (" << std::fixed << std::setprecision(1) 
                  << (active_edges * 100.0f / total_edges) << "%)";
    }
    std::cout << "\n";
    std::cout << "  Total coactivations: " << std::setw(10) << total_coactivations << "\n\n";
    
    std::cout << "⚖️  Weights:\n";
    std::cout << "  Avg node weight:     " << std::setw(10) << std::fixed << std::setprecision(6)
              << (total_nodes > 0 ? total_node_weight / total_nodes : 0.0f) << "\n";
    std::cout << "  Avg edge weight:     " << std::setw(10) 
              << (total_edges > 0 ? total_edge_weight / total_edges : 0.0f) << "\n\n";
    
    // Storage efficiency
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  STORAGE EFFICIENCY\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    size_t nodes_file_size = get_file_size(nodes_path);
    size_t edges_file_size = get_file_size(edges_path);
    size_t total_size = nodes_file_size + edges_file_size;
    
    std::cout << "💾 Disk Usage:\n";
    std::cout << "  Nodes file:          " << std::setw(10) << format_bytes(nodes_file_size) << "\n";
    std::cout << "  Edges file:          " << std::setw(10) << format_bytes(edges_file_size) << "\n";
    std::cout << "  Total:               " << std::setw(10) << format_bytes(total_size) << "\n\n";
    
    if (total_nodes > 0 && total_edges > 0) {
        size_t bytes_per_node = nodes_file_size / total_nodes;
        size_t bytes_per_edge = edges_file_size / total_edges;
        
        std::cout << "📊 Per-Item:\n";
        std::cout << "  Bytes/node:          " << std::setw(10) << bytes_per_node << " bytes\n";
        std::cout << "  Bytes/edge:          " << std::setw(10) << bytes_per_edge << " bytes\n\n";
    }
    
    // Efficiency metrics
    if (total_nodes > 0) {
        float efficiency = (active_nodes * 100.0f / total_nodes);
        std::cout << "🎯 Efficiency:\n";
        std::cout << "  Utilization:         " << std::setw(10) << std::fixed << std::setprecision(1)
                  << efficiency << "% (active/total)\n";
        
        if (efficiency < 10) {
            std::cout << "  Status:              Sparse (most nodes unused)\n";
        } else if (efficiency < 50) {
            std::cout << "  Status:              Moderate (some nodes active)\n";
        } else {
            std::cout << "  Status:              Dense (many nodes active)\n";
        }
        std::cout << "\n";
    }
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  SUMMARY\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    std::cout << "  Brain Size:          " << total_nodes << " nodes, " << total_edges << " connections\n";
    std::cout << "  Disk Footprint:      " << format_bytes(total_size) << "\n";
    std::cout << "  Learning State:      " << active_nodes << " active nodes, " 
              << total_activations << " total activations\n";
    std::cout << "  LEAP Inference:      " << leap_count << " inferred connections\n\n";
    
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ SNAPSHOT COMPLETE                                 ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    return 0;
}

