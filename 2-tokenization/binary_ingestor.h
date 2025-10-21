#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <memory>
#include <fstream>

namespace melvin {

/**
 * BinaryIngestor - Universal Data Tokenization
 *
 * Converts ANY input (text, images, audio, video, binary files, streams)
 * into discrete binary tokens using fixed-size chunking and SHA-256 hashing.
 *
 * Core principle: All data becomes binary tokens first.
 * Meaning emerges from repeated patterns, not pre-decoded structures.
 */
class BinaryIngestor {
public:
    /**
     * Configuration for binary tokenization
     */
    struct Config {
        size_t chunk_size_bits = 512;    // 64 bytes = 512 bits
        size_t overlap_bits = 64;        // 8 bytes overlap between chunks
        bool enable_compression = true;   // Compress repeated sequences
        bool enable_metadata = true;     // Store additional metadata
    };

    /**
     * Binary token representing a chunk of data
     */
    struct BinaryToken {
        NodeID id;              // SHA-256 hash of chunk data
        std::vector<uint8_t> data;  // Raw binary data
        size_t original_offset; // Offset in original stream
        size_t size_bytes;      // Actual size of this chunk
        std::string data_hash;  // SHA-256 hash as hex string
        uint64_t timestamp;     // When this token was created
        std::string source_type; // "file", "stream", "text", "image", etc.

        // Convert to Melvin Node
        Node to_node() const;
    };

    explicit BinaryIngestor(const Config& config = Config());
    ~BinaryIngestor() = default;

    /**
     * Ingest from any input source
     */
    std::vector<BinaryToken> ingest_file(const std::string& file_path);
    std::vector<BinaryToken> ingest_stream(std::istream& stream);
    std::vector<BinaryToken> ingest_bytes(const std::vector<uint8_t>& data);
    std::vector<BinaryToken> ingest_string(const std::string& text);

    /**
     * Batch ingestion for multiple sources
     */
    std::vector<BinaryToken> ingest_batch(const std::vector<std::string>& file_paths);

    /**
     * Get statistics
     */
    struct Stats {
        size_t total_chunks = 0;
        size_t unique_chunks = 0;
        size_t duplicate_chunks = 0;
        size_t total_bytes_processed = 0;
        double compression_ratio = 1.0;
    };
    Stats get_stats() const { return stats_; }

    /**
     * Reset statistics
     */
    void reset_stats();

private:
    Config config_;
    Stats stats_;

    /**
     * Core chunking algorithm
     */
    std::vector<BinaryToken> chunk_data(const std::vector<uint8_t>& data,
                                       const std::string& source_type);

    /**
     * Generate SHA-256 hash for data
     */
    std::string sha256_hash(const std::vector<uint8_t>& data);

    /**
     * Read file as bytes
     */
    std::vector<uint8_t> read_file_bytes(const std::string& file_path);

    /**
     * Read stream as bytes
     */
    std::vector<uint8_t> read_stream_bytes(std::istream& stream);
};

} // namespace melvin

