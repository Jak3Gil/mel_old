/**
 * @file context_field.cpp
 * @brief Implements Melvin's contextual activation system.
 *
 * Context = transient activation field over AtomicGraph nodes.
 * Each cycle, activations spread through semantically related edges,
 * decay over time, and bias attention and reasoning subsystems.
 *
 * Inspired by human working memory, predictive coding, and
 * the distributed nature of cortical context.
 */

#include "context_field.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace melvin {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

ContextField::ContextField(AtomicGraph& graph)
    : graph_(graph)
    , mode_(CognitiveMode::EXPLORING)
    , cycle_count_(0)
    , total_injections_(0)
    , total_diffusions_(0)
{
    std::cout << "🧠 ContextField initialized (working memory active)" << std::endl;
}

ContextField::~ContextField() {}

// ============================================================================
// ACTIVATION INJECTION
// ============================================================================

void ContextField::inject(uint64_t node_id, float amount, ActivationSource source) {
    if (!graph_.has_node(node_id)) {
        return;
    }
    
    auto& state = get_or_create_state(node_id);
    
    // Source-specific weighting
    float source_weight = 1.0f;
    switch (source) {
        case SENSORY_INPUT:   source_weight = 0.8f; break;  // Strong but transient
        case REASONING_OUTPUT: source_weight = 1.0f; break;  // Strongest (from thoughts)
        case MEMORY_RECALL:   source_weight = 0.6f; break;  // Moderate
        case CURIOSITY_BOOST: source_weight = 0.7f; break;  // Notable
        case GOAL_INJECTION:  source_weight = 1.2f; break;  // Very strong (explicit goal)
    }
    
    // Inject activation (saturating add)
    state.activation = std::min(1.0f, state.activation + amount * source_weight);
    state.last_updated = cycle_count_;
    
    total_injections_++;
}

void ContextField::inject_batch(const std::vector<uint64_t>& nodes, float amount, ActivationSource source) {
    for (uint64_t node_id : nodes) {
        inject(node_id, amount, source);
    }
}

void ContextField::clear() {
    states_.clear();
}

// ============================================================================
// ACTIVATION DYNAMICS
// ============================================================================

void ContextField::diffuse(float factor) {
    /**
     * Activation spreads through graph edges
     * Like neural excitation flowing through synapses
     * 
     * Key insight: Connected concepts co-activate
     * If "fire" is active, "heat" and "danger" also activate
     */
    
    std::unordered_map<uint64_t, float> diffusion_buffer;
    
    // For each active node, spread to neighbors
    for (const auto& [node_id, state] : states_) {
        if (state.activation < 0.1f) continue;  // Too weak to spread
        
        // Get neighbors (respecting semantic relations)
        auto neighbors = graph_.neighbors(node_id, {
            Rel::CO_OCCURS_WITH,
            Rel::SIMILAR_FEATURES,
            Rel::CAUSES,
            Rel::EXPECTS,
            Rel::USED_FOR
        }, 20);
        
        // Spread activation to each neighbor
        for (uint64_t neighbor_id : neighbors) {
            float edge_weight = graph_.get_edge_weight(node_id, neighbor_id, Rel::CO_OCCURS_WITH);
            if (edge_weight == 0) {
                edge_weight = 0.5f;  // Default if relation exists
            }
            
            float spread = state.activation * edge_weight * factor;
            diffusion_buffer[neighbor_id] += spread;
        }
    }
    
    // Apply diffused activations
    for (const auto& [node_id, diff] : diffusion_buffer) {
        auto& state = get_or_create_state(node_id);
        state.activation = std::min(1.0f, state.activation + diff);
        state.last_updated = cycle_count_;
    }
    
    total_diffusions_++;
}

void ContextField::decay() {
    /**
     * Exponential decay of activations
     * Like synaptic activity fading over time
     * 
     * This implements forgetting - concepts not reinforced fade away
     */
    
    std::vector<uint64_t> to_remove;
    
    for (auto& [node_id, state] : states_) {
        // Apply exponential decay
        state.activation *= state.decay_rate;
        
        // Move toward baseline
        state.activation = state.activation * 0.9f + state.baseline * 0.1f;
        
        // Remove if too weak
        if (state.activation < 0.01f) {
            to_remove.push_back(node_id);
        }
    }
    
    // Clean up dead activations
    for (uint64_t id : to_remove) {
        states_.erase(id);
    }
}

