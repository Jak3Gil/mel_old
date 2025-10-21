#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

namespace melvin::v2 {

// ============================================================================
// TYPE ALIASES (V1 Compatible)
// ============================================================================

using NodeID = uint16_t;      // 2 bytes - supports 65K concepts (v1 compatible)
using EdgeID = uint32_t;      // 4 bytes - supports 4B edges
using EpisodeID = uint32_t;   // 4 bytes - episode identifiers
using GenomeID = uint64_t;    // 8 bytes - genome identifiers
using Timestamp = uint64_t;   // nanoseconds since epoch

// ============================================================================
// UTILITY FUNCTIONS (Forward declared for use in constructors)
// ============================================================================

inline Timestamp get_timestamp_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

// ============================================================================
// THOUGHT STRUCTURE (Global Workspace Content)
// ============================================================================

struct Thought {
    uint64_t id;                        // Unique thought ID
    std::string type;                   // "percept", "goal", "hypothesis", "action", "memory"
    float salience;                     // 0.0-1.0 importance/activation
    std::vector<NodeID> concept_refs;   // Referenced concepts from semantic memory
    Timestamp created_at;               // When this thought emerged
    
    // Optional metadata
    std::string source_module;          // Which module generated this
    float confidence;                   // Certainty (0.0-1.0)
    
    Thought() 
        : id(0), salience(0.0f), created_at(0), confidence(1.0f) {}
    
    Thought(uint64_t id_, const std::string& type_, float sal)
        : id(id_), type(type_), salience(sal), 
          created_at(get_timestamp_ns()), confidence(1.0f) {}
};

// ============================================================================
// WORKING MEMORY SLOT
// ============================================================================

struct WMSlot {
    NodeID item_ref;                // Reference to concept node
    std::string binding_tag;        // Role/context tag ("agent", "object", "goal")
    float precision;                // How well maintained (0.0-1.0)
    float decay_time;               // Time until forgotten (seconds)
    Timestamp last_updated;         // Last refresh timestamp
    bool is_active;                 // Currently in use
    
    WMSlot() 
        : item_ref(0), precision(0.0f), decay_time(0.0f), 
          last_updated(0), is_active(false) {}
    
    WMSlot(NodeID item, const std::string& tag, float prec, float decay)
        : item_ref(item), binding_tag(tag), precision(prec), 
          decay_time(decay), last_updated(get_timestamp_ns()), is_active(true) {}
};

// ============================================================================
// NEUROMODULATOR STATE
// ============================================================================

struct NeuromodState {
    float dopamine;         // Reward prediction error → plasticity
    float norepinephrine;   // Unexpected uncertainty → exploration
    float acetylcholine;    // Expected uncertainty → attention boost
    float serotonin;        // Stability/patience → suppress impulsivity
    
    Timestamp last_update;
    
    NeuromodState() 
        : dopamine(0.5f), norepinephrine(0.5f), 
          acetylcholine(0.5f), serotonin(0.5f),
          last_update(0) {}
};

// ============================================================================
// PERCEPTION STRUCTURES
// ============================================================================

struct PerceptualFeatures {
    std::vector<float> visual_features;   // Edge density, color, motion
    std::vector<float> audio_features;    // Spectral features
    float saliency;                       // Bottom-up attention
    float novelty;                        // Difference from prediction
    
    PerceptualFeatures() : saliency(0.0f), novelty(0.0f) {}
};

struct PerceivedObject {
    NodeID object_id;
    PerceptualFeatures features;
    int x, y, width, height;              // Bounding box
    float confidence;
    
    PerceivedObject() : object_id(0), x(0), y(0), width(0), height(0), confidence(0.0f) {}
};

// ============================================================================
// ATTENTION STRUCTURES
// ============================================================================

struct AttentionScores {
    float saliency;      // Bottom-up (stimulus-driven)
    float relevance;     // Top-down (goal-driven)
    float curiosity;     // Prediction error
    float need;          // Homeostatic drive
    float persistence;   // Temporal continuity
    
    AttentionScores() 
        : saliency(0.0f), relevance(0.0f), curiosity(0.0f), 
          need(0.0f), persistence(0.0f) {}
    
    float compute_total(float alpha, float beta, float gamma, float delta, float epsilon) const {
        return alpha * saliency + beta * relevance + gamma * curiosity + 
               delta * need + epsilon * persistence;
    }
};

// ============================================================================
// ADDITIONAL UTILITY FUNCTIONS
// ============================================================================

inline uint64_t get_timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

inline double timestamp_to_seconds(Timestamp ts) {
    return ts / 1e9;
}

inline Timestamp seconds_to_timestamp(double seconds) {
    return static_cast<Timestamp>(seconds * 1e9);
}

} // namespace melvin::v2

