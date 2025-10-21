#pragma once

#include "audio_pipeline.h"
#include "../core/atomic_graph.h"
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace melvin {
namespace audio {

// ============================================================================
// AUDIO NODE TYPES
// ============================================================================

// Audio-specific node types (extends AtomicGraph types)
enum AudioNodeType : uint8_t {
    AUDIO_WORD = 10,        // Individual word
    AUDIO_PHRASE = 11,      // Multi-word phrase
    AUDIO_SOUND = 12,       // Ambient/environmental sound
    AUDIO_CATEGORY = 13,    // Sound category (e.g., "animal", "mechanical")
    AUDIO_SPEAKER = 14,     // Speaker identity
    AUDIO_EMOTION = 15,     // Emotional tone
    AUDIO_LOCATION = 16     // Sound source location
};

// Audio-specific edge types (extends AtomicGraph Relation enum)
enum AudioRelation : uint8_t {
    SPOKEN_AT = 10,         // Word spoken at timestamp
    HEARD_AS = 11,          // Sound interpreted as concept
    FOLLOWS_SPEECH = 12,    // Temporal sequence in speech
    CAUSED_BY_SOUND = 13,   // Causal relationship
    SOUNDS_LIKE = 14        // Similarity between sounds
};

// ============================================================================
// CROSS-MODAL EVENT STRUCTURES
// ============================================================================

/**
 * VisualEvent - Simplified vision event for cross-modal sync
 */
struct VisualEvent {
    uint64_t id;
    float timestamp;
    std::string label;       // Object/scene name
    std::string type;        // "object", "scene", "action"
    float confidence;
    
    VisualEvent() : id(0), timestamp(0.0f), confidence(0.0f) {}
    VisualEvent(uint64_t i, float t, const std::string& l, const std::string& tp, float c = 1.0f)
        : id(i), timestamp(t), label(l), type(tp), confidence(c) {}
};

/**
 * TextEvent - Text/reasoning event for cross-modal sync
 */
struct TextEvent {
    uint64_t id;
    float timestamp;
    std::string text;
    std::string type;        // "thought", "query", "response"
    
    TextEvent() : id(0), timestamp(0.0f) {}
    TextEvent(uint64_t i, float t, const std::string& txt, const std::string& tp)
        : id(i), timestamp(t), text(txt), type(tp) {}
};

/**
 * ActionEvent - Motor/action event for cross-modal sync
 */
struct ActionEvent {
    uint64_t id;
    float timestamp;
    std::string action;      // Action name
    std::string result;      // "success", "failure", "partial"
    
    ActionEvent() : id(0), timestamp(0.0f) {}
    ActionEvent(uint64_t i, float t, const std::string& a, const std::string& r)
        : id(i), timestamp(t), action(a), result(r) {}
};

// ============================================================================
// AUDIO BRIDGE CLASS
// ============================================================================

/**
 * AudioBridge - Integrates audio events into AtomicGraph
 * 
 * Responsibilities:
 * - Convert AudioEvent → Graph nodes
 * - Create audio-specific edges
 * - Cross-modal synchronization (audio ↔ vision ↔ text)
 * - Temporal association within window
 * - Causal relationship inference
 * - Reinforcement of co-occurrence patterns
 * 
 * Design:
 * - Stateless processing: Each event processed independently
 * - Temporal windows: Link events close in time
 * - Multi-modal: Sync with vision, text, actions
 * - Persistent: All nodes/edges saved to graph
 */
class AudioBridge {
public:
    /**
     * Configuration for audio bridge
     */
    struct Config {
        float temporal_window = 3.0f;           // Seconds for co-occurrence
        float min_confidence = 0.3f;            // Minimum event confidence
        bool create_word_nodes = true;          // Create node per word
        bool create_phrase_nodes = true;        // Create node per phrase
        bool enable_cross_modal = true;         // Enable cross-modal linking
        bool enable_causal_inference = true;    // Infer cause-effect
        float edge_decay_rate = 0.99f;          // Edge weight decay per tick
    };
    
    AudioBridge();
    explicit AudioBridge(const Config& config);
    ~AudioBridge();
    
    // ========================================================================
    // EVENT PROCESSING
    // ========================================================================
    
    /**
     * Process single audio event and integrate into graph
     * @param event Audio event to process
     * @param graph Target graph to modify
     * @return Node IDs created/updated
     */
    std::vector<uint64_t> process(const AudioEvent& event, AtomicGraph& graph);
    
    /**
     * Process batch of audio events
     * @param events Vector of audio events
     * @param graph Target graph to modify
     */
    void process_batch(const std::vector<AudioEvent>& events, AtomicGraph& graph);
    
    // ========================================================================
    // CROSS-MODAL SYNCHRONIZATION
    // ========================================================================
    
    /**
     * Synchronize audio with other modalities
     * Creates CO_OCCURS_WITH edges between temporally close events
     * 
     * @param audio_events Recent audio events
     * @param visual_events Recent visual events
     * @param text_events Recent text events
     * @param action_events Recent action events
     * @param graph Target graph to modify
     */
    void sync_with(
        const std::vector<AudioEvent>& audio_events,
        const std::vector<VisualEvent>& visual_events,
        const std::vector<TextEvent>& text_events,
        const std::vector<ActionEvent>& action_events,
        AtomicGraph& graph
    );
    
    /**
     * Simplified sync with just vision
     */
    void sync_with_vision(
        const std::vector<AudioEvent>& audio_events,
        const std::vector<VisualEvent>& visual_events,
        AtomicGraph& graph
    );
    
    // ========================================================================
    // CAUSAL INFERENCE
    // ========================================================================
    
    /**
     * Infer causal relationships from temporal patterns
     * If action follows audio, create CAUSES edge
     * 
     * @param graph Graph to analyze and modify
     */
    void infer_causality(AtomicGraph& graph);
    
    /**
     * Reinforce edges based on co-activation patterns
     * @param graph Graph to modify
     * @param decay_factor Decay rate for unused edges
     */
    void reinforce_patterns(AtomicGraph& graph, float decay_factor = 0.99f);
    
    // ========================================================================
    // UTILITIES
    // ========================================================================
    
    /**
     * Get or create node for audio label
     */
    uint64_t get_or_create_audio_node(
        const std::string& label, 
        uint8_t type,
        AtomicGraph& graph
    );
    
    /**
     * Check if two events are temporally close
     */
    bool is_temporally_close(float t1, float t2) const;
    
    /**
     * Get statistics
     */
    void print_stats() const;
    
private:
    // Configuration
    Config config_;
    
    // Statistics
    size_t events_processed_;
    size_t nodes_created_;
    size_t edges_created_;
    size_t cross_modal_links_;
    
    // Temporal tracking
    std::unordered_map<uint64_t, float> node_last_activation_;
    
    // Private methods
    void process_speech_event(const AudioEvent& event, AtomicGraph& graph);
    void process_ambient_event(const AudioEvent& event, AtomicGraph& graph);
    void create_word_nodes(const std::string& phrase, uint64_t phrase_node, 
                          float timestamp, AtomicGraph& graph);
    void link_temporally_close_nodes(uint64_t node_a, uint64_t node_b, 
                                     float timestamp, AtomicGraph& graph);
};

} // namespace audio
} // namespace melvin