void ContextField::normalize() {
    /**
     * Normalize activations to prevent runaway excitation
     * Like homeostatic regulation in the brain
     */
    
    if (states_.empty()) return;
    
    // Find max activation
    float max_act = 0;
    for (const auto& [_, state] : states_) {
        max_act = std::max(max_act, state.activation);
    }
    
    // Normalize if too high
    if (max_act > 1.5f) {
        float scale = 1.0f / max_act;
        for (auto& [_, state] : states_) {
            state.activation *= scale;
        }
    }
}

void ContextField::settle(int iterations) {
    /**
     * Let activations settle into stable state
     * Like Hopfield network convergence
     * 
     * Useful after major context shift (new goal, surprising event)
     */
    
    for (int i = 0; i < iterations; ++i) {
        diffuse(0.05f);
        decay();
        normalize();
    }
}

// ========================================================================
// CONTEXT QUERIES
// ========================================================================

std::vector<uint64_t> ContextField::active(float threshold) const {
    std::vector<uint64_t> result;
    
    for (const auto& [node_id, state] : states_) {
        if (state.activation >= threshold) {
            result.push_back(node_id);
        }
    }
    
    return result;
}

float ContextField::get_activation(uint64_t node_id) const {
    auto it = states_.find(node_id);
    if (it == states_.end()) {
        return 0.0f;
    }
    return it->second.activation;
}

