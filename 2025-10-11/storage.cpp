#include "storage.h"
#include "vm.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <random>
#include <iostream>
#include <sstream>
#include <iomanip>

// Full Node and Edge definitions (global scope, synchronized with other modules)
struct Node {
    uint64_t id = 0;
    std::string text;
    uint32_t type = 0;
    int freq = 0;
    bool pinned = false;
    std::vector<float> emb;
    std::vector<float> embedding;
    float attention_weight = 0.0f;
    uint64_t last_accessed = 0;
    float semantic_strength = 1.0f;
    float activation = 0.0f;
};

struct Edge {
    uint64_t u, v;
    uint64_t locB;
    float weight;
    float w_core;
    float w_ctx;
    uint32_t count;
    
    void update_frequency(uint64_t total_nodes) { (void)total_nodes; }
};

namespace melvin {

// Utility functions for endianness
bool is_big_endian() {
    uint32_t test = 0x12345678;
    return *reinterpret_cast<uint8_t*>(&test) == 0x12;
}

uint64_t swap_endian(uint64_t value) {
    return ((value & 0xFF00000000000000ULL) >> 56) |
           ((value & 0x00FF000000000000ULL) >> 40) |
           ((value & 0x0000FF0000000000ULL) >> 24) |
           ((value & 0x000000FF00000000ULL) >> 8)  |
           ((value & 0x00000000FF000000ULL) << 8)  |
           ((value & 0x0000000000FF0000ULL) << 24) |
           ((value & 0x000000000000FF00ULL) << 40) |
           ((value & 0x00000000000000FFULL) << 56);
}

uint32_t swap_endian(uint32_t value) {
    return ((value & 0xFF000000) >> 24) |
           ((value & 0x00FF0000) >> 8)  |
           ((value & 0x0000FF00) << 8)  |
           ((value & 0x000000FF) << 24);
}

uint16_t swap_endian(uint16_t value) {
    return ((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8);
}

// Convert to big-endian for disk storage
uint64_t to_big_endian(uint64_t value) {
    if (is_big_endian()) {
        return value;
    } else {
        return swap_endian(value);
    }
}

uint32_t to_big_endian(uint32_t value) {
    if (is_big_endian()) {
        return value;
    } else {
        return swap_endian(value);
    }
}

uint16_t to_big_endian(uint16_t value) {
    if (is_big_endian()) {
        return value;
    } else {
        return swap_endian(value);
    }
}

// Convert from big-endian for disk reading
uint64_t from_big_endian(uint64_t value) {
    return to_big_endian(value); // Same operation
}

uint32_t from_big_endian(uint32_t value) {
    return to_big_endian(value); // Same operation
}

uint16_t from_big_endian(uint16_t value) {
    return to_big_endian(value); // Same operation
}

// File I/O utilities
bool write_file_header(const std::string& path, const FileHeader& header) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    
    FileHeader be_header = header;
    be_header.magic = to_big_endian(be_header.magic);
    be_header.version = to_big_endian(be_header.version);
    be_header.merkle_root = to_big_endian(be_header.merkle_root);
    be_header.ts_created = to_big_endian(be_header.ts_created);
    be_header.ts_updated = to_big_endian(be_header.ts_updated);
    be_header.num_nodes = to_big_endian(be_header.num_nodes);
    be_header.num_edges = to_big_endian(be_header.num_edges);
    be_header.num_paths = to_big_endian(be_header.num_paths);
    
    file.write(reinterpret_cast<const char*>(&be_header), sizeof(be_header));
    return file.good();
}

bool read_file_header(const std::string& path, FileHeader& header) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file.good()) return false;
    
    header.magic = from_big_endian(header.magic);
    header.version = from_big_endian(header.version);
    header.merkle_root = from_big_endian(header.merkle_root);
    header.ts_created = from_big_endian(header.ts_created);
    header.ts_updated = from_big_endian(header.ts_updated);
    header.num_nodes = from_big_endian(header.num_nodes);
    header.num_edges = from_big_endian(header.num_edges);
    header.num_paths = from_big_endian(header.num_paths);
    
    return true;
}

// Simple in-memory store implementation
class MemoryStore : public Store {
private:
    std::unordered_map<NodeID, std::pair<NodeRecHeader, std::vector<uint8_t>>, NodeIDHash> nodes_;
    std::unordered_map<EdgeID, EdgeRec, EdgeIDHash> edges_;
    std::unordered_map<PathID, std::vector<EdgeID>, PathIDHash> paths_;
    
    // Adjacency indices
    std::unordered_map<NodeID, std::vector<EdgeID>, NodeIDHash> out_edges_;
    std::unordered_map<NodeID, std::vector<EdgeID>, NodeIDHash> in_edges_;
    
