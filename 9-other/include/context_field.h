/**
 * @file context_field.h
 * @brief Implements Melvin's contextual activation system.
 *
 * Context = transient activation field over AtomicGraph nodes.
 * Each cycle, activations spread through semantically related edges,
 * decay over time, and bias attention and reasoning subsystems.
 *
 * Inspired by human working memory, predictive coding, and
 * the distributed nature of cortical context.
 * 
 * Biological parallels:
 * - Prefrontal cortex: Working memory (active concept maintenance)
 * - Hippocampus: Episodic replay (memory-driven activation)
 * - Pulvinar thalamus: Attention routing (context biases focus)
 * - Dopamine system: Novelty signals (curiosity modulation)
 */

#pragma once

#include "melvin_types.h"
#include "melvin_graph.h"
#include <unordered_map>
#include <vector>
#include <string>

namespace melvin {

// ============================================================================
// CONTEXT FIELD - Living Activation Energy Over Graph
// ============================================================================

/**
 * Per-node state tracking
 * Represents transient activation level (working memory)
 */
struct NodeState {
    float activation;          // Current activation [0, 1]
    float decay_rate;          // How fast it fades (default: 0.97 per cycle)
    float baseline;            // Resting activation level
    uint64_t last_updated;     // Cycle number
    
    NodeState() 
        : activation(0.0f)
        , decay_rate(0.97f)
        , baseline(0.0f)
        , last_updated(0)
    {}
};

/**
 * Activation source types
 */
enum ActivationSource {
    SENSORY_INPUT,      // Bottom-up from perception
    REASONING_OUTPUT,   // Top-down from thoughts
    MEMORY_RECALL,      // Episodic memory activation
    CURIOSITY_BOOST,    // Novelty-driven
    GOAL_INJECTION      // Explicit goal-setting
};

/**
 * ContextField - Distributed activation field over knowledge graph
 * 
 * This is Melvin's "working memory" - what he's currently thinking about.
 * 
 * Key concepts:
 * - Activation spreads through connected nodes (like neural excitation)
 * - Inactive concepts decay exponentially (forgetting)
 * - Multiple sources inject activation (sensory, reasoning, memory)
 * - The active field biases both attention and reasoning
 * 
 * Why this matters:
 * Instead of rigid attention weights (α=0.45, β=0.35, γ=0.20),
 * the weights become DYNAMIC based on current context!
 * 
 * Example:
 * - If "fire" is highly active → α (saliency) increases (look for bright/hot)
 * - If "searching" is active → β (goal) increases (relevance matters more)
 * - If exploring new area → γ (curiosity) increases (novelty matters more)
 */
class ContextField {
public:
    explicit ContextField(AtomicGraph& graph);
    ~ContextField();
    
    // ========================================================================
    // ACTIVATION INJECTION
    // ========================================================================
    
    /**
     * Inject activation into specific node
     * 
     * @param node_id Target node
     * @param amount Activation amount [0, 1]
     * @param source What caused this activation
     * 
     * Example:
     *   inject(fire_node, 0.8, SENSORY_INPUT) ← Saw fire
     *   inject(search_node, 0.9, GOAL_INJECTION) ← Set goal to search
     */
    void inject(uint64_t node_id, float amount, ActivationSource source);
    
    /**
     * Inject activation into multiple nodes (batch)
     */
    void inject_batch(const std::vector<uint64_t>& nodes, float amount, ActivationSource source);
    
    /**
     * Clear all activations (reset working memory)
     */
    void clear();
    
    // ========================================================================
    // ACTIVATION DYNAMICS
    // ========================================================================
    
    /**
     * Diffuse activation through graph edges
     * Like neural excitation spreading through synapses
     * 
     * @param factor Diffusion strength (default: 0.05)
     * 
     * Algorithm:
     * For each active node:
     *   For each connected neighbor:
     *     neighbor.activation += this.activation × edge.weight × factor
     */
    void diffuse(float factor = 0.05f);
    
    /**
     * Decay all activations (forgetting)
     * Inactive concepts fade from working memory
     * 
     * activation_new = activation_old × decay_rate
     */
    void decay();
    
    /**
     * Normalize activations to [0, 1] range
     * Prevents runaway excitation
     */
    void normalize();
    
    /**
     * Settle activations (run multiple diffuse-decay cycles)
     * Like attractor dynamics in Hopfield networks
     * 
     * @param iterations Number of settle steps (default: 5)
     */
    void settle(int iterations = 5);
    
    // ========================================================================
    // CONTEXT QUERIES
    // ========================================================================
    