std::vector<std::pair<uint64_t, float>> ContextField::get_top_active(int k) const {
    std::vector<std::pair<uint64_t, float>> all_active;
    
    for (const auto& [node_id, state] : states_) {
        if (state.activation > 0.01f) {
            all_active.push_back({node_id, state.activation});
        }
    }
    
    // Sort by activation (descending)
    std::sort(all_active.begin(), all_active.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Return top k
    if (all_active.size() > (size_t)k) {
        all_active.resize(k);
    }
    
    return all_active;
}

bool ContextField::is_active(uint64_t node_id, float threshold) const {
    return get_activation(node_id) >= threshold;
}

// ============================================================================
// DYNAMIC ATTENTION WEIGHTS (KEY INNOVATION!)
// ============================================================================

ContextField::AttentionWeights ContextField::compute_dynamic_weights() const {
    /**
     * THIS IS THE MAGIC!
     * 
     * Instead of rigid α=0.45, β=0.35, γ=0.20,
     * weights change based on what Melvin is thinking about.
     * 
     * The CONTEXT shapes how attention is distributed!
     */
    
    AttentionWeights weights;
    
    // Start with mode-based defaults
    auto params = get_mode_params();
    weights = params.attention_weights;
    
    // Modulate based on active concepts
    auto top_concepts = get_top_active(5);
    
    for (const auto& [node_id, activation] : top_concepts) {
        const Node* node = graph_.get_node(node_id);
        if (!node) continue;
        
        // Check concept type (would need labels in real system)
        // For now, use activation strength to modulate
        
        if (activation > 0.7f) {
            // High activation = strong context
            // Increase goal weight (focused mode)
            weights.beta += 0.1f * activation;
            weights.alpha -= 0.05f * activation;  // Decrease saliency
        }
    }
    
    // Normalize weights to sum to 1.0
    float total = weights.alpha + weights.beta + weights.gamma;
    if (total > 0) {
        weights.alpha /= total;
        weights.beta /= total;
        weights.gamma /= total;
    }
    
    // Explain the weighting
    if (weights.alpha > 0.5f) {
        weights.reason = "Saliency-driven (bottom-up)";
    } else if (weights.beta > 0.4f) {
        weights.reason = "Goal-driven (top-down)";
    } else if (weights.gamma > 0.3f) {
        weights.reason = "Curiosity-driven (exploring)";
    } else {
        weights.reason = "Balanced attention";
    }
    
    return weights;
}

float ContextField::get_feature_bias(const std::string& feature_type) const {
    /**
     * Contextual feature biasing
     * 
     * If Melvin is thinking about "fire":
     * - "red" features get boosted
     * - "blue" features get suppressed
     * 
     * Returns multiplier for feature scores
     */
    
    // Default: no bias
    float bias = 1.0f;
    
    // Check if any active concepts relate to this feature
    // (Simplified - would use proper feature-concept mapping)
    auto top = get_top_active(10);
    for (const auto& [node_id, activation] : top) {
        // Strong activation slightly boosts related features
        bias += activation * 0.1f;
    }
    
    return std::min(2.0f, std::max(0.5f, bias));
}

// ============================================================================
// CONTEXT SHAPING
// ============================================================================

void ContextField::set_mode(CognitiveMode mode) {
    mode_ = mode;
    
    // Adjust decay rates based on mode
    for (auto& [_, state] : states_) {
        switch (mode) {
            case EXPLORING:
                state.decay_rate = 0.95f;  // Fast decay (don't dwell)
                break;
            case SEARCHING:
                state.decay_rate = 0.98f;  // Slow decay (maintain focus)
                break;
            case MONITORING:
                state.decay_rate = 0.97f;  // Default
                break;
            case LEARNING:
                state.decay_rate = 0.99f;  // Very slow (consolidate)
                break;
        }
    }
}

ContextField::ModeParams ContextField::get_mode_params() const {
    ModeParams params;
    
    switch (mode_) {
        case EXPLORING:
            // High curiosity, broad attention
            params.diffusion_factor = 0.08f;  // Spread widely
            params.decay_rate = 0.95f;         // Decay fast
            params.attention_weights = {0.35f, 0.25f, 0.40f, "Exploring (curiosity-driven)"};
            break;
            
        case SEARCHING:
            // High goal focus, narrow attention
            params.diffusion_factor = 0.03f;  // Spread narrowly
            params.decay_rate = 0.98f;         // Decay slow
            params.attention_weights = {0.30f, 0.55f, 0.15f, "Searching (goal-driven)"};
            break;
            
        case MONITORING:
            // Balanced, default mode
            params.diffusion_factor = 0.05f;
            params.decay_rate = 0.97f;
            params.attention_weights = {0.45f, 0.35f, 0.20f, "Monitoring (balanced)"};
            break;
            
        case LEARNING:
            // High memory integration
            params.diffusion_factor = 0.06f;  // Medium spread
            params.decay_rate = 0.99f;         // Very slow decay
            params.attention_weights = {0.40f, 0.30f, 0.30f, "Learning (memory-focused)"};
            break;
    }
    
    return params;
}

void ContextField::update_from_thought(uint64_t subject, uint64_t predicate, uint64_t object) {
    /**
     * Reinforce context based on generated thought
     * 
     * When Melvin thinks "fire causes heat":
     * - "fire" activation increases
     * - "heat" activation increases
     * - "causes" relation gets activated
     * 
     * This creates a self-reinforcing loop:
     * Thought → Context → Next attention → Next thought
     */
    
    inject(subject, 0.6f, REASONING_OUTPUT);
    inject(predicate, 0.4f, REASONING_OUTPUT);
    inject(object, 0.6f, REASONING_OUTPUT);
    
    // Spread a bit
    diffuse(0.03f);
}

void ContextField::update_from_perception(const std::vector<uint64_t>& perceived_nodes) {
    /**
     * Update context from what was just perceived
     * Bottom-up activation from sensory input
     */
    
    inject_batch(perceived_nodes, 0.5f, SENSORY_INPUT);
}

void ContextField::update_from_goal(const std::string& goal_description) {
    /**
     * Set explicit goal - strong context injection
     * Top-down activation from prefrontal cortex
     */
    
    uint64_t goal_node = graph_.get_or_create_concept(goal_description);
    inject(goal_node, 1.0f, GOAL_INJECTION);
    
    // Spread to related concepts
    diffuse(0.08f);
}

// ============================================================================
// VISUALIZATION
// ============================================================================

void ContextField::visualize(int top_n) const {
    auto top = get_top_active(top_n);
    
    if (top.empty()) {
        std::cout << "  (No active context)\n";
        return;
    }
    
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🧠 CONTEXT FIELD - Active Concepts (Working Memory)       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "  Cycle: " << cycle_count_ << " | Mode: ";
    switch (mode_) {
        case EXPLORING: std::cout << "EXPLORING"; break;
        case SEARCHING: std::cout << "SEARCHING"; break;
        case MONITORING: std::cout << "MONITORING"; break;
        case LEARNING: std::cout << "LEARNING"; break;
    }
    std::cout << "\n\n";
    
    std::cout << "  Node ID          | Activation | Bar\n";
    std::cout << "  ───────────────────────────────────────────────\n";
    
    for (const auto& [node_id, activation] : top) {
        int bar_len = std::max(0, std::min(40, (int)(activation * 40)));
        std::string bar(bar_len, '#');  // Use ASCII character
        
        std::cout << "  " << std::setw(16) << node_id 
                  << " | " << std::fixed << std::setprecision(2) << std::setw(4) << activation
                  << "       | " << bar << "\n";
    }
    
    std::cout << "\n";
}

std::string ContextField::get_summary() const {
    auto weights = compute_dynamic_weights();
    
    std::ostringstream oss;
    oss << "Context: " << states_.size() << " active nodes | ";
    oss << "Weights: α=" << std::fixed << std::setprecision(2) << weights.alpha
        << " β=" << weights.beta
        << " γ=" << weights.gamma
        << " (" << weights.reason << ")";
    
    return oss.str();
}

ContextField::ContextSnapshot ContextField::export_snapshot() const {
    ContextSnapshot snap;
    snap.cycle_number = cycle_count_;
    
    auto top = get_top_active(50);
    for (const auto& [node_id, activation] : top) {
        snap.node_ids.push_back(node_id);
        snap.activations.push_back(activation);
        
        const Node* node = graph_.get_node(node_id);
        snap.labels.push_back(node ? "node_" + std::to_string(node_id) : "unknown");
    }
    
    return snap;
}

// ============================================================================
// STATISTICS
// ============================================================================

ContextField::ContextStats ContextField::get_stats() const {
    ContextStats stats;
    stats.total_nodes_tracked = states_.size();
    stats.currently_active = active(0.2f).size();
    stats.total_injections = total_injections_;
    stats.total_diffusions = total_diffusions_;
    
    float sum = 0, max_act = 0;
    for (const auto& [_, state] : states_) {
        sum += state.activation;
        max_act = std::max(max_act, state.activation);
    }
    
    stats.avg_activation = states_.empty() ? 0 : sum / states_.size();
    stats.max_activation = max_act;
    
    return stats;
}

void ContextField::print_stats() const {
    auto stats = get_stats();
    auto weights = compute_dynamic_weights();
    
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ContextField Statistics                                 ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    std::cout << "  Total nodes tracked: " << stats.total_nodes_tracked << "\n";
    std::cout << "  Currently active:    " << stats.currently_active << "\n";
    std::cout << "  Avg activation:      " << std::fixed << std::setprecision(3) 
              << stats.avg_activation << "\n";
    std::cout << "  Max activation:      " << stats.max_activation << "\n";
    std::cout << "  Total injections:    " << stats.total_injections << "\n";
    std::cout << "  Total diffusions:    " << stats.total_diffusions << "\n";
    std::cout << "\n";
    std::cout << "  Current attention weights (dynamic!):\n";
    std::cout << "    α (saliency):  " << weights.alpha << "\n";
    std::cout << "    β (goal):      " << weights.beta << "\n";
    std::cout << "    γ (curiosity): " << weights.gamma << "\n";
    std::cout << "    Reason: " << weights.reason << "\n";
    std::cout << std::endl;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

NodeState& ContextField::get_or_create_state(uint64_t node_id) {
    auto it = states_.find(node_id);
    if (it == states_.end()) {
        states_[node_id] = NodeState();
        it = states_.find(node_id);
    }
    return it->second;
}

} // namespace melvin

