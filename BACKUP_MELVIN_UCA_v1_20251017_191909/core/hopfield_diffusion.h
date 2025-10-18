#pragma once

#include "types.h"
#include "storage.h"
#include <vector>
#include <cmath>
#include <random>

namespace melvin {
namespace hopfield {

/**
 * 🧠 Hopfield-Diffusion Reasoning System
 * 
 * Combines:
 * - Hopfield-style stable memory (vector embeddings + energy function)
 * - Diffusion-based reasoning (attention-weighted propagation)
 * - Blended Hebbian + gradient learning
 * 
 * [Hopfield-Diffusion Upgrade]
 */
class HopfieldDiffusion {
public:
    struct Config {
        int embedding_dim = 32;           // Dimension of vector embeddings
        float beta = 5.0f;                // Attention sharpness (temperature^-1)
        float eta_local = 1e-3f;          // Local Hebbian learning rate
        float eta_global = 1e-4f;         // Global energy-based learning rate
        float activation_leak = 0.98f;    // Mild activation decay per step
        float energy_epsilon = 1e-4f;     // Convergence threshold (ΔE)
        int max_diffusion_steps = 20;     // Max iterations
        bool normalize_embeddings = true; // Keep embeddings unit length
        bool verbose = false;             // Print energy logs
        
        Config() {}
    };
    
    explicit HopfieldDiffusion(const Config& config = Config());
    ~HopfieldDiffusion();
    
    // ========================================================================
    // EMBEDDING OPERATIONS
    // ========================================================================
    
    // Initialize embedding with small random values
    void initialize_embedding(Node& node);
    
    // Normalize embedding to unit length
    void normalize_embedding(Node& node);
    
    // Compute cosine similarity between two nodes
    float cosine_similarity(const Node& a, const Node& b) const;
    
    // ========================================================================
    // ATTENTION-BASED DIFFUSION
    // ========================================================================
    
    // Run attention-weighted diffusion step
    // Returns: change in total energy
    float diffusion_step(
        std::vector<Node>& nodes,
        Storage* storage
    );
    
    // Run full diffusion until convergence or max steps
    // Returns: final energy and number of steps taken
    struct DiffusionResult {
        float final_energy = 0.0f;
        int steps_taken = 0;
        bool converged = false;
        std::vector<float> energy_trajectory;  // Energy at each step
    };
    
    DiffusionResult run_diffusion(
        std::vector<Node>& nodes,
        Storage* storage
    );
    
    // ========================================================================
    // HOPFIELD ENERGY FUNCTION
    // ========================================================================
    
    // Compute total system energy (Hopfield-style)
    // E = -Σ β * cos_sim(i,j) * a[i] * a[j]
    float compute_energy(const std::vector<Node>& nodes);
    
    // ========================================================================
    // LEARNING RULES
    // ========================================================================
    
    // Update edge weights using blended learning
    // Combines local Hebbian + global gradient terms
    void update_weights(
        std::vector<Node>& nodes,
        std::vector<Edge>& edges,
        Storage* storage
    );
    
    // Apply Hebbian learning to edge weights
    void hebbian_update(Edge& edge, const Node& from, const Node& to);
    
    // Apply gradient-based update using similarity
    void gradient_update(Edge& edge, const Node& from, const Node& to);
    
    // ========================================================================
    // NORMALIZATION & STABILITY
    // ========================================================================
    
    // Apply activation leak and embedding normalization
    void normalize_pass(std::vector<Node>& nodes);
    
    // ========================================================================
    // CONFIGURATION & STATS
    // ========================================================================
    
    void set_config(const Config& config);
    const Config& get_config() const;
    
    struct Stats {
        uint64_t diffusion_runs = 0;
        uint64_t total_steps = 0;
        uint64_t convergences = 0;
        float avg_final_energy = 0.0f;
        float avg_steps_per_run = 0.0f;
        float min_energy_seen = 0.0f;
        float max_energy_seen = 0.0f;
    };
    
    Stats get_stats() const;
    void reset_stats();
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// STANDALONE HELPER FUNCTIONS
// ============================================================================

// Initialize embeddings for all nodes in storage
void initialize_all_embeddings(Storage* storage, int embedding_dim = 32);

// Ensure all nodes have embeddings (lazy initialization)
void ensure_embeddings(std::vector<Node>& nodes, int embedding_dim = 32);

} // namespace hopfield
} // namespace melvin

