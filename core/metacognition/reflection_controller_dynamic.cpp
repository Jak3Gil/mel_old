/**
 * @file reflection_controller_dynamic.cpp
 * @brief Implementation of reflection controller
 */

#include "reflection_controller_dynamic.h"
#include <numeric>
#include <cmath>

namespace melvin {
namespace metacognition {

ReflectionController::ReflectionController() :
    genome_(nullptr),
    current_mode_(ReasoningMode::EXPLORATORY),
    cycles_since_last_switch_(0),
    exploratory_count_(1),
    exploitative_count_(0),
    focused_count_(0),
    deep_count_(0),
    backpressure_count_(0),
    total_switches_(0)
{}

void ReflectionController::observe(const melvin::metrics::ReasoningMetrics& metrics) {
    current_metrics_ = metrics;
    
    // Update observation windows
    confidence_window_.push_back(metrics.confidence);
    coherence_window_.push_back(metrics.coherence);
    active_nodes_window_.push_back(metrics.active_node_count);
    
    // Prune windows
    if (confidence_window_.size() > WINDOW_SIZE) {
        confidence_window_.pop_front();
    }
    if (coherence_window_.size() > WINDOW_SIZE) {
        coherence_window_.pop_front();
    }
    if (active_nodes_window_.size() > WINDOW_SIZE) {
        active_nodes_window_.pop_front();
    }
    
    cycles_since_last_switch_++;
}

bool ReflectionController::should_switch_mode() const {
    // Need cooldown between switches
    if (cycles_since_last_switch_ < MIN_CYCLES_BETWEEN_SWITCHES) {
        return false;
    }
    
    // Immediate switch conditions
    if (current_metrics_.backpressure_active) {
        return true;
    }
    
    // Check if current mode is failing
    switch (current_mode_) {
        case ReasoningMode::EXPLORATORY:
            // Switch if confidence improving or became high
            return is_confidence_improving() || is_confidence_high();
            
        case ReasoningMode::EXPLOITATIVE:
            // Switch if confidence dropped
            return is_confidence_low();
            
        case ReasoningMode::FOCUSED:
            // Switch if coherence improved
            return is_coherence_high() || is_coherence_improving();
            
        case ReasoningMode::DEEP:
            // Switch if we found good paths (confidence improved)
            return is_confidence_improving();
            
        case ReasoningMode::BACKPRESSURE:
            // Switch when backpressure resolved
            return !is_backpressure_active();
            
        default:
            return false;
    }
}

bool ReflectionController::reflect_and_adapt() {
    if (!should_switch_mode()) {
        return false;
    }
    
    ReasoningMode old_mode = current_mode_;
    ReasoningMode new_mode = decide_mode();
    
    if (new_mode == old_mode) {
        return false;
    }
    
    // Switch mode
    current_mode_ = new_mode;
    cycles_since_last_switch_ = 0;
    total_switches_++;
    
    // Apply mode-specific adaptations
    switch (new_mode) {
        case ReasoningMode::EXPLORATORY:
            apply_exploratory_mode();
            exploratory_count_++;
            log_switch(old_mode, new_mode, "Low confidence - exploring");
            break;
            
        case ReasoningMode::EXPLOITATIVE:
            apply_exploitative_mode();
            exploitative_count_++;
            log_switch(old_mode, new_mode, "High confidence - exploiting");
            break;
            
        case ReasoningMode::FOCUSED:
            apply_focused_mode();
            focused_count_++;
            log_switch(old_mode, new_mode, "Low coherence - focusing");
            break;
            
        case ReasoningMode::DEEP:
            apply_deep_mode();
            deep_count_++;
            log_switch(old_mode, new_mode, "Shallow paths - deepening");
            break;
            
        case ReasoningMode::BACKPRESSURE:
            apply_backpressure_mode();
            backpressure_count_++;
            log_switch(old_mode, new_mode, "Too many nodes - throttling");
            break;
    }
    
    return true;
}

ReflectionController::ModeStats ReflectionController::get_stats() const {
    ModeStats stats;
    stats.exploratory_count = exploratory_count_;
    stats.exploitative_count = exploitative_count_;
    stats.focused_count = focused_count_;
    stats.deep_count = deep_count_;
    stats.backpressure_count = backpressure_count_;
    stats.total_switches = total_switches_;
    return stats;
}

void ReflectionController::reset() {
    current_mode_ = ReasoningMode::EXPLORATORY;
    cycles_since_last_switch_ = 0;
    confidence_window_.clear();
    coherence_window_.clear();
    active_nodes_window_.clear();
    reflection_history_.clear();
}

ReasoningMode ReflectionController::decide_mode() {
    // Priority order
    
    // 1. Backpressure (highest priority)
    if (is_backpressure_active()) {
        return ReasoningMode::BACKPRESSURE;
    }
    
    // 2. Focus if scattered
    if (is_coherence_low() && !is_coherence_improving()) {
        return ReasoningMode::FOCUSED;
    }
    
    // 3. Deep search if shallow and uncertain
    if (is_confidence_low() && current_metrics_.avg_path_length < 3.0f) {
        return ReasoningMode::DEEP;
    }
    
    // 4. Exploit if confident and coherent
    if (is_confidence_high() && is_coherence_high()) {
        return ReasoningMode::EXPLOITATIVE;
    }
    
    // 5. Explore if uncertain
    if (is_confidence_low()) {
        return ReasoningMode::EXPLORATORY;
    }
    
    // 6. Default: stay in current mode
    return current_mode_;
}

bool ReflectionController::is_confidence_low() const {
    float avg = get_avg_confidence();
    if (genome_) {
        return avg < genome_->reasoning_params().exploratory_threshold;
    }
    return avg < 0.4f;
}

bool ReflectionController::is_confidence_high() const {
    float avg = get_avg_confidence();
    if (genome_) {
        return avg > genome_->reasoning_params().exploitative_threshold;
    }
    return avg > 0.8f;
}

bool ReflectionController::is_coherence_low() const {
    float avg = get_avg_coherence();
    return avg < 0.4f;
}

bool ReflectionController::is_coherence_high() const {
    float avg = get_avg_coherence();
    return avg > 0.7f;
}

bool ReflectionController::is_backpressure_active() const {
    return current_metrics_.backpressure_active ||
           get_avg_active_nodes() > 10000.0f;
}

bool ReflectionController::is_confidence_improving() const {
    if (confidence_window_.size() < 3) return false;
    
    // Compare recent vs older
    float recent = 0.0f;
    float older = 0.0f;
    
    size_t mid = confidence_window_.size() / 2;
    for (size_t i = mid; i < confidence_window_.size(); i++) {
        recent += confidence_window_[i];
    }
    for (size_t i = 0; i < mid; i++) {
        older += confidence_window_[i];
    }
    
    recent /= (confidence_window_.size() - mid);
    older /= mid;
    
    return recent > older + 0.05f;  // 5% improvement
}

bool ReflectionController::is_coherence_improving() const {
    if (coherence_window_.size() < 3) return false;
    
    float recent = 0.0f;
    float older = 0.0f;
    
    size_t mid = coherence_window_.size() / 2;
    for (size_t i = mid; i < coherence_window_.size(); i++) {
        recent += coherence_window_[i];
    }
    for (size_t i = 0; i < mid; i++) {
        older += coherence_window_[i];
    }
    
    recent /= (coherence_window_.size() - mid);
    older /= mid;
    
    return recent > older + 0.05f;
}

void ReflectionController::apply_exploratory_mode() {
    if (!genome_) return;
    
    auto& params = genome_->reasoning_params();
    
    // Increase temperature for more exploration
    params.temperature *= 1.2f;
    params.temperature = std::min(params.temperature, 2.0f);
    
    // Lower thresholds to allow more paths
    params.semantic_threshold *= 0.9f;
    params.confidence_threshold *= 0.9f;
}

void ReflectionController::apply_exploitative_mode() {
    if (!genome_) return;
    
    auto& params = genome_->reasoning_params();
    
    // Decrease temperature for exploitation
    params.temperature *= 0.8f;
    params.temperature = std::max(params.temperature, 0.3f);
    
    // Raise thresholds to be more selective
    params.semantic_threshold *= 1.1f;
    params.confidence_threshold *= 1.1f;
}

void ReflectionController::apply_focused_mode() {
    if (!genome_) return;
    
    auto& params = genome_->reasoning_params();
    
    // Increase semantic bias for focus
    params.semantic_bias_weight += 0.05f;
    params.activation_weight -= 0.05f;
    params.normalize_weights();
    
    // Lower temperature
    params.temperature *= 0.9f;
}

void ReflectionController::apply_deep_mode() {
    if (!genome_) return;
    
    auto& params = genome_->reasoning_params();
    
    // Allow deeper traversal
    params.deep_reasoning_threshold *= 0.9f;
    
    // Increase coherence weight to prefer good paths
    params.coherence_weight += 0.05f;
    params.activation_weight -= 0.05f;
    params.normalize_weights();
}

void ReflectionController::apply_backpressure_mode() {
    if (!genome_) return;
    
    auto& params = genome_->reasoning_params();
    
    // Drastically increase thresholds
    params.semantic_threshold *= 1.5f;
    params.confidence_threshold *= 1.3f;
    
    // Lower temperature to reduce spread
    params.temperature *= 0.7f;
}

void ReflectionController::log_switch(
    ReasoningMode old_mode, 
    ReasoningMode new_mode, 
    const std::string& reason
) {
    ReflectionEvent event;
    event.timestamp = static_cast<float>(total_switches_);
    event.old_mode = old_mode;
    event.new_mode = new_mode;
    event.reason = reason;
    event.confidence = current_metrics_.confidence;
    event.coherence = current_metrics_.coherence;
    event.novelty = current_metrics_.novelty;
    event.active_nodes = current_metrics_.active_node_count;
    
    reflection_history_.push_back(event);
    
    if (reflection_history_.size() > MAX_HISTORY) {
        prune_history();
    }
}

void ReflectionController::prune_history() {
    while (reflection_history_.size() > MAX_HISTORY) {
        reflection_history_.pop_front();
    }
}

float ReflectionController::get_avg_confidence() const {
    if (confidence_window_.empty()) return 0.0f;
    
    float sum = std::accumulate(confidence_window_.begin(), 
                                confidence_window_.end(), 
                                0.0f);
    return sum / confidence_window_.size();
}

float ReflectionController::get_avg_coherence() const {
    if (coherence_window_.empty()) return 0.0f;
    
    float sum = std::accumulate(coherence_window_.begin(), 
                                coherence_window_.end(), 
                                0.0f);
    return sum / coherence_window_.size();
}

float ReflectionController::get_avg_active_nodes() const {
    if (active_nodes_window_.empty()) return 0.0f;
    
    float sum = std::accumulate(active_nodes_window_.begin(), 
                                active_nodes_window_.end(), 
                                0.0f);
    return sum / active_nodes_window_.size();
}

} // namespace metacognition
} // namespace melvin

