/*
 * MELVIN UNIFIED BRAIN - STORAGE LAYER
 * Binary-native memory substrate (nodes + edges + persistence)
 */

#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>

namespace melvin_storage {

// ==================== CORE STRUCTURES ====================

struct Node {
    uint32_t type;              // NodeType enum
    std::string data;           // Text payload or encoded data
    float weight = 1.0f;        // Node importance/activation
    uint32_t sensory_type = 0;  // 0=text, 1=audio, 2=image, 3=motor
    float modality_weight = 1.0f;        // Modality-specific weight
    float cross_modal_coherence = 0.5f;  // Cross-modal binding strength
    uint64_t ts_created = 0;    // Creation timestamp
    uint64_t ts_accessed = 0;   // Last access timestamp
    int reinforcements = 0;     // Usage count
};

// Edge connection types
enum class EdgeType : uint8_t {
    EXACT = 0,  // 00 - Factual memory from training (bidirectional)
    LEAP = 1    // 10 - Intelligent inference (one-way, probabilistic)
};

struct Edge {
    int a, b;                   // Source and destination node indices
    EdgeType type = EdgeType::EXACT;  // Connection type
    uint32_t rel;               // Relation type (for compatibility)
    float w = 1.0f;             // Effective weight
    float w_core = 0.3f;        // Durable weight component
    float w_ctx = 0.7f;         // Context-specific component
    uint32_t count = 1;         // Usage count
    uint32_t successes = 0;     // Successful uses (for promotion)
    uint32_t failures = 0;      // Failed uses (for deletion)
    float leap_score = 0.0f;    // Initial leap score (if LEAP)
    uint64_t ts_last = 0;       // Last reinforcement timestamp
    bool is_cross_modal = false; // Cross-modal edge flag
    float cross_modal_bonus = 0.0f; // Extra cross-modal weight
    
