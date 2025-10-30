/**
 * @file dynamic_genome.h
 * @brief Extended genome with adaptive reasoning parameters
 * 
 * Adds self-tuning parameters for dynamic intelligence:
 * - Semantic biasing weights
 * - Learning rates
 * - Adaptive thresholds
 * - Mode control parameters
 */

#ifndef MELVIN_DYNAMIC_GENOME_H
#define MELVIN_DYNAMIC_GENOME_H

#include <string>
#include <unordered_map>
#include <vector>

namespace melvin {
namespace evolution {

/**
 * @brief Dynamic parameters for adaptive reasoning
 * 
 * These parameters self-tune based on reasoning success
 */
struct DynamicReasoningParams {
    // Scoring weights (α, β, γ) - sum to 1.0
    float activation_weight;       // β - raw activation influence
    float semantic_bias_weight;    // α - semantic alignment influence
    float coherence_weight;        // γ - path coherence influence
    
    // Learning parameters
    float learning_rate;           // η - edge weight update rate
    float adaptation_rate;         // κ - self-tuning speed
    float confidence_decay;        // how fast to forget unreliable edges
    
    // Traversal control
    float temperature;             // β(t) - exploration vs exploitation
    float confidence_threshold;    // θ - minimum confidence to answer
    float semantic_threshold;      // minimum similarity to follow edge
    
    // Mode switching thresholds
    float exploratory_threshold;   // switch to exploration when confidence below this
    float exploitative_threshold;  // switch to exploitation when confidence above this
    float deep_reasoning_threshold; // trigger multi-hop when complexity above this
    
    // Working memory
    float base_decay_rate;         // baseline memory decay
    float confidence_decay_factor; // how much confidence reduces decay
    int working_memory_slots;      // 4-7 active concepts
    
    // Multi-modal
    float text_modality_weight;    // relative importance
    float vision_modality_weight;
    float audio_modality_weight;
    
    // Conversational parameters (Phase 2-4)
    float theta_frequency;         // Hz - turn-taking rhythm (4-8 Hz)
    float speech_threshold;        // theta value to start speaking (0.5-0.9)
    float listen_threshold;        // theta value to start listening (-0.5-0)
    float energy_threshold;        // minimum field energy to speak (0.3-0.8)
    float min_state_duration;      // minimum time per state in seconds (0.5-2.0)
    
    // Emotional modulation parameters
    float base_tempo;              // baseline speech tempo multiplier (0.8-1.2)
    float novelty_tempo_scale;     // how much novelty affects tempo (0.3-0.8)
    float confidence_tempo_scale;  // how much confidence affects tempo (0.2-0.5)
    float arousal_pitch_scale;     // how much arousal affects pitch (0.5-2.0)
    float hedge_confidence_threshold; // confidence below which to hedge (0.2-0.6)
    
    // Goal stack parameters
    float goal_decay_rate;         // importance decay per turn (0.90-0.98)
    float goal_reactivation_boost; // importance boost on re-mention (0.1-0.4)
    float goal_overlap_threshold;  // minimum overlap to re-activate (0.15-0.35)
    int max_turns_inactive;        // turns before pruning goal (5-15)
    float min_goal_importance;     // minimum importance to keep (0.1-0.3)
    int max_context_nodes;         // context nodes from goals (10-30)
    
    // Self-monitoring parameters
    float self_monitoring_gain;    // corollary discharge attenuation (0.1-0.3)
    float output_history_window;   // seconds to keep in history (1.0-5.0)
    
