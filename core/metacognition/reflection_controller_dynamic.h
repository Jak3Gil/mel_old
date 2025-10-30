/**
 * @file reflection_controller_dynamic.h
 * @brief Autonomous mode switching and meta-cognitive monitoring
 * 
 * Monitors reasoning performance and automatically:
 * - Switches between exploration/exploitation
 * - Adjusts temperature and thresholds
 * - Triggers deep reasoning when needed
 * - Manages backpressure
 */

#ifndef MELVIN_REFLECTION_CONTROLLER_DYNAMIC_H
#define MELVIN_REFLECTION_CONTROLLER_DYNAMIC_H

#include "core/metrics/reasoning_metrics.h"
#include "core/evolution/dynamic_genome.h"
#include <deque>
#include <string>

namespace melvin {
namespace metacognition {

/**
 * @brief Reasoning modes
 */
enum class ReasoningMode {
    EXPLORATORY,    // Low confidence - explore more paths
    EXPLOITATIVE,   // High confidence - exploit known paths
    FOCUSED,        // Too scattered - increase focus
    DEEP,           // Shallow paths failing - go deeper
    BACKPRESSURE    // Too many active nodes - throttle
};

/**
 * @brief Reflection event for logging
 */
struct ReflectionEvent {
    float timestamp;
    ReasoningMode old_mode;
    ReasoningMode new_mode;
    std::string reason;
    
    // Metrics snapshot
    float confidence;
    float coherence;
    float novelty;
    int active_nodes;
};

/**
 * @brief Autonomous reflection controller
 * 
 * Meta-cognitive layer that observes reasoning and adapts strategy
 */
class ReflectionController {
public:
    ReflectionController();
    ~ReflectionController() = default;
    
    /**
     * @brief Set genome for parameter control
     */
    void set_genome(melvin::evolution::DynamicGenome* genome) {
        genome_ = genome;
    }
    
    /**
     * @brief Observe current reasoning state
     * 
     * Call after each reasoning cycle
     */
    void observe(const melvin::metrics::ReasoningMetrics& metrics);
    
    /**
     * @brief Get current reasoning mode
     */
    ReasoningMode current_mode() const { return current_mode_; }
    
    /**
     * @brief Should we switch modes?
     */
    bool should_switch_mode() const;
    
    /**
     * @brief Execute mode switch if needed
     * 
     * Returns true if mode changed
     */
    bool reflect_and_adapt();
    
    /**
     * @brief Get reflection history
     */
    const std::deque<ReflectionEvent>& history() const { 
        return reflection_history_; 
    }
    
    /**
     * @brief Get mode switch statistics
     */
    struct ModeStats {
        int exploratory_count;
        int exploitative_count;
        int focused_count;
        int deep_count;
        int backpressure_count;
        int total_switches;
    };
    
    ModeStats get_stats() const;
    
    /**
     * @brief Reset controller state
     */
    void reset();
    
private:
    melvin::evolution::DynamicGenome* genome_;
    
    // Current state
    ReasoningMode current_mode_;
    melvin::metrics::ReasoningMetrics current_metrics_;
    
    // History
    std::deque<ReflectionEvent> reflection_history_;
    static constexpr size_t MAX_HISTORY = 100;
    
    // Observation window
    std::deque<float> confidence_window_;
    std::deque<float> coherence_window_;
    std::deque<int> active_nodes_window_;
    static constexpr size_t WINDOW_SIZE = 5;
    
    // Mode switch cooldown
    int cycles_since_last_switch_;
    static constexpr int MIN_CYCLES_BETWEEN_SWITCHES = 3;
    
    // Statistics
    int exploratory_count_;
    int exploitative_count_;
    int focused_count_;
    int deep_count_;
    int backpressure_count_;
    int total_switches_;
    
    // Mode decision logic
    ReasoningMode decide_mode();
    
    bool is_confidence_low() const;
    bool is_confidence_high() const;
    bool is_coherence_low() const;
    bool is_coherence_high() const;
    bool is_backpressure_active() const;
    bool is_confidence_improving() const;
    bool is_coherence_improving() const;
    
    // Mode-specific adaptations
    void apply_exploratory_mode();
    void apply_exploitative_mode();
    void apply_focused_mode();
    void apply_deep_mode();
    void apply_backpressure_mode();
    
    // Helpers
    void log_switch(ReasoningMode old_mode, ReasoningMode new_mode, const std::string& reason);
    void prune_history();
    float get_avg_confidence() const;
    float get_avg_coherence() const;
    float get_avg_active_nodes() const;
};

/**
 * @brief Mode name for display
 */
inline const char* mode_name(ReasoningMode mode) {
    switch (mode) {
        case ReasoningMode::EXPLORATORY: return "EXPLORATORY";
        case ReasoningMode::EXPLOITATIVE: return "EXPLOITATIVE";
        case ReasoningMode::FOCUSED: return "FOCUSED";
        case ReasoningMode::DEEP: return "DEEP";
        case ReasoningMode::BACKPRESSURE: return "BACKPRESSURE";
        default: return "UNKNOWN";
    }
}

} // namespace metacognition
} // namespace melvin

#endif // MELVIN_REFLECTION_CONTROLLER_DYNAMIC_H

