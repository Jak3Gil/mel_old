#include "ReflectionEngine.hpp"
#include <algorithm>
#include <cmath>
#include <mutex>

namespace melvin {

ReflectionEngine::ReflectionEngine(std::shared_ptr<LearningEngine> learning_engine,
                                 std::shared_ptr<ReasoningEngine> reasoning_engine)
    : learning_engine_(learning_engine), reasoning_engine_(reasoning_engine),
      confidence_threshold_(0.2f), repetition_threshold_(0.3f), contradiction_threshold_(0.5f),
      micro_evolution_enabled_(true), micro_population_size_(10), micro_generations_(5),
      reflection_count_(0), micro_evolution_count_(0), improvement_rate_(0.0f) {
}

bool ReflectionEngine::detect_stagnation(const Stats& stats) {
    update_stats(stats);
    reflection_count_++;
    
    // Check individual stagnation indicators
    bool low_confidence = detect_low_confidence(stats);
    bool repetition = detect_repetition(stats);
    bool contradiction = detect_contradiction(stats);
    bool oscillation = detect_oscillation(stats);
    
    // Check for stagnation patterns in recent history
    bool pattern_stagnation = is_stagnation_pattern(stats_history_);
    
    // Stagnation detected if multiple indicators are present
    int stagnation_indicators = 0;
    if (low_confidence) stagnation_indicators++;
    if (repetition) stagnation_indicators++;
    if (contradiction) stagnation_indicators++;
    if (oscillation) stagnation_indicators++;
    if (pattern_stagnation) stagnation_indicators++;
    
    return stagnation_indicators >= 2;
}

void ReflectionEngine::micro_evolve(MelvinParams& params) {
    if (!micro_evolution_enabled_) return;
    
    micro_evolution_count_++;
    
    // Spawn micro population
    spawn_micro_population(params, micro_population_size_);
    
    // Run micro evolution
    run_micro_evolution(params, micro_generations_);
}

void ReflectionEngine::update_stats(const Stats& stats) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_history_.push_back(stats);
    
    // Keep only recent history (last 1000 entries)
    if (stats_history_.size() > 1000) {
        stats_history_.erase(stats_history_.begin(), stats_history_.begin() + 100);
    }
    
    // Update improvement rate
    update_improvement_rate(stats);
}

Stats ReflectionEngine::get_current_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    if (stats_history_.empty()) {
        return Stats();
    }
    
    return stats_history_.back();
}

std::vector<Stats> ReflectionEngine::get_stats_history(size_t count) const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    std::vector<Stats> recent;
    size_t start = (stats_history_.size() > count) ? stats_history_.size() - count : 0;
    
    for (size_t i = start; i < stats_history_.size(); ++i) {
        recent.push_back(stats_history_[i]);
    }
    
    return recent;
}

bool ReflectionEngine::detect_low_confidence(const Stats& stats) {
    return stats.average_confidence < confidence_threshold_;
}

bool ReflectionEngine::detect_repetition(const Stats& stats) {
    return stats.repetition_rate > repetition_threshold_;
}

bool ReflectionEngine::detect_contradiction(const Stats& stats) {
    return stats.contradiction_count > contradiction_threshold_;
}

bool ReflectionEngine::detect_oscillation(const Stats& stats) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    if (stats_history_.size() < 10) return false;
    
    // Check for oscillation in confidence values
    std::vector<float> recent_confidences;
    for (size_t i = stats_history_.size() - 10; i < stats_history_.size(); ++i) {
        recent_confidences.push_back(stats_history_[i].average_confidence);
    }
    
    // Count direction changes
    int direction_changes = 0;
    for (size_t i = 1; i < recent_confidences.size(); ++i) {
        if ((recent_confidences[i] > recent_confidences[i-1]) != 
            (recent_confidences[i-1] > recent_confidences[i-2])) {
            direction_changes++;
        }
    }
    
    // Oscillation if too many direction changes
    return direction_changes > 6;
}

void ReflectionEngine::spawn_micro_population(MelvinParams& base_params, size_t population_size) {
    // Create a small population of genomes based on current parameters
    // This would involve:
    // 1. Converting MelvinParams to DynamicGenome
    // 2. Creating mutations of the base genome
    // 3. Setting up the micro population
    
    // For now, we'll simulate the population creation
}

void ReflectionEngine::run_micro_evolution(MelvinParams& params, size_t generations) {
    // Run a small-scale evolution to find better parameters
    // This would involve:
    // 1. Creating a micro population
    // 2. Running fitness evaluation
    // 3. Selecting and mutating the best genomes
    // 4. Repeating for the specified generations
    
    // For now, we'll simulate the micro evolution
}

void ReflectionEngine::adopt_best_genome(MelvinParams& params, const DynamicGenome& best_genome) {
    // Adopt the best genome from micro evolution
    // This would involve:
    // 1. Converting DynamicGenome back to MelvinParams
    // 2. Updating the system parameters
    // 3. Logging the parameter changes
    
    // For now, we'll simulate the adoption
}

