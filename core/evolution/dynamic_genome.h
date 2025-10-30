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
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // ACTIVATION FIELD GENES (Spreading dynamics)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    float global_decay_rate;       // field-wide energy dissipation (0.01-0.1)
    float activation_threshold;    // min energy to be "active" (0.05-0.3)
    float spreading_factor;        // how much activation spreads (0.5-0.95)
    float max_activation;          // energy ceiling per node (1.0-10.0)
    float kwta_sparsity;           // top-k winners to keep (0.05-0.2)
    int max_active_nodes;          // hard limit on active nodes (100-2000)
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // HEBBIAN LEARNING GENES (Synaptic plasticity)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    float hebbian_learning_rate;   // η - "fire together wire together" (0.001-0.05)
    float anti_hebbian_rate;       // weakening for non-coactivated (0.0-0.01)
    float min_edge_weight;         // prune edges below this (0.01-0.1)
    float max_edge_weight;         // cap edge weights (1.0-10.0)
    float weight_decay;            // synaptic forgetting (0.001-0.01)
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // SCORING GENES (Concept ranking)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    float activation_weight;       // β - raw activation influence
    float semantic_bias_weight;    // α - semantic alignment influence
    float coherence_weight;        // γ - path coherence influence
    float recency_weight;          // δ - how much time matters (0.0-0.3)
    float novelty_weight;          // ε - reward for new concepts (0.0-0.5)
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // META-LEARNING GENES (Self-tuning)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    float learning_rate;           // η - edge weight update rate
    float adaptation_rate;         // κ - genome self-tuning speed
    float confidence_decay;        // how fast to forget unreliable edges
    float mutation_rate;           // probability of random parameter change (0.0-0.1)
    float mutation_magnitude;      // size of random perturbations (0.01-0.2)
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // TRAVERSAL GENES (Path following)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    float temperature;             // β(t) - exploration vs exploitation
    float confidence_threshold;    // θ - minimum confidence to answer
    float semantic_threshold;      // minimum similarity to follow edge
    int max_hops;                  // maximum reasoning depth (2-10)
    float hop_decay;               // energy loss per hop (0.7-0.95)
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // MODE SWITCHING GENES (Adaptive behavior)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    float exploratory_threshold;   // switch to exploration when confidence below this
    float exploitative_threshold;  // switch to exploitation when confidence above this
    float deep_reasoning_threshold; // trigger multi-hop when complexity above this
    float mode_switching_hysteresis; // prevent rapid mode flipping (0.05-0.2)
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // WORKING MEMORY GENES (Attention buffer)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    float base_decay_rate;         // baseline memory decay
    float confidence_decay_factor; // how much confidence reduces decay
    int working_memory_slots;      // 4-7 active concepts
    float salience_threshold;      // min importance to enter WM (0.1-0.5)
    float wm_refresh_boost;        // energy boost for WM items (0.1-0.5)
    
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
    
    // Baseline/Spontaneous Activity Parameters (Default Mode Network)
    float baseline_activity_min;        // minimum resting activity (2.0-5.0 nodes)
    float baseline_activity_max;        // maximum resting activity (5.0-15.0 nodes)
    float baseline_adaptation_rate;     // how fast baseline tracks recent activity (0.01-0.1)
    float curiosity_baseline_scale;     // how much curiosity boosts baseline (0.1-0.5)
    float boredom_baseline_scale;       // how much boredom boosts baseline (0.05-0.3)
    float baseline_decay_multiplier;    // faster decay for spontaneous thoughts (1.2-2.0)
    float baseline_power_budget;        // max % of total energy for baseline (0.03-0.10)
    float dmn_cycle_period;             // seconds between network focus switches (5.0-30.0)
    float introspection_bias;           // probability of self-related vs random (0.3-0.8)
    float novelty_exploration_weight;   // weight for curiosity-driven exploration (0.2-0.7)
    
    DynamicReasoningParams() :
        // Activation field defaults
        global_decay_rate(0.05f),
        activation_threshold(0.15f),
        spreading_factor(0.85f),
        max_activation(5.0f),
        kwta_sparsity(0.1f),
        max_active_nodes(1000),
        // Hebbian learning defaults
        hebbian_learning_rate(0.01f),
        anti_hebbian_rate(0.005f),
        min_edge_weight(0.05f),
        max_edge_weight(5.0f),
        weight_decay(0.005f),
        // Scoring defaults
        activation_weight(0.4f),
        semantic_bias_weight(0.4f),
        coherence_weight(0.15f),
        recency_weight(0.05f),
        novelty_weight(0.2f),
        // Meta-learning defaults
        learning_rate(0.01f),
        adaptation_rate(0.005f),
        confidence_decay(0.001f),
        mutation_rate(0.02f),
        mutation_magnitude(0.1f),
        // Traversal defaults
        temperature(1.0f),
        confidence_threshold(0.5f),
        semantic_threshold(0.3f),
        max_hops(5),
        hop_decay(0.85f),
        // Mode switching defaults
        exploratory_threshold(0.4f),
        exploitative_threshold(0.8f),
        deep_reasoning_threshold(0.7f),
        mode_switching_hysteresis(0.1f),
        // Working memory defaults
        base_decay_rate(0.1f),
        confidence_decay_factor(0.5f),
        working_memory_slots(7),
        salience_threshold(0.25f),
        wm_refresh_boost(0.3f),
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
        output_history_window(2.0f),
        // Baseline activity defaults
        baseline_activity_min(3.0f),
        baseline_activity_max(8.0f),
        baseline_adaptation_rate(0.05f),
        curiosity_baseline_scale(0.3f),
        boredom_baseline_scale(0.15f),
        baseline_decay_multiplier(1.5f),
        baseline_power_budget(0.05f),
        dmn_cycle_period(15.0f),
        introspection_bias(0.6f),
        novelty_exploration_weight(0.4f)
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
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // CONTINUOUS EVOLUTION (NEW)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    // Evolve towards intelligence when idle (no prompt)
    void evolve_towards_intelligence(float dt);
    
    // Random mutation for exploration
    void mutate_random_genes(int count = 3);
    
    // Get all gene pointers for generic access
    std::vector<std::pair<std::string, float*>> get_all_gene_ptrs();
    
    // Set any gene by name
    void set_gene(const std::string& name, float value);
    
    // Get any gene by name
    float get_gene(const std::string& name) const;
    
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

