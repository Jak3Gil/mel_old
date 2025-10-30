#ifndef THREE_CHANNEL_FEEDBACK_H
#define THREE_CHANNEL_FEEDBACK_H

#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include "../fields/activation_field_unified.h"
#include "../evolution/genome.h"

namespace melvin {
namespace feedback {

/**
 * @brief Three-Channel Feedback System
 * 
 * Differentiates feedback into three distinct channels:
 * 1. Sensory Feedback - Physical consequences (camera, mic, motors)
 * 2. Cognitive Feedback - Internal predictions and errors
 * 3. Evolutionary Feedback - Long-term parameter adaptation
 */

// ============================================================================
// Channel 1: Sensory Feedback
// ============================================================================

struct SensoryFeedback {
    struct SensoryEvent {
        enum class Modality { VISION, AUDIO, MOTOR, TOUCH } modality;
        std::vector<float> observation;  // Raw sensory data
        float reward;  // Immediate reward/punishment (-1 to +1)
        std::chrono::high_resolution_clock::time_point timestamp;
        
        SensoryEvent(Modality m, const std::vector<float>& obs, float r)
            : modality(m), observation(obs), reward(r),
              timestamp(std::chrono::high_resolution_clock::now()) {}
    };
    
    std::vector<SensoryEvent> recent_events;
    size_t max_history = 1000;
    
    // Add new sensory feedback
    void add_event(const SensoryEvent& event);
    
    // Re-inject physical consequences into activation field
    void process_sensory_loop(fields::UnifiedActivationField& field);
    
    // Compute correlation between actions and rewards
    std::unordered_map<int, float> compute_action_values(
        const std::vector<int>& action_nodes);
};

// ============================================================================
// Channel 2: Cognitive Feedback
// ============================================================================

struct CognitiveFeedback {
    struct PredictionError {
        int predicted_node;
        int actual_node;
        float error_magnitude;  // |predicted - actual|
        float confidence;       // How confident was the prediction?
        std::vector<float> context_vector;  // What was the context?
        std::chrono::high_resolution_clock::time_point timestamp;
        
        PredictionError(int pred, int actual, float err, float conf,
                       const std::vector<float>& ctx)
            : predicted_node(pred), actual_node(actual),
              error_magnitude(err), confidence(conf), context_vector(ctx),
              timestamp(std::chrono::high_resolution_clock::now()) {}
    };
    
    std::vector<PredictionError> error_history;
    size_t max_history = 10000;
    
    // Attention bias adjustment
    std::unordered_map<int, float> attention_bias;  // Node -> bias weight
    
    // Add prediction error
    void add_error(const PredictionError& error);
    
    // Update attention bias based on error patterns
    void update_attention_bias();
    
    // Adjust edge weights based on prediction success
    void adjust_edge_weights(fields::UnifiedActivationField& field);
    
    // Compute surprise (Bayesian surprise or KL divergence)
    float compute_surprise(const std::vector<float>& predicted_dist,
                          const std::vector<float>& actual_dist);
    
    // Meta-learning: adjust learning rate based on error trends
    float adaptive_learning_rate(float base_rate);
};

// ============================================================================
// Channel 3: Evolutionary Feedback
// ============================================================================

struct EvolutionaryFeedback {
    struct FitnessMetrics {
        float prediction_accuracy;  // % correct predictions
        float energy_efficiency;    // Energy used per correct output
        float learning_speed;       // Rate of improvement
        float novelty_seeking;      // Exploration vs exploitation
        float coherence;            // Field stability
        
        // Compute overall fitness
        float compute_fitness(const evolution::Genome& genome) const;
    };
    
    // Genome population (5-10 variants)
    std::vector<evolution::Genome> population;
    size_t population_size = 7;
    
    // Fitness history for each genome
    std::vector<std::vector<float>> fitness_history;
    
    // Rolling window for temporal smoothing
    size_t smoothing_window = 100;  // 100 cycles
    std::vector<FitnessMetrics> recent_metrics;
    
    // Initialize population
    void initialize_population();
    
    // Evaluate current genome fitness
    void evaluate_fitness(const FitnessMetrics& metrics, int genome_index);
    
    // Compute smoothed fitness over N cycles
    float get_smoothed_fitness(int genome_index) const;
    
    // Select best genome from population
    int select_best_genome() const;
    
    // Crossover two genomes
    evolution::Genome crossover(const evolution::Genome& parent1,
                                const evolution::Genome& parent2);
    
    // Mutate only correlated genes
    void selective_mutation(evolution::Genome& genome,
                           const std::vector<std::string>& gene_names);
    
    // Find genes correlated with fitness variance
    std::vector<std::string> find_correlated_genes(
        const evolution::Genome& genome,
        const std::vector<float>& fitness_values);
    
    // Evolution step (every N cycles, not every tick)
    void evolution_step(int current_cycle);
    
    // Get current best genome
    evolution::Genome& get_current_genome();
};

// ============================================================================
// Unified Feedback Coordinator
// ============================================================================

class FeedbackCoordinator {
public:
    FeedbackCoordinator(fields::UnifiedActivationField& field);
    
    // Process all three feedback channels
    void process_feedback(float dt);
    
    // Get individual channels
    SensoryFeedback& sensory() { return sensory_; }
    CognitiveFeedback& cognitive() { return cognitive_; }
    EvolutionaryFeedback& evolutionary() { return evolutionary_; }
    
    // Get current active genome
    evolution::Genome& get_genome() { return evolutionary_.get_current_genome(); }
    
    // Statistics
    struct Stats {
        float avg_prediction_error;
        float avg_sensory_reward;
        float current_fitness;
        int evolution_generation;
        float attention_stability;
    };
    Stats get_stats() const;
    
private:
    fields::UnifiedActivationField& field_;
    
    SensoryFeedback sensory_;
    CognitiveFeedback cognitive_;
    EvolutionaryFeedback evolutionary_;
    
    int cycle_count_ = 0;
    
    // Channel update frequencies
    float sensory_update_rate_ = 30.0f;   // 30 Hz (every frame)
    float cognitive_update_rate_ = 10.0f;  // 10 Hz (every 100ms)
    float evolutionary_update_rate_ = 0.01f;  // Once per 100 cycles
    
    std::chrono::high_resolution_clock::time_point last_sensory_update_;
    std::chrono::high_resolution_clock::time_point last_cognitive_update_;
    std::chrono::high_resolution_clock::time_point last_evolutionary_update_;
};

} // namespace feedback
} // namespace melvin

#endif // THREE_CHANNEL_FEEDBACK_H