    // Configuration
    float beta_ctx_ = 0.01f;
    float beta_core_ = 0.001f;
    float tau_mid_ = 0.01f;
    float tau_high_ = 0.05f;
    
    // Statistics
    size_t node_count_ = 0;
    size_t edge_count_ = 0;
    size_t path_count_ = 0;
    
    uint64_t get_timestamp() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    }
    
public:
    MemoryStore() = default;
    ~MemoryStore() override = default;
    
    NodeID upsert_node(const NodeRecHeader& header, const std::vector<uint8_t>& payload) override {
        // Create a temporary header for ID computation
        NodeRecHeader temp_header = header;
        temp_header.ts_created = get_timestamp();
        temp_header.ts_updated = temp_header.ts_created;
        temp_header.payload_len = static_cast<uint32_t>(payload.size());
        
        // Compute content-addressed ID
        NodeID id = {};  // Simplified for now
        
        // Store the node
        nodes_[id] = {temp_header, payload};
        node_count_++;
        
        return id;
    }
    
    bool get_node(const NodeID& id, NodeRecHeader& header, std::vector<uint8_t>& payload) override {
        auto it = nodes_.find(id);
        if (it == nodes_.end()) return false;
        
        header = it->second.first;
        payload = it->second.second;
        return true;
    }
    
    bool node_exists(const NodeID& id) const override {
        return nodes_.find(id) != nodes_.end();
    }
    
    EdgeID upsert_edge(const EdgeRec& edge) override {
        // Compute content-addressed ID
        NodeID src_id, dst_id;
        std::copy(edge.src, edge.src + 32, src_id.begin());
        std::copy(edge.dst, edge.dst + 32, dst_id.begin());
        
        // Create deterministic edge ID from source, relation, and destination
        EdgeID id = {};
        std::hash<std::string> hasher;
        
        // Combine source, relation, and destination into a hash
        std::string combined;
        combined.append(reinterpret_cast<const char*>(src_id.data()), 32);
        combined.append(reinterpret_cast<const char*>(&edge.rel), sizeof(edge.rel));
        combined.append(reinterpret_cast<const char*>(dst_id.data()), 32);
        
        size_t hash_value = hasher(combined);
        std::memcpy(id.data(), &hash_value, std::min(sizeof(hash_value), id.size()));
        
        // Store the edge
        edges_[id] = edge;
        edge_count_++;
        
        // Update adjacency indices
        out_edges_[src_id].push_back(id);
        in_edges_[dst_id].push_back(id);
        
        return id;
    }
    
    bool get_edge(const EdgeID& id, EdgeRec& edge) override {
        auto it = edges_.find(id);
        if (it == edges_.end()) return false;
        
        edge = it->second;
        return true;
    }
    
    bool edge_exists(const EdgeID& id) const override {
        return edges_.find(id) != edges_.end();
    }
    
    std::vector<EdgeRec> get_edges_from(const NodeID& node) override {
        std::vector<EdgeRec> result;
        auto it = out_edges_.find(node);
        if (it != out_edges_.end()) {
            for (const auto& edge_id : it->second) {
                auto edge_it = edges_.find(edge_id);
                if (edge_it != edges_.end()) {
                    result.push_back(edge_it->second);
                }
            }
        }
        
        // Debug output
        std::cout << "🔍 MemoryStore::get_edges_from: Found " << result.size() << " edges from node\n";
        if (result.empty()) {
            std::cout << "   Node not found in out_edges_ map (size: " << out_edges_.size() << ")\n";
        }
        
        return result;
    }
    
    std::vector<EdgeRec> get_edges_to(const NodeID& node) override {
        std::vector<EdgeRec> result;
        auto it = in_edges_.find(node);
        if (it != in_edges_.end()) {
            for (const auto& edge_id : it->second) {
                auto edge_it = edges_.find(edge_id);
                if (edge_it != edges_.end()) {
                    result.push_back(edge_it->second);
                }
            }
        }
        return result;
    }
    
    bool get_adj(const NodeID& id, const RelMask& mask, AdjView& view) override {
        return get_out_edges(id, mask, view);
    }
    
    bool get_out_edges(const NodeID& id, const RelMask& mask, AdjView& view) override {
        auto it = out_edges_.find(id);
        if (it == out_edges_.end()) {
            view.edges = nullptr;
            view.count = 0;
            view.capacity = 0;
            return false;
        }
        
        // Filter edges by relation mask
        std::vector<EdgeRec> filtered_edges;
        for (const EdgeID& edge_id : it->second) {
            auto edge_it = edges_.find(edge_id);
            if (edge_it != edges_.end()) {
                Rel rel = static_cast<Rel>(edge_it->second.rel);
                if (mask.test(rel)) {
                    filtered_edges.push_back(edge_it->second);
                }
            }
        }
        
        // This is a simplified implementation - in a real system,
        // we'd want to avoid copying and use direct memory access
        static std::vector<EdgeRec> cached_edges;
        cached_edges = std::move(filtered_edges);
        
        view.edges = cached_edges.data();
        view.count = cached_edges.size();
        view.capacity = cached_edges.capacity();
        
        return true;
    }
    
    bool get_in_edges(const NodeID& id, const RelMask& mask, AdjView& view) override {
        auto it = in_edges_.find(id);
        if (it == in_edges_.end()) {
            view.edges = nullptr;
            view.count = 0;
            view.capacity = 0;
            return false;
        }
        
        // Filter edges by relation mask
        std::vector<EdgeRec> filtered_edges;
        for (const EdgeID& edge_id : it->second) {
            auto edge_it = edges_.find(edge_id);
            if (edge_it != edges_.end()) {
                Rel rel = static_cast<Rel>(edge_it->second.rel);
                if (mask.test(rel)) {
                    filtered_edges.push_back(edge_it->second);
                }
            }
        }
        
        static std::vector<EdgeRec> cached_edges;
        cached_edges = std::move(filtered_edges);
        
        view.edges = cached_edges.data();
        view.count = cached_edges.size();
        view.capacity = cached_edges.capacity();
        
        return true;
    }
    
    PathID compose_path(const std::vector<EdgeID>& edges, float& out_score) override {
        if (edges.empty()) {
            out_score = 0.0f;
            return PathID{};
        }
        
        // Compute path ID
        PathID id = {};  // Simplified for now
        
        // Simple path scoring (will be enhanced later)
        out_score = 1.0f / edges.size(); // Basic geometric mean placeholder
        
        // Store the path
        paths_[id] = edges;
        path_count_++;
        
        return id;
    }
    
    bool get_path(const PathID& id, std::vector<EdgeID>& edges) override {
        auto it = paths_.find(id);
        if (it == paths_.end()) return false;
        
        edges = it->second;
        return true;
    }
    
    void compact() override {
        // In-memory store doesn't need compaction
        // This would be implemented for persistent storage
    }
    
    void verify_checksums() override {
        // Verify all stored data integrity
        // This would check checksums in persistent storage
    }
    
    void decay_pass(float beta_ctx, float beta_core) override {
        beta_ctx_ = beta_ctx;
        beta_core_ = beta_core;
        
        uint64_t now = get_timestamp();
        std::vector<EdgeID> prune_candidates;
        
        // Apply decay to all edges with different rates for CTX vs CORE
        for (auto& [id, edge] : edges_) {
            // CTX track: fast decay, noisy by design
            edge.w_ctx = std::max(0.0f, edge.w_ctx * (1.0f - beta_ctx));
            
            // CORE track: slow decay, resistant to decay
            edge.w_core = std::max(0.0f, edge.w_core * (1.0f - beta_core));
            
            // Update effective weight with proper ctx/core mix
            edge.w = 0.7f * edge.w_core + 0.3f * edge.w_ctx;
            
            // Log PRUNE candidates (edges with very low effective weight)
            if (edge.w < 0.01f && edge.count < 2) {
                prune_candidates.push_back(id);
            }
        }
        
        // Log prune candidates for GC pass
        if (!prune_candidates.empty()) {
            // In a real implementation, this would be logged to a separate file
            // for the garbage collection pass to process
        }
    }
    
    size_t node_count() const override { return node_count_; }
    size_t edge_count() const override { return edge_count_; }
    size_t path_count() const override { return path_count_; }
    
    void set_decay_params(float beta_ctx, float beta_core) override {
        beta_ctx_ = beta_ctx;
        beta_core_ = beta_core;
    }
    
    void set_thresholds(float tau_mid, float tau_high) override {
        tau_mid_ = tau_mid;
        tau_high_ = tau_high;
    }
};

