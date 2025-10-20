/**
 * @file melvin_context.h
 * @brief Unified Multimodal Context System
 * 
 * This is Melvin's "consciousness" - the unified field of active meaning
 * that integrates vision, audio, and text into a coherent world model.
 * 
 * Biological parallels:
 * - Parietal-temporal-frontal integration zones (multimodal fusion)
 * - Hippocampus (episodic context snapshots)
 * - Default mode network (continuous world modeling)
 * - Predictive coding (expectation vs reality)
 * 
 * Theoretical basis:
 * - Global Workspace Theory (Baars): Conscious content = active context
 * - Predictive Processing (Friston): Brain as prediction machine
 * - Associative Memory (Hopfield): Attractor dynamics for coherence
 */

#pragma once

#include "melvin_types.h"
#include "melvin_graph.h"
#include <vector>
#include <deque>
#include <unordered_map>
#include <string>
#include <chrono>

namespace melvin {

// ============================================================================
// MULTIMODAL INPUT FRAMES
// ============================================================================

/**
 * Vision frame data
 */
struct VisionFrame {
    uint64_t frame_id;
    std::vector<uint64_t> object_nodes;     // Detected objects
    std::vector<uint64_t> feature_nodes;    // Visual features
    std::vector<float> saliencies;          // Per-object saliency
    double timestamp;
};

/**
 * Audio frame data
 */
struct AudioFrame {
    uint64_t frame_id;
    std::vector<uint64_t> sound_nodes;      // Detected sounds
    std::vector<uint64_t> phoneme_nodes;    // Speech phonemes (if any)
    std::vector<float> amplitudes;          // Per-sound amplitude
    double timestamp;
};

/**
 * Text/linguistic input
 */
struct TextInput {
    std::string text;
    std::vector<uint64_t> concept_nodes;    // Parsed concepts
    std::vector<uint64_t> relation_nodes;   // Extracted relations
    double timestamp;
};

// ============================================================================
// ATTENTION SCORES (A,R,N,T,C Model)
// ============================================================================

/**
 * A,R,N,T,C attention scoring
 * (Alternative to S,G,C - more comprehensive)
 */
struct FocusScores {
    float appearance;      // A: Visual/audio salience (bottom-up)
    float relevance;       // R: Goal relevance (top-down)
    float need;            // N: Urgency/motivation
    float temporal;        // T: Persistence over time
    float curiosity;       // C: Novelty/prediction error
    
    // Combined score
    float combined() const {
        return appearance * 0.40f + relevance * 0.30f + need * 0.20f +
               temporal * 0.05f + curiosity * 0.05f;
    }
    
    FocusScores() : appearance(0), relevance(0), need(0), temporal(0), curiosity(0) {}
};

// ============================================================================
// CONTEXT FIELD - Unified Multimodal Working Memory
// ============================================================================

/**
 * ContextField - Unified multimodal context system
 * 
 * This represents Melvin's "current understanding of reality"
 * 
 * What it contains:
 * - Active visual objects ("I see a cup")
 * - Active sounds ("I hear a clink")
 * - Active concepts ("tea", "kitchen", "morning")
 * - Active relations ("cup contains tea", "clink from cup")
 * - Predictions ("steam will appear", "person will drink")
 * 
 * How it works:
 * 1. Inputs inject activation into nodes
 * 2. Activation spreads through semantic edges
 * 3. Weak activations decay (forgetting)
 * 4. Strong activations persist (working memory)
 * 5. Predictions generated from active patterns
 * 6. Errors trigger learning (LEAP formation)
 * 
 * This is the "living meaning" flowing through Melvin's brain!
 */
class ContextField {
public:
    ContextField(AtomicGraph& graph);
    ~ContextField();
    
    // ========================================================================
    // MULTIMODAL INPUT INTEGRATION
    // ========================================================================
    
    /**
     * Update context from vision input
     * Injects visual nodes, spreads to related concepts
     */
    void update_from_vision(const VisionFrame& vf);
    
    /**
     * Update context from audio input
     * Injects audio nodes, links temporal sequences
     */
    void update_from_audio(const AudioFrame& af);
    
    /**
     * Update context from text/linguistic input
     * Injects concepts, strengthens semantic relations
     */
    void update_from_text(const TextInput& text);
    
    /**
     * Update from reasoning output
     * Thoughts reinforce context
     */
    void update_from_reasoning(uint64_t subject, uint64_t predicate, uint64_t object);
    
    // ========================================================================
    // CONTEXT DYNAMICS (Hopfield-like Attractor)
    // ========================================================================
    
    /**
     * Spread activation through graph edges
     * Like neural excitation flowing through synapses
     * 
     * Algorithm:
     * For each active node:
     *   For each edge to neighbor:
     *     neighbor.activation += node.activation × edge.weight × diffusion_rate
     * 
     * Creates attractor dynamics - related concepts co-activate
     */
    void spread_activation(float diffusion_rate = 0.05f);
    
    /**
     * Decay inactive nodes (forgetting)
     * activation_new = activation_old × decay_rate
     */
    void apply_decay(float dt);
    
    /**
     * Normalize activations to prevent runaway excitation
     */
    void normalize();
    
    /**
     * Settle into stable attractor state
     * Run multiple diffuse-decay cycles until convergence
     */
    void settle(int max_iterations = 10);
    
