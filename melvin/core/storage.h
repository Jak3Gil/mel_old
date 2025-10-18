#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace melvin {

/**
 * 📦 Storage - Binary Persistence Layer
 * 
 * Manages nodes and edges with efficient binary format.
 * Thread-safe, transaction-safe, snapshot support.
 * 
 * Note: Runtime fields (activation, weights) are NOT persisted
 *       Only core 4-byte node and 8-byte edge structures are saved
 */
class Storage {
public:
    Storage();
    ~Storage();
    
    // ========================================================================
    // LOAD / SAVE
    // ========================================================================
    
    bool load(const std::string& nodes_path, const std::string& edges_path);
    bool save(const std::string& nodes_path, const std::string& edges_path);
    
    // ========================================================================
    // NODE OPERATIONS
    // ========================================================================
    
    NodeID create_node(const std::string& content, NodeType type = NodeType::CONCEPT);
    bool get_node(const NodeID& id, Node& out) const;
    bool update_node(const Node& node);
    bool delete_node(const NodeID& id);
    
    std::vector<Node> find_nodes(const std::string& content_substring) const;
    std::vector<Node> get_all_nodes() const;
    
    // Direct access to node vector (for adaptive weighting)
    std::vector<Node>& get_nodes_mut();
    
    // ========================================================================
    // EDGE OPERATIONS
    // ========================================================================
    
    EdgeID create_edge(const NodeID& from, const NodeID& to, RelationType rel, float weight = 1.0f);
    bool get_edge(const EdgeID& id, Edge& out) const;
    bool update_edge(const Edge& edge);
    bool delete_edge(const EdgeID& id);
    
    std::vector<Edge> get_edges_from(const NodeID& node_id) const;
    std::vector<Edge> get_edges_to(const NodeID& node_id) const;
    std::vector<Edge> get_edges(const NodeID& from, const NodeID& to) const;
    std::vector<Edge> get_all_edges() const;
    
    // Direct access to edge vector (for adaptive weighting)
    std::vector<Edge>& get_edges_mut();
    
    // ========================================================================
    // GRAPH QUERIES
    // ========================================================================
    
    std::vector<NodeID> get_neighbors(const NodeID& node_id) const;
    std::vector<ReasoningPath> find_paths(const NodeID& from, const NodeID& to, int max_hops = 5) const;
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    size_t node_count() const;
    size_t edge_count() const;
    size_t edge_count_by_type(RelationType type) const;
    void print_stats() const;
    
    // ========================================================================
    // SNAPSHOTS
    // ========================================================================
    
    bool create_snapshot(const std::string& snapshot_path);
    bool restore_snapshot(const std::string& snapshot_path);
    
    // ========================================================================
    // EXPORT
    // ========================================================================
    
    void export_to_file(const std::string& path) const;
    void export_to_json(const std::string& path) const;
    
    // ========================================================================
    // HELPERS
    // ========================================================================
    
    std::string get_node_content(NodeID id) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace melvin