// Factory functions
// Persistent Store Implementation
class PersistentStore : public Store {
private:
    std::string store_dir_;
    std::unordered_map<NodeID, std::pair<NodeRecHeader, std::vector<uint8_t>>, NodeIDHash> nodes_;
    std::unordered_map<EdgeID, EdgeRec, EdgeIDHash> edges_;
    std::unordered_map<PathID, std::vector<EdgeID>, PathIDHash> paths_;
    
    // Adjacency indices
    std::unordered_map<NodeID, std::vector<EdgeID>, NodeIDHash> out_edges_;
    std::unordered_map<NodeID, std::vector<EdgeID>, NodeIDHash> in_edges_;
    
    // Configuration
    float beta_ctx_ = 0.01f;
    float beta_core_ = 0.001f;
    float tau_mid_ = 0.01f;
    float tau_high_ = 0.05f;
    
    // Statistics
    size_t node_count_ = 0;
    size_t edge_count_ = 0;
    size_t path_count_ = 0;
    
    uint64_t get_timestamp() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    }
    
    std::string get_nodes_file() const {
        return store_dir_ + "/nodes.melvin";
    }
    
    std::string get_edges_file() const {
        return store_dir_ + "/edges.melvin";
    }
    
    std::string get_paths_file() const {
        return store_dir_ + "/paths.melvin";
    }
    
    void ensure_store_dir() {
        if (!store_dir_.empty()) {
            std::filesystem::create_directories(store_dir_);
        }
    }
    
