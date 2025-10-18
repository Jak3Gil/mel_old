#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace melvin {

// ============================================================================
// CORE TYPES - Compact representations for graph nodes and edges
// ============================================================================

/**
 * Node types mapping to cognitive hierarchy
 */
enum class NodeType : uint8_t {
    FRAME = 0,          // Camera frame (temporal marker)
    VISUAL_PATCH,       // Raw sensory patch (Retina/V1)
    OBJECT,             // Grouped object (V2/V4)
    SCENE,              // Scene context (Parahippocampal)
    CONCEPT,            // Abstract concept (IT/Temporal)
    EVENT               // Action/event (Prefrontal)
};

/**
 * Relation types - biological connectivity patterns
 */
enum class Rel : uint8_t {
    TEMPORAL_NEXT = 0,      // Sequential in time
    SPATIALLY_NEAR,         // Spatial proximity
    SIMILAR_FEATURES,       // Feature similarity
    PART_OF,                // Compositional
    CO_OCCURS_WITH,         // Statistical co-occurrence
    OBSERVED_AS,            // Observation link
    NAMED_AS,               // Label assignment
    USED_FOR,               // Functional relation
    EXPECTS,                // Predictive
    CAUSES,                 // Causal
    ACTIVE_FOCUS,           // Current attention
    INHIBITS                // Inhibition of return
};

/**
 * Compact node: 24 bytes
 * Flexible payload slots for different node types
 */
struct Node {
    uint64_t id;           // Unique identifier
    NodeType type;         // Node category
    uint8_t _pad[3];       // Alignment padding
    uint32_t a, b;         // Generic uint32 slots (coordinates, indices, etc.)
    float c, d;            // Generic float slots (features, scores, etc.)
    
    Node() : id(0), type(NodeType::CONCEPT), a(0), b(0), c(0), d(0) {
        _pad[0] = _pad[1] = _pad[2] = 0;
    }
} __attribute__((packed));

/**
 * Compact edge: 24 bytes
 */
struct Edge {
    uint64_t a;            // Source node id
    uint64_t b;            // Target node id
    Rel r;                 // Relation type
    uint8_t _pad[3];       // Alignment padding
    float w;               // Weight/strength
    
    Edge() : a(0), b(0), r(Rel::CO_OCCURS_WITH), w(0) {
        _pad[0] = _pad[1] = _pad[2] = 0;
    }
    
    Edge(uint64_t from, uint64_t to, Rel rel, float weight = 1.0f)
        : a(from), b(to), r(rel), w(weight) {
        _pad[0] = _pad[1] = _pad[2] = 0;
    }
} __attribute__((packed));

/**
 * Frame metadata
 */
struct FrameMeta {
    uint64_t id;           // Frame identifier
    uint64_t ts;           // Timestamp (microseconds)
    uint16_t w, h;         // Width, height
    uint8_t cam;           // Camera index
    uint8_t _pad[5];       // Alignment
    
    FrameMeta() : id(0), ts(0), w(0), h(0), cam(0) {
        _pad[0] = _pad[1] = _pad[2] = _pad[3] = _pad[4] = 0;
    }
} __attribute__((packed));

/**
 * Focus selection result
 */
struct FocusSelection {
    uint64_t node_id;      // Selected node
    float score;           // Combined focus score
    
    FocusSelection() : node_id(0), score(0) {}
    FocusSelection(uint64_t id, float s) : node_id(id), score(s) {}
};

// ============================================================================
// CONSTANTS - Biological parameters
// ============================================================================

namespace constants {

// Attention weights (FEF/SC weighting)
constexpr float ALPHA_SALIENCY = 0.45f;      // Bottom-up salience
constexpr float BETA_GOAL = 0.35f;           // Top-down goal relevance
constexpr float GAMMA_CURIOSITY = 0.20f;     // Novelty/prediction error

// Learning rates
constexpr float DECAY_LAMBDA = 0.0025f;      // Edge decay per second
constexpr float REINFORCE_ETA = 0.10f;       // Hebbian reinforcement rate

// Curiosity parameters
constexpr float CURIOSITY_MIN_BOOST = 0.15f; // Min boost for novelty
constexpr float CURIOSITY_MAX_BOOST = 0.40f; // Max boost for novelty
constexpr float CURIOSITY_CAP = 1.0f;        // Maximum curiosity score

// LEAP formation
constexpr float LEAP_ERROR_THRESHOLD = 0.12f; // Min error reduction to form LEAP

// Timing
constexpr float TARGET_HZ = 20.0f;           // Target cognitive cycle rate
constexpr float MIN_CYCLE_MS = 1000.0f / TARGET_HZ;

} // namespace constants

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Hash (frame_id, patch_x, patch_y) into unique node ID
 */
constexpr inline uint64_t hash_patch_id(uint64_t frame_id, uint32_t px, uint32_t py) {
    return (frame_id << 32) | ((uint64_t)px << 16) | (uint64_t)py;
}

/**
 * Hash string into node ID
 */
inline uint64_t hash_string(const std::string& str) {
    uint64_t hash = 14695981039346656037ULL; // FNV-1a offset
    for (char c : str) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL; // FNV-1a prime
    }
    return hash;
}

/**
 * Relation type to string
 */
inline const char* rel_to_string(Rel r) {
    switch (r) {
        case Rel::TEMPORAL_NEXT: return "TEMPORAL_NEXT";
        case Rel::SPATIALLY_NEAR: return "SPATIALLY_NEAR";
        case Rel::SIMILAR_FEATURES: return "SIMILAR_FEATURES";
        case Rel::PART_OF: return "PART_OF";
        case Rel::CO_OCCURS_WITH: return "CO_OCCURS_WITH";
        case Rel::OBSERVED_AS: return "OBSERVED_AS";
        case Rel::NAMED_AS: return "NAMED_AS";
        case Rel::USED_FOR: return "USED_FOR";
        case Rel::EXPECTS: return "EXPECTS";
        case Rel::CAUSES: return "CAUSES";
        case Rel::ACTIVE_FOCUS: return "ACTIVE_FOCUS";
        case Rel::INHIBITS: return "INHIBITS";
        default: return "UNKNOWN";
    }
}

} // namespace melvin