    bool is_exact() const { return type == EdgeType::EXACT; }
    bool is_leap() const { return type == EdgeType::LEAP; }
};

// ==================== GLOBAL STORAGE ====================

inline std::vector<Node> nodes;
inline std::vector<Edge> edges;
inline std::unordered_map<std::string, int> node_lookup;  // text → node_id
inline std::unordered_map<int, std::vector<int>> adjacency;  // node_id → edge_ids

// ==================== NODE OPERATIONS ====================

inline int create_node(const std::string& data, uint32_t type = 0, uint32_t sensory_type = 0) {
    // Check if node already exists
    auto it = node_lookup.find(data);
    if (it != node_lookup.end()) {
        nodes[it->second].reinforcements++;
        nodes[it->second].ts_accessed = std::chrono::system_clock::now().time_since_epoch().count();
        return it->second;
    }
    
    // Create new node
    Node node;
    node.type = type;
    node.data = data;
    node.weight = 1.0f;
    node.sensory_type = sensory_type;
    node.ts_created = std::chrono::system_clock::now().time_since_epoch().count();
    node.ts_accessed = node.ts_created;
    node.reinforcements = 1;
    
    int node_id = (int)nodes.size();
    nodes.push_back(node);
    node_lookup[data] = node_id;
    
    return node_id;
}

inline void touch_node(int node_id) {
    if (node_id >= 0 && node_id < (int)nodes.size()) {
        nodes[node_id].ts_accessed = std::chrono::system_clock::now().time_since_epoch().count();
        nodes[node_id].reinforcements++;
    }
}

// ==================== EDGE OPERATIONS ====================

inline int connect(int a, int b, uint32_t rel, float w = 1.0f, EdgeType type = EdgeType::EXACT) {
    if (a < 0 || a >= (int)nodes.size() || b < 0 || b >= (int)nodes.size()) {
        return -1;
    }
    
    // Check if edge already exists
    for (size_t i = 0; i < edges.size(); ++i) {
        if (edges[i].a == a && edges[i].b == b && edges[i].rel == rel) {
            // Reinforce existing edge
            edges[i].count++;
            edges[i].w = std::min(1.0f, edges[i].w + 0.05f);
            edges[i].ts_last = std::chrono::system_clock::now().time_since_epoch().count();
            return (int)i;
        }
    }
    
    // Create new edge
    Edge edge;
    edge.a = a;
    edge.b = b;
    edge.type = type;
    edge.rel = rel;
    edge.w = w;
    edge.w_core = type == EdgeType::EXACT ? 0.5f : 0.2f;  // EXACT starts stronger
    edge.w_ctx = type == EdgeType::EXACT ? 0.5f : 0.3f;
    edge.count = 1;
    edge.ts_last = std::chrono::system_clock::now().time_since_epoch().count();
    
    // Check if cross-modal
    if (nodes[a].sensory_type != nodes[b].sensory_type) {
        edge.is_cross_modal = true;
    }
    
    int edge_id = (int)edges.size();
    edges.push_back(edge);
    
    // Update adjacency
    adjacency[a].push_back(edge_id);
    
    // If EXACT connection, create bidirectional link
    if (type == EdgeType::EXACT) {
        Edge reverse_edge = edge;
        reverse_edge.a = b;
        reverse_edge.b = a;
        int reverse_id = (int)edges.size();
        edges.push_back(reverse_edge);
        adjacency[b].push_back(reverse_id);
    }
    
    return edge_id;
}

inline void reinforce_edge(int edge_id, float reward = 0.05f) {
    if (edge_id >= 0 && edge_id < (int)edges.size()) {
        edges[edge_id].w_core = std::min(1.0f, edges[edge_id].w_core + reward * 0.3f);
        edges[edge_id].w_ctx = std::min(1.0f, edges[edge_id].w_ctx + reward * 0.7f);
        edges[edge_id].w = 0.3f * edges[edge_id].w_core + 0.7f * edges[edge_id].w_ctx;
        edges[edge_id].count++;
        edges[edge_id].ts_last = std::chrono::system_clock::now().time_since_epoch().count();
        
        // Extra boost for cross-modal edges
        if (edges[edge_id].is_cross_modal) {
            edges[edge_id].cross_modal_bonus += 0.1f * reward;
        }
    }
}

// ==================== PERSISTENCE ====================

// CRITICAL: Always use the SAME brain file location - no exceptions!
inline void save_brain(const std::string& filename = "data/melvin_brain.bin") {
    // Force all saves to go to the canonical location
    const std::string canonical_path = "data/melvin_brain.bin";
    
    std::ofstream file(canonical_path, std::ios::binary);
    if (!file) {
        // Try creating data directory
        std::cout << "Creating data directory...\n";
        system("mkdir -p data");
        file.open(canonical_path, std::ios::binary);
        if (!file) return;
    }
    
    // Save header
    uint32_t node_count = nodes.size();
    uint32_t edge_count = edges.size();
    file.write((char*)&node_count, sizeof(node_count));
    file.write((char*)&edge_count, sizeof(edge_count));
    
    // Save nodes
    for (const auto& node : nodes) {
        file.write((char*)&node.type, sizeof(node.type));
        file.write((char*)&node.sensory_type, sizeof(node.sensory_type));
        file.write((char*)&node.weight, sizeof(node.weight));
        file.write((char*)&node.modality_weight, sizeof(node.modality_weight));
        file.write((char*)&node.cross_modal_coherence, sizeof(node.cross_modal_coherence));
        file.write((char*)&node.ts_created, sizeof(node.ts_created));
        file.write((char*)&node.ts_accessed, sizeof(node.ts_accessed));
        file.write((char*)&node.reinforcements, sizeof(node.reinforcements));
        
        uint32_t data_len = node.data.length();
        file.write((char*)&data_len, sizeof(data_len));
        file.write(node.data.c_str(), data_len);
    }
    
    // Save edges
    for (const auto& edge : edges) {
        file.write((char*)&edge.a, sizeof(edge.a));
        file.write((char*)&edge.b, sizeof(edge.b));
        uint8_t edge_type = static_cast<uint8_t>(edge.type);
        file.write((char*)&edge_type, sizeof(edge_type));
        file.write((char*)&edge.rel, sizeof(edge.rel));
        file.write((char*)&edge.w, sizeof(edge.w));
        file.write((char*)&edge.w_core, sizeof(edge.w_core));
        file.write((char*)&edge.w_ctx, sizeof(edge.w_ctx));
        file.write((char*)&edge.count, sizeof(edge.count));
        file.write((char*)&edge.successes, sizeof(edge.successes));
        file.write((char*)&edge.failures, sizeof(edge.failures));
        file.write((char*)&edge.leap_score, sizeof(edge.leap_score));
        file.write((char*)&edge.ts_last, sizeof(edge.ts_last));
        file.write((char*)&edge.is_cross_modal, sizeof(edge.is_cross_modal));
        file.write((char*)&edge.cross_modal_bonus, sizeof(edge.cross_modal_bonus));
    }
    
    std::cout << "💾 Saved: " << node_count << " nodes, " << edge_count << " edges\n";
}

// CRITICAL: Always load from the SAME brain file location - no exceptions!
inline void load_brain(const std::string& filename = "data/melvin_brain.bin") {
    // Force all loads to come from the canonical location
    const std::string canonical_path = "data/melvin_brain.bin";
    
    std::ifstream file(canonical_path, std::ios::binary);
    if (!file) {
        std::cout << "⚠️  No existing brain found at " << canonical_path << " - starting fresh\n";
        return;
    }
    
    // Clear existing data
    nodes.clear();
    edges.clear();
    node_lookup.clear();
    adjacency.clear();
    
    // Load header
    uint32_t node_count, edge_count;
    file.read((char*)&node_count, sizeof(node_count));
    file.read((char*)&edge_count, sizeof(edge_count));
    
    // Load nodes
    for (uint32_t i = 0; i < node_count; ++i) {
        Node node;
        file.read((char*)&node.type, sizeof(node.type));
        file.read((char*)&node.sensory_type, sizeof(node.sensory_type));
        file.read((char*)&node.weight, sizeof(node.weight));
        file.read((char*)&node.modality_weight, sizeof(node.modality_weight));
        file.read((char*)&node.cross_modal_coherence, sizeof(node.cross_modal_coherence));
        file.read((char*)&node.ts_created, sizeof(node.ts_created));
        file.read((char*)&node.ts_accessed, sizeof(node.ts_accessed));
        file.read((char*)&node.reinforcements, sizeof(node.reinforcements));
        
        uint32_t data_len;
        file.read((char*)&data_len, sizeof(data_len));
        node.data.resize(data_len);
        file.read(&node.data[0], data_len);
        
        nodes.push_back(node);
        node_lookup[node.data] = i;
    }
    
    // Load edges
    for (uint32_t i = 0; i < edge_count; ++i) {
        Edge edge;
        file.read((char*)&edge.a, sizeof(edge.a));
        file.read((char*)&edge.b, sizeof(edge.b));
        uint8_t edge_type;
        file.read((char*)&edge_type, sizeof(edge_type));
        edge.type = static_cast<EdgeType>(edge_type);
        file.read((char*)&edge.rel, sizeof(edge.rel));
        file.read((char*)&edge.w, sizeof(edge.w));
        file.read((char*)&edge.w_core, sizeof(edge.w_core));
        file.read((char*)&edge.w_ctx, sizeof(edge.w_ctx));
        file.read((char*)&edge.count, sizeof(edge.count));
        file.read((char*)&edge.successes, sizeof(edge.successes));
        file.read((char*)&edge.failures, sizeof(edge.failures));
        file.read((char*)&edge.leap_score, sizeof(edge.leap_score));
        file.read((char*)&edge.ts_last, sizeof(edge.ts_last));
        file.read((char*)&edge.is_cross_modal, sizeof(edge.is_cross_modal));
        file.read((char*)&edge.cross_modal_bonus, sizeof(edge.cross_modal_bonus));
        
        edges.push_back(edge);
        adjacency[edge.a].push_back(i);
    }
    
    std::cout << "💾 Loaded: " << node_count << " nodes, " << edge_count << " edges\n";
}

// ==================== STATISTICS ====================

inline void print_stats() {
    std::cout << "📊 Brain Stats:\n";
    std::cout << "   Nodes: " << nodes.size() << "\n";
    std::cout << "   Edges: " << edges.size() << "\n";
    
    // Count by modality
    std::map<uint32_t, int> modality_counts;
    for (const auto& node : nodes) {
        modality_counts[node.sensory_type]++;
    }
    
    std::cout << "   By modality:\n";
    const char* modality_names[] = {"Text", "Audio", "Image", "Motor"};
    for (const auto& [type, count] : modality_counts) {
        if (type < 4) {
            std::cout << "     " << modality_names[type] << ": " << count << "\n";
        }
    }
    
    // Count by edge type
    int exact_count = 0, leap_count = 0, cross_modal_count = 0;
    for (const auto& edge : edges) {
        if (edge.is_exact()) exact_count++;
        if (edge.is_leap()) leap_count++;
        if (edge.is_cross_modal) cross_modal_count++;
    }
    std::cout << "   EXACT edges: " << exact_count << "\n";
    std::cout << "   LEAP edges: " << leap_count << "\n";
    std::cout << "   Cross-modal edges: " << cross_modal_count << "\n";
}

} // namespace melvin_storage

