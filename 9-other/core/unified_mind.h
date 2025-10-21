#pragma once

#include "atomic_graph.h"
#include "attention_manager.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>

namespace melvin {

// ============================================================================
// CORE STRUCTURES
// ============================================================================

/**
 * Sensory input from all modalities
 */
struct SensoryFrame {
    double timestamp;
    
    // Vision
    std::vector<FocusCandidate> visual_candidates;
    
    // Audio
    std::vector<FocusCandidate> audio_candidates;
    
    // Internal state
    float battery_level;
    float temperature;
    std::string current_goal;
    
    SensoryFrame() : timestamp(0), battery_level(1.0f), temperature(0.5f) {}
};

/**
 * Selected focus target (one per cycle)
 */
struct FocusTarget {
    std::string id;           // e.g. "object_5", "sound_3"
    std::string type;         // "visual", "audio", "internal"
    FocusCandidate candidate; // Full candidate data
    
    bool is_valid() const { return !id.empty(); }
};

/**
 * Structured thought = subject + predicate + object
 */
struct Thought {
    std::string subject;      // e.g. "object_5"
    std::string predicate;    // e.g. "is", "moves", "relates_to"
    std::string object;       // e.g. "person", "fire", "center"
    
    double timestamp;
    float confidence;
    
    // Keywords for feedback
    std::vector<std::string> keywords;
    
    // Full sentence
    std::string to_sentence() const {
        return subject + " " + predicate + " " + object;
    }
    
    Thought() : timestamp(0), confidence(0.5f) {}
};

/**
 * Output representation
 */
struct OutputExpression {
    std::string text;         // "I see object_5"
    std::string speech_text;  // For TTS
    float emotional_tone;     // 0=neutral, 1=excited, -1=concerned
    
    double timestamp;
    
    OutputExpression() : emotional_tone(0), timestamp(0) {}
};

// ============================================================================
// FEEDBACK BUS
// ============================================================================

/**
 * Routes reasoning output back to attention system
 * Creates dynamic biases based on what Melvin is thinking about
 */
class FeedbackBus {
public:
    FeedbackBus();
    ~FeedbackBus();
    
    /**
     * Process a thought and extract biases
     */
    void process_thought(const Thought& thought);
    
    /**
     * Get current context biases for attention
     */
    std::unordered_map<std::string, float> get_attention_biases() const;
    
    /**
     * Decay biases over time (so they don't persist forever)
     */
    void decay_biases(float decay_rate = 0.95f);
    
    /**
     * Clear all biases
     */
    void clear();
    
private:
    std::unordered_map<std::string, float> keyword_biases_;  // keyword -> bias weight
    std::unordered_map<std::string, int> keyword_mentions_;  // keyword -> count
};

// ============================================================================
// UNIFIED MIND
// ============================================================================

/**
 * Central orchestrator for the complete cognitive loop:
 * Input → Perception → Attention → Reasoning → Output → Feedback
 */
class UnifiedMind {
public:
    UnifiedMind();
    ~UnifiedMind();
    
    // ========================================================================
    // INITIALIZATION
    // ========================================================================
    
    /**
     * Load existing knowledge and state
     */
    bool load(const std::string& nodes_path, const std::string& edges_path);
    
    /**
     * Save knowledge and state
     */
    bool save(const std::string& nodes_path, const std::string& edges_path);
    
    // ========================================================================
    // MAIN COGNITIVE LOOP
    // ========================================================================
    
    /**
     * Run one complete cognitive cycle
     * Returns: output expression (if any)
     */
    OutputExpression unified_cycle(const SensoryFrame& input);
    
    /**
     * Get current focus target
     */
    const FocusTarget& get_current_focus() const { return current_focus_; }
    
    /**
     * Get last thought
     */
    const Thought& get_last_thought() const { return last_thought_; }
    
    // ========================================================================
    // STATE MANAGEMENT
    // ========================================================================
    
    /**
     * Set cognitive state (affects attention weights)
     */
    void set_state(MelvinState state);
    
    /**
     * Get statistics
     */
    void print_status() const;
    
    /**
     * Get knowledge graph
     */
    AtomicGraph* get_graph() { return graph_.get(); }
    
private:
    // ========================================================================
    // PIPELINE STAGES
    // ========================================================================
    
    /**
     * STAGE 1: Perception - Extract focus candidates
     */
    void perception_stage(const SensoryFrame& input);
    
    /**
     * STAGE 2: Attention - Select focus target
     */
    FocusTarget attention_stage();
    
    /**
     * STAGE 3: Reasoning - Interpret focus and generate thought
     */
    Thought reasoning_stage(const FocusTarget& focus);
    
    /**
     * STAGE 4: Output - Express thought
     */
    OutputExpression output_stage(const Thought& thought);
    
    /**
     * STAGE 5: Feedback - Update biases and memory
     */
    void feedback_stage(const Thought& thought, const FocusTarget& focus);
    
    // ========================================================================
    // STATE
    // ========================================================================
    
    std::unique_ptr<AtomicGraph> graph_;
    std::unique_ptr<AttentionManager> attention_;
    std::unique_ptr<FeedbackBus> feedback_;
    
    // Current cycle state
    std::vector<FocusCandidate> current_candidates_;
    FocusTarget current_focus_;
    Thought last_thought_;
    
    // Cycle counter
    uint64_t cycle_count_;
    
    // Timing
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace melvin

