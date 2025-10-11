#pragma once

#include "melvin_types.h"
#include "storage.h"
#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <unordered_map>

namespace melvin {
namespace scale_demo {

// Binary record writer conformant with Melvin's format
// Writes:
//   - Taught Nodes: 0001 [raw_bytes] 0001
//   - Thought Nodes: 1110 [path_data] 1110
//   - Connections: 0101 [NodeA] [REL-2b] [NodeB] [Weight] 0101
class BinaryRecordWriter {
public:
    struct Config {
        std::string output_path;
        size_t flush_block_size = 8 * 1024 * 1024;  // 8 MB
        bool use_mmap = true;
    };
    
    BinaryRecordWriter(const Config& config);
    ~BinaryRecordWriter();
    
    // Taught node: raw token/content
    NodeID emit_taught_node(const std::string& content, NodeType type = NodeType::SYMBOL);
    
    // Thought node: reasoning path
    NodeID emit_thought_node(const std::vector<NodeID>& path);
    
    // Connection: NodeA --REL--> NodeB with weight
    EdgeID emit_connection(const NodeID& src, Rel rel, const NodeID& dst, float weight = 1.0f);
    
    // Increment weight on existing connection
    void reinforce_connection(const EdgeID& edge_id);
    
    // Batch write
    void flush();
    
    // Statistics
    size_t nodes_written() const { return nodes_written_; }
    size_t edges_written() const { return edges_written_; }
    size_t bytes_written() const { return bytes_written_; }
    
private:
    Config config_;
    std::ofstream file_;
    std::vector<uint8_t> buffer_;
    size_t nodes_written_ = 0;
    size_t edges_written_ = 0;
    size_t bytes_written_ = 0;
    
    // Weight tracking for reinforcement
    std::unordered_map<EdgeID, float, EdgeIDHash> edge_weights_;
    
    void write_sentinel(uint16_t sentinel);
    void write_bytes(const uint8_t* data, size_t len);
    void write_bytes(const std::vector<uint8_t>& data);
    
    NodeID compute_node_id(const std::vector<uint8_t>& payload);
    EdgeID compute_edge_id(const NodeID& src, Rel rel, const NodeID& dst);
};

// Index builder for fast lookups
class RecordIndex {
public:
    RecordIndex();
    
    // Register node at file offset
    void register_node(const NodeID& id, size_t offset, const std::string& token = "");
    
    // Register edge
    void register_edge(const EdgeID& id, size_t offset);
    
    // Lookup
    bool get_node_offset(const NodeID& id, size_t& offset) const;
    bool get_edge_offset(const EdgeID& id, size_t& offset) const;
    
    // Token to recent nodes (ring buffer, last 1024)
    std::vector<NodeID> get_nodes_by_token(const std::string& token) const;
    
    // Statistics
    size_t node_count() const { return node_offsets_.size(); }
    size_t edge_count() const { return edge_offsets_.size(); }
    
private:
    std::unordered_map<NodeID, size_t, NodeIDHash> node_offsets_;
    std::unordered_map<EdgeID, size_t, EdgeIDHash> edge_offsets_;
    
    // Token -> ring buffer of recent NodeIDs
    struct TokenRing {
        std::vector<NodeID> nodes;
        size_t next_idx = 0;
        static constexpr size_t RING_SIZE = 1024;
        
        void add(const NodeID& id);
        std::vector<NodeID> get_recent() const;
    };
    
    std::unordered_map<std::string, TokenRing> token_index_;
};

// Batch ingestion coordinator
class BatchIngestionCoordinator {
public:
    struct Stats {
        size_t nodes_ingested = 0;
        size_t edges_created = 0;
        size_t temporal_edges = 0;
        size_t leap_edges = 0;
        size_t similar_edges = 0;
        size_t queries_processed = 0;
        double total_time_ms = 0.0;
    };
    
    BatchIngestionCoordinator(BinaryRecordWriter& writer, RecordIndex& index);
    
    // Ingest text sentence with temporal + leap edges
    void ingest_sentence(const std::vector<std::string>& tokens,
                        const std::vector<std::pair<int, int>>& temporal_edges,
                        const std::vector<std::pair<int, int>>& leap_edges);
    
    // Ingest audio frames with NEXT edges
    void ingest_audio_frames(const std::vector<uint8_t>& phoneme_codes,
                            const std::vector<std::string>& motif_labels);
    
    // Ingest image embedding with similarity edges
    void ingest_image_embedding(const std::vector<int8_t>& embedding,
                               const std::string& label,
                               const std::vector<NodeID>& similar_nodes);
    
    // Process query and create thought node
    void process_query(const std::string& query);
    
    const Stats& get_stats() const { return stats_; }
    
private:
    BinaryRecordWriter& writer_;
    RecordIndex& index_;
    Stats stats_;
    
    // Cache for token -> NodeID
    std::unordered_map<std::string, NodeID> token_cache_;
    
    NodeID get_or_create_token_node(const std::string& token);
};

} // namespace scale_demo
} // namespace melvin

