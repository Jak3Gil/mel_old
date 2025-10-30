#ifndef CONTINUOUS_REASONING_H
#define CONTINUOUS_REASONING_H

#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>

namespace melvin {
namespace cognitive_field {

/**
 * Continuous Reasoning Engine
 * 
 * Fuses EnergyField + Hopfield dynamics into a single continuous system
 * running at 10-30Hz. Instead of discrete "predict → evaluate → consolidate",
 * reasoning happens as continuous attractor convergence and energy diffusion.
 * 
 * Reasoning stability is detected automatically via variance < ε, not
 * by explicit step counts.
 */

class ContinuousReasoning {
public:
    ContinuousReasoning(size_t embedding_dim = 128);
    
    // ========================================================================
    // Continuous Dynamics (10-30Hz)
    // ========================================================================
    
    /**
     * Update reasoning state (called every 33-100ms)
     * @param dt - Time delta in seconds
     */
    void update(float dt);
    
    /**
     * Get current reasoning rate
     */
    float get_update_rate_hz() const { return update_rate_hz_; }
    void set_update_rate_hz(float rate) { update_rate_hz_ = rate; }
    
    // ========================================================================
    // Energy Dynamics
    // ========================================================================
    
    /**
     * Energy injection from inputs
     */
    void inject_energy(int node_id, float energy);
    
    /**
     * Energy diffusion through edges
     * Happens continuously during update()
     */
    void diffuse_energy(float dt);
    
    /**
     * Get current energy of a node
     */
    float get_energy(int node_id) const;
    
    /**
     * Get total field energy
     */
    float get_total_energy() const;
    
    // ========================================================================
    // Hopfield Dynamics
    // ========================================================================
    
    /**
     * Hopfield attractor update
     * Pulls node activations toward stable patterns
     */
    void hopfield_step(float dt);
    
    /**
     * Set Hopfield weight matrix
     * @param weights - Symmetric weight matrix (node_i, node_j) -> weight
     */
    void set_hopfield_weights(const std::unordered_map<std::pair<int, int>, float>& weights);
    
    /**
     * Compute Hopfield energy (stability measure)
     * Lower energy = more stable attractor
     */
    float compute_hopfield_energy() const;
    
    // ========================================================================
    // Attractor Convergence Detection
    // ========================================================================
    
    /**
     * Check if reasoning has converged to a stable state
     * Convergence = variance in activations < epsilon
     */
    bool has_converged() const;
    
    /**
     * Get convergence variance
     */
    float get_convergence_variance() const;
    
    /**
     * Get time spent in current attractor
     */
    float get_attractor_duration_ms() const;
    
    /**
     * Force reset to new attractor basin
     */
    void reset_attractor();
    
    // ========================================================================
    // State Access
    // ========================================================================
    
    /**
     * Get current activation of a node
     */
    float get_activation(int node_id) const;
    
    /**
     * Get all active nodes (activation > threshold)
     */
    std::vector<int> get_active_nodes(float threshold = 0.1f) const;
    
    /**
     * Get node state (energy + activation + embedding)
     */
    struct NodeState {
        float energy;
        float activation;
        std::vector<float> embedding;
    };
    NodeState get_node_state(int node_id) const;
    
    // ========================================================================
    // Parameters (Genome-Controlled)
    // ========================================================================
    
    struct Parameters {
        float energy_decay_rate = 0.95f;
        float energy_diffusion_rate = 0.3f;
        float hopfield_update_strength = 0.1f;
        float convergence_epsilon = 0.01f;
        float min_energy_threshold = 0.001f;
        float activation_temperature = 1.0f;  // For sigmoid steepness
    };
    
    void set_parameters(const Parameters& params);
    Parameters get_parameters() const { return params_; }
    
    // ========================================================================
    // Statistics
    // ========================================================================
    
    struct Stats {
        size_t total_updates;
        float avg_update_duration_ms;
        float current_hopfield_energy;
        float convergence_variance;
        size_t convergence_count;
        float avg_attractor_duration_ms;
        size_t active_node_count;
        float total_field_energy;
    };
    
    Stats get_stats() const;
    
private:
    size_t embedding_dim_;
    float update_rate_hz_ = 30.0f;
    
    // Node states
    struct NodeData {
        float energy = 0.0f;
        float activation = 0.0f;
        float prev_activation = 0.0f;  // For convergence detection
        std::vector<float> embedding;
        std::chrono::high_resolution_clock::time_point last_update;
    };
    
    std::unordered_map<int, NodeData> nodes_;
    mutable std::mutex nodes_mutex_;
    
    // Hopfield weights
    std::unordered_map<std::pair<int, int>, float> hopfield_weights_;
    mutable std::mutex weights_mutex_;
    
    // Convergence tracking
    bool converged_ = false;
    float convergence_variance_ = 1.0f;
    std::chrono::high_resolution_clock::time_point attractor_start_time_;
    
    // Parameters
    Parameters params_;
    
    // Statistics
    Stats stats_;
    
    // Helper functions
    float compute_activation_variance() const;
    void update_convergence_state();
    
    // Hash function for pair<int, int> (for unordered_map)
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator() (const std::pair<T1, T2>& p) const {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            return h1 ^ (h2 << 1);
        }
    };
};

} // namespace cognitive_field
} // namespace melvin

#endif // CONTINUOUS_REASONING_H