public:
    PersistentStore(const std::string& dir) : store_dir_(dir) {
        ensure_store_dir();
        load_from_disk();
    }
    
    ~PersistentStore() override {
        save_to_disk();
    }
    
    NodeID upsert_node(const NodeRecHeader& header, const std::vector<uint8_t>& payload) override {
        // Create a temporary header for ID computation
        NodeRecHeader temp_header = header;
        temp_header.ts_created = get_timestamp();
        temp_header.ts_updated = temp_header.ts_created;
        temp_header.payload_len = static_cast<uint32_t>(payload.size());
        
        // Compute content-addressed ID
        NodeID id = {};  // Simplified for now
        
        // Store the node
        nodes_[id] = {temp_header, payload};
        node_count_++;
        
        return id;
    }
    
    bool get_node(const NodeID& id, NodeRecHeader& header, std::vector<uint8_t>& payload) override {
        auto it = nodes_.find(id);
        if (it != nodes_.end()) {
            header = it->second.first;
            payload = it->second.second;
            return true;
        }
        return false;
    }
    
    bool node_exists(const NodeID& id) const override {
        return nodes_.find(id) != nodes_.end();
    }
    
    EdgeID upsert_edge(const EdgeRec& edge) override {
        // Convert edge arrays to NodeIDs
        NodeID src, dst;
        std::copy(edge.src, edge.src + 32, src.begin());
        std::copy(edge.dst, edge.dst + 32, dst.begin());
        
        // Create deterministic edge ID from source, relation, and destination
        EdgeID id = {};
        std::hash<std::string> hasher;
        
        // Combine source, relation, and destination into a hash
        std::string combined;
        combined.append(reinterpret_cast<const char*>(src.data()), 32);
        combined.append(reinterpret_cast<const char*>(&edge.rel), sizeof(edge.rel));
        combined.append(reinterpret_cast<const char*>(dst.data()), 32);
        
        size_t hash_value = hasher(combined);
        std::memcpy(id.data(), &hash_value, std::min(sizeof(hash_value), id.size()));
        
        edges_[id] = edge;
        edge_count_++;
        
        // Update adjacency indices
        out_edges_[src].push_back(id);
        in_edges_[dst].push_back(id);
        
        return id;
    }
    
    bool get_edge(const EdgeID& id, EdgeRec& edge) override {
        auto it = edges_.find(id);
        if (it != edges_.end()) {
            edge = it->second;
            return true;
        }
        return false;
    }
    
    bool edge_exists(const EdgeID& id) const override {
        return edges_.find(id) != edges_.end();
    }
    
    std::vector<EdgeRec> get_edges_from(const NodeID& node) override {
        std::vector<EdgeRec> result;
        auto it = out_edges_.find(node);
        if (it != out_edges_.end()) {
            for (const auto& edge_id : it->second) {
                auto edge_it = edges_.find(edge_id);
                if (edge_it != edges_.end()) {
                    result.push_back(edge_it->second);
                }
            }
        }
        
        // Debug output
        std::cout << "🔍 MemoryStore::get_edges_from: Found " << result.size() << " edges from node\n";
        if (result.empty()) {
            std::cout << "   Node not found in out_edges_ map (size: " << out_edges_.size() << ")\n";
        }
        
        return result;
    }
    
    std::vector<EdgeRec> get_edges_to(const NodeID& node) override {
        std::vector<EdgeRec> result;
        auto it = in_edges_.find(node);
        if (it != in_edges_.end()) {
            for (const auto& edge_id : it->second) {
                auto edge_it = edges_.find(edge_id);
                if (edge_it != edges_.end()) {
                    result.push_back(edge_it->second);
                }
            }
        }
        return result;
    }
    
    PathID upsert_path(const std::vector<EdgeID>& edges) {
        PathID id = {};  // Simplified for now
        paths_[id] = edges;
        path_count_++;
        return id;
    }
    
    bool get_path(const PathID& id, std::vector<EdgeID>& edges) override {
        auto it = paths_.find(id);
        if (it != paths_.end()) {
            edges = it->second;
            return true;
        }
        return false;
    }
    
    bool path_exists(const PathID& id) const {
        return paths_.find(id) != paths_.end();
    }
    
    void decay_pass(float beta_ctx, float beta_core) override {
        beta_ctx_ = beta_ctx;
        beta_core_ = beta_core;
        
        uint64_t now = get_timestamp();
        std::vector<EdgeID> prune_candidates;
        
        for (auto& [id, edge] : edges_) {
            // CTX track: fast decay, noisy by design
            edge.w_ctx *= (1.0f - beta_ctx);
            edge.w_ctx = std::max(0.0f, edge.w_ctx);
            
            // CORE track: slow decay, resistant to decay
            edge.w_core *= (1.0f - beta_core);
            edge.w_core = std::max(0.0f, edge.w_core);
            
            // Update effective weight with proper ctx/core mix
            edge.w = 0.7f * edge.w_core + 0.3f * edge.w_ctx;
            
            // Log PRUNE candidates for GC pass
            if (edge.w < 0.01f && edge.count < 2) {
                prune_candidates.push_back(id);
            }
        }
        
        // Log prune candidates for GC pass
        if (!prune_candidates.empty()) {
            // In a real implementation, this would be logged to a separate file
            // for the garbage collection pass to process
        }
    }
    
    void compact() override {
        // Remove edges with very low weights
        auto it = edges_.begin();
        while (it != edges_.end()) {
            if (it->second.w < 0.01f) {
                it = edges_.erase(it);
                edge_count_--;
            } else {
                ++it;
            }
        }
    }
    
    void set_thresholds(float tau_mid, float tau_high) override {
        tau_mid_ = tau_mid;
        tau_high_ = tau_high;
    }
    
    void set_decay_params(float beta_ctx, float beta_core) override {
        beta_ctx_ = beta_ctx;
        beta_core_ = beta_core;
    }
    
    size_t node_count() const override { return node_count_; }
    size_t edge_count() const override { return edge_count_; }
    size_t path_count() const override { return path_count_; }
    
    // Additional required virtual methods
    bool get_adj(const NodeID& id, const RelMask& mask, AdjView& view) override {
        // Simple implementation - return all edges from the node
        auto it = out_edges_.find(id);
        if (it != out_edges_.end()) {
            view.edges = nullptr; // Would need to store EdgeRec pointers
            view.count = it->second.size();
            view.capacity = it->second.size();
            return true;
        }
        return false;
    }
    
    bool get_out_edges(const NodeID& id, const RelMask& mask, AdjView& view) override {
        auto it = out_edges_.find(id);
        if (it == out_edges_.end()) {
            view.edges = nullptr;
            view.count = 0;
            view.capacity = 0;
            return false;
        }
        
        // Filter edges by relation mask
        std::vector<EdgeRec> filtered_edges;
        for (const EdgeID& edge_id : it->second) {
            auto edge_it = edges_.find(edge_id);
            if (edge_it != edges_.end()) {
                Rel rel = static_cast<Rel>(edge_it->second.rel);
                if (mask.test(rel)) {
                    filtered_edges.push_back(edge_it->second);
                }
            }
        }
        
        // This is a simplified implementation - in a real system,
        // we'd want to avoid copying and use direct memory access
        static std::vector<EdgeRec> cached_edges;
        cached_edges = std::move(filtered_edges);
        
        view.edges = cached_edges.data();
        view.count = cached_edges.size();
        view.capacity = cached_edges.capacity();
        
        return true;
    }
    
    bool get_in_edges(const NodeID& id, const RelMask& mask, AdjView& view) override {
        auto it = in_edges_.find(id);
        if (it != in_edges_.end()) {
            view.edges = nullptr; // Would need to store EdgeRec pointers
            view.count = it->second.size();
            view.capacity = it->second.size();
            return true;
        }
        return false;
    }
    
    PathID compose_path(const std::vector<EdgeID>& edges, float& out_score) override {
        PathID id = {};  // Simplified for now
        paths_[id] = edges;
        path_count_++;
        
        // Calculate path score
        out_score = 1.0f; // Simple implementation
        for (const auto& edge_id : edges) {
            auto it = edges_.find(edge_id);
            if (it != edges_.end()) {
                out_score *= it->second.w;
            }
        }
        
        return id;
    }
    
    void verify_checksums() override {
        // Simple implementation - just verify data integrity
        for (const auto& [id, node_data] : nodes_) {
            if (node_data.first.payload_len != node_data.second.size()) {
                throw std::runtime_error("Node payload length mismatch");
            }
        }
    }
    
    // Persistent storage methods
    void save_to_disk() {
        if (store_dir_.empty()) return;
        
        ensure_store_dir();
        
        // Save binary data only
        save_nodes_binary();
        save_edges_binary();
        save_paths_binary();
    }
    
    void load_from_disk() {
        if (store_dir_.empty()) return;
        
        // Load binary data only
        load_nodes_binary();
        load_edges_binary();
        load_paths_binary();
    }
    
