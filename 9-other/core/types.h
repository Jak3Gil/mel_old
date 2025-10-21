#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace melvin {

// ============================================================================
// TOKEN-BASED TYPE DEFINITIONS
// ============================================================================

// Lightweight node ID (like LLM tokens)
using NodeID = uint16_t;  // 2 bytes - supports 65K unique concepts
using EdgeID = uint32_t;  // 4 bytes - supports 4 billion edges

// Node types
enum class NodeType : uint8_t {
    CONCEPT = 0,
    PHRASE = 1,
    SENSORY = 2,      // Audio, image, etc
    ABSTRACT = 3,     // High-level reasoning nodes
    EMERGENT_DIM = 4, // Emergent dimensional nodes
    VISUAL_PATCH = 5, // Low-level visual feature patch
    OBJECT_NODE = 6,  // Grouped patches forming an object
    SCENE_NODE = 7,   // Collection of objects in temporal context
    EVENT_NODE = 8    // Temporal sequence of scenes
};

// Edge relation types (SIMPLIFIED - Only 2 types!)
enum class RelationType : uint8_t {
    EXACT = 0,         // Direct from input (dog - is - a - mammal)
    LEAP = 1           // Inferred connection (guessed between nodes without exact)
};

// Driver field types (emergent dimensions)
enum class DriverType : uint8_t {
    NONE = 0,
    EMERGENT = 1,      // Discovered through experience
    TEMPORARY = 2,     // Short-lived dimensional influence
    CONSOLIDATED = 3   // Long-term stable dimension
};

// ============================================================================
// COMPACT NODE STRUCTURE (4 bytes on disk AND in memory!)
// ============================================================================

struct Node {
    NodeID id;                    // 2 bytes - lightweight token ID
    uint8_t type_flags;           // Type (4 bits) + Flags (4 bits)
    uint8_t metadata_idx;         // Index into metadata table (0 = defaults)
    
    // Episodic memory fields
    mutable uint64_t creation_timestamp = 0;  // When this node was created
    mutable uint32_t episode_id = 0;          // Which episode created it
    
    // Runtime fields stored separately (see RuntimeState below)
    // Kept here for API compatibility, but loaded on-demand
    mutable float activation = 0.0f;      // Current activation level
    mutable uint64_t activations = 0;     // Total times activated
    mutable float weight = 0.0f;          // Adaptive importance
    mutable std::vector<float> embedding; // GNN: learnable embedding vector
    mutable float predicted_activation = 0.0f; // GNN: predicted next state
    
    // Helper methods
    NodeType get_type() const {
        return static_cast<NodeType>(type_flags & 0x0F);
    }
    
    void set_type(NodeType t) {
        type_flags = (type_flags & 0xF0) | (static_cast<uint8_t>(t) & 0x0F);
    }
    
    bool has_metadata() const {
        return metadata_idx != 0;
    }
};

// ============================================================================
// COMPACT EDGE STRUCTURE (8 bytes on disk AND in memory!)
// ============================================================================

struct Edge {
    NodeID from_id;               // 2 bytes
    NodeID to_id;                 // 2 bytes
    RelationType relation;        // 1 byte (EXACT or LEAP)
    uint8_t weight_scaled;        // 1 byte (0-255, represents 0.0-1.0)
    uint16_t metadata_idx;        // 2 bytes (index into edge metadata)
    
    // Episodic memory fields
    mutable uint64_t creation_timestamp = 0;  // When this edge was created
    mutable uint32_t episode_id = 0;          // Which episode created it
    
    // Runtime fields stored separately (see RuntimeState below)
    // Kept here for API compatibility, but loaded on-demand
    mutable uint64_t coactivations = 0;   // Times both nodes active together
    mutable float adaptive_weight = 0.0f; // Locally normalized weight
    mutable float confidence = 1.0f;      // Relation type confidence
    
    // Helper methods
    float get_weight() const {
        return weight_scaled / 255.0f;
    }
    