    DynamicReasoningParams() :
        activation_weight(0.4f),
        semantic_bias_weight(0.4f),
        coherence_weight(0.2f),
        learning_rate(0.01f),
        adaptation_rate(0.005f),
        confidence_decay(0.001f),
        temperature(1.0f),
        confidence_threshold(0.5f),
        semantic_threshold(0.3f),
        exploratory_threshold(0.4f),
        exploitative_threshold(0.8f),
        deep_reasoning_threshold(0.7f),
        base_decay_rate(0.1f),
        confidence_decay_factor(0.5f),
        working_memory_slots(7),
        text_modality_weight(1.0f),
        vision_modality_weight(1.0f),
        audio_modality_weight(1.0f),
        // Conversational defaults
        theta_frequency(6.0f),
        speech_threshold(0.65f),
        listen_threshold(-0.3f),
        energy_threshold(0.5f),
        min_state_duration(0.8f),
        base_tempo(1.0f),
        novelty_tempo_scale(0.6f),
        confidence_tempo_scale(0.3f),
        arousal_pitch_scale(1.5f),
        hedge_confidence_threshold(0.4f),
        goal_decay_rate(0.95f),
        goal_reactivation_boost(0.25f),
        goal_overlap_threshold(0.25f),
        max_turns_inactive(10),
        min_goal_importance(0.15f),
        max_context_nodes(20),
        self_monitoring_gain(0.2f),
        output_history_window(2.0f)
    {}
    
    void normalize_weights() {
        float sum = activation_weight + semantic_bias_weight + coherence_weight;
        if (sum > 0) {
            activation_weight /= sum;
            semantic_bias_weight /= sum;
            coherence_weight /= sum;
        }
    }
    
    void normalize_modality_weights() {
        float sum = text_modality_weight + vision_modality_weight + audio_modality_weight;
        if (sum > 0) {
            text_modality_weight /= sum;
            vision_modality_weight /= sum;
            audio_modality_weight /= sum;
        }
    }
    
    // Apply conversational genome to controllers
    void apply_to_turn_taking(void* controller_ptr) const;
    void apply_to_emotional_modulator(void* modulator_ptr) const;
    void apply_to_goal_stack(void* goal_stack_ptr) const;
};

/**
 * @brief Evolution tracking for meta-learning
 */
struct ReasoningProfile {
    std::string query_type;
    float avg_confidence;
    float avg_coherence;
    float success_rate;
    int num_attempts;
    DynamicReasoningParams best_params;
    
    void update_from_episode(float confidence, float coherence, bool success) {
        avg_confidence = (avg_confidence * num_attempts + confidence) / (num_attempts + 1);
        avg_coherence = (avg_coherence * num_attempts + coherence) / (num_attempts + 1);
        success_rate = (success_rate * num_attempts + (success ? 1.0f : 0.0f)) / (num_attempts + 1);
        num_attempts++;
    }
};

/**
 * @brief Extended genome with dynamic intelligence parameters
 */
class DynamicGenome {
public:
    DynamicGenome();
    ~DynamicGenome() = default;
    
    // Access dynamic parameters
    DynamicReasoningParams& reasoning_params() { return reasoning_params_; }
    const DynamicReasoningParams& reasoning_params() const { return reasoning_params_; }
    
    // Meta-learning
    void log_reasoning_episode(
        const std::string& query_type,
        float confidence,
        float coherence,
        bool success
    );
    
    const ReasoningProfile* get_profile(const std::string& query_type) const;
    void apply_profile(const std::string& query_type);
    
    // Adaptive tuning
    void tune_from_feedback(
        float confidence,
        float coherence,
        bool success
    );
    
    void adapt_temperature(float current_confidence);
    void adapt_thresholds(float avg_path_length);
    
    // Serialization
    void save(const std::string& filepath) const;
    void load(const std::string& filepath);
    
private:
    DynamicReasoningParams reasoning_params_;
    std::unordered_map<std::string, ReasoningProfile> profiles_;
    
    // History for meta-learning
    std::vector<float> recent_confidences_;
    std::vector<bool> recent_successes_;
    static constexpr size_t HISTORY_SIZE = 100;
    
    void prune_history();
    float compute_recent_success_rate() const;
};

} // namespace evolution
} // namespace melvin

#endif // MELVIN_DYNAMIC_GENOME_H

