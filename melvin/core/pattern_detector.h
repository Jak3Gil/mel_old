#pragma once

#include "types.h"
#include "storage.h"
#include <vector>
#include <unordered_map>
#include <string>

namespace melvin {

/**
 * PatternDetector - Finds and compresses repeated binary sequences
 *
 * Detects when the same binary patterns appear repeatedly across
 * different inputs, creating PatternNodes that represent compressed
 * knowledge structures.
 */
class PatternDetector {
public:
    /**
     * Configuration for pattern detection
     */
    struct Config {
        size_t min_pattern_length = 64;      // Minimum bytes for a pattern
        size_t max_pattern_length = 1024;    // Maximum bytes for a pattern
        size_t min_occurrences = 3;          // Minimum times pattern must appear
        float compression_threshold = 0.8f;  // Min compression ratio to create pattern
        bool enable_cross_file_patterns = true; // Look for patterns across files
    };

    /**
     * Pattern information
     */
    struct Pattern {
        std::string hash;           // SHA-256 hash of pattern data
        std::vector<uint8_t> data;  // The pattern bytes
        size_t length;              // Length in bytes
        size_t occurrences;         // How many times it appears
        std::vector<NodeID> node_ids; // Nodes that contain this pattern
        std::string pattern_type;   // "sequence", "structure", "semantic"
        float compression_ratio;    // Space saved by using this pattern
    };

    explicit PatternDetector(const Config& config = Config());
    ~PatternDetector() = default;

    /**
     * Analyze storage and detect patterns
     */
    std::vector<Pattern> detect_patterns(Storage* storage);

    /**
     * Find patterns in a specific set of nodes
     */
    std::vector<Pattern> detect_patterns_in_nodes(const std::vector<Node>& nodes);

    /**
     * Compress patterns into PatternNodes
     */
    std::vector<Node> create_pattern_nodes(const std::vector<Pattern>& patterns);

    /**
     * Get statistics
     */
    struct Stats {
        size_t patterns_detected = 0;
        size_t patterns_created = 0;
        size_t bytes_compressed = 0;
        size_t nodes_analyzed = 0;
        double avg_compression_ratio = 0.0;
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
     * Find repeated byte sequences
     */
    std::vector<Pattern> find_repeated_sequences(const std::vector<Node>& nodes);

    /**
     * Analyze pattern compression effectiveness
     */
    bool is_worthwhile_pattern(const Pattern& pattern);

    /**
     * Generate pattern metadata
     */
    void enrich_pattern_metadata(Pattern& pattern, const std::vector<Node>& nodes);

    /**
     * Check if pattern already exists
     */
    bool pattern_exists(const std::string& hash, Storage* storage);

    /**
     * Extract subsequences from binary data
     */
    std::vector<std::pair<std::vector<uint8_t>, size_t>> extract_subsequences(
        const std::vector<uint8_t>& data,
        size_t min_len,
        size_t max_len);
};

} // namespace melvin

