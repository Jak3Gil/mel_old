#pragma once

#include "teaching_format.h"
#include "../../learning_hooks.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Forward declarations
struct Node;
struct Edge;

namespace melvin {
namespace teaching {

// Ingestion options
struct IngestOptions {
    bool snapshot_after = true;  // Save snapshot after ingestion
    bool verbose = true;         // Print progress
    bool strict = false;         // Stop on first error
    bool deduplicate = true;     // Skip duplicate edges
};

// Ingestion result
struct IngestResult {
    bool success = true;
    uint32_t nodes_added = 0;
    uint32_t edges_added = 0;
    uint32_t edges_updated = 0;
    uint32_t duplicates_skipped = 0;
    uint32_t rules_added = 0;
    std::vector<std::string> errors;
    std::vector<std::string> new_node_labels;
};

// Edge key for deduplication
struct EdgeKey {
    std::string from;
    std::string rel;
    std::string to;
    
    bool operator==(const EdgeKey& other) const {
        return from == other.from && rel == other.rel && to == other.to;
    }
};

} // namespace teaching
} // namespace melvin

// Hash for EdgeKey
namespace std {
template<>
struct hash<melvin::teaching::EdgeKey> {
    size_t operator()(const melvin::teaching::EdgeKey& k) const {
        return hash<string>()(k.from) ^ (hash<string>()(k.rel) << 1) ^ (hash<string>()(k.to) << 2);
    }
};
}

namespace melvin {
namespace teaching {

// Main ingestion class
class TeachingIngestor {
public:
    // Ingest a parsed teaching document
    static IngestResult ingest(
        const TchDoc& doc,
        std::unordered_map<uint64_t, ::Node>& nodes,
        std::vector<::Edge>& edges,
        const IngestOptions& opts = IngestOptions()
    );
    
private:
    // Helper: Find or create node by text
    static uint64_t find_or_create_node(
        const std::string& text,
        std::unordered_map<uint64_t, ::Node>& nodes,
        IngestResult& result
    );
    
    // Helper: Create edge between nodes
    static bool create_or_update_edge(
        uint64_t from_id,
        uint64_t to_id,
        const std::string& rel,
        float weight,
        std::vector<::Edge>& edges,
        std::unordered_set<EdgeKey>& seen_edges,
        IngestResult& result,
        bool allow_duplicates
    );
    
    // Helper: Ingest a FACT
    static void ingest_fact(
        const TchFact& fact,
        std::unordered_map<uint64_t, ::Node>& nodes,
        std::vector<::Edge>& edges,
        std::unordered_set<EdgeKey>& seen_edges,
        IngestResult& result,
        const IngestOptions& opts
    );
    
    // Helper: Ingest an ASSOCIATION
    static void ingest_association(
        const TchAssoc& assoc,
        std::unordered_map<uint64_t, ::Node>& nodes,
        std::vector<::Edge>& edges,
        std::unordered_set<EdgeKey>& seen_edges,
        IngestResult& result,
        const IngestOptions& opts
    );
    
    // Helper: Normalize text for edge key
    static std::string normalize_for_key(const std::string& text);
};

} // namespace teaching
} // namespace melvin

