/*
 * MELVIN CONTINUOUS LEARNING + EVOLUTION PIPELINE
 * 
 * Metrics and evaluation system for monitoring cognitive performance
 * and triggering evolution cycles when parameters are suboptimal.
 */

#pragma once

#include <vector>
#include <deque>
#include <string>
#include <chrono>
#include <unordered_map>
#include "melvin_types.h"
#include "predictive_sampler.h"

namespace melvin {

// ==================== METRIC CATEGORIES ====================

// Predictive Performance Metrics
struct PredictiveMetrics {
    float avg_traversal_entropy = 0.0f;  // How confident predictions are
    float top2_margin = 0.0f;            // How clearly best choice stands out
    float success_rate = 0.0f;           // % of traversals ending in valid output
    int total_traversals = 0;
    int successful_traversals = 0;
    
    void update(float entropy, float margin, bool success) {
        total_traversals++;
        if (success) successful_traversals++;
        
        // Rolling average updates
        avg_traversal_entropy = 0.9f * avg_traversal_entropy + 0.1f * entropy;
        top2_margin = 0.9f * top2_margin + 0.1f * margin;
        success_rate = static_cast<float>(successful_traversals) / total_traversals;
    }
};

// Memory Health Metrics
struct MemoryMetrics {
    float edge_reuse_ratio = 0.0f;       // % of traversals reusing existing edges
    float thought_replay_stability = 0.0f; // % of successful replays from Thought nodes
    int total_edge_uses = 0;
    int reused_edges = 0;
    int thought_replays = 0;
    int successful_thought_replays = 0;
    
    void update(bool reused_edge, bool thought_replay, bool success) {
        total_edge_uses++;
        if (reused_edge) reused_edges++;
        if (thought_replay) {
            thought_replays++;
            if (success) successful_thought_replays++;
        }
        
        edge_reuse_ratio = static_cast<float>(reused_edges) / total_edge_uses;
        thought_replay_stability = thought_replays > 0 ? 
            static_cast<float>(successful_thought_replays) / thought_replays : 0.0f;
    }
};

// Cognitive Dynamics Metrics
struct CognitiveMetrics {
    float entropy_trend = 0.0f;          // Δ (entropy / 1k steps)
    float coherence_drift = 0.0f;        // Cosine distance between context embeddings
    std::deque<float> entropy_history;   // Rolling window for trend calculation
    std::vector<float> last_context_emb; // Previous context embedding
    
    void update(float entropy, const std::vector<float>& context_emb) {
        // Update entropy trend
        entropy_history.push_back(entropy);
        if (entropy_history.size() > 1000) {
            entropy_history.pop_front();
        }
        
        if (entropy_history.size() >= 100) {
            // Calculate trend over last 100 steps
            float recent_avg = 0.0f, older_avg = 0.0f;
            int half_size = entropy_history.size() / 2;
            
            for (int i = 0; i < half_size; ++i) {
                recent_avg += entropy_history[entropy_history.size() - 1 - i];
                older_avg += entropy_history[half_size - 1 - i];
            }
            recent_avg /= half_size;
            older_avg /= half_size;
            
            entropy_trend = recent_avg - older_avg;
        }
        
        // Update coherence drift
        if (!last_context_emb.empty() && !context_emb.empty()) {
            coherence_drift = 1.0f - cosine_similarity(last_context_emb, context_emb);
        }
        
        last_context_emb = context_emb;
    }
    
private:
    float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b) {
        if (a.empty() || b.empty()) return 0.0f;
        
        float dot_product = 0.0f;
        float norm_a = 0.0f;
        float norm_b = 0.0f;
        
        size_t min_size = std::min(a.size(), b.size());
        for (size_t i = 0; i < min_size; ++i) {
            dot_product += a[i] * b[i];
            norm_a += a[i] * a[i];
            norm_b += b[i] * b[i];
        }
        
        if (norm_a == 0.0f || norm_b == 0.0f) return 0.0f;
        return dot_product / (std::sqrt(norm_a) * std::sqrt(norm_b));
    }
};

// ==================== COMPREHENSIVE METRICS ====================

struct CognitivePerformanceMetrics {
    PredictiveMetrics predictive;
    MemoryMetrics memory;
    CognitiveMetrics cognitive;
    
    // Evolution trigger flags
    bool should_trigger_evolution() const {
        return predictive.avg_traversal_entropy > 1.5f ||     // Too random
               predictive.success_rate < 0.6f ||              // Poor success rate
               cognitive.coherence_drift > 0.25f ||           // High drift
               (std::abs(cognitive.entropy_trend) < 0.01f &&  // Stagnation
                predictive.total_traversals > 1000);
    }
    
