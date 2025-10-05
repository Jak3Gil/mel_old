#pragma once

#include "melvin_types.h"
#include "uca_types.h"
#include "../learning/LearningEngine.hpp"
#include "../reasoning/ReasoningEngine.hpp"
#include "../evolution/Evolution.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <atomic>

namespace melvin {

/**
 * Stats - Represents system performance statistics
 */
struct Stats {
    float average_confidence;
    float success_rate;
    float repetition_rate;
    float contradiction_count;
    size_t total_cycles;
    uint64_t timestamp;
    
    Stats() : average_confidence(0.0f), success_rate(0.0f), repetition_rate(0.0f),
              contradiction_count(0.0f), total_cycles(0), timestamp(0) {}
};

/**
 * ReflectionEngine - Detects stagnation and triggers micro-evolution
 * 
 * Responsibilities:
 * - Monitor system performance
 * - Detect stagnation patterns
 * - Trigger micro-evolution when needed
 * - Manage parameter adaptation
 */
class ReflectionEngine {
public:
    ReflectionEngine(std::shared_ptr<LearningEngine> learning_engine,
                    std::shared_ptr<ReasoningEngine> reasoning_engine);
    ~ReflectionEngine() = default;

    // Core reflection methods
    bool detect_stagnation(const Stats& stats);
    void micro_evolve(MelvinParams& params);
    
    // Performance monitoring
    void update_stats(const Stats& stats);
    Stats get_current_stats() const;
    std::vector<Stats> get_stats_history(size_t count = 100) const;
    
    // Stagnation detection
    bool detect_low_confidence(const Stats& stats);
    bool detect_repetition(const Stats& stats);
    bool detect_contradiction(const Stats& stats);
    bool detect_oscillation(const Stats& stats);
    
    // Micro-evolution
    void spawn_micro_population(MelvinParams& base_params, size_t population_size = 10);
    void run_micro_evolution(MelvinParams& params, size_t generations = 5);
    void adopt_best_genome(MelvinParams& params, const DynamicGenome& best_genome);
    
    // Configuration
    void set_confidence_threshold(float threshold);
    void set_repetition_threshold(float threshold);
    void set_contradiction_threshold(float threshold);
    void set_micro_evolution_enabled(bool enabled);
    
    // Statistics
    size_t get_reflection_count() const;
    size_t get_micro_evolution_count() const;
    float get_improvement_rate() const;
    void reset_statistics();

private:
    std::shared_ptr<LearningEngine> learning_engine_;
    std::shared_ptr<ReasoningEngine> reasoning_engine_;
    
    // Performance tracking
    std::vector<Stats> stats_history_;
    mutable std::mutex stats_mutex_;
    
    // Stagnation detection thresholds
    float confidence_threshold_;
    float repetition_threshold_;
    float contradiction_threshold_;
    
    // Micro-evolution settings
    bool micro_evolution_enabled_;
    size_t micro_population_size_;
    size_t micro_generations_;
    
    // Statistics
    std::atomic<size_t> reflection_count_;
    std::atomic<size_t> micro_evolution_count_;
    std::atomic<float> improvement_rate_;
    
    // Internal methods
    bool is_stagnation_pattern(const std::vector<Stats>& recent_stats);
    float compute_improvement_rate(const Stats& old_stats, const Stats& new_stats);
    void update_improvement_rate(const Stats& stats);
    
    // Micro-evolution helpers
    DynamicGenome create_micro_genome(const MelvinParams& base_params);
    DynamicGenome mutate_micro_genome(const DynamicGenome& genome);
    float evaluate_micro_genome(const DynamicGenome& genome);
    void run_micro_fitness_evaluation(const std::vector<DynamicGenome>& population);
};

} // namespace melvin