private:
    
    
    
    
    void save_nodes_binary() {
        std::ofstream file(get_nodes_file(), std::ios::binary);
        if (!file.is_open()) return;
        
        for (const auto& [id, node_data] : nodes_) {
            file.write(reinterpret_cast<const char*>(id.data()), 32);
            file.write(reinterpret_cast<const char*>(&node_data.first), sizeof(NodeRecHeader));
            file.write(reinterpret_cast<const char*>(node_data.second.data()), node_data.second.size());
        }
    }
    
    void load_nodes_binary() {
        std::ifstream file(get_nodes_file(), std::ios::binary);
        if (!file.is_open()) return;
        
        while (file.good()) {
            NodeID id;
            file.read(reinterpret_cast<char*>(id.data()), 32);
            if (file.gcount() != 32) break;
            
            NodeRecHeader header;
            file.read(reinterpret_cast<char*>(&header), sizeof(header));
            if (file.gcount() != sizeof(header)) break;
            
            std::vector<uint8_t> payload(header.payload_len);
            file.read(reinterpret_cast<char*>(payload.data()), header.payload_len);
            if (file.gcount() != static_cast<std::streamsize>(header.payload_len)) break;
            
            nodes_[id] = {header, payload};
            node_count_++;
        }
    }
    
    void save_edges_binary() {
        std::ofstream file(get_edges_file(), std::ios::binary);
        if (!file.is_open()) return;
        
        for (const auto& [id, edge] : edges_) {
            file.write(reinterpret_cast<const char*>(id.data()), 32);
            file.write(reinterpret_cast<const char*>(&edge), sizeof(EdgeRec));
        }
    }
    
    void load_edges_binary() {
        std::ifstream file(get_edges_file(), std::ios::binary);
        if (!file.is_open()) return;
        
        while (file.good()) {
            EdgeID id;
            file.read(reinterpret_cast<char*>(id.data()), 32);
            if (file.gcount() != 32) break;
            
            EdgeRec edge;
            file.read(reinterpret_cast<char*>(&edge), sizeof(edge));
            if (file.gcount() != sizeof(edge)) break;
            
            edges_[id] = edge;
            edge_count_++;
            
            // Update adjacency indices
            NodeID src, dst;
            std::copy(edge.src, edge.src + 32, src.begin());
            std::copy(edge.dst, edge.dst + 32, dst.begin());
            out_edges_[src].push_back(id);
            in_edges_[dst].push_back(id);
        }
    }
    
    void save_paths_binary() {
        std::ofstream file(get_paths_file(), std::ios::binary);
        if (!file.is_open()) return;
        
        for (const auto& [id, path] : paths_) {
            file.write(reinterpret_cast<const char*>(id.data()), 32);
            uint32_t edge_count = static_cast<uint32_t>(path.size());
            file.write(reinterpret_cast<const char*>(&edge_count), sizeof(edge_count));
            for (const auto& edge_id : path) {
                file.write(reinterpret_cast<const char*>(edge_id.data()), 32);
            }
        }
    }
    
    void load_paths_binary() {
        std::ifstream file(get_paths_file(), std::ios::binary);
        if (!file.is_open()) return;
        
        while (file.good()) {
            PathID id;
            file.read(reinterpret_cast<char*>(id.data()), 32);
            if (file.gcount() != 32) break;
            
            uint32_t edge_count;
            file.read(reinterpret_cast<char*>(&edge_count), sizeof(edge_count));
            if (file.gcount() != sizeof(edge_count)) break;
            
            std::vector<EdgeID> path(edge_count);
            for (uint32_t i = 0; i < edge_count; ++i) {
                file.read(reinterpret_cast<char*>(path[i].data()), 32);
                if (file.gcount() != 32) break;
            }
            
            paths_[id] = path;
            path_count_++;
        }
    }
    
    std::string node_id_to_string(const NodeID& id) const {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (size_t i = 0; i < 8; ++i) {
            uint32_t word = (static_cast<uint32_t>(id[i*4]) << 24) |
                           (static_cast<uint32_t>(id[i*4+1]) << 16) |
                           (static_cast<uint32_t>(id[i*4+2]) << 8) |
                           static_cast<uint32_t>(id[i*4+3]);
            oss << std::setw(8) << word;
        }
        return oss.str();
    }
    
    NodeID string_to_node_id(const std::string& str) const {
        NodeID id;
        std::istringstream iss(str);
        iss >> std::hex;
        
        for (size_t i = 0; i < 8; ++i) {
            uint32_t word;
            iss >> word;
            id[i*4] = static_cast<uint8_t>((word >> 24) & 0xFF);
            id[i*4+1] = static_cast<uint8_t>((word >> 16) & 0xFF);
            id[i*4+2] = static_cast<uint8_t>((word >> 8) & 0xFF);
            id[i*4+3] = static_cast<uint8_t>(word & 0xFF);
        }
        return id;
    }
    
    std::string edge_id_to_string(const EdgeID& id) const {
        return node_id_to_string(id); // Same format
    }
    
    EdgeID string_to_edge_id(const std::string& str) const {
        return string_to_node_id(str); // Same format
    }
};

