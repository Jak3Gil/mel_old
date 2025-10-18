#include "binary_ingestor.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <openssl/sha.h>  // Requires OpenSSL development libraries

namespace melvin {

BinaryIngestor::BinaryIngestor(const Config& config) : config_(config) {
    reset_stats();
}

std::vector<BinaryIngestor::BinaryToken> BinaryIngestor::ingest_file(const std::string& file_path) {
    auto bytes = read_file_bytes(file_path);
    return chunk_data(bytes, "file:" + file_path);
}

std::vector<BinaryIngestor::BinaryToken> BinaryIngestor::ingest_stream(std::istream& stream) {
    auto bytes = read_stream_bytes(stream);
    return chunk_data(bytes, "stream");
}

std::vector<BinaryIngestor::BinaryToken> BinaryIngestor::ingest_bytes(const std::vector<uint8_t>& data) {
    return chunk_data(data, "bytes");
}

std::vector<BinaryIngestor::BinaryToken> BinaryIngestor::ingest_string(const std::string& text) {
    std::vector<uint8_t> bytes(text.begin(), text.end());
    return chunk_data(bytes, "string");
}

std::vector<BinaryIngestor::BinaryToken> BinaryIngestor::ingest_batch(const std::vector<std::string>& file_paths) {
    std::vector<BinaryToken> all_tokens;

    for (const auto& path : file_paths) {
        try {
            auto tokens = ingest_file(path);
            all_tokens.insert(all_tokens.end(), tokens.begin(), tokens.end());
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to ingest " << path << ": " << e.what() << std::endl;
        }
    }

    return all_tokens;
}

void BinaryIngestor::reset_stats() {
    stats_ = Stats{};
}

std::vector<BinaryIngestor::BinaryToken> BinaryIngestor::chunk_data(
    const std::vector<uint8_t>& data,
    const std::string& source_type) {

    std::vector<BinaryToken> tokens;
    size_t data_size = data.size();
    size_t chunk_size_bytes = config_.chunk_size_bits / 8;
    size_t overlap_bytes = config_.overlap_bits / 8;

    if (data_size == 0) {
        return tokens;
    }

    stats_.total_bytes_processed += data_size;

    // Calculate number of chunks needed
    size_t num_chunks = 0;
    if (data_size <= chunk_size_bytes) {
        num_chunks = 1;
    } else {
        // Account for overlap
        size_t effective_chunk_size = chunk_size_bytes - overlap_bytes;
        num_chunks = (data_size - chunk_size_bytes + effective_chunk_size - 1) / effective_chunk_size + 1;
    }

    std::unordered_set<std::string> seen_hashes;

    for (size_t i = 0; i < num_chunks; ++i) {
        size_t start_offset = i * (chunk_size_bytes - overlap_bytes);
        size_t end_offset = std::min(start_offset + chunk_size_bytes, data_size);

        if (start_offset >= data_size) {
            break;
        }

        // Extract chunk
        std::vector<uint8_t> chunk_data(data.begin() + start_offset,
                                       data.begin() + end_offset);

        // Generate hash
        std::string hash = sha256_hash(chunk_data);

        // Create token
        BinaryToken token;
        token.id = std::hash<std::string>{}(hash);  // Simple hash for NodeID
        token.data = chunk_data;
        token.original_offset = start_offset;
        token.size_bytes = chunk_data.size();
        token.data_hash = hash;
        token.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        token.source_type = source_type;

        tokens.push_back(token);

        // Track uniqueness
        if (seen_hashes.find(hash) == seen_hashes.end()) {
            seen_hashes.insert(hash);
            stats_.unique_chunks++;
        } else {
            stats_.duplicate_chunks++;
        }
    }

    stats_.total_chunks += tokens.size();
    return tokens;
}

std::string BinaryIngestor::sha256_hash(const std::vector<uint8_t>& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.data(), data.size());
    SHA256_Final(hash, &sha256);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

std::vector<uint8_t> BinaryIngestor::read_file_bytes(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + file_path);
    }

    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> bytes(file_size);
    file.read(reinterpret_cast<char*>(bytes.data()), file_size);

    return bytes;
}

std::vector<uint8_t> BinaryIngestor::read_stream_bytes(std::istream& stream) {
    std::vector<uint8_t> bytes;
    char buffer[4096];

    while (stream.read(buffer, sizeof(buffer))) {
        bytes.insert(bytes.end(),
                    reinterpret_cast<uint8_t*>(buffer),
                    reinterpret_cast<uint8_t*>(buffer + stream.gcount()));
    }

    // Get remaining bytes
    stream.read(buffer, sizeof(buffer));
    bytes.insert(bytes.end(),
                reinterpret_cast<uint8_t*>(buffer),
                reinterpret_cast<uint8_t*>(buffer + stream.gcount()));

    return bytes;
}

Node BinaryIngestor::BinaryToken::to_node() const {
    Node node;
    node.id = id;
    node.content = data_hash;  // Store hash as content
    node.type = NodeType::BINARY;
    node.activation = 0.0f;
    node.base_potential = 1.0f;
    node.weight = 1.0f;
    node.metadata["binary_size"] = std::to_string(size_bytes);
    node.metadata["data_hash"] = data_hash;
    node.metadata["source_type"] = source_type;
    node.metadata["timestamp"] = std::to_string(timestamp);
    node.metadata["original_offset"] = std::to_string(original_offset);

    return node;
}

} // namespace melvin