    void set_weight(float w) {
        weight_scaled = static_cast<uint8_t>(w * 255.0f);
    }
    
    // Get effective weight (combines base + adaptive)
    float get_effective_weight() const {
        float base = get_weight();
        return adaptive_weight > 0.0f ? (base * 0.5f + adaptive_weight * 0.5f) : base;
    }
    
    bool has_metadata() const {
        return metadata_idx != 0;
    }
};

// ============================================================================
// RUNTIME STATE (Stored separately from core structures)
// ============================================================================

// Sparse storage for node runtime data (only non-zero values)
struct NodeRuntime {
    float activation = 0.0f;
    uint64_t activations = 0;
    float weight = 0.0f;
    std::vector<float> embedding;  // GNN: learnable vector representation
    float predicted_activation = 0.0f;  // GNN: predicted next state
};

// Sparse storage for edge runtime data (only non-zero values)
struct EdgeRuntime {
    uint64_t coactivations = 0;
    float adaptive_weight = 0.0f;
    float confidence = 1.0f;
};

// ============================================================================
// OPTIONAL METADATA (Only stored if non-default)
// ============================================================================

struct NodeMetadata {
    // Context field properties
    float activation = 0.0f;
    float base_potential = 1.0f;
    
    // Emergent dimensional properties
    DriverType driver_type = DriverType::NONE;
    float variance_impact = 0.0f;
    float influence_weight = 0.0f;
    
    // Temporal tracking
    uint64_t created_at = 0;
    uint64_t last_accessed = 0;
    uint32_t access_count = 0;
    
    // Embeddings (optional)
    std::vector<float> embedding;
    
    bool is_default() const {
        return activation == 0.0f && 
               base_potential == 1.0f &&
               driver_type == DriverType::NONE &&
               variance_impact == 0.0f &&
               influence_weight == 0.0f &&
               access_count == 0 &&
               embedding.empty();
    }
};

struct EdgeMetadata {
    float confidence = 1.0f;
    float decay_rate = 0.99f;
    uint64_t last_reinforced = 0;
    uint32_t reinforcement_count = 0;
    
    // SRS scheduling
    uint64_t next_review = 0;
    float ease_factor = 2.5f;
    uint32_t interval_days = 0;
    
    bool is_default() const {
        return confidence == 1.0f &&
               decay_rate == 0.99f &&
               reinforcement_count == 0 &&
               next_review == 0;
    }
};

// ============================================================================
// REASONING PATH (uses lightweight IDs)
// ============================================================================

struct ReasoningPath {
    std::vector<NodeID> nodes;    // Just IDs!
    std::vector<EdgeID> edges;    // Just IDs!
    float confidence = 0.0f;
    float total_weight = 0.0f;
    
    bool is_valid() const {
        return !nodes.empty() && nodes.size() == edges.size() + 1;
    }
    
    int hop_count() const {
        return static_cast<int>(edges.size());
    }
};

// ============================================================================
// QUERY & ANSWER
// ============================================================================

struct Query {
    std::string text;
    std::vector<float> embedding;
    std::unordered_map<std::string, std::string> metadata;
};

struct Answer {
    std::string text;
    float confidence = 0.0f;
    std::vector<ReasoningPath> paths;
    std::unordered_map<std::string, float> metrics;
};

// ============================================================================
// EPISODIC MEMORY
// ============================================================================

struct Episode {
    uint32_t id = 0;
    uint64_t start_time = 0;
    uint64_t end_time = 0;
    std::string context;
    std::vector<NodeID> node_sequence;  // Temporal order of nodes
    std::vector<EdgeID> edge_sequence;  // Temporal order of edges
    bool is_active = false;             // Currently ongoing episode
    
    uint64_t duration() const {
        return end_time > start_time ? end_time - start_time : 0;
    }
    
    size_t size() const {
        return node_sequence.size();
    }
};

} // namespace melvin
