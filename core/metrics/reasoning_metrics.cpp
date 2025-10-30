/**
 * @file reasoning_metrics.cpp
 * @brief Implementation of reasoning metrics tracker
 */

#include "reasoning_metrics.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace melvin {
namespace metrics {

ReasoningMetricsTracker::ReasoningMetricsTracker() {
    reset();
}

void ReasoningMetricsTracker::update_from_activation(
    const std::vector<int>& active_nodes,
    const std::vector<float>& activations,
    const std::unordered_set<int>& working_memory
) {
    current_metrics_.active_node_count = active_nodes.size();
    
    // Compute core metrics
    current_metrics_.coherence = compute_coherence(active_nodes, activations);
    current_metrics_.novelty = compute_novelty(active_nodes, working_memory);
    current_metrics_.entropy = compute_entropy(activations);
    current_metrics_.energy_variance = compute_energy_variance(activations);
    
    // Detect status flags
    current_metrics_.needs_exploration = (current_metrics_.novelty < 0.3f);
    current_metrics_.needs_focus = (current_metrics_.coherence < 0.4f);
    current_metrics_.backpressure_active = (active_nodes.size() > 10000);
    
    update_history();
}

void ReasoningMetricsTracker::update_from_paths(
    const std::vector<std::vector<int>>& reasoning_paths,
    const std::vector<float>& path_strengths
) {
    current_metrics_.total_paths = reasoning_paths.size();
    
    if (!reasoning_paths.empty()) {
        // Compute average path length
        float total_length = 0.0f;
        for (const auto& path : reasoning_paths) {
            total_length += path.size();
        }
        current_metrics_.avg_path_length = total_length / reasoning_paths.size();
        
        // Compute confidence from path strengths
        if (!path_strengths.empty()) {
            float max_strength = *std::max_element(path_strengths.begin(), path_strengths.end());
            float avg_strength = std::accumulate(path_strengths.begin(), path_strengths.end(), 0.0f) 
                               / path_strengths.size();
            
            // Confidence = blend of max and average path strength
            current_metrics_.confidence = 0.7f * max_strength + 0.3f * avg_strength;
        }
    }
}

void ReasoningMetricsTracker::update_semantic_alignment(
    const std::vector<float>& query_embedding,
    const std::vector<float>& answer_embedding
) {
    current_metrics_.semantic_alignment = cosine_similarity(query_embedding, answer_embedding);
    
    // Boost confidence if semantic alignment is high
    current_metrics_.confidence = 0.7f * current_metrics_.confidence +
                                 0.3f * current_metrics_.semantic_alignment;
}

float ReasoningMetricsTracker::get_avg_coherence(size_t window) const {
    if (coherence_history_.empty()) return current_metrics_.coherence;
    
    size_t n = std::min(window, coherence_history_.size());
    float sum = 0.0f;
    for (size_t i = 0; i < n; i++) {
        sum += coherence_history_[coherence_history_.size() - 1 - i];
    }
    return sum / n;
}

float ReasoningMetricsTracker::get_avg_confidence(size_t window) const {
    if (confidence_history_.empty()) return current_metrics_.confidence;
    
    size_t n = std::min(window, confidence_history_.size());
    float sum = 0.0f;
    for (size_t i = 0; i < n; i++) {
        sum += confidence_history_[confidence_history_.size() - 1 - i];
    }
    return sum / n;
}

bool ReasoningMetricsTracker::is_improving() const {
    if (confidence_history_.size() < 5) return false;
    
    // Compare recent vs older confidence
    float recent = get_avg_confidence(3);
    float older = 0.0f;
    for (size_t i = 3; i < std::min(size_t(6), confidence_history_.size()); i++) {
        older += confidence_history_[confidence_history_.size() - 1 - i];
    }
    older /= std::min(size_t(3), confidence_history_.size() - 3);
    
    return recent > older + 0.05f;  // Improving if >5% increase
}

bool ReasoningMetricsTracker::should_explore() const {
    return current_metrics_.confidence < 0.4f && 
           current_metrics_.novelty > 0.6f;
}

bool ReasoningMetricsTracker::should_exploit() const {
    return current_metrics_.confidence > 0.8f &&
           current_metrics_.coherence > 0.6f;
}

bool ReasoningMetricsTracker::should_focus() const {
    return current_metrics_.coherence < 0.4f ||
           current_metrics_.backpressure_active;
}

bool ReasoningMetricsTracker::should_deepen() const {
    return current_metrics_.avg_path_length < 3.0f &&
           current_metrics_.confidence < 0.6f;
}

void ReasoningMetricsTracker::reset() {
    current_metrics_ = ReasoningMetrics();
    coherence_history_.clear();
    confidence_history_.clear();
    novelty_history_.clear();
}

float ReasoningMetricsTracker::compute_coherence(
    const std::vector<int>& active_nodes,
    const std::vector<float>& activations
) {
    if (activations.empty()) return 0.0f;
    
    // Use helper function
    return compute_activation_coherence(activations);
}

float ReasoningMetricsTracker::compute_novelty(
    const std::vector<int>& active_nodes,
    const std::unordered_set<int>& working_memory
) {
    // Use helper function
    return compute_novelty_score(active_nodes, working_memory);
}

float ReasoningMetricsTracker::compute_entropy(
    const std::vector<float>& activations
) {
    if (activations.empty()) return 0.0f;
    
    // Normalize to probabilities
    float sum = 0.0f;
    for (float act : activations) {
        sum += act;
    }
    
    if (sum < EPSILON) return 0.0f;
    
    // Compute Shannon entropy
    float entropy = 0.0f;
    for (float act : activations) {
        if (act > EPSILON) {
            float p = act / sum;
            entropy -= p * std::log2(p);
        }
    }
    
    return entropy;
}

float ReasoningMetricsTracker::compute_energy_variance(
    const std::vector<float>& activations
) {
    if (activations.empty()) return 0.0f;
    
    // Compute mean
    float mean = std::accumulate(activations.begin(), activations.end(), 0.0f) 
               / activations.size();
    
    // Compute variance
    float variance = 0.0f;
    for (float act : activations) {
        float diff = act - mean;
        variance += diff * diff;
    }
    variance /= activations.size();
    
    return std::sqrt(variance);
}

float ReasoningMetricsTracker::cosine_similarity(
    const std::vector<float>& a,
    const std::vector<float>& b
) const {
    if (a.size() != b.size() || a.empty()) return 0.0f;
    
    float dot = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;
    
    for (size_t i = 0; i < a.size(); i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    float denom = std::sqrt(norm_a) * std::sqrt(norm_b);
    return (denom > EPSILON) ? (dot / denom) : 0.0f;
}

void ReasoningMetricsTracker::update_history() {
    coherence_history_.push_back(current_metrics_.coherence);
    confidence_history_.push_back(current_metrics_.confidence);
    novelty_history_.push_back(current_metrics_.novelty);
    
    if (coherence_history_.size() > MAX_HISTORY) {
        prune_history();
    }
}

void ReasoningMetricsTracker::prune_history() {
    while (coherence_history_.size() > MAX_HISTORY) {
        coherence_history_.pop_front();
    }
    while (confidence_history_.size() > MAX_HISTORY) {
        confidence_history_.pop_front();
    }
    while (novelty_history_.size() > MAX_HISTORY) {
        novelty_history_.pop_front();
    }
}

} // namespace metrics
} // namespace melvin

