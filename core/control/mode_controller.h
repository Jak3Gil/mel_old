#ifndef MODE_CONTROLLER_H
#define MODE_CONTROLLER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <chrono>

// Forward declare Genome
namespace melvin {
namespace evolution {
    class Genome;
}
}

namespace melvin {
namespace control {

// The three operational modes
enum OperationalMode {
    LISTENING,   // Input only - build knowledge
    THINKING,    // Internal only - plan/consolidate
    ACTING       // Output to world - execute actions
};

// Mode transition reasons (for logging/debugging)
enum TransitionReason {
    SAFETY_OVERRIDE,
    GOAL_ACHIEVED,
    GOAL_SET,
    CONFIDENCE_HIGH,
    CONFIDENCE_LOW,
    INPUT_AVAILABLE,
    CONSOLIDATION_DUE,
    EVOLUTION_DUE,
    ENERGY_HIGH,
    ENERGY_LOW,
    EXPLORATION_DRIVE,
    IDLE_TIMEOUT,
    PREDICTION_ERROR_HIGH,
    PLAN_READY
};

// System state used for mode decision
struct SystemState {
    // Goals
    bool has_active_goal = false;
    std::string current_goal;
    float goal_confidence = 0.0f;
    
    // Energy
    float total_graph_energy = 0.0f;
    float energy_input_rate = 0.0f;  // How much energy from sensors
    
    // Knowledge
    int total_nodes = 0;
    int total_edges = 0;
    float knowledge_completeness = 0.0f;  // Do we know enough?
    
    // Inputs
    bool visual_input_available = false;
    bool audio_input_available = false;
    bool text_input_available = false;
    bool motor_feedback_available = false;
    
    // Safety
    bool emergency_stop_active = false;
    bool collision_detected = false;
    bool motor_torque_exceeded = false;
    float obstacle_distance = 999.0f;  // meters
    
    // Performance
    float recent_prediction_accuracy = 0.0f;
    float recent_success_rate = 0.0f;
    std::deque<float> recent_errors;
    
    // Timing
    float time_since_last_input = 0.0f;
    float time_since_last_action = 0.0f;
    float time_since_last_consolidation = 0.0f;
    float time_since_last_evolution = 0.0f;
    
    // Meta-learning
    std::string learning_phase;  // EXPLORATION, REFINEMENT, EXPLOITATION
    float exploration_rate = 0.3f;
    float learning_rate = 0.1f;
    
    // Action planning
    bool plan_exists = false;
    float plan_confidence = 0.0f;
    std::vector<std::string> planned_actions;
};

class ModeController {
public:
    ModeController();
    ModeController(evolution::Genome* genome);  // Use genome for thresholds
    
    // Main decision function - MELVIN calls this every cycle
    OperationalMode determine_mode(const SystemState& state);
    
    // Update state (called by main loop)
    void update_state(const SystemState& new_state);
    
    // Force mode (for emergencies)
    void force_mode(OperationalMode mode, const std::string& reason);
    
    // Query current mode
    OperationalMode get_current_mode() const { return current_mode_; }
    std::string get_mode_name(OperationalMode mode) const;
    TransitionReason get_last_transition_reason() const { return last_transition_reason_; }
    
    // Set genome (for reading evolved thresholds)
    void set_genome(evolution::Genome* genome);
    
    // Statistics
    float get_time_in_mode(OperationalMode mode) const;
    int get_mode_switch_count() const { return mode_switch_count_; }
    
private:
    OperationalMode current_mode_;
    SystemState current_state_;
    TransitionReason last_transition_reason_;
    
    // Mode history
    std::deque<std::pair<OperationalMode, float>> mode_history_;
    int mode_switch_count_;
    
    // Timing
    std::chrono::steady_clock::time_point mode_start_time_;
    std::unordered_map<OperationalMode, float> time_in_mode_;
    
    // Genome pointer (for reading evolved thresholds)
    evolution::Genome* genome_;
    
    // Decision functions
    bool check_safety(const SystemState& state);
    bool should_act(const SystemState& state);
    bool should_think(const SystemState& state);
    bool should_listen(const SystemState& state);
    
    // Helper to get threshold from genome
    float get_threshold(const std::string& name, float default_val) const;
    
    // Transition logging
    void log_transition(OperationalMode from, OperationalMode to, TransitionReason reason);
};

} // namespace control
} // namespace melvin

#endif // MODE_CONTROLLER_H

