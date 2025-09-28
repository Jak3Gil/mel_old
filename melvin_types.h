#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

namespace melvin {

// Binary ID types - all content-addressed, 32 bytes each
using NodeID = std::array<uint8_t, 32>;
using EdgeID = std::array<uint8_t, 32>;
using PathID = std::array<uint8_t, 32>;

// Relation types for edges
enum class Rel : uint32_t {
    ISA = 0,
    PART_OF = 1,
    DRINKS = 2,
    CAUSES = 3,
    NEXT = 4,
    SIMILAR_TO = 5,
    CONTRADICTS = 6,
    GENERATED_FROM = 7,
    EFFECT = 8,
    HAS_PROPERTY = 9,
    LOCATED_IN = 10,
    TEMPORAL_BEFORE = 11,
    TEMPORAL_AFTER = 12,
    // Add more as needed
    MAX_REL = 255
};

// Node types
enum class NodeType : uint32_t {
    SYMBOL = 0,
    CONCEPT = 1,
    PATH = 2,
    META_COG_STEP = 3,
    ABSTRACTION = 4,
    MAX_TYPE = 255
};

// Node flags
enum NodeFlags : uint32_t {
    HYPOTHESIS = 1 << 0,
    DURABLE = 1 << 1,
    ORACLE_USED = 1 << 2,
    DEPRECATED = 1 << 3,
    LOCKED = 1 << 4,
    GENERALIZED = 1 << 5
};

// Edge flags
enum EdgeFlags : uint32_t {
    EDGE_HYPOTHESIS = 1 << 0,
    EDGE_DEPRECATED = 1 << 1,
    EDGE_LOCKED = 1 << 2,
    EDGE_GENERALIZED = 1 << 3,
    EDGE_INFERRED = 1 << 4
};

// View kinds for output generation
enum class ViewKind : uint8_t {
    TEXT_VIEW = 0,
    AUDIO_VIEW = 1,
    CONTROL_VIEW = 2,
    DEBUG_VIEW = 3
};

// VM opcodes
enum class Opcode : uint8_t {
    LINK = 0,
    WALK = 1,
    COMPOSE = 2,
    REINFORCE = 3,
    DECAY = 4,
    MERGE = 5,
    CONTRADICT = 6,
    EMIT = 7,
    TRACE = 8,
    HALT = 255
};

// Instinct driver vector (5 dimensions)
struct Drivers {
    float curiosity = 0.5f;
    float efficiency = 0.5f;
    float consistency = 0.5f;
    float social = 0.5f;
    float survival = 0.5f;
    
    Drivers() = default;
    Drivers(float c, float e, float co, float s, float su) 
        : curiosity(c), efficiency(e), consistency(co), social(s), survival(su) {}
};

// COG_STEP operation types
enum class CogOp : uint8_t {
    INGEST = 0,
    RECALL = 1,
    EXPLORE = 2,
    INTEGRATE = 3,
    DECIDE = 4,
    OUTPUT = 5,
    REINFORCE = 6,
    PRUNE = 7,
    PATCH = 8
};

// Error flags for COG_STEP
enum ErrorFlags : uint32_t {
    NO_ERROR = 0,
    REPETITION_DETECTED = 1 << 0,
    OSCILLATION_DETECTED = 1 << 1,
    CONTRADICTION_LOOP = 1 << 2,
    LOW_CONFIDENCE = 1 << 3,
    BEAM_EXHAUSTED = 1 << 4,
    DEPTH_LIMIT_REACHED = 1 << 5,
    MEMORY_EXHAUSTED = 1 << 6
};

// Utility functions for binary operations
template<typename ID>
inline std::enable_if_t<std::is_same_v<ID, NodeID> || std::is_same_v<ID, EdgeID> || std::is_same_v<ID, PathID>, bool>
is_zero_id(const ID& id) {
    return std::all_of(id.begin(), id.end(), [](uint8_t b) { return b == 0; });
}

// Hash comparison for IDs
struct NodeIDHash {
    size_t operator()(const NodeID& id) const {
        size_t hash = 0;
        for (size_t i = 0; i < 8; ++i) {
            hash ^= (static_cast<size_t>(id[i*4]) << 24) |
                   (static_cast<size_t>(id[i*4+1]) << 16) |
                   (static_cast<size_t>(id[i*4+2]) << 8) |
                   static_cast<size_t>(id[i*4+3]);
        }
        return hash;
    }
};

struct EdgeIDHash {
    size_t operator()(const EdgeID& id) const {
        size_t hash = 0;
        for (size_t i = 0; i < 8; ++i) {
            hash ^= (static_cast<size_t>(id[i*4]) << 24) |
                   (static_cast<size_t>(id[i*4+1]) << 16) |
                   (static_cast<size_t>(id[i*4+2]) << 8) |
                   static_cast<size_t>(id[i*4+3]);
        }
        return hash;
    }
};

struct PathIDHash {
    size_t operator()(const PathID& id) const {
        size_t hash = 0;
        for (size_t i = 0; i < 8; ++i) {
            hash ^= (static_cast<size_t>(id[i*4]) << 24) |
                   (static_cast<size_t>(id[i*4+1]) << 16) |
                   (static_cast<size_t>(id[i*4+2]) << 8) |
                   static_cast<size_t>(id[i*4+3]);
        }
        return hash;
    }
};

// Equality operators for IDs
template<typename ID>
inline std::enable_if_t<std::is_same_v<ID, NodeID> || std::is_same_v<ID, EdgeID> || std::is_same_v<ID, PathID>, bool>
operator==(const ID& a, const ID& b) {
    return std::equal(a.begin(), a.end(), b.begin());
}

template<typename ID>
inline std::enable_if_t<std::is_same_v<ID, NodeID> || std::is_same_v<ID, EdgeID> || std::is_same_v<ID, PathID>, bool>
operator!=(const ID& a, const ID& b) {
    return !(a == b);
}

// Less-than operators for ordering (lexicographic)
template<typename ID>
inline std::enable_if_t<std::is_same_v<ID, NodeID> || std::is_same_v<ID, EdgeID> || std::is_same_v<ID, PathID>, bool>
operator<(const ID& a, const ID& b) {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
}

} // namespace melvin
