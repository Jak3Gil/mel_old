#ifndef SPREADING_ACTIVATION_H
#define SPREADING_ACTIVATION_H

#include <unordered_map>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <deque>

namespace melvin {
namespace reasoning {

// GAP 1: Prediction-Error Feedback
struct PredictionError {
    int predicted_node;
    int actual_node;
    float error_magnitude;
    float timestamp;
};

// GAP 2: Enhanced Energy System
struct EnergyDynamics {
    float energy = 0.0f;              // Current energy level
    float energy_input = 0.0f;        // Input from sensors this cycle
    float energy_buffer = 0.0f;       // Spreading from neighbors
    float energy_trace = 0.0f;        // Eligibility trace for credit assignment
    float salience = 1.0f;            // Input importance
    float novelty_bonus = 1.0f;       // New node bonus
    
    // Temporal tracking
    int activation_count = 0;
    float last_activation_time = 0.0f;
    
    void reset() {
        energy_input = 0.0f;
        energy_buffer = 0.0f;
    }
};

// GAP 3: Attention Mechanism
struct AttentionWeights {
    float base_attention = 1.0f;
    float goal_relevance = 1.0f;
    float surprise_value = 0.0f;
    float combined = 1.0f;
    
    void compute(float edge_weight, float goal_similarity, float surprise) {
        base_attention = edge_weight;
        goal_relevance = goal_similarity;
        surprise_value = surprise;
        combined = base_attention * (1.0f + goal_relevance) * (1.0f + surprise_value);
    }
};

// GAP 4: Temporal Edges
enum EdgeType {
    SEMANTIC,    // Co-occurrence
    TEMPORAL,    // A then B
    CAUSAL,      // A causes B
    PREDICTIVE   // If A, expect B
};

struct TemporalEdge {
    int src;
    int dst;
    float weight;
    EdgeType type = SEMANTIC;
    float delay_ms = 0.0f;           // Time between A and B
    float certainty = 1.0f;          // Confidence in relationship
    float conductance = 1.0f;        // Energy flow efficiency
    AttentionWeights attention;      // Attention gating
    float trace = 0.0f;              // Eligibility trace
};

// GAP 5: Meta-Learning
enum LearningPhase {
    EXPLORATION,    // High explore, high learning
    REFINEMENT,     // Medium explore, medium learning
    EXPLOITATION    // Low explore, consolidate
};

struct MetaLearner {
    float learning_rate = 0.1f;
    float exploration_rate = 0.3f;
    float consolidation_threshold = 0.8f;
    
    float recent_success_rate = 0.0f;
    float recent_surprise_rate = 0.0f;
    LearningPhase phase = EXPLORATION;
    
    int cycle_count = 0;
    std::deque<float> recent_errors;
    
    void adapt(float success, float surprise);
};

class ActivationField {
public:
    ActivationField(float decay_rate = 0.9f, float spread_rate = 0.3f, float min_activation = 0.01f);
    ~ActivationField();
    
    // GAP 1: Prediction & Feedback
    int predict_next_node(int current_node);
    void apply_prediction_error(const PredictionError& error);
    
    // GAP 2: Enhanced Energy Management
    void inject_energy(int node_id, float strength, float salience = 1.0f, float novelty = 1.0f);
    float get_energy(int node_id) const;
    EnergyDynamics& get_energy_dynamics(int node_id);
    void update_energy_dynamics(const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph);
    
    // GAP 3: Attention Control
    void set_goal_node(int goal_id, const std::vector<float>& goal_embedding);
    void compute_attention_weights(std::unordered_map<int, std::vector<TemporalEdge>>& edges,
                                   const std::unordered_map<int, std::vector<float>>& embeddings);
    
    // GAP 4: Temporal Predictions
    std::vector<int> predict_future_states(int current_node, 
                                           const std::unordered_map<int, std::vector<TemporalEdge>>& temporal_edges,
                                           float lookahead_ms);
    
    // GAP 5: Meta-Learning
    MetaLearner& get_meta_learner() { return meta_learner_; }
    void update_meta_parameters();
    
    // GAP 6: Consolidation Support
    std::vector<int> get_high_energy_nodes(float threshold = 0.5f) const;
    void decay_eligibility_traces(float decay_factor = 0.95f);
    
    // Original interface (enhanced)
    void activate(int node_id, float strength = 1.0f);
    float get_activation(int node_id) const;
    std::unordered_map<int, float> get_active_nodes(float threshold = 0.05f) const;
    
    // Background spreading loop
    void start_background_loop();
    void stop_background_loop();
    void tick(const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph);
    
    // Configuration
    void set_tick_rate(float hz) { tick_rate_ = hz; }
    float get_tick_rate() const { return tick_rate_; }
    void set_decay_rate(float rate) { decay_rate_ = rate; }
    
private:
    void background_loop();
    float compute_goal_similarity(int node_id, const std::vector<float>& goal_emb,
                                 const std::unordered_map<int, std::vector<float>>& embeddings);
    
    mutable std::mutex activation_mutex_;
    std::unordered_map<int, float> activations_;  // Legacy support
    std::unordered_map<int, EnergyDynamics> energy_map_;  // Enhanced energy system
    
    float decay_rate_;
    float spread_rate_;
    float min_activation_;
    float tick_rate_;
    
    // Energy system parameters
    float base_input_energy_ = 10.0f;
    float novelty_bonus_multiplier_ = 2.0f;
    float motor_cost_ = 5.0f;
    
    // Attention state
    int current_goal_node_ = -1;
    std::vector<float> current_goal_embedding_;
    
    // Meta-learning
    MetaLearner meta_learner_;
    
    // Prediction tracking
    std::deque<PredictionError> recent_predictions_;
    int last_predicted_node_ = -1;
    
    std::atomic<bool> running_;
    std::thread background_thread_;
    
    // Graph reference for background spreading
    const std::unordered_map<int, std::vector<std::pair<int, float>>>* graph_ptr_;
    
    float current_time_ = 0.0f;
};

} // namespace reasoning
} // namespace melvin

#endif // SPREADING_ACTIVATION_H
