#pragma once

#include "types.h"
#include "storage.h"
#include <vector>
#include <memory>

namespace melvin {
namespace energy {

/**
 * ⚡ Energy Field - Attention & Autonomous Thinking
 * 
 * Mathematical model of attention and consciousness:
 * - Energy flows through graph (diffusion)
 * - Decays naturally (forgetting)
 * - Regenerates from curiosity (prediction errors)
 * - Enables autonomous thinking (without input)
 * 
 * Key insight: Attention = where energy concentrates
 * Energy from prediction mismatch = curiosity!
 */
class EnergyField {
public:
    struct Config {
        float decay_rate = 0.95f;        // Energy fade per step
        float noise_floor = 0.001f;      // Minimum baseline activity
        float curiosity_gain = 0.05f;    // Prediction error → energy
        float diffusion_rate = 0.1f;     // Energy transfer rate
        float attention_gamma = 2.0f;    // Attention concentration exponent
        float regeneration_threshold = 0.1f; // Min energy to keep thinking
        bool verbose = false;            // Log energy flow
        
        Config();
    };
    
    explicit EnergyField(const Config& config = Config());
    ~EnergyField();
    
    /**
     * Inject energy from external input
     */
    void inject_input_energy(
        const std::vector<NodeID>& input_nodes,
        std::vector<Node>& nodes,
        float energy_amount = 1.0f
    );
    
    /**
     * Diffuse energy along edges (attention flow)
     */
    void diffuse(
        std::vector<Node>& nodes,
        const std::vector<Edge>& edges,
        Storage* storage
    );
    
    /**
     * Autonomous thinking - keep reasoning without input
     * Uses prediction errors as curiosity signals
     */
    void idle_think(
        std::vector<Node>& nodes,
        const std::vector<Edge>& edges,
        Storage* storage
    );
    
    /**
     * Reinforce pathways with reward signal
     */
    void reinforce(
        std::vector<Node>& nodes,
        const std::vector<NodeID>& path,
        float reward
    );
    
    /**
     * Compute attention distribution (softmax over energy)
     * Attention_i = E_i^γ / Σ E_j^γ
     */
    std::vector<float> compute_attention(const std::vector<Node>& nodes) const;
    
    /**
     * Get total energy in system
     */
    float get_total_energy() const { return total_energy_; }
    
    /**
     * Check if system is still "thinking" (energy above threshold)
     */
    bool is_active() const {
        return total_energy_ > config_.regeneration_threshold;
    }
    
    /**
     * Statistics
     */
    struct Stats {
        float total_energy = 0.0f;
        float max_activation = 0.0f;
        float avg_activation = 0.0f;
        float curiosity_injected = 0.0f;
        int diffusion_steps = 0;
        int idle_steps = 0;
    };
    
    Stats get_stats() const { return stats_; }
    void reset_stats() { stats_ = Stats(); }
    
private:
    Config config_;
    Stats stats_;
    float total_energy_ = 0.0f;
    
    // Helper: Random noise
    float random_noise() const {
        return ((rand() % 1000) / 1000.0f - 0.5f) * config_.noise_floor;
    }
};

} // namespace energy
} // namespace melvin