    /**
     * Get all active nodes above threshold
     * These are what Melvin is "currently thinking about"
     * 
     * @param threshold Minimum activation to consider active (default: 0.2)
     * @return List of active node IDs
     */
    std::vector<uint64_t> active(float threshold = 0.2f) const;
    
    /**
     * Get activation level for specific node
     * 
     * @param node_id Node to query
     * @return Activation level [0, 1], 0 if not active
     */
    float get_activation(uint64_t node_id) const;
    
    /**
     * Get top-k most active nodes
     * 
     * @param k Number of nodes to return
     * @return List of (node_id, activation) pairs, sorted by activation
     */
    std::vector<std::pair<uint64_t, float>> get_top_active(int k = 10) const;
    
    /**
     * Check if node is active
     */
    bool is_active(uint64_t node_id, float threshold = 0.2f) const;
    
    // ========================================================================
    // DYNAMIC ATTENTION WEIGHTS (Key Innovation!)
    // ========================================================================
    
    /**
     * Compute attention weights based on current context
     * 
     * This is the MAJOR UPGRADE!
     * Instead of rigid α=0.45, β=0.35, γ=0.20,
     * weights adapt based on what Melvin is thinking about.
     * 
     * Examples:
     * - If "exploring" active → increase γ (curiosity matters more)
     * - If "searching" active → increase β (goal matters more)
     * - If "danger" active → increase α (saliency matters more)
     * 
     * @return (alpha, beta, gamma) tuple for current context
     */
    struct AttentionWeights {
        float alpha;    // Saliency weight
        float beta;     // Goal weight
        float gamma;    // Curiosity weight
        std::string reason;  // Why these weights?
    };
    
    AttentionWeights compute_dynamic_weights() const;
    
    /**
     * Get contextual bias for specific visual feature
     * Used to boost attention scores based on active concepts
     * 
     * @param feature_type Feature to check (e.g., "red", "moving", "round")
     * @return Bias multiplier [0.5, 2.0]
     */
    float get_feature_bias(const std::string& feature_type) const;
    
    // ========================================================================
    // CONTEXT SHAPING
    // ========================================================================
    
    /**
     * Set cognitive mode (shapes activation dynamics)
     */
    enum CognitiveMode {
        EXPLORING,      // High curiosity, broad attention
        SEARCHING,      // High goal focus, narrow attention
        MONITORING,     // Balanced, maintenance mode
        LEARNING        // High memory integration
    };
    
    void set_mode(CognitiveMode mode);
    CognitiveMode get_mode() const { return mode_; }
    
    /**
     * Update context based on recent thought
     * Reinforces nodes mentioned in thought
     */
    void update_from_thought(uint64_t subject, uint64_t predicate, uint64_t object);
    
    /**
     * Update context from sensory input
     */
    void update_from_perception(const std::vector<uint64_t>& perceived_nodes);
    
    /**
     * Update context from goal
     */
    void update_from_goal(const std::string& goal_description);
    
    // ========================================================================
    // VISUALIZATION & DEBUG
    // ========================================================================
    
    /**
     * Print current context state (console visualization)
     * Shows top-N active nodes with bar graph
     */
    void visualize(int top_n = 15) const;
    
    /**
     * Get context summary as string
     */
    std::string get_summary() const;
    
    /**
     * Export context for external visualization
     */
    struct ContextSnapshot {
        std::vector<uint64_t> node_ids;
        std::vector<float> activations;
        std::vector<std::string> labels;
        uint64_t cycle_number;
    };
    ContextSnapshot export_snapshot() const;
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct ContextStats {
        size_t total_nodes_tracked;
        size_t currently_active;
        float avg_activation;
        float max_activation;
        uint64_t total_injections;
        uint64_t total_diffusions;
    };
    
    ContextStats get_stats() const;
    void print_stats() const;
    
private:
    // ========================================================================
    // INTERNAL STATE
    // ========================================================================
    
    AtomicGraph& graph_;
    std::unordered_map<uint64_t, NodeState> states_;
    
    CognitiveMode mode_;
    uint64_t cycle_count_;
    
    // Stats tracking
    uint64_t total_injections_;
    uint64_t total_diffusions_;
    
    // ========================================================================
    // HELPER FUNCTIONS
    // ========================================================================
    
    NodeState& get_or_create_state(uint64_t node_id);
    
    // Compute mode-specific parameters
    struct ModeParams {
        float diffusion_factor;
        float decay_rate;
        AttentionWeights attention_weights;
    };
    ModeParams get_mode_params() const;
};

} // namespace melvin

