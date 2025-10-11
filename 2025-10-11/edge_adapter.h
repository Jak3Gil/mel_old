#pragma once

#include "storage.h"
#include "melvin_types.h"
#include <cstring>
#include <unordered_map>
#include <vector>

namespace melvin {

// Runtime lightweight edge for reasoning (uint64_t-based)
struct RuntimeEdge {
    uint64_t u, v;          // Source and destination node IDs
    float weight;           // Combined weight [0,1]
    float w_core;           // Core durable weight
    float w_ctx;            // Context weight
    uint32_t count;         // Usage count
    uint8_t rel;            // Relation type
    float last_used;        // Timestamp of last use
    
    RuntimeEdge() : u(0), v(0), weight(0.5f), w_core(0.3f), w_ctx(0.2f), count(1), rel(0), last_used(0.0f) {}
    RuntimeEdge(uint64_t src, uint64_t dst, float w, uint8_t r) 
        : u(src), v(dst), weight(w), w_core(w*0.3f), w_ctx(w*0.7f), count(1), rel(r), last_used(0.0f) {}
};

// Storage adapter: converts between disk format (EdgeRec) and runtime format (RuntimeEdge)
class EdgeAdapter {
public:
    // Convert on-disk EdgeRec to runtime RuntimeEdge
    static RuntimeEdge from_storage(const EdgeRec& disk_edge) {
        RuntimeEdge edge;
        
        // Extract source and destination as uint64_t
        // Take first 8 bytes of the 32-byte NodeID
        std::memcpy(&edge.u, disk_edge.src, sizeof(uint64_t));
        std::memcpy(&edge.v, disk_edge.dst, sizeof(uint64_t));
        
        edge.rel = static_cast<uint8_t>(disk_edge.rel);
        edge.weight = disk_edge.w;
        edge.w_core = disk_edge.w_core;
        edge.w_ctx = disk_edge.w_ctx;
        edge.count = disk_edge.count;
        edge.last_used = static_cast<float>(disk_edge.ts_last) / 1e9f; // ns to seconds
        
        return edge;
    }
    
    // Convert runtime RuntimeEdge to on-disk EdgeRec
    static EdgeRec to_storage(const RuntimeEdge& runtime_edge) {
        EdgeRec disk_edge{};
        
        // Convert uint64_t to 32-byte NodeID (zero-padded)
        std::memcpy(disk_edge.src, &runtime_edge.u, sizeof(uint64_t));
        std::memcpy(disk_edge.dst, &runtime_edge.v, sizeof(uint64_t));
        
        disk_edge.rel = runtime_edge.rel;
        disk_edge.w = runtime_edge.weight;
        disk_edge.w_core = runtime_edge.w_core;
        disk_edge.w_ctx = runtime_edge.w_ctx;
        disk_edge.count = runtime_edge.count;
        disk_edge.ts_last = static_cast<uint64_t>(runtime_edge.last_used * 1e9f); // seconds to ns
        disk_edge.layer = 0;
        disk_edge.flags = 0;
        disk_edge.pad = 0;
        
        return disk_edge;
    }
    
    // Batch conversion helpers
    static std::vector<RuntimeEdge> load_runtime_edges(const std::vector<EdgeRec>& disk_edges) {
        std::vector<RuntimeEdge> runtime_edges;
        runtime_edges.reserve(disk_edges.size());
        for (const auto& disk_edge : disk_edges) {
            runtime_edges.push_back(from_storage(disk_edge));
        }
        return runtime_edges;
    }
    
    static std::vector<EdgeRec> save_runtime_edges(const std::vector<RuntimeEdge>& runtime_edges) {
        std::vector<EdgeRec> disk_edges;
        disk_edges.reserve(runtime_edges.size());
        for (const auto& runtime_edge : runtime_edges) {
            disk_edges.push_back(to_storage(runtime_edge));
        }
        return disk_edges;
    }
};

// Unified edge access interface for reasoning system
class ReasoningGraph {
private:
    std::unordered_map<uint64_t, std::vector<size_t>> adj_; // node_id -> edge indices
    std::vector<RuntimeEdge> edges_;
    
public:
    ReasoningGraph() = default;
    
    // Load from storage system
    void load_from_storage(GraphStore* store) {
        // This would iterate through storage and convert edges
        // For now, we'll populate from G_edges directly in melvin.cpp
    }
    
    // Add edge
    size_t add_edge(const RuntimeEdge& edge) {
        edges_.push_back(edge);
        size_t idx = edges_.size() - 1;
        adj_[edge.u].push_back(idx);
        return idx;
    }
    
    // Get edge
    const RuntimeEdge& get_edge(size_t idx) const {
        return edges_[idx];
    }
    
    RuntimeEdge& get_edge_mut(size_t idx) {
        return edges_[idx];
    }
    
    // Get adjacency
    const std::vector<size_t>& get_adj(uint64_t node_id) const {
        static const std::vector<size_t> empty;
        auto it = adj_.find(node_id);
        return it != adj_.end() ? it->second : empty;
    }
    
    // Get all edges (const)
    const std::vector<RuntimeEdge>& edges() const { return edges_; }
    
    // Get all edges (mutable)
    std::vector<RuntimeEdge>& edges_mut() { return edges_; }
    
    // Get adjacency map
    const std::unordered_map<uint64_t, std::vector<size_t>>& adjacency() const { return adj_; }
    
    size_t edge_count() const { return edges_.size(); }
    size_t node_count() const { return adj_.size(); }
};

} // namespace melvin