std::unique_ptr<Store> open_store(const std::string& dir) {
    // Use persistent store if directory is provided, otherwise use memory store
    if (!dir.empty()) {
        return std::make_unique<PersistentStore>(dir);
    } else {
        return std::make_unique<MemoryStore>();
    }
}

std::unique_ptr<VM> create_vm(Store* store, uint64_t seed) {
    return std::make_unique<MelvinVM>(store, seed);
}

// Simple interface for loading, creating, and adding to storage
class SimpleStorage {
private:
    std::unique_ptr<Store> store_;
    std::string store_dir_;
    
public:
    SimpleStorage(const std::string& dir) : store_dir_(dir) {
        store_ = open_store(dir);
        if (!store_) {
            throw std::runtime_error("Failed to create store");
        }
    }
    
    ~SimpleStorage() {
        // Store will auto-save on destruction
    }
    
    void add_knowledge(const std::string& text) {
        if (text.empty()) return;
        
        // Create a simple node for the text
        NodeRecHeader header;
        header.type = static_cast<uint32_t>(NodeType::SYMBOL);
        header.flags = 0;
        header.payload_len = static_cast<uint32_t>(text.size());
        header.ts_created = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        header.ts_updated = header.ts_created;
        
        std::vector<uint8_t> payload(text.begin(), text.end());
        store_->upsert_node(header, payload);
    }
    
