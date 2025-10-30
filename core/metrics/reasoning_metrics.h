/**
 * @file reasoning_metrics.h
 * @brief Real-time KPI tracking for adaptive reasoning
 * 
 * Monitors:
 * - Coherence: How focused is the activation?
 * - Novelty: How much new information?
 * - Confidence: How reliable is the reasoning?
 * - Entropy: Information content
 * - Energy variance: Spread of activation
 */

#ifndef MELVIN_REASONING_METRICS_H
#define MELVIN_REASONING_METRICS_H

#include <vector>
#include <deque>
#include <unordered_set>
#include <cmath>
#include <algorithm>

namespace melvin {
namespace metrics {

/**
 * @brief Real-time reasoning quality metrics
 */
struct ReasoningMetrics {
    // Core metrics
    float coherence;        // intra_weight / (inter_weight + ε)
    float novelty;          // 1 - overlap(active, memory)
    float confidence;       // f(path_strength, semantic_alignment)
    float entropy;          // information content of activation field
    float energy_variance;  // spread of activation
    
    // Derived metrics
    float avg_path_length;  // average reasoning chain length
    float semantic_alignment; // query-to-answer similarity
    int active_node_count;
    int total_paths;
    
    // Status flags
    bool converged;
    bool backpressure_active;
    bool needs_exploration;
    bool needs_focus;
    
    ReasoningMetrics() :
        coherence(0.0f),
        novelty(1.0f),
        confidence(0.0f),
        entropy(0.0f),
        energy_variance(0.0f),
        avg_path_length(0.0f),
        semantic_alignment(0.0f),
        active_node_count(0),
        total_paths(0),
        converged(false),
        backpressure_active(false),
        needs_exploration(false),
        needs_focus(false)
    {}
};

/**
 * @brief Tracks and computes reasoning metrics over time
 */
class ReasoningMetricsTracker {
public:
    ReasoningMetricsTracker();
    ~ReasoningMetricsTracker() = default;
    
    // Update from activation field
    void update_from_activation(
        const std::vector<int>& active_nodes,
        const std::vector<float>& activations,
        const std::unordered_set<int>& working_memory
    );
    
    // Update from graph structure
    void update_from_paths(
        const std::vector<std::vector<int>>& reasoning_paths,
        const std::vector<float>& path_strengths
    );
    
    // Update semantic metrics
    void update_semantic_alignment(
        const std::vector<float>& query_embedding,
        const std::vector<float>& answer_embedding
    );
    
    // Get current metrics
    const ReasoningMetrics& current() const { return current_metrics_; }
    ReasoningMetrics& current() { return current_metrics_; }
    
    // History for temporal analysis
    float get_avg_coherence(size_t window = 10) const;
    float get_avg_confidence(size_t window = 10) const;
    bool is_improving() const;
    
    // Mode detection
    bool should_explore() const;
    bool should_exploit() const;
    bool should_focus() const;
    bool should_deepen() const;
    
    void reset();
    
private:
    ReasoningMetrics current_metrics_;
    
    // Rolling history
    std::deque<float> coherence_history_;
    std::deque<float> confidence_history_;
    std::deque<float> novelty_history_;
    
    static constexpr size_t MAX_HISTORY = 50;
    static constexpr float EPSILON = 1e-6f;
    
    // Computation helpers
    float compute_coherence(
        const std::vector<int>& active_nodes,
        const std::vector<float>& activations
    );
    
    float compute_novelty(
        const std::vector<int>& active_nodes,
        const std::unordered_set<int>& working_memory
    );
    
    float compute_entropy(
        const std::vector<float>& activations
    );
    
    float compute_energy_variance(
        const std::vector<float>& activations
    );
    
    float cosine_similarity(
        const std::vector<float>& a,
        const std::vector<float>& b
    ) const;
    
    void update_history();
    void prune_history();
};

/**
 * @brief Compute coherence (how focused is activation?)
 * 
 * Coherence = (sum of top 10% activations) / (sum of all activations)
 * High coherence = focused reasoning
 * Low coherence = scattered, unfocused
 */
inline float compute_activation_coherence(const std::vector<float>& activations) {
    if (activations.empty()) return 0.0f;
    
    std::vector<float> sorted = activations;
    std::sort(sorted.begin(), sorted.end(), std::greater<float>());
    
    size_t top_n = std::max(size_t(1), sorted.size() / 10);
    
    float top_sum = 0.0f;
    for (size_t i = 0; i < top_n; i++) {
        top_sum += sorted[i];
    }
    
    float total_sum = 0.0f;
    for (float act : activations) {
        total_sum += act;
    }
    
    return (total_sum > 0) ? (top_sum / total_sum) : 0.0f;
}

/**
 * @brief Compute novelty (how much new information?)
 * 
 * Novelty = 1 - (overlap between active and memory / active)
 * High novelty = exploring new territory
 * Low novelty = revisiting known concepts
 */
inline float compute_novelty_score(
    const std::vector<int>& active_nodes,
    const std::unordered_set<int>& memory_nodes
) {
    if (active_nodes.empty()) return 1.0f;
    
    int overlap = 0;
    for (int node : active_nodes) {
        if (memory_nodes.count(node)) {
            overlap++;
        }
    }
    
    return 1.0f - (static_cast<float>(overlap) / active_nodes.size());
}

} // namespace metrics
} // namespace melvin

#endif // MELVIN_REASONING_METRICS_H

