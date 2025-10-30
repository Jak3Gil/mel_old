/**
 * @file validator.h
 * @brief Cognitive OS go-live validator
 * 
 * Continuously tests timing, reasoning, safety, and memory
 * Produces readiness reports and auto-tunes genome
 */

#ifndef MELVIN_VALIDATOR_H
#define MELVIN_VALIDATOR_H

#include "cognitive_os/cognitive_os.h"
#include <vector>
#include <string>
#include <deque>
#include <chrono>

namespace melvin {
namespace validator {

/**
 * @brief Rolling KPI statistics
 */
struct KPIs {
    double timestamp;
    float tick_jitter;
    int active_nodes;
    float energy_var;
    float coherence;
    float entropy;
    float confidence;
    uint64_t dropped_msgs;
    float cpu_usage;
    float gpu_usage;
};

/**
 * @brief Test query for reasoning validation
 */
struct TestQuery {
    std::string question;
    std::string expected_keyword;  // Answer should contain this
    float min_confidence;
};

/**
 * @brief Test results
 */
struct TestResults {
    // Timing
    bool timing_ok;
    float avg_jitter;
    float max_jitter;
    float missed_deadline_rate;
    
    // Scheduler
    bool scheduler_fair;
    float cognition_on_time;
    float learning_on_time;
    
    // Field Health
    bool field_healthy;
    float coherence_gain;
    float entropy_reduction;
    int convergence_ticks;
    
    // Reasoning Quality
    bool reasoning_ok;
    float accuracy;
    float avg_confidence;
    float avg_chain_length;
    
    // Stress Tests
    bool stress_passed;
    float recovery_time_ms;
    bool no_crashes;
    
    // Safety
    bool safety_ok;
    float throttle_response_ms;
    
    // Memory
    bool memory_ok;
    float memory_growth_pct;
    int edges_pruned;
    
    // Overall
    bool ready_for_deployment;
    std::vector<std::string> warnings;
    std::vector<std::string> recommendations;
};

/**
 * @brief Main validator class
 */
class Validator {
public:
    Validator(cognitive_os::CognitiveOS* os);
    ~Validator() = default;
    
    /**
     * @brief Start validation (runs in background)
     */
    void start(int duration_seconds = 600);
    
    /**
     * @brief Stop validation
     */
    void stop();
    
    /**
     * @brief Run all tests and generate report
     */
    TestResults run_validation_suite();
    
    /**
     * @brief Generate readiness report
     */
    void generate_report(const std::string& filepath);
    
    /**
     * @brief Get current KPIs
     */
    const std::deque<KPIs>& kpi_history() const { return kpi_history_; }
    
private:
    cognitive_os::CognitiveOS* os_;
    std::thread validator_thread_;
    std::atomic<bool> running_{false};
    
    // KPI history (30 seconds rolling)
    std::deque<KPIs> kpi_history_;
    static constexpr size_t MAX_HISTORY = 600;  // 30s at 20ms ticks
    
    // Test queries
    std::vector<TestQuery> test_queries_;
    
    // Results
    TestResults latest_results_;
    
    // Validation loop
    void validation_loop(int duration_seconds);
    void collect_kpis();
    
    // Test suites
    bool test_timing();
    bool test_scheduler_fairness();
    bool test_field_health();
    bool test_reasoning_quality();
    bool test_stress_scenarios();
    bool test_safety_response();
    bool test_memory_hygiene();
    
    // Stress scenarios
    void stress_vision_burst();
    void stress_audio_flood();
    void stress_conflicting_goals();
    void stress_empty_knowledge();
    
    // Auto-tuning
    void auto_tune_genome();
    
    // Helpers
    void init_test_queries();
    float compute_jitter();
    float compute_memory_growth();
};

} // namespace validator
} // namespace melvin

#endif // MELVIN_VALIDATOR_H