void ReflectionEngine::set_confidence_threshold(float threshold) {
    confidence_threshold_ = std::max(0.0f, std::min(1.0f, threshold));
}

void ReflectionEngine::set_repetition_threshold(float threshold) {
    repetition_threshold_ = std::max(0.0f, std::min(1.0f, threshold));
}

void ReflectionEngine::set_contradiction_threshold(float threshold) {
    contradiction_threshold_ = std::max(0.0f, std::min(1.0f, threshold));
}

void ReflectionEngine::set_micro_evolution_enabled(bool enabled) {
    micro_evolution_enabled_ = enabled;
}

size_t ReflectionEngine::get_reflection_count() const {
    return reflection_count_.load();
}

size_t ReflectionEngine::get_micro_evolution_count() const {
    return micro_evolution_count_.load();
}

float ReflectionEngine::get_improvement_rate() const {
    return improvement_rate_.load();
}

void ReflectionEngine::reset_statistics() {
    reflection_count_.store(0);
    micro_evolution_count_.store(0);
    improvement_rate_.store(0.0f);
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_history_.clear();
}

bool ReflectionEngine::is_stagnation_pattern(const std::vector<Stats>& recent_stats) {
    if (recent_stats.size() < 20) return false;
    
    // Check for stagnation patterns in the last 20 cycles
    std::vector<Stats> last_20(recent_stats.end() - 20, recent_stats.end());
    
    // Check if confidence is consistently low
    float avg_confidence = 0.0f;
    for (const auto& stats : last_20) {
        avg_confidence += stats.average_confidence;
    }
    avg_confidence /= last_20.size();
    
    if (avg_confidence < confidence_threshold_) {
        return true;
    }
    
    // Check if success rate is consistently low
    float avg_success = 0.0f;
    for (const auto& stats : last_20) {
        avg_success += stats.success_rate;
    }
    avg_success /= last_20.size();
    
    if (avg_success < 0.3f) {
        return true;
    }
    
    // Check if repetition rate is consistently high
    float avg_repetition = 0.0f;
    for (const auto& stats : last_20) {
        avg_repetition += stats.repetition_rate;
    }
    avg_repetition /= last_20.size();
    
    if (avg_repetition > repetition_threshold_) {
        return true;
    }
    
    return false;
}

float ReflectionEngine::compute_improvement_rate(const Stats& old_stats, const Stats& new_stats) {
    // Compute improvement rate based on key metrics
    float confidence_improvement = new_stats.average_confidence - old_stats.average_confidence;
    float success_improvement = new_stats.success_rate - old_stats.success_rate;
    float repetition_improvement = old_stats.repetition_rate - new_stats.repetition_rate; // Lower is better
    
    // Weighted average of improvements
    float improvement = 0.4f * confidence_improvement + 
                       0.4f * success_improvement + 
                       0.2f * repetition_improvement;
    
    return improvement;
}

void ReflectionEngine::update_improvement_rate(const Stats& stats) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    if (stats_history_.size() >= 2) {
        const Stats& old_stats = stats_history_[stats_history_.size() - 2];
        float improvement = compute_improvement_rate(old_stats, stats);
        
        // Update running average of improvement rate
        float current_rate = improvement_rate_.load();
        improvement_rate_ = 0.9f * current_rate + 0.1f * improvement;
    }
}

DynamicGenome ReflectionEngine::create_micro_genome(const MelvinParams& base_params) {
    // Create a micro genome based on current parameters
    // This would involve:
    // 1. Converting MelvinParams to DynamicGenome
    // 2. Adding small random variations
    // 3. Ensuring parameter bounds are respected
    
    DynamicGenome genome;
    // For now, we'll simulate the genome creation
    return genome;
}

DynamicGenome ReflectionEngine::mutate_micro_genome(const DynamicGenome& genome) {
    // Mutate a micro genome
    // This would involve:
    // 1. Applying small random changes
    // 2. Ensuring parameter bounds are respected
    // 3. Maintaining parameter relationships
    
    DynamicGenome mutated = genome;
    // For now, we'll simulate the mutation
    return mutated;
}

float ReflectionEngine::evaluate_micro_genome(const DynamicGenome& genome) {
    // Evaluate a micro genome's fitness
    // This would involve:
    // 1. Running a quick simulation with the genome
    // 2. Measuring performance metrics
    // 3. Computing a fitness score
    
    // For now, we'll simulate the evaluation
    return 0.5f; // Placeholder fitness score
}

void ReflectionEngine::run_micro_fitness_evaluation(const std::vector<DynamicGenome>& population) {
    // Run fitness evaluation on the micro population
    // This would involve:
    // 1. Running each genome through a quick test
    // 2. Measuring performance metrics
    // 3. Ranking the genomes by fitness
    
    // For now, we'll simulate the evaluation
}

} // namespace melvin
