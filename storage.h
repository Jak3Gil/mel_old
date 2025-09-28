#pragma once

#include "melvin_types.h"
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace melvin {

// Packed on-disk record structures
#pragma pack(push, 1)
struct NodeRecHeader {
    uint32_t type;        // NodeType
    uint32_t flags;       // NodeFlags
    uint64_t ts_created;  // ns since epoch (BE on disk)
    uint64_t ts_updated;  // ns
    uint32_t payload_len; // N
    uint32_t degree_hint; // optional fast degree (may be 0)
    // followed by: uint8_t payload[N], provenance varints, uint32_t checksum
};
#pragma pack(pop)

#pragma pack(push, 1)
struct EdgeRec {
    uint8_t  src[32];   // NodeID
    uint32_t rel;       // Rel enum
    uint8_t  dst[32];   // NodeID
    uint16_t layer;     // 0=explicit,1=inferred,2=generalized,...
    float    w;         // effective (cached mix for fast reads) ∈ [0,1]
    float    w_core;    // durable track ∈ [0,1]
    float    w_ctx;     // context track ∈ [0,1]
    uint64_t ts_last;   // last reinforcement ns
    uint32_t count;     // total uses/support
    uint32_t flags;     // EdgeFlags
    uint32_t pad;       // alignment
};
#pragma pack(pop)

#pragma pack(push, 1)
struct CogStep {
    uint8_t  op;             // CogOp enum
    float    conf;           // 0..1 (post-calibration)
    float    drivers[5];     // snapshot of instinct vector
    uint32_t num_inputs;
    uint32_t num_outputs;
    uint64_t latency_us;
    uint32_t err_flags;      // ErrorFlags bitfield
    uint32_t pad;            // alignment
    // inputs: NodeID/EdgeID list (fixed 32B each)
    // outputs: NodeID/EdgeID list
};
#pragma pack(pop)

#pragma pack(push, 1)
struct FileHeader {
    uint32_t magic;          // 0x4D454C56 ("MELV")
    uint32_t version;        // format version
    uint8_t  endianness;     // 0=BE, 1=LE
    uint8_t  alignment;      // always 1
    uint8_t  reserved[2];    // padding
    uint64_t merkle_root;    // checksum of all data
    uint64_t ts_created;     // creation timestamp
    uint64_t ts_updated;     // last update timestamp
    uint32_t num_nodes;      // total node count
    uint32_t num_edges;      // total edge count
    uint32_t num_paths;      // total path count
    uint32_t reserved2;      // padding
};
#pragma pack(pop)

// Adjacency view for cache-friendly traversal
struct AdjView {
    const EdgeRec* edges;
    size_t count;
    size_t capacity;
};

// Relation mask for fast filtering
struct RelMask {
    uint64_t mask_low;   // for relations 0-63
    uint64_t mask_high;  // for relations 64-127
    
    RelMask() : mask_low(0), mask_high(0) {}
    
    void set(Rel rel) {
        uint32_t r = static_cast<uint32_t>(rel);
        if (r < 64) {
            mask_low |= (1ULL << r);
        } else if (r < 128) {
            mask_high |= (1ULL << (r - 64));
        }
    }
    
    void clear(Rel rel) {
        uint32_t r = static_cast<uint32_t>(rel);
        if (r < 64) {
            mask_low &= ~(1ULL << r);
        } else if (r < 128) {
            mask_high &= ~(1ULL << (r - 64));
        }
    }
    
    bool test(Rel rel) const {
        uint32_t r = static_cast<uint32_t>(rel);
        if (r < 64) {
            return (mask_low & (1ULL << r)) != 0;
        } else if (r < 128) {
            return (mask_high & (1ULL << (r - 64))) != 0;
        }
        return false;
    }
    
    void set_all() {
        mask_low = ~0ULL;
        mask_high = ~0ULL;
    }
    
    void clear_all() {
        mask_low = 0;
        mask_high = 0;
    }
};

// Byte buffer for output generation
class ByteBuf {
private:
    std::vector<uint8_t> data_;
    size_t pos_ = 0;
    
public:
    ByteBuf() = default;
    explicit ByteBuf(size_t capacity) : data_(capacity) {}
    
    void reserve(size_t capacity) { data_.reserve(capacity); }
    size_t size() const { return data_.size(); }
    size_t capacity() const { return data_.capacity(); }
    const uint8_t* data() const { return data_.data(); }
    uint8_t* data() { return data_.data(); }
    
    void append(const uint8_t* bytes, size_t len) {
        data_.insert(data_.end(), bytes, bytes + len);
    }
    
    void append(const std::string& str) {
        append(reinterpret_cast<const uint8_t*>(str.data()), str.size());
    }
    
    void append(uint8_t byte) {
        data_.push_back(byte);
    }
    
    void clear() {
        data_.clear();
        pos_ = 0;
    }
    
    std::string to_string() const {
        return std::string(reinterpret_cast<const char*>(data_.data()), data_.size());
    }
};

// Forward declarations
class Store;
class VM;

// Store interface
class Store {
public:
    virtual ~Store() = default;
    
    // Node operations
    virtual NodeID upsert_node(const NodeRecHeader& header, const std::vector<uint8_t>& payload) = 0;
    virtual bool get_node(const NodeID& id, NodeRecHeader& header, std::vector<uint8_t>& payload) = 0;
    virtual bool node_exists(const NodeID& id) const = 0;
    
    // Edge operations
    virtual EdgeID upsert_edge(const EdgeRec& edge) = 0;
    virtual bool get_edge(const EdgeID& id, EdgeRec& edge) = 0;
    virtual bool edge_exists(const EdgeID& id) const = 0;
    virtual std::vector<EdgeRec> get_edges_from(const NodeID& node) = 0;
    virtual std::vector<EdgeRec> get_edges_to(const NodeID& node) = 0;
    
    // Adjacency operations
    virtual bool get_adj(const NodeID& id, const RelMask& mask, AdjView& view) = 0;
    virtual bool get_out_edges(const NodeID& id, const RelMask& mask, AdjView& view) = 0;
    virtual bool get_in_edges(const NodeID& id, const RelMask& mask, AdjView& view) = 0;
    
    // Path operations
    virtual PathID compose_path(const std::vector<EdgeID>& edges, float& out_score) = 0;
    virtual bool get_path(const PathID& id, std::vector<EdgeID>& edges) = 0;
    
    // Maintenance operations
    virtual void compact() = 0;
    virtual void verify_checksums() = 0;
    virtual void decay_pass(float beta_ctx, float beta_core) = 0;
    
    // Statistics
    virtual size_t node_count() const = 0;
    virtual size_t edge_count() const = 0;
    virtual size_t path_count() const = 0;
    
    // Configuration
    virtual void set_decay_params(float beta_ctx, float beta_core) = 0;
    virtual void set_thresholds(float tau_mid, float tau_high) = 0;
};

// VM interface
class VM {
public:
    virtual ~VM() = default;
    
    // Execution
    virtual void exec(const std::vector<uint8_t>& bytecode) = 0;
    virtual void exec(const uint8_t* bytecode, size_t len) = 0;
    
    // State management
    virtual void set_drivers(const Drivers& drivers) = 0;
    virtual Drivers get_drivers() const = 0;
    virtual void set_seed(uint64_t seed) = 0;
    
    // Output generation
    virtual bool emit_view(const NodeID& node, ViewKind kind, ByteBuf& out) = 0;
    
    // Statistics
    virtual size_t steps_executed() const = 0;
    virtual size_t memory_usage() const = 0;
};

// Factory functions
std::unique_ptr<Store> open_store(const std::string& dir);
std::unique_ptr<VM> create_vm(Store* store, uint64_t seed);

// Utility functions
bool is_big_endian();
uint64_t swap_endian(uint64_t value);
uint32_t swap_endian(uint32_t value);
uint16_t swap_endian(uint16_t value);

// File I/O utilities
bool write_file_header(const std::string& path, const FileHeader& header);
bool read_file_header(const std::string& path, FileHeader& header);

} // namespace melvin