    // Fitness score for evolution
    float compute_fitness() const {
        const float w1 = 0.3f, w2 = 0.2f, w3 = 0.3f, w4 = 0.2f;
        
        float H = predictive.avg_traversal_entropy;
        float M = predictive.top2_margin;
        float S = predictive.success_rate;
        float D = cognitive.coherence_drift;
        
        return w1 * (1.0f - H) + w2 * M + w3 * S - w4 * D;
    }
    
    // Get summary string for logging
    std::string get_summary() const {
        std::stringstream ss;
        ss << "Metrics: entropy=" << std::fixed << std::setprecision(3) << predictive.avg_traversal_entropy
           << ", margin=" << predictive.top2_margin
           << ", success=" << predictive.success_rate
           << ", reuse=" << memory.edge_reuse_ratio
           << ", drift=" << cognitive.coherence_drift
           << ", trend=" << cognitive.entropy_trend
           << ", fitness=" << compute_fitness();
        return ss.str();
    }
};

// ==================== METRICS COLLECTOR ====================

class MetricsCollector {
private:
    CognitivePerformanceMetrics metrics_;
    std::deque<CognitivePerformanceMetrics> history_;
    size_t max_history_size_ = 10000;  // Keep last 10k measurements
    
public:
    MetricsCollector() = default;
    
    // Update metrics after a reasoning step
    void record_reasoning_step(
        const std::vector<Candidate>& candidates,
        bool success,
        bool reused_edge = false,
        bool thought_replay = false,
        const std::vector<float>& context_emb = {}
    ) {
        if (candidates.empty()) return;
        
        // Calculate entropy and top-2 margin
        float entropy = compute_entropy(candidates);
        
        float top2_margin = 0.0f;
        if (candidates.size() >= 2) {
            std::vector<float> probs;
            for (const auto& c : candidates) {
                probs.push_back(c.score);
            }
            std::sort(probs.begin(), probs.end(), std::greater<float>());
            top2_margin = probs[0] - probs[1];
        }
        
        // Update all metric categories
        metrics_.predictive.update(entropy, top2_margin, success);
        metrics_.memory.update(reused_edge, thought_replay, success);
        metrics_.cognitive.update(entropy, context_emb);
        
        // Store in history
        history_.push_back(metrics_);
        if (history_.size() > max_history_size_) {
            history_.pop_front();
        }
    }
    
    // Get current metrics
    const CognitivePerformanceMetrics& get_current_metrics() const {
        return metrics_;
    }
    
    // Get metrics history for trend analysis
    const std::deque<CognitivePerformanceMetrics>& get_history() const {
        return history_;
    }
    
    // Check if evolution should be triggered
    bool should_trigger_evolution() const {
        return metrics_.should_trigger_evolution();
    }
    
    // Get average metrics over recent window
    CognitivePerformanceMetrics get_averaged_metrics(size_t window_size = 1000) const {
        if (history_.empty()) return metrics_;
        
        CognitivePerformanceMetrics avg;
        size_t count = std::min(window_size, history_.size());
        
        for (size_t i = history_.size() - count; i < history_.size(); ++i) {
            const auto& m = history_[i];
            avg.predictive.avg_traversal_entropy += m.predictive.avg_traversal_entropy;
            avg.predictive.top2_margin += m.predictive.top2_margin;
            avg.predictive.success_rate += m.predictive.success_rate;
            avg.memory.edge_reuse_ratio += m.memory.edge_reuse_ratio;
            avg.memory.thought_replay_stability += m.memory.thought_replay_stability;
            avg.cognitive.coherence_drift += m.cognitive.coherence_drift;
            avg.cognitive.entropy_trend += m.cognitive.entropy_trend;
        }
        
        avg.predictive.avg_traversal_entropy /= count;
        avg.predictive.top2_margin /= count;
        avg.predictive.success_rate /= count;
        avg.memory.edge_reuse_ratio /= count;
        avg.memory.thought_replay_stability /= count;
        avg.cognitive.coherence_drift /= count;
        avg.cognitive.entropy_trend /= count;
        
        return avg;
    }
    
    // Reset metrics (for evolution cycles)
    void reset() {
        metrics_ = CognitivePerformanceMetrics{};
        history_.clear();
    }
    
    // Log current state
    void log_status() const {
        std::cout << "📊 " << metrics_.get_summary() << "\n";
        if (should_trigger_evolution()) {
            std::cout << "🚨 Evolution trigger conditions met!\n";
        }
    }
};

} // namespace melvin