    void add_knowledge_from_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Could not open file: " << filename << std::endl;
            return;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line[0] != '#') { // Skip empty lines and comments
                add_knowledge(line);
            }
        }
        file.close();
    }
    
    void show_stats() {
        std::cout << "📊 Total Nodes: " << store_->node_count() << std::endl;
        std::cout << "📊 Total Connections: " << store_->edge_count() << std::endl;
    }
    
    size_t get_node_count() const { return store_->node_count(); }
    size_t get_edge_count() const { return store_->edge_count(); }
    size_t get_path_count() const { return store_->path_count(); }
    
    Store* get_store() { return store_.get(); }
};

// C interface for simple storage operations
extern "C" {
    typedef struct simple_storage_t simple_storage_t;
    
    simple_storage_t* simple_storage_create(const char* dir) {
        try {
            return reinterpret_cast<simple_storage_t*>(new SimpleStorage(dir));
        } catch (...) {
            return nullptr;
        }
    }
    
    void simple_storage_destroy(simple_storage_t* storage) {
        if (storage) {
            delete reinterpret_cast<SimpleStorage*>(storage);
        }
    }
    
    void simple_storage_add_knowledge(simple_storage_t* storage, const char* text) {
        if (storage && text) {
            reinterpret_cast<SimpleStorage*>(storage)->add_knowledge(text);
        }
    }
    
    void simple_storage_add_file(simple_storage_t* storage, const char* filename) {
        if (storage && filename) {
            reinterpret_cast<SimpleStorage*>(storage)->add_knowledge_from_file(filename);
        }
    }
    
    void simple_storage_show_stats(simple_storage_t* storage) {
        if (storage) {
            reinterpret_cast<SimpleStorage*>(storage)->show_stats();
        }
    }
    
    int simple_storage_is_loaded(simple_storage_t* storage) {
        return (storage) ? 1 : 0;
    }
}

// Brain snapshot implementation for in-memory learning

// Simple CRC32 implementation
uint32_t compute_crc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

