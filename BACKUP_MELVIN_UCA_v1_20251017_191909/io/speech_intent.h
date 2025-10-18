#pragma once

#include "../core/atomic_graph.h"
#include <string>
#include <vector>
#include <chrono>

namespace melvin {
namespace io {

/**
 * SpeechIntent - Cognitive Speech Output System
 * 
 * Converts Melvin's verbal output into graph nodes, enabling:
 * - Self-awareness of what was said
 * - Memory of spoken phrases
 * - Connection between speech and underlying concepts
 * - Temporal sequencing of utterances
 * - Self-recognition in audio feedback
 * 
 * Every word Melvin speaks becomes a node, linked to:
 * - Its semantic meaning
 * - Previous/next words (temporal chain)
 * - The audio waveform produced
 * - The agent marker (Melvin himself)
 */
class SpeechIntent {
public:
    /**
     * Speech node types (extends standard node types)
     */
    enum SpeechNodeType : uint8_t {
        SPEECH_OUTPUT = 20,    // Raw speech event
        SPOKEN_WORD = 21,      // Individual word spoken
        UTTERANCE = 22,        // Complete phrase/sentence
        PHONEME = 23,          // Phonetic unit (future)
        SPEAKER_AGENT = 24     // Speaker identity
    };
    
    /**
     * Speech relation types (extends standard relations)
     */
    enum SpeechRelation : uint8_t {
        SPOKEN_AS = 20,        // Text → Audio
        SELF_PRODUCED = 21,    // Audio → Agent
        DERIVES_FROM = 22,     // Speech → Concept
        HEARD_AS = 23,         // Speech → Audio feedback
        UTTERED_BY = 24        // Speech → Speaker
    };
    
    SpeechIntent(AtomicGraph& graph);
    ~SpeechIntent();
    
    // ========================================================================
    // SPEECH OUTPUT PROCESSING
    // ========================================================================
    
    /**
     * Process text output and create graph representation
     * 
     * @param text Text that Melvin is about to speak
     * @param concept_root Root concept that prompted this speech (optional)
     * @return Speech node ID
     */
    uint64_t process_output(const std::string& text, uint64_t concept_root = 0);
    
    /**
     * Process output with explicit cause
     * 
     * @param text Text to speak
     * @param cause_node Node that caused this utterance
     * @return Speech node ID
     */
    uint64_t process_with_cause(const std::string& text, uint64_t cause_node);
    
    /**
     * Mark speech event as completed (audio generated)
     * 
     * @param speech_id Speech node ID
     * @param audio_file Path to generated audio file (optional)
     */
    void mark_spoken(uint64_t speech_id, const std::string& audio_file = "");
    
    // ========================================================================
    // SELF-RECOGNITION
    // ========================================================================
    
    /**
     * Check if an audio event matches Melvin's own speech
     * 
     * @param audio_label Recognized audio text
     * @param timestamp When audio was heard
     * @return true if this is Melvin's own voice
     */
    bool is_self_speech(const std::string& audio_label, float timestamp);
    
    /**
     * Create self-recognition link (speech output → audio input)
     * 
     * @param speech_id Output speech node
     * @param audio_id Input audio node (from AudioPipeline)
     */
    void link_self_recognition(uint64_t speech_id, uint64_t audio_id);
    
    // ========================================================================
    // REFLECTION & MEMORY
    // ========================================================================
    
    /**
     * Get what Melvin said recently
     * 
     * @param time_window Seconds to look back
     * @return Vector of speech nodes
     */
    std::vector<uint64_t> get_recent_speech(float time_window = 10.0f);
    
    /**
     * Find what Melvin said about a concept
     * 
     * @param concept_label Concept to search for
     * @return Vector of speech nodes
     */
    std::vector<uint64_t> find_speech_about(const std::string& concept_label);
    
    /**
     * Get the last thing Melvin said
     */
    uint64_t get_last_speech() const { return last_speech_id_; }
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    size_t get_speech_count() const { return speech_count_; }
    size_t get_word_count() const { return word_count_; }
    size_t get_self_recognition_count() const { return self_recognition_count_; }
    
    void print_stats() const;
    
private:
    AtomicGraph& graph_;
    
    // Speech tracking
    uint64_t last_speech_id_ = 0;
    uint64_t melvin_agent_id_ = 0;
    
    // Statistics
    size_t speech_count_ = 0;
    size_t word_count_ = 0;
    size_t self_recognition_count_ = 0;
    
    // Recent speech tracking (for self-recognition)
    struct SpeechRecord {
        uint64_t speech_id;
        std::string text;
        float timestamp;
    };
    std::vector<SpeechRecord> recent_speech_;
    
    // Helper methods
    std::vector<std::string> tokenize(const std::string& text);
    std::string get_concept(const std::string& word);
    std::string generate_speech_id();
    float get_current_timestamp();
    void cleanup_old_records(float current_time);
};

} // namespace io
} // namespace melvin

