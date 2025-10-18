#include "melvin_graph.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cmath>

namespace melvin {

AtomicGraph::AtomicGraph() 
    : next_node_id_(1000)
    , next_object_id_(1)
{}

AtomicGraph::~AtomicGraph() {}

// ============================================================================
// NODE CREATION
// ============================================================================

uint64_t AtomicGraph::get_or_create_concept(std::string_view label) {
    std::string label_str(label);
    
    auto it = label_to_id_.find(label_str);
    if (it != label_to_id_.end()) {
        return it->second;
    }
    
    uint64_t id = hash_string(label_str);
    Node n;
    n.id = id;
    n.type = NodeType::CONCEPT;
    nodes_[id] = n;
    label_to_id_[label_str] = id;
    
    return id;
}

uint64_t AtomicGraph::get_or_create_visual_patch(uint64_t frame_id, int px, int py) {
    uint64_t id = hash_patch_id(frame_id, px, py);
    
    if (nodes_.find(id) != nodes_.end()) {
        return id;
    }
    
    Node n;
    n.id = id;
    n.type = NodeType::VISUAL_PATCH;
    n.a = px;
    n.b = py;
    nodes_[id] = n;
    
    return id;
}

uint64_t AtomicGraph::create_frame(const FrameMeta& frame_meta) {
    uint64_t id = frame_meta.id;
    
    Node n;
    n.id = id;
    n.type = NodeType::FRAME;
    n.a = frame_meta.w;
    n.b = frame_meta.h;
    nodes_[id] = n;
    
    return id;
}

uint64_t AtomicGraph::create_object(int center_x, int center_y) {
    uint64_t id = next_object_id_++;
    
    Node n;
    n.id = id;
    n.type = NodeType::OBJECT;
    n.a = center_x;
    n.b = center_y;
    nodes_[id] = n;
    
    return id;
}

// ============================================================================
// EDGE CHEMISTRY
// ============================================================================

std::string AtomicGraph::edge_key(uint64_t a, uint64_t b, Rel r) const {
    return std::to_string(a) + "_" + std::to_string(b) + "_" + std::to_string(static_cast<int>(r));
}

void AtomicGraph::add_or_bump_edge(uint64_t a, uint64_t b, Rel r, float inc) {
    std::string key = edge_key(a, b, r);
    
    auto it = edge_index_.find(key);
    if (it != edge_index_.end()) {
        // Edge exists, bump weight
        edges_[it->second].w += inc;
    } else {
        // New edge
        Edge e(a, b, r, inc);
        size_t idx = edges_.size();
        edges_.push_back(e);
        edge_index_[key] = idx;
        
        // Update adjacency
        adjacency_[a].push_back(idx);
    }
}

void AtomicGraph::decay_edges(float seconds, float lambda) {
    float decay_factor = std::exp(-lambda * seconds);
    
    for (auto& edge : edges_) {
        edge.w *= decay_factor;
    }
    
    // Remove very weak edges
    edges_.erase(
        std::remove_if(edges_.begin(), edges_.end(),
            [](const Edge& e) { return e.w < 0.01f; }),
        edges_.end()
    );
    
    // Rebuild indices
    edge_index_.clear();
    adjacency_.clear();
    for (size_t i = 0; i < edges_.size(); ++i) {
        const auto& e = edges_[i];
        std::string key = edge_key(e.a, e.b, e.r);
        edge_index_[key] = i;
        adjacency_[e.a].push_back(i);
    }
}

void AtomicGraph::reinforce(uint64_t a, uint64_t b, Rel r, float eta) {
    add_or_bump_edge(a, b, r, eta);
}

void AtomicGraph::add_temp_next(uint64_t a, uint64_t b) {
    add_or_bump_edge(a, b, Rel::TEMPORAL_NEXT, constants::REINFORCE_ETA);
}

bool AtomicGraph::maybe_form_leap(uint64_t a, uint64_t b, float error_delta, float threshold) {
    if (error_delta < threshold) {
        return false;
    }
    
    // Check if shortcut doesn't already exist
    std::string key = edge_key(a, b, Rel::EXPECTS);
    if (edge_index_.find(key) != edge_index_.end()) {
        return false;
    }
    
    // Form LEAP
    add_or_bump_edge(a, b, Rel::EXPECTS, 0.7f);
    return true;
}

// ============================================================================
// QUERIES
// ============================================================================

std::vector<uint64_t> AtomicGraph::neighbors(uint64_t id, 
                                              std::initializer_list<Rel> rels,
                                              size_t k) const {
    std::vector<uint64_t> result;
    
    auto it = adjacency_.find(id);
    if (it == adjacency_.end()) {
        return result;
    }
    
    for (size_t idx : it->second) {
        const Edge& e = edges_[idx];
        
        // Check if relation matches filter
        bool matches = rels.size() == 0;
        for (Rel r : rels) {
            if (e.r == r) {
                matches = true;
                break;
            }
        }
        
        if (matches) {
            result.push_back(e.b);
            if (result.size() >= k) break;
        }
    }
    
    return result;
}

std::vector<uint64_t> AtomicGraph::all_neighbors(uint64_t id, size_t k) const {
    return neighbors(id, {}, k);
}

float AtomicGraph::get_edge_weight(uint64_t a, uint64_t b, Rel r) const {
    std::string key = edge_key(a, b, r);
    auto it = edge_index_.find(key);
    if (it == edge_index_.end()) {
        return 0.0f;
    }
    return edges_[it->second].w;
}

bool AtomicGraph::has_node(uint64_t id) const {
    return nodes_.find(id) != nodes_.end();
}

const Node* AtomicGraph::get_node(uint64_t id) const {
    auto it = nodes_.find(id);
    if (it == nodes_.end()) {
        return nullptr;
    }
    return &it->second;
}

// ============================================================================
// PERSISTENCE
// ============================================================================

bool AtomicGraph::save(const std::string& nodes_path, const std::string& edges_path) const {
    // Save nodes
    std::ofstream nf(nodes_path, std::ios::binary);
    if (!nf) return false;
    
    size_t node_count = nodes_.size();
    nf.write(reinterpret_cast<const char*>(&node_count), sizeof(node_count));
    for (const auto& [id, node] : nodes_) {
        nf.write(reinterpret_cast<const char*>(&node), sizeof(Node));
    }
    nf.close();
    
    // Save edges
    std::ofstream ef(edges_path, std::ios::binary);
    if (!ef) return false;
    
    size_t edge_count = edges_.size();
    ef.write(reinterpret_cast<const char*>(&edge_count), sizeof(edge_count));
    for (const auto& edge : edges_) {
        ef.write(reinterpret_cast<const char*>(&edge), sizeof(Edge));
    }
    ef.close();
    
    return true;
}

bool AtomicGraph::load(const std::string& nodes_path, const std::string& edges_path) {
    // Load nodes
    std::ifstream nf(nodes_path, std::ios::binary);
    if (!nf) return false;
    
    size_t node_count;
    nf.read(reinterpret_cast<char*>(&node_count), sizeof(node_count));
    
    nodes_.clear();
    for (size_t i = 0; i < node_count; ++i) {
        Node node;
        nf.read(reinterpret_cast<char*>(&node), sizeof(Node));
        nodes_[node.id] = node;
    }
    nf.close();
    
    // Load edges
    std::ifstream ef(edges_path, std::ios::binary);
    if (!ef) return false;
    
    size_t edge_count;
    ef.read(reinterpret_cast<char*>(&edge_count), sizeof(edge_count));
    
    edges_.clear();
    for (size_t i = 0; i < edge_count; ++i) {
        Edge edge;
        ef.read(reinterpret_cast<char*>(&edge), sizeof(Edge));
        edges_.push_back(edge);
    }
    ef.close();
    
    // Rebuild indices
    edge_index_.clear();
    adjacency_.clear();
    for (size_t i = 0; i < edges_.size(); ++i) {
        const auto& e = edges_[i];
        std::string key = edge_key(e.a, e.b, e.r);
        edge_index_[key] = i;
        adjacency_[e.a].push_back(i);
    }
    
    return true;
}

// ============================================================================
// STATISTICS
// ============================================================================

void AtomicGraph::print_stats() const {
    std::cout << "\n╔══════════════════════════════════════════╗\n";
    std::cout << "║  AtomicGraph Statistics                  ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n";
    std::cout << "  Nodes: " << nodes_.size() << "\n";
    std::cout << "  Edges: " << edges_.size() << "\n";
    
    // Count by type
    std::unordered_map<NodeType, size_t> type_counts;
    for (const auto& [id, node] : nodes_) {
        type_counts[node.type]++;
    }
    
    std::cout << "  Node types:\n";
    std::cout << "    FRAME: " << type_counts[NodeType::FRAME] << "\n";
    std::cout << "    VISUAL_PATCH: " << type_counts[NodeType::VISUAL_PATCH] << "\n";
    std::cout << "    OBJECT: " << type_counts[NodeType::OBJECT] << "\n";
    std::cout << "    CONCEPT: " << type_counts[NodeType::CONCEPT] << "\n";
    
    // Average edge weight
    float avg_weight = 0;
    if (!edges_.empty()) {
        for (const auto& e : edges_) {
            avg_weight += e.w;
        }
        avg_weight /= edges_.size();
    }
    std::cout << "  Avg edge weight: " << avg_weight << "\n";
    std::cout << std::endl;
}

} // namespace melvin


