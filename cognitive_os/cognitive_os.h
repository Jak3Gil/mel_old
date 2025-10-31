/**
 * @file cognitive_os.h
 * @brief Main Cognitive OS - Always-on system coordinator
 * 
 * Manages all services, scheduler, and global state
 */

#ifndef MELVIN_COGNITIVE_OS_H
#define MELVIN_COGNITIVE_OS_H

#include "event_bus.h"
#include "field_facade.h"
#include "service_base.h"
#include "metrics.h"
#include "core/unified_intelligence.h"
#include <vector>
#include <thread>
#include <atomic>
#include <memory>

namespace melvin {
namespace cognitive_os {

/**
 * @brief Service budgets in milliseconds
 */
struct ServiceBudgets {
    float vision = 4.0f;
    float audio = 3.0f;
    float motor = 2.0f;
    float cognition = 5.0f;
    float attention = 1.0f;
    float wm = 1.0f;
    float learning = 2.0f;
    float reflection = 1.0f;
    float consolidation = 1.0f;
};

/**
 * @brief System arousal state (neuromodulator analog)
 */
struct ArousalState {
    float noradrenaline;  // 0-1, drives exploration
    float dopamine;       // 0-1, drives exploitation
    float acetylcholine;  // 0-1, drives attention
};

/**
 * @brief Main Cognitive OS
 * 
 * Always-on system with concurrent services
 */
class CognitiveOS {
public:
    CognitiveOS();
    ~CognitiveOS();
    
    /**
     * @brief Attach core systems
     */
    void attach(
        melvin::intelligence::UnifiedIntelligence* intelligence,
        FieldFacade* field
    );
    
    /**
     * @brief Start all services (spawns threads)
     */
    void start();
    
    /**
     * @brief Stop all services
     */
    void stop();
    
    /**
     * @brief Join (blocks until stopped)
     */
    void join();
    
    /**
     * @brief Get event bus
     */
    EventBus* event_bus() { return &bus_; }
    
    /**
     * @brief Get field
     */
    FieldFacade* field() { return field_; }
    
    /**
     * @brief Set id_to_word map for internal query generation
     */
    void set_word_map(const std::unordered_map<int, std::string>* map) { id_to_word_ = map; }

    /**
     * @brief Provide node degree map (for curiosity bias toward low-degree nodes)
     */
    void set_node_degrees(const std::unordered_map<int, int>* deg) { node_degree_ = deg; }

    /**
     * @brief Hint that a large unified graph is loaded (adjusts dampers)
     */
    void set_large_graph(bool v) { large_graph_ = v; }
    
    /**
     * @brief Get metrics logger
     */
    MetricsLogger* metrics() { return &metrics_; }
    
    /**
     * @brief Check if running
     */
    bool is_running() const {
        return running_.load(std::memory_order_relaxed);
    }
    
private:
    // Core components
    EventBus bus_;
    FieldFacade* field_;
    melvin::intelligence::UnifiedIntelligence* intelligence_;
    MetricsLogger metrics_;
    const std::unordered_map<int, std::string>* id_to_word_{nullptr};  // For internal query generation
    const std::unordered_map<int, int>* node_degree_{nullptr};          // For curiosity bias
    bool large_graph_{false};
    
    // Services (lightweight, run in scheduler thread)
    std::vector<std::unique_ptr<ServiceBase>> services_;
    
    // Scheduler
    std::thread scheduler_thread_;
    std::atomic<bool> running_{false};
    
    // State
    ServiceBudgets budgets_;
    ArousalState arousal_;
    
    // Scheduler methods
    void scheduler_loop();
    void run_tick();
    void compute_arousal(const FieldMetrics& metrics);
    void adapt_budgets(const FieldMetrics& metrics, float cpu_load);
    void update_genome_from_arousal();
    
    // Service execution
    void run_service(ServiceBase* service, float budget_ms);
    
    // Built-in mini-services (run inline for efficiency)
    void tick_cognition(float budget_ms);
    void tick_attention(float budget_ms);
    void tick_working_memory(float budget_ms);
    void tick_learning(float budget_ms);
    void tick_reflection(float budget_ms);
    void tick_field_maintenance(float budget_ms);
    
    // Working memory state
    struct WMSlot {
        int node_id;
        float strength;
        int age;
    };
    std::vector<WMSlot> working_memory_;
    static constexpr int MAX_WM_SLOTS = 7;
    
    // Stats
    uint64_t total_ticks_{0};
    double last_tick_time_{0.0};
    
    // Adaptive baseline activity state
    float rolling_avg_activity_{5.0f};         // Exponential moving average of active nodes
    float target_baseline_activity_{5.0f};     // Dynamic target for baseline
    float recent_prediction_error_{0.0f};      // Curiosity metric
    float boredom_accumulator_{0.0f};          // Increases when low novelty
    double last_dmn_switch_{0.0};              // For network cycling
    enum class DMNFocus { INTROSPECTION, SALIENCE, EXPLORATION } dmn_focus_{DMNFocus::INTROSPECTION};
    std::vector<int> recent_active_nodes_;     // For contextual baseline seeding
            double last_internal_query_time_{0.0};     // For autonomous text outputs
    
    // Self-tuning state (evolution feedback)
    float baseline_drift_{0.0f};               // How far actual is from target (error signal)
    int consecutive_dead_ticks_{0};            // Ticks with 0 nodes (death detection)
    static constexpr int MAX_DEAD_TICKS = 20;  // After this, trigger emergency evolution
    
    double get_timestamp() const;
    float estimate_cpu_load() const;
    
    // Adaptive baseline helpers
    void update_baseline_targets(int active_nodes, float entropy, float coherence);
    void evolve_baseline_parameters();  // Self-tuning when baseline fails
    std::vector<int> sample_contextual_seeds(int k);
    float compute_curiosity_drive() const;
    float compute_boredom_drive() const;
};

} // namespace cognitive_os
} // namespace melvin

#endif // MELVIN_COGNITIVE_OS_H

