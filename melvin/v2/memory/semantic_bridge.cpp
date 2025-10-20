#include "semantic_bridge.h"
#include <algorithm>
#include <iostream>
#include <queue>
#include <unordered_set>

namespace melvin::v2::memory {

// ============================================================================
// SEMANTIC BRIDGE IMPLEMENTATION
// ============================================================================

SemanticBridge::SemanticBridge(melvin::AtomicGraph* v1_graph, const Config& config)
    : v1_graph_(v1_graph), config_(config) {
    
    if (!v1_graph_) {
        throw std::runtime_error("SemanticBridge: v1_graph cannot be null");
    }
    
    stats_ = Stats{};
    
    if (config_.verbose) {
        auto graph_stats = get_graph_stats();
        std::cout << "[SemanticBridge] Initialized with v1 graph: "
                  << graph_stats.node_count << " nodes, "
                  << graph_stats.edge_count << " edges" << std::endl;
        
        if (config_.readonly_mode) {
            std::cout << "[SemanticBridge] ⚠️  Read-only mode enabled" << std::endl;
        }
    }
}

SemanticBridge::~SemanticBridge() = default;

// ========================================================================
// CONCEPT QUERIES
// ========================================================================

std::vector<NodeID> SemanticBridge::find_related(NodeID concept_id, float threshold) {
    stats_.queries_served++;
    
    // Check cache first
    std::vector<NodeID> cached;
    if (config_.enable_caching && check_cache(concept_id, cached)) {
        stats_.cache_hits++;
        return cached;
    }
    
    stats_.cache_misses++;
    
    // Query v1 graph
    uint64_t v1_id = v2_to_v1_node_id(concept_id);
    auto v1_neighbors = v1_graph_->neighbors(v1_id);
    
    // Convert to v2 format and filter by threshold
    std::vector<std::pair<NodeID, float>> weighted_neighbors;
    
    for (uint64_t v1_neighbor_id : v1_neighbors) {
        float weight = v1_graph_->get_edge_weight(v1_id, v1_neighbor_id, 255);  // Any relation
        
        if (weight >= threshold && weight >= config_.min_confidence) {
            NodeID v2_id = v1_to_v2_node_id(v1_neighbor_id);
            weighted_neighbors.push_back({v2_id, weight});
        }
    }
    
    // Sort by weight (descending)
    std::sort(weighted_neighbors.begin(), weighted_neighbors.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
    
    // Extract just the IDs
    std::vector<NodeID> result;
    for (const auto& [id, weight] : weighted_neighbors) {
        result.push_back(id);
    }
    
    // Update cache
    if (config_.enable_caching) {
        update_cache(concept_id, result);
    }
    
    return result;
}

std::vector<std::pair<NodeID, int>> SemanticBridge::multi_hop_query(NodeID start, int max_hops) {
    stats_.queries_served++;
    
    // BFS from start node
    std::vector<std::pair<NodeID, int>> reachable;
    std::unordered_set<NodeID> visited;
    std::queue<std::pair<NodeID, int>> queue;
    
    queue.push({start, 0});
    visited.insert(start);
    
    while (!queue.empty() && static_cast<int>(visited.size()) < 1000) {  // Safety limit
        auto [current, distance] = queue.front();
        queue.pop();
        
        if (distance >= max_hops) {
            continue;
        }
        
        // Add to reachable
        if (current != start) {
            reachable.push_back({current, distance});
        }
        
        // Get neighbors
        auto neighbors = find_related(current, config_.min_confidence);
        
        for (NodeID neighbor : neighbors) {
            if (visited.find(neighbor) == visited.end()) {
                visited.insert(neighbor);
                queue.push({neighbor, distance + 1});
            }
        }
    }
    
    return reachable;
}

std::vector<NodeID> SemanticBridge::find_by_label(const std::string& substring) {
    stats_.queries_served++;
    
    // Query v1 graph
    auto v1_nodes = v1_graph_->find_nodes(substring);
    
    // Convert to v2 format
    std::vector<NodeID> result;
    for (uint64_t v1_id : v1_nodes) {
        result.push_back(v1_to_v2_node_id(v1_id));
    }
    
    return result;
}

std::string SemanticBridge::get_label(NodeID concept_id) {
    uint64_t v1_id = v2_to_v1_node_id(concept_id);
    auto v1_node = v1_graph_->get_node(v1_id);
    
    if (v1_node) {
        return std::string(v1_node->label);
    }
    
    return "";
}

float SemanticBridge::get_edge_weight(NodeID from, NodeID to) {
    uint64_t v1_from = v2_to_v1_node_id(from);
    uint64_t v1_to = v2_to_v1_node_id(to);
    
    return v1_graph_->get_edge_weight(v1_from, v1_to, 255);  // Any relation
}

// ========================================================================
// CONCEPT CREATION
// ========================================================================

NodeID SemanticBridge::create_concept(const std::string& label, uint8_t type) {
    if (config_.readonly_mode) {
        stats_.readonly_rejections++;
        
        if (config_.verbose) {
            std::cout << "[SemanticBridge] ⚠️  Create rejected (readonly): " << label << std::endl;
        }
        
        // Try to find existing instead
        auto existing = find_by_label(label);
        if (!existing.empty()) {
            return existing[0];
        }
        
        return 0;  // Failed
    }
    
    // Create in v1 graph
    uint64_t v1_id = v1_graph_->get_or_create_node(label, type);
    
    stats_.concepts_created++;
    
    if (config_.verbose) {
        std::cout << "[SemanticBridge] Created: " << label << " (id=" << v1_id << ")" << std::endl;
    }
    
    return v1_to_v2_node_id(v1_id);
}

bool SemanticBridge::add_relation(NodeID from, NodeID to, uint8_t relation_type, float weight) {
    if (config_.readonly_mode) {
        stats_.readonly_rejections++;
        
        if (config_.verbose) {
            std::cout << "[SemanticBridge] ⚠️  Add relation rejected (readonly): "
                      << from << " → " << to << std::endl;
        }
        
        return false;
    }
    
    // Add to v1 graph
    uint64_t v1_from = v2_to_v1_node_id(from);
    uint64_t v1_to = v2_to_v1_node_id(to);
    
    v1_graph_->add_edge(v1_from, v1_to, relation_type, weight);
    
    stats_.relations_added++;
    
    if (config_.verbose) {
        std::cout << "[SemanticBridge] Added relation: " 
                  << from << " → " << to << " (w=" << weight << ")" << std::endl;
    }
    
    // Invalidate cache
    if (config_.enable_caching) {
        query_cache_.clear();
    }
    
    return true;
}

bool SemanticBridge::strengthen_relation(NodeID from, NodeID to, float increment) {
    if (config_.readonly_mode) {
        stats_.readonly_rejections++;
        return false;
    }
    
    // Get current weight
    float current = get_edge_weight(from, to);
    
    if (current <= 0.0f) {
        // Edge doesn't exist - create it
        return add_relation(from, to, 1, increment);  // CO_OCCURS_WITH
    }
    
    // Strengthen existing
    uint64_t v1_from = v2_to_v1_node_id(from);
    uint64_t v1_to = v2_to_v1_node_id(to);
    
    v1_graph_->add_edge(v1_from, v1_to, 1, increment);  // Increment weight
    
    // Invalidate cache
    if (config_.enable_caching) {
        query_cache_.clear();
    }
    
    return true;
}

// ========================================================================
// MODE CONTROL
// ========================================================================

void SemanticBridge::set_readonly_mode(bool readonly) {
    config_.readonly_mode = readonly;
    
    if (config_.verbose) {
        std::cout << "[SemanticBridge] Readonly mode: " 
                  << (readonly ? "ENABLED" : "DISABLED") << std::endl;
    }
}

// ========================================================================
// STATISTICS
// ========================================================================

SemanticBridge::GraphStats SemanticBridge::get_graph_stats() const {
    GraphStats stats;
    
    stats.node_count = v1_graph_->node_count();
    stats.edge_count = v1_graph_->edge_count();
    
    // Approximate EXACT vs LEAP counts
    stats.exact_edges = stats.edge_count / 23;
    stats.leap_edges = stats.edge_count - stats.exact_edges;
    stats.storage_bytes = stats.node_count * 40 + stats.edge_count * 24;
    
    return stats;
}

SemanticBridge::Stats SemanticBridge::get_stats() const {
    return stats_;
}

void SemanticBridge::reset_stats() {
    stats_ = Stats{};
}

// ========================================================================
// PERSISTENCE
// ========================================================================

bool SemanticBridge::save(const std::string& nodes_path, const std::string& edges_path) {
    if (config_.verbose) {
        std::cout << "[SemanticBridge] Saving v1 graph..." << std::endl;
    }
    
    try {
        v1_graph_->save(nodes_path, edges_path);
        return true;
    } catch (...) {
        return false;
    }
}

bool SemanticBridge::load(const std::string& nodes_path, const std::string& edges_path) {
    if (config_.verbose) {
        std::cout << "[SemanticBridge] Loading v1 graph..." << std::endl;
    }
    
    try {
        v1_graph_->load(nodes_path, edges_path);
        
        // Clear cache
        if (config_.enable_caching) {
            query_cache_.clear();
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

// ========================================================================
// CONFIGURATION
// ========================================================================

void SemanticBridge::set_config(const Config& config) {
    config_ = config;
    
    if (!config_.enable_caching) {
        query_cache_.clear();
    }
}

SemanticBridge::Config SemanticBridge::get_config() const {
    return config_;
}

// ========================================================================
// PRIVATE HELPERS
// ========================================================================

std::vector<NodeID> SemanticBridge::get_neighbors_cached(NodeID node_id) {
    std::vector<NodeID> cached;
    if (check_cache(node_id, cached)) {
        return cached;
    }
    
    uint64_t v1_id = v2_to_v1_node_id(node_id);
    auto v1_neighbors = v1_graph_->neighbors(v1_id);
    
    std::vector<NodeID> result;
    for (uint64_t v1_neighbor : v1_neighbors) {
        result.push_back(v1_to_v2_node_id(v1_neighbor));
    }
    
    update_cache(node_id, result);
    
    return result;
}

void SemanticBridge::update_cache(NodeID key, const std::vector<NodeID>& neighbors) {
    if (!config_.enable_caching) return;
    
    if (query_cache_.size() >= config_.cache_size) {
        query_cache_.erase(query_cache_.begin());
    }
    
    CacheEntry entry;
    entry.key = key;
    entry.neighbors = neighbors;
    entry.cached_at = get_timestamp_ns();
    
    query_cache_.push_back(entry);
}

bool SemanticBridge::check_cache(NodeID key, std::vector<NodeID>& out) {
    if (!config_.enable_caching) return false;
    
    for (const auto& entry : query_cache_) {
        if (entry.key == key) {
            out = entry.neighbors;
            return true;
        }
    }
    
    return false;
}

} // namespace melvin::v2::memory

