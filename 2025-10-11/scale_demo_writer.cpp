#include "scale_demo_writer.h"
#include <cstring>
#include <chrono>
#include <openssl/sha.h>

namespace melvin {
namespace scale_demo {

// Sentinels for record types (as per spec)
constexpr uint16_t SENTINEL_TAUGHT_NODE = 0x0001;
constexpr uint16_t SENTINEL_THOUGHT_NODE = 0x1110;
constexpr uint16_t SENTINEL_CONNECTION = 0x0101;

// ============================================================================
// BinaryRecordWriter
// ============================================================================

BinaryRecordWriter::BinaryRecordWriter(const Config& config) : config_(config) {
    file_.open(config.output_path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!file_.is_open()) {
        throw std::runtime_error("Failed to open output file: " + config.output_path);
    }
    
    buffer_.reserve(config.flush_block_size);
}

BinaryRecordWriter::~BinaryRecordWriter() {
    flush();
    file_.close();
}

NodeID BinaryRecordWriter::emit_taught_node(const std::string& content, NodeType type) {
    // Format: 0001 [raw_bytes] 0001
    
    std::vector<uint8_t> payload(content.begin(), content.end());
    NodeID id = compute_node_id(payload);
    
    // Write opening sentinel
    write_sentinel(SENTINEL_TAUGHT_NODE);
    
    // Write node type (optional metadata, 4 bytes)
    uint32_t type_val = static_cast<uint32_t>(type);
    write_bytes(reinterpret_cast<const uint8_t*>(&type_val), sizeof(type_val));
    
    // Write payload length
    uint32_t payload_len = static_cast<uint32_t>(payload.size());
    write_bytes(reinterpret_cast<const uint8_t*>(&payload_len), sizeof(payload_len));
    
    // Write payload
    write_bytes(payload);
    
    // Write closing sentinel
    write_sentinel(SENTINEL_TAUGHT_NODE);
    
    nodes_written_++;
    
    if (buffer_.size() >= config_.flush_block_size) {
        flush();
    }
    
    return id;
}

NodeID BinaryRecordWriter::emit_thought_node(const std::vector<NodeID>& path) {
    // Format: 1110 [path_data] 1110
    
    std::vector<uint8_t> payload;
    
    // Serialize path as sequence of NodeIDs
    uint32_t path_len = static_cast<uint32_t>(path.size());
    payload.insert(payload.end(), 
                   reinterpret_cast<const uint8_t*>(&path_len),
                   reinterpret_cast<const uint8_t*>(&path_len) + sizeof(path_len));
    
    for (const auto& node_id : path) {
        payload.insert(payload.end(), node_id.begin(), node_id.end());
    }
    
    NodeID id = compute_node_id(payload);
    
    // Write opening sentinel
    write_sentinel(SENTINEL_THOUGHT_NODE);
    
    // Write payload length
    uint32_t payload_len = static_cast<uint32_t>(payload.size());
    write_bytes(reinterpret_cast<const uint8_t*>(&payload_len), sizeof(payload_len));
    
    // Write payload
    write_bytes(payload);
    
    // Write closing sentinel
    write_sentinel(SENTINEL_THOUGHT_NODE);
    
    nodes_written_++;
    
    if (buffer_.size() >= config_.flush_block_size) {
        flush();
    }
    
    return id;
}

EdgeID BinaryRecordWriter::emit_connection(const NodeID& src, Rel rel, const NodeID& dst, float weight) {
    // Format: 0101 [NodeA] [REL-2b] [NodeB] [Weight] 0101
    
    EdgeID edge_id = compute_edge_id(src, rel, dst);
    
    // Write opening sentinel
    write_sentinel(SENTINEL_CONNECTION);
    
    // Write source NodeID (32 bytes)
    write_bytes(src.data(), 32);
    
    // Write relation (4 bytes, using 2 bits conceptually)
    uint32_t rel_val = static_cast<uint32_t>(rel);
    write_bytes(reinterpret_cast<const uint8_t*>(&rel_val), sizeof(rel_val));
    
    // Write destination NodeID (32 bytes)
    write_bytes(dst.data(), 32);
    
    // Write weight (4 bytes float)
    write_bytes(reinterpret_cast<const uint8_t*>(&weight), sizeof(weight));
    
    // Write closing sentinel
    write_sentinel(SENTINEL_CONNECTION);
    
    // Track weight for reinforcement
    edge_weights_[edge_id] = weight;
    
    edges_written_++;
    
    if (buffer_.size() >= config_.flush_block_size) {
        flush();
    }
    
    return edge_id;
}

void BinaryRecordWriter::reinforce_connection(const EdgeID& edge_id) {
    auto it = edge_weights_.find(edge_id);
    if (it != edge_weights_.end()) {
        it->second += 1.0f;
    }
}

void BinaryRecordWriter::flush() {
    if (!buffer_.empty()) {
        file_.write(reinterpret_cast<const char*>(buffer_.data()), buffer_.size());
        bytes_written_ += buffer_.size();
        buffer_.clear();
    }
    file_.flush();
}

void BinaryRecordWriter::write_sentinel(uint16_t sentinel) {
    write_bytes(reinterpret_cast<const uint8_t*>(&sentinel), sizeof(sentinel));
}

void BinaryRecordWriter::write_bytes(const uint8_t* data, size_t len) {
    buffer_.insert(buffer_.end(), data, data + len);
}

void BinaryRecordWriter::write_bytes(const std::vector<uint8_t>& data) {
    buffer_.insert(buffer_.end(), data.begin(), data.end());
}

NodeID BinaryRecordWriter::compute_node_id(const std::vector<uint8_t>& payload) {
    NodeID id{};
    SHA256(payload.data(), payload.size(), id.data());
    return id;
}

EdgeID BinaryRecordWriter::compute_edge_id(const NodeID& src, Rel rel, const NodeID& dst) {
    std::vector<uint8_t> combined;
    combined.insert(combined.end(), src.begin(), src.end());
    
    uint32_t rel_val = static_cast<uint32_t>(rel);
    combined.insert(combined.end(), 
                   reinterpret_cast<const uint8_t*>(&rel_val),
                   reinterpret_cast<const uint8_t*>(&rel_val) + sizeof(rel_val));
    
    combined.insert(combined.end(), dst.begin(), dst.end());
    
    EdgeID id{};
    SHA256(combined.data(), combined.size(), id.data());
    return id;
}

// ============================================================================
// RecordIndex
// ============================================================================

RecordIndex::RecordIndex() {}

void RecordIndex::register_node(const NodeID& id, size_t offset, const std::string& token) {
    node_offsets_[id] = offset;
    
    if (!token.empty()) {
        token_index_[token].add(id);
    }
}

void RecordIndex::register_edge(const EdgeID& id, size_t offset) {
    edge_offsets_[id] = offset;
}

bool RecordIndex::get_node_offset(const NodeID& id, size_t& offset) const {
    auto it = node_offsets_.find(id);
    if (it != node_offsets_.end()) {
        offset = it->second;
        return true;
    }
    return false;
}

bool RecordIndex::get_edge_offset(const EdgeID& id, size_t& offset) const {
    auto it = edge_offsets_.find(id);
    if (it != edge_offsets_.end()) {
        offset = it->second;
        return true;
    }
    return false;
}

std::vector<NodeID> RecordIndex::get_nodes_by_token(const std::string& token) const {
    auto it = token_index_.find(token);
    if (it != token_index_.end()) {
        return it->second.get_recent();
    }
    return {};
}

void RecordIndex::TokenRing::add(const NodeID& id) {
    if (nodes.size() < RING_SIZE) {
        nodes.push_back(id);
    } else {
        nodes[next_idx] = id;
        next_idx = (next_idx + 1) % RING_SIZE;
    }
}

std::vector<NodeID> RecordIndex::TokenRing::get_recent() const {
    return nodes;
}

// ============================================================================
// BatchIngestionCoordinator
// ============================================================================

BatchIngestionCoordinator::BatchIngestionCoordinator(BinaryRecordWriter& writer, RecordIndex& index)
    : writer_(writer), index_(index) {}

void BatchIngestionCoordinator::ingest_sentence(
    const std::vector<std::string>& tokens,
    const std::vector<std::pair<int, int>>& temporal_edges,
    const std::vector<std::pair<int, int>>& leap_edges) {
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create nodes for each token
    std::vector<NodeID> node_ids;
    for (const auto& token : tokens) {
        NodeID id = get_or_create_token_node(token);
        node_ids.push_back(id);
        stats_.nodes_ingested++;
    }
    
    // Create temporal edges
    for (const auto& [src_idx, dst_idx] : temporal_edges) {
        if (src_idx >= 0 && src_idx < static_cast<int>(node_ids.size()) &&
            dst_idx >= 0 && dst_idx < static_cast<int>(node_ids.size())) {
            writer_.emit_connection(node_ids[src_idx], Rel::TEMPORAL, node_ids[dst_idx], 1.0f);
            stats_.temporal_edges++;
            stats_.edges_created++;
        }
    }
    
    // Create leap edges
    for (const auto& [src_idx, dst_idx] : leap_edges) {
        if (src_idx >= 0 && src_idx < static_cast<int>(node_ids.size()) &&
            dst_idx >= 0 && dst_idx < static_cast<int>(node_ids.size())) {
            writer_.emit_connection(node_ids[src_idx], Rel::LEAP, node_ids[dst_idx], 0.5f);
            stats_.leap_edges++;
            stats_.edges_created++;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    stats_.total_time_ms += std::chrono::duration<double, std::milli>(end - start).count();
}

void BatchIngestionCoordinator::ingest_audio_frames(
    const std::vector<uint8_t>& phoneme_codes,
    const std::vector<std::string>& motif_labels) {
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create frame nodes
    std::vector<NodeID> frame_ids;
    for (uint8_t code : phoneme_codes) {
        std::string frame_token = "ph_" + std::to_string(code);
        NodeID id = get_or_create_token_node(frame_token);
        frame_ids.push_back(id);
        stats_.nodes_ingested++;
    }
    
    // Create NEXT (TEMPORAL) edges for consecutive frames
    for (size_t i = 0; i + 1 < frame_ids.size(); ++i) {
        writer_.emit_connection(frame_ids[i], Rel::TEMPORAL, frame_ids[i + 1], 1.0f);
        stats_.temporal_edges++;
        stats_.edges_created++;
    }
    
    // Create motif label nodes and bind them
    for (const auto& label : motif_labels) {
        NodeID label_id = get_or_create_token_node(label);
        // Bind motif to first frame (simplified)
        if (!frame_ids.empty()) {
            writer_.emit_connection(frame_ids[0], Rel::ISA, label_id, 1.0f);
            stats_.edges_created++;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    stats_.total_time_ms += std::chrono::duration<double, std::milli>(end - start).count();
}

void BatchIngestionCoordinator::ingest_image_embedding(
    const std::vector<int8_t>& embedding,
    const std::string& label,
    const std::vector<NodeID>& similar_nodes) {
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create image node with embedding as payload
    std::vector<uint8_t> payload(embedding.begin(), embedding.end());
    std::string payload_str(payload.begin(), payload.end());
    
    NodeID img_id = writer_.emit_taught_node(payload_str, NodeType::MODALITY_IMAGE);
    stats_.nodes_ingested++;
    
    // Bind to label
    NodeID label_id = get_or_create_token_node(label);
    writer_.emit_connection(img_id, Rel::ISA, label_id, 1.0f);
    stats_.edges_created++;
    
    // Create similarity edges
    for (const auto& similar_id : similar_nodes) {
        writer_.emit_connection(img_id, Rel::GENERALIZATION, similar_id, 0.8f);
        stats_.similar_edges++;
        stats_.edges_created++;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    stats_.total_time_ms += std::chrono::duration<double, std::milli>(end - start).count();
}

void BatchIngestionCoordinator::process_query(const std::string& query) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Tokenize query and find existing nodes
    std::istringstream iss(query);
    std::vector<NodeID> path;
    std::string word;
    
    while (iss >> word) {
        auto nodes = index_.get_nodes_by_token(word);
        if (!nodes.empty()) {
            path.push_back(nodes[0]);  // Take most recent
        }
    }
    
    // Create thought node if we found a path
    if (!path.empty()) {
        writer_.emit_thought_node(path);
        stats_.queries_processed++;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    stats_.total_time_ms += std::chrono::duration<double, std::milli>(end - start).count();
}

NodeID BatchIngestionCoordinator::get_or_create_token_node(const std::string& token) {
    auto it = token_cache_.find(token);
    if (it != token_cache_.end()) {
        return it->second;
    }
    
    NodeID id = writer_.emit_taught_node(token, NodeType::SYMBOL);
    token_cache_[token] = id;
    index_.register_node(id, writer_.bytes_written(), token);
    
    return id;
}

} // namespace scale_demo
} // namespace melvin

