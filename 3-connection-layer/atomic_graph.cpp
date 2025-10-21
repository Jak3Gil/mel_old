/*
 * AtomicGraph Implementation
 * Minimal binary graph for vision + language
 */

#include "atomic_graph.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstring>

namespace melvin {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

AtomicGraph::AtomicGraph() {
    // Reserve space for efficiency
    nodes_.reserve(10000);
    edges_.reserve(50000);
}

AtomicGraph::~AtomicGraph() = default;

// ============================================================================
// NODE OPERATIONS
// ============================================================================

uint64_t AtomicGraph::get_or_create_node(const std::string& label, uint8_t type) {
    // Check if node already exists
    auto it = label_to_id_.find(label);
    if (it != label_to_id_.end()) {
        return it->second;
    }
    
    // Create new node
    uint64_t node_id = next_id_++;
    
    Node node(node_id, type, label);
    nodes_.push_back(node);
    
    label_to_id_[label] = node_id;
    
    return node_id;
}

const Node* AtomicGraph::get_node(uint64_t id) const {
    for (const auto& node : nodes_) {
        if (node.id == id) {
            return &node;
        }
    }
    return nullptr;
}

std::vector<uint64_t> AtomicGraph::find_nodes(const std::string& substring) const {
    std::vector<uint64_t> results;
    
    for (const auto& node : nodes_) {
        std::string label_str(node.label);
        if (label_str.find(substring) != std::string::npos) {
            results.push_back(node.id);
        }
    }
    
    return results;
}

// ============================================================================
// EDGE OPERATIONS
// ============================================================================

int AtomicGraph::find_edge_index(uint64_t a, uint64_t b, uint8_t rel) const {
    for (size_t i = 0; i < edges_.size(); ++i) {
        const auto& edge = edges_[i];
        if (edge.a == a && edge.b == b && edge.rel == rel) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void AtomicGraph::add_edge(uint64_t a, uint64_t b, uint8_t rel, float inc) {
    // Find existing edge
    int idx = find_edge_index(a, b, rel);
    
    if (idx >= 0) {
        // Increment existing edge weight
        edges_[idx].w += inc;
    } else {
        // Create new edge
        Edge edge(a, b, rel, inc);
        edges_.push_back(edge);
    }
}

void AtomicGraph::decay_edges(float factor) {
    for (auto& edge : edges_) {
        edge.w *= factor;
    }
}

std::vector<uint64_t> AtomicGraph::neighbors(uint64_t id, uint8_t rel_filter) const {
    std::vector<uint64_t> result;
    
    for (const auto& edge : edges_) {
        // Check relation filter
        if (rel_filter != 255 && edge.rel != rel_filter) {
            continue;
        }
        
        // Check if id is source or destination
        if (edge.a == id) {
            result.push_back(edge.b);
        } else if (edge.b == id) {
            result.push_back(edge.a);
        }
    }
    
    return result;
}

float AtomicGraph::get_edge_weight(uint64_t a, uint64_t b, uint8_t rel) const {
    int idx = find_edge_index(a, b, rel);
    if (idx >= 0) {
        return edges_[idx].w;
    }
    return 0.0f;
}

// ============================================================================
// PERSISTENCE
// ============================================================================

void AtomicGraph::save(const std::string& path_nodes, const std::string& path_edges) const {
    // Save nodes
    {
        std::ofstream out(path_nodes, std::ios::binary);
        if (!out) {
            std::cerr << "[AtomicGraph] ERROR: Cannot open " << path_nodes << " for writing\n";
            return;
        }
        
        // Write count
        uint64_t count = nodes_.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        
        // Write nodes
        for (const auto& node : nodes_) {
            out.write(reinterpret_cast<const char*>(&node), sizeof(Node));
        }
        
        out.close();
    }
    
    // Save edges
    {
        std::ofstream out(path_edges, std::ios::binary);
        if (!out) {
            std::cerr << "[AtomicGraph] ERROR: Cannot open " << path_edges << " for writing\n";
            return;
        }
        
        // Write count
        uint64_t count = edges_.size();
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        
        // Write edges
        for (const auto& edge : edges_) {
            out.write(reinterpret_cast<const char*>(&edge), sizeof(Edge));
        }
        
        out.close();
    }
}

void AtomicGraph::load(const std::string& path_nodes, const std::string& path_edges) {
    // Clear existing data
    nodes_.clear();
    edges_.clear();
    label_to_id_.clear();
    next_id_ = 1;
    
    // Load nodes
    {
        std::ifstream in(path_nodes, std::ios::binary);
        if (!in) {
            // File doesn't exist, start fresh
            return;
        }
        
        // Read count
        uint64_t count = 0;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        
        // Read nodes
        nodes_.reserve(count);
        for (uint64_t i = 0; i < count; ++i) {
            Node node;
            in.read(reinterpret_cast<char*>(&node), sizeof(Node));
            nodes_.push_back(node);
            
            // Rebuild label map
            label_to_id_[std::string(node.label)] = node.id;
            
            // Update next_id
            if (node.id >= next_id_) {
                next_id_ = node.id + 1;
            }
        }
        
        in.close();
    }
    
    // Load edges
    {
        std::ifstream in(path_edges, std::ios::binary);
        if (!in) {
            // File doesn't exist, that's okay
            return;
        }
        
        // Read count
        uint64_t count = 0;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        
        // Read edges
        edges_.reserve(count);
        for (uint64_t i = 0; i < count; ++i) {
            Edge edge;
            in.read(reinterpret_cast<char*>(&edge), sizeof(Edge));
            edges_.push_back(edge);
        }
        
        in.close();
    }
}

// ============================================================================
// STATISTICS
// ============================================================================

void AtomicGraph::print_stats() const {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ⚛️  ATOMIC GRAPH STATISTICS                                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Nodes:           " << nodes_.size() << "\n";
    std::cout << "Edges:           " << edges_.size() << "\n";
    std::cout << "Total weight:    " << total_weight() << "\n";
    
    // Count by type
    size_t concepts = 0, instances = 0, features = 0;
    for (const auto& node : nodes_) {
        if (node.type == 0) concepts++;
        else if (node.type == 1) instances++;
        else features++;
    }
    
    std::cout << "\nNode types:\n";
    std::cout << "  Concepts:      " << concepts << "\n";
    std::cout << "  Instances:     " << instances << "\n";
    std::cout << "  Features:      " << features << "\n";
    
    // Count by relation
    std::unordered_map<uint8_t, size_t> rel_counts;
    for (const auto& edge : edges_) {
        rel_counts[edge.rel]++;
    }
    
    std::cout << "\nEdge relations:\n";
    const char* rel_names[] = {"INSTANCE_OF", "CO_OCCURS_WITH", "OBSERVED_AS", 
                               "NEAR", "TEMPORAL_NEXT", "NAMED"};
    for (const auto& pair : rel_counts) {
        if (pair.first < 6) {
            std::cout << "  " << rel_names[pair.first] << ": " << pair.second << "\n";
        }
    }
    
    // Memory usage
    size_t node_bytes = nodes_.size() * sizeof(Node);
    size_t edge_bytes = edges_.size() * sizeof(Edge);
    size_t total_bytes = node_bytes + edge_bytes;
    
    std::cout << "\nMemory usage:\n";
    std::cout << "  Nodes:         " << node_bytes << " bytes (" 
              << node_bytes / 1024.0 << " KB)\n";
    std::cout << "  Edges:         " << edge_bytes << " bytes (" 
              << edge_bytes / 1024.0 << " KB)\n";
    std::cout << "  Total:         " << total_bytes << " bytes (" 
              << total_bytes / 1024.0 / 1024.0 << " MB)\n";
    
    std::cout << "\n";
}

uint64_t AtomicGraph::total_weight() const {
    uint64_t total = 0;
    for (const auto& edge : edges_) {
        total += static_cast<uint64_t>(edge.w);
    }
    return total;
}

} // namespace melvin

