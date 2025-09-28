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
        
        EdgeID id = {};  // Simplified for now
        
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
    
    std::string get_memory_file() const {
        return store_dir_ + "/memory.txt";
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
        
        EdgeID id = {};  // Simplified for nowsrc, static_cast<Rel>(edge.rel), dst, edge.layer);
        
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
        
        // Save memory as human-readable text
        save_memory_text();
        
        // Save binary data
        save_nodes_binary();
        save_edges_binary();
        save_paths_binary();
    }
    
    void load_from_disk() {
        if (store_dir_.empty()) return;
        
        // Load memory text first
        load_memory_text();
        
        // Load binary data
        load_nodes_binary();
        load_edges_binary();
        load_paths_binary();
    }
    
private:
    void save_memory_text() {
        std::ofstream file(get_memory_file());
        if (!file.is_open()) return;
        
        file << "# Melvin Persistent Memory\n";
        file << "# Generated: " << get_timestamp() << "\n";
        file << "# Nodes: " << node_count_ << "\n";
        file << "# Edges: " << edge_count_ << "\n";
        file << "# Paths: " << path_count_ << "\n\n";
        
        // Save nodes as text
        for (const auto& [id, node_data] : nodes_) {
            const auto& [header, payload] = node_data;
            std::string content(payload.begin(), payload.end());
            
            file << "NODE " << node_id_to_string(id) << " ";
            file << "TYPE:" << header.type << " ";
            file << "FLAGS:" << header.flags << " ";
            file << "CREATED:" << header.ts_created << " ";
            file << "UPDATED:" << header.ts_updated << " ";
            file << "CONTENT:\"" << content << "\"\n";
        }
        
        // Save edges as text
        for (const auto& [id, edge] : edges_) {
            NodeID src, dst;
            std::copy(edge.src, edge.src + 32, src.begin());
            std::copy(edge.dst, edge.dst + 32, dst.begin());
            
            file << "EDGE " << edge_id_to_string(id) << " ";
            file << "SRC:" << node_id_to_string(src) << " ";
            file << "REL:" << edge.rel << " ";
            file << "DST:" << node_id_to_string(dst) << " ";
            file << "W:" << edge.w << " ";
            file << "W_CORE:" << edge.w_core << " ";
            file << "W_CTX:" << edge.w_ctx << " ";
            file << "COUNT:" << edge.count << "\n";
        }
    }
    
    void load_memory_text() {
        std::ifstream file(get_memory_file());
        if (!file.is_open()) return;
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            std::string type;
            iss >> type;
            
            if (type == "NODE") {
                load_node_from_text(iss);
            } else if (type == "EDGE") {
                load_edge_from_text(iss);
            }
        }
    }
    
    void load_node_from_text(std::istringstream& iss) {
        std::string id_str, type_str, flags_str, created_str, updated_str, content_str;
        iss >> id_str >> type_str >> flags_str >> created_str >> updated_str >> content_str;
        
        // Parse content (remove quotes)
        if (content_str.length() > 2 && content_str[0] == '"' && content_str.back() == '"') {
            content_str = content_str.substr(1, content_str.length() - 2);
        }
        
        NodeID id = string_to_node_id(id_str);
        NodeRecHeader header;
        header.type = std::stoul(type_str.substr(4)); // Remove "TYPE:"
        header.flags = std::stoul(flags_str.substr(6)); // Remove "FLAGS:"
        header.ts_created = std::stoull(created_str.substr(8)); // Remove "CREATED:"
        header.ts_updated = std::stoull(updated_str.substr(8)); // Remove "UPDATED:"
        header.payload_len = static_cast<uint32_t>(content_str.length());
        
        std::vector<uint8_t> payload(content_str.begin(), content_str.end());
        nodes_[id] = {header, payload};
        node_count_++;
    }
    
    void load_edge_from_text(std::istringstream& iss) {
        std::string id_str, src_str, rel_str, dst_str, w_str, w_core_str, w_ctx_str, count_str;
        iss >> id_str >> src_str >> rel_str >> dst_str >> w_str >> w_core_str >> w_ctx_str >> count_str;
        
        EdgeID id = string_to_edge_id(id_str);
        EdgeRec edge;
        
        // Parse source and destination
        NodeID src = string_to_node_id(src_str.substr(4)); // Remove "SRC:"
        NodeID dst = string_to_node_id(dst_str.substr(4)); // Remove "DST:"
        std::copy(src.begin(), src.end(), edge.src);
        std::copy(dst.begin(), dst.end(), edge.dst);
        
        edge.rel = std::stoul(rel_str.substr(4)); // Remove "REL:"
        edge.w = std::stof(w_str.substr(2)); // Remove "W:"
        edge.w_core = std::stof(w_core_str.substr(7)); // Remove "W_CORE:"
        edge.w_ctx = std::stof(w_ctx_str.substr(6)); // Remove "W_CTX:"
        edge.count = std::stoul(count_str.substr(6)); // Remove "COUNT:"
        
        edges_[id] = edge;
        edge_count_++;
        
        // Update adjacency indices
        out_edges_[src].push_back(id);
        in_edges_[dst].push_back(id);
    }
    
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

} // namespace melvin