bool save_brain_snapshot(
    const char* filepath,
    const std::unordered_map<uint64_t, ::Node>& nodes,
    const std::vector<::Edge>& edges
) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[SNAPSHOT] Failed to open file for writing: " << filepath << std::endl;
        return false;
    }
    
    try {
        // Build string table
        std::vector<std::string> string_table;
        std::unordered_map<std::string, uint32_t> string_index;
        
        for (const auto& [id, node] : nodes) {
            if (string_index.find(node.text) == string_index.end()) {
                string_index[node.text] = string_table.size();
                string_table.push_back(node.text);
            }
        }
        
        // Calculate string table size
        uint32_t string_table_size = 0;
        for (const auto& str : string_table) {
            string_table_size += 4 + str.size(); // 4 bytes length + string data
        }
        
        // Write header
        BrainSnapshotHeader header;
        header.magic[0] = 'M';
        header.magic[1] = 'L';
        header.magic[2] = 'V';
        header.magic[3] = 'N';
        header.version = 1;
        header.num_nodes = nodes.size();
        header.num_edges = edges.size();
        header.string_table_size = string_table_size;
        header.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        header.checksum = 0; // Will compute later
        
        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        
        // Write string table
        for (const auto& str : string_table) {
            uint32_t len = str.size();
            file.write(reinterpret_cast<const char*>(&len), sizeof(len));
            file.write(str.data(), len);
        }
        
        // Write nodes
        for (const auto& [id, node] : nodes) {
            CompactNode cn;
            cn.id = id;
            cn.string_id = string_index[node.text];
            cn.roles[0] = 0.5f; // Default role scores
            cn.roles[1] = 0.5f;
            cn.roles[2] = 0.5f;
            cn.flags = 0;
            cn.embed_dim = node.embedding.size();
            
            file.write(reinterpret_cast<const char*>(&cn), sizeof(cn));
            
            // Write embedding if present
            if (cn.embed_dim > 0) {
                file.write(reinterpret_cast<const char*>(node.embedding.data()), 
                          cn.embed_dim * sizeof(float));
            }
        }
        
        // Write edges
        for (const auto& edge : edges) {
            CompactEdge ce;
            ce.from_id = edge.u;
            ce.to_id = edge.v;
            ce.rel_type = 0; // Default relation
            ce.weight = edge.weight;
            ce.count = edge.count;
            ce.last_ts = 0;
            
            file.write(reinterpret_cast<const char*>(&ce), sizeof(ce));
        }
        
        file.close();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[SNAPSHOT] Error saving: " << e.what() << std::endl;
        return false;
    }
}

bool load_brain_snapshot(
    const char* filepath,
    std::unordered_map<uint64_t, ::Node>& nodes,
    std::vector<::Edge>& edges
) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[SNAPSHOT] File not found (this is ok for first run): " << filepath << std::endl;
        return false;
    }
    
    try {
        // Read header
        BrainSnapshotHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        
        // Verify magic
        if (header.magic[0] != 'M' || header.magic[1] != 'L' || 
            header.magic[2] != 'V' || header.magic[3] != 'N') {
            std::cerr << "[SNAPSHOT] Invalid magic number" << std::endl;
            return false;
        }
        
        std::cout << "[SNAPSHOT] Loading " << header.num_nodes << " nodes, " 
                  << header.num_edges << " edges" << std::endl;
        
        // Read string table
        std::vector<std::string> string_table;
        for (uint32_t i = 0; i < header.num_nodes; i++) {
            uint32_t len;
            file.read(reinterpret_cast<char*>(&len), sizeof(len));
            if (len > 0 && len < 10000) { // Sanity check
                std::string str(len, '\0');
                file.read(&str[0], len);
                string_table.push_back(str);
            }
        }
        
        // Read nodes
        nodes.clear();
        for (uint32_t i = 0; i < header.num_nodes; i++) {
            CompactNode cn;
            file.read(reinterpret_cast<char*>(&cn), sizeof(cn));
            
            Node node;
            node.id = cn.id;
            if (cn.string_id < string_table.size()) {
                node.text = string_table[cn.string_id];
            }
            node.type = 0;
            node.freq = 0;
            node.pinned = false;
            node.attention_weight = 0.0f;
            node.last_accessed = 0;
            node.semantic_strength = 1.0f;
            node.activation = 0.0f;
            
            // Read embedding if present
            if (cn.embed_dim > 0 && cn.embed_dim < 10000) {
                node.embedding.resize(cn.embed_dim);
                file.read(reinterpret_cast<char*>(node.embedding.data()), 
                         cn.embed_dim * sizeof(float));
            }
            
            nodes[node.id] = node;
        }
        
        // Read edges
        edges.clear();
        for (uint32_t i = 0; i < header.num_edges; i++) {
            CompactEdge ce;
            file.read(reinterpret_cast<char*>(&ce), sizeof(ce));
            
            Edge edge;
            edge.u = ce.from_id;
            edge.v = ce.to_id;
            edge.locB = ce.to_id;
            edge.weight = ce.weight;
            edge.w_core = ce.weight;
            edge.w_ctx = 0.0f;
            edge.count = ce.count;
            
            edges.push_back(edge);
        }
        
        file.close();
        std::cout << "[SNAPSHOT] Loaded successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[SNAPSHOT] Error loading: " << e.what() << std::endl;
        return false;
    }
}

} // namespace melvin