    // ========================================================================
    // ATTENTION COMPUTATION (A,R,N,T,C Model)
    // ========================================================================
    
    /**
     * Compute focus scores for all active nodes
     * Returns nodes ranked by A,R,N,T,C combined score
     */
    std::vector<std::pair<uint64_t, FocusScores>> compute_focus(
        float need_level = 0.5f  // Current task urgency
    );
    
    /**
     * Get top-K context anchors
     * These are what Melvin is "consciously aware of"
     */
    std::vector<uint64_t> get_active_context(float threshold = 0.3f) const;
    
    /**
     * Get focus scores for specific node
     */
    FocusScores get_scores(uint64_t node_id) const;
    
    // ========================================================================
    // PREDICTIVE CODING
    // ========================================================================
    
    /**
     * Generate predictions about next sensory inputs
     * Uses EXPECTS edges from active concepts
     * 
     * Example: If "dark_cloud" active, predict "rain"
     */
    struct Prediction {
        uint64_t predicted_node;
        float confidence;
        double expected_time;
        std::vector<uint64_t> basis_nodes;  // What supports this prediction
    };
    
    std::vector<Prediction> predict_next_state();
    
    /**
     * Compare predictions to actual inputs
     * Returns prediction error for learning
     * 
     * High error → High curiosity → Triggers LEAP formation
     */
    float compute_prediction_error(const std::vector<uint64_t>& actual_nodes);
    
    /**
     * Learn from prediction errors
     * Strengthens correct predictions, weakens wrong ones
     * Forms LEAP edges when beneficial
     */
    void learn_from_error(float error, const std::vector<uint64_t>& actual_nodes);
    
    // ========================================================================
    // EPISODIC MEMORY (Scene Snapshots)
    // ========================================================================
    
    /**
     * Snapshot of current context state
     */
    struct ContextSnapshot {
        uint64_t snapshot_id;
        double timestamp;
        
        std::vector<uint64_t> visual_nodes;
        std::vector<uint64_t> audio_nodes;
        std::vector<uint64_t> concept_nodes;
        std::vector<std::pair<uint64_t, float>> activations;  // node_id, activation
        
        // Summary
        std::string scene_description;
    };
    
    /**
     * Create snapshot of current context
     * Like taking a photo of what Melvin understands right now
     */
    ContextSnapshot create_snapshot();
    
    /**
     * Store snapshot as episodic memory
     * Creates SceneNode in graph linked to all active nodes
     */
    void store_episode(const ContextSnapshot& snapshot);
    
    /**
     * Recall similar past contexts
     * Uses graph similarity to find related episodes
     */
    std::vector<ContextSnapshot> recall_similar_contexts(
        const ContextSnapshot& query,
        int top_k = 5
    );
    
    // ========================================================================
    // MODALITY WEIGHTS (Dynamic Adaptation)
    // ========================================================================
    
    /**
     * Set relative importance of each modality
     * Adapts based on task and environment
     */
    void set_modality_weights(float vision, float audio, float text);
    
    /**
     * Compute adaptive modality weights
     * Example: Dark environment → boost audio weight
     */
    void compute_adaptive_weights();
    
    // ========================================================================
    // QUERIES & VISUALIZATION
    // ========================================================================
    
    /**
     * Get current world understanding as string
     */
    std::string describe_current_context() const;
    
    /**
     * Visualize active context (console)
     */
    void visualize_context(int top_n = 15) const;
    
    /**
     * Get statistics
     */
    struct ContextStats {
        size_t total_active_nodes;
        size_t visual_nodes;
        size_t audio_nodes;
        size_t concept_nodes;
        float avg_activation;
        float max_activation;
        size_t num_predictions;
        float avg_prediction_error;
    };
    ContextStats get_stats() const;
    void print_stats() const;
    
private:
    // ========================================================================
    // INTERNAL STATE
    // ========================================================================
    
    AtomicGraph& graph_;
    
    // Activation field (the core of context)
    std::unordered_map<uint64_t, float> activation_;
    std::unordered_map<uint64_t, int> persistence_frames_;  // For T score
    
    // Modality weights
    float alpha_vision_;
    float alpha_audio_;
    float alpha_text_;
    
    // Recent context history (for temporal coherence)
    std::deque<ContextSnapshot> recent_snapshots_;
    
    // Predictions (for predictive coding)
    std::vector<Prediction> active_predictions_;
    float cumulative_prediction_error_;
    
    // Focus anchors (current conscious content)
    std::vector<uint64_t> focus_anchors_;
    
    // Timing
    std::chrono::steady_clock::time_point last_update_;
    uint64_t cycle_count_;
    
    // Statistics
    ContextStats stats_;
    
    // ========================================================================
    // HELPER FUNCTIONS
    // ========================================================================
    
    // Compute appearance score (visual/audio salience)
    float compute_appearance(uint64_t node_id) const;
    
    // Compute relevance (goal matching)
    float compute_relevance(uint64_t node_id) const;
    
    // Compute temporal persistence
    float compute_temporal(uint64_t node_id) const;
    
    // Compute curiosity (prediction error)
    float compute_curiosity(uint64_t node_id) const;
    
    // Check if node matches prediction
    bool matches_prediction(uint64_t node_id) const;
};

} // namespace melvin

