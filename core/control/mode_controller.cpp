#include "mode_controller.h"
#include "../evolution/genome.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace melvin {
namespace control {

ModeController::ModeController()
    : current_mode_(LISTENING)
    , mode_switch_count_(0)
    , mode_start_time_(std::chrono::steady_clock::now())
    , genome_(nullptr) {
    
    // Initialize mode timing
    time_in_mode_[LISTENING] = 0.0f;
    time_in_mode_[THINKING] = 0.0f;
    time_in_mode_[ACTING] = 0.0f;
    
    std::cout << "🎛️  Mode Controller initialized (default: LISTENING)" << std::endl;
    std::cout << "⚠️  No genome set - using fallback defaults" << std::endl;
}

ModeController::ModeController(evolution::Genome* genome)
    : current_mode_(LISTENING)
    , mode_switch_count_(0)
    , mode_start_time_(std::chrono::steady_clock::now())
    , genome_(genome) {
    
    // Initialize mode timing
    time_in_mode_[LISTENING] = 0.0f;
    time_in_mode_[THINKING] = 0.0f;
    time_in_mode_[ACTING] = 0.0f;
    
    std::cout << "🎛️  Mode Controller initialized with genome (default: LISTENING)" << std::endl;
    std::cout << "🧬 Using evolved thresholds from genome!" << std::endl;
}

void ModeController::set_genome(evolution::Genome* genome) {
    genome_ = genome;
    std::cout << "🧬 Genome linked to Mode Controller - thresholds will evolve!" << std::endl;
}

float ModeController::get_threshold(const std::string& name, float default_val) const {
    if (genome_) {
        return genome_->get(name);
    }
    return default_val;  // Fallback if no genome
}

std::string ModeController::get_mode_name(OperationalMode mode) const {
    switch (mode) {
        case LISTENING: return "LISTENING";
        case THINKING: return "THINKING";
        case ACTING: return "ACTING";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// MAIN DECISION FUNCTION - THIS IS THE AUTONOMOUS MODE SELECTOR
// ============================================================================

OperationalMode ModeController::determine_mode(const SystemState& state) {
    // Update current state
    current_state_ = state;
    
    // ========================================================================
    // PRIORITY 1: SAFETY (ALWAYS FIRST!)
    // ========================================================================
    if (!check_safety(state)) {
        if (current_mode_ != LISTENING) {
            log_transition(current_mode_, LISTENING, SAFETY_OVERRIDE);
        }
        return LISTENING;  // EMERGENCY: Stop acting, observe only
    }
    
    // ========================================================================
    // PRIORITY 2: ACTING (Execute if ready)
    // ========================================================================
    if (should_act(state)) {
        if (current_mode_ != ACTING) {
            log_transition(current_mode_, ACTING, 
                state.plan_exists ? PLAN_READY : GOAL_SET);
        }
        return ACTING;
    }
    
    // ========================================================================
    // PRIORITY 3: THINKING (Plan or consolidate)
    // ========================================================================
    if (should_think(state)) {
        if (current_mode_ != THINKING) {
            TransitionReason reason = CONSOLIDATION_DUE;
            if (state.has_active_goal && !state.plan_exists) {
                reason = CONFIDENCE_LOW;  // Need to plan
            } else if (state.time_since_last_consolidation > consolidation_interval_) {
                reason = CONSOLIDATION_DUE;
            } else if (state.time_since_last_evolution > evolution_interval_) {
                reason = EVOLUTION_DUE;
            }
            log_transition(current_mode_, THINKING, reason);
        }
        return THINKING;
    }
    
    // ========================================================================
    // PRIORITY 4: LISTENING (Default - always learning)
    // ========================================================================
    if (should_listen(state)) {
        if (current_mode_ != LISTENING) {
            log_transition(current_mode_, LISTENING, INPUT_AVAILABLE);
        }
        return LISTENING;
    }
    
    // ========================================================================
    // FALLBACK: Stay in current mode or default to THINKING
    // ========================================================================
    if (current_mode_ == THINKING) {
        return THINKING;  // Continue thinking
    }
    
    // Last resort: switch to thinking
    if (current_mode_ != THINKING) {
        log_transition(current_mode_, THINKING, IDLE_TIMEOUT);
    }
    return THINKING;
}

// ============================================================================
// SAFETY CHECK - Can we safely act?
// ============================================================================

bool ModeController::check_safety(const SystemState& state) {
    // Emergency stop pressed
    if (state.emergency_stop_active) {
        std::cout << "🚨 EMERGENCY STOP ACTIVE!" << std::endl;
        return false;
    }
    
    // Collision detected
    if (state.collision_detected) {
        std::cout << "🚨 COLLISION DETECTED!" << std::endl;
        return false;
    }
    
    // Motor torque exceeded
    if (state.motor_torque_exceeded) {
        std::cout << "🚨 MOTOR TORQUE EXCEEDED!" << std::endl;
        return false;
    }
    
    // Obstacle too close
    float min_safe_distance = get_threshold("mode_min_safe_distance", 0.3f);
    if (state.obstacle_distance < min_safe_distance) {
        std::cout << "⚠️  OBSTACLE TOO CLOSE: " << state.obstacle_distance << "m (min: " << min_safe_distance << "m)" << std::endl;
        return false;
    }
    
    // Recent prediction errors too high (system confused)
    if (state.recent_errors.size() > 10) {
        float avg_error = 0.0f;
        for (float error : state.recent_errors) {
            avg_error += error;
        }
        avg_error /= state.recent_errors.size();
        
        float max_error_threshold = get_threshold("mode_max_error_rate", 0.7f);
        if (avg_error > max_error_threshold) {
            std::cout << "⚠️  PREDICTION ERROR TOO HIGH: " << (avg_error * 100) << "% (max: " << (max_error_threshold * 100) << "%)" << std::endl;
            return false;
        }
    }
    
    return true;  // Safe!
}

// ============================================================================
// SHOULD_ACT - Decide if we should execute physical actions
// ============================================================================

bool ModeController::should_act(const SystemState& state) {
    // No active goal = no reason to act
    if (!state.has_active_goal) {
        return false;
    }
    
    // -----------------------------------------------------------------------
    // CHECK 1: Do we have a plan?
    // -----------------------------------------------------------------------
    if (!state.plan_exists) {
        // No plan yet - need to think first
        return false;
    }
    
    // -----------------------------------------------------------------------
    // CHECK 2: Are we confident enough?
    // -----------------------------------------------------------------------
    float confidence_threshold = get_threshold("mode_confidence_threshold", 0.7f);
    if (state.plan_confidence < confidence_threshold) {
        // Not confident enough - need more thinking
        std::cout << "⚠️  Plan confidence too low: " << state.plan_confidence 
                  << " < " << confidence_threshold << " (evolved)" << std::endl;
        return false;
    }
    
    // -----------------------------------------------------------------------
    // CHECK 3: Do we have enough knowledge?
    // -----------------------------------------------------------------------
    float min_knowledge = get_threshold("mode_min_knowledge_for_action", 0.3f);
    if (state.knowledge_completeness < min_knowledge) {
        // Don't know enough yet - keep learning
        std::cout << "⚠️  Insufficient knowledge: " << state.knowledge_completeness 
                  << " < " << min_knowledge << " (evolved)" << std::endl;
        return false;
    }
    
    // -----------------------------------------------------------------------
    // CHECK 4: Is energy high enough?
    // -----------------------------------------------------------------------
    float energy_low = get_threshold("mode_energy_low_threshold", 5.0f);
    if (state.total_graph_energy < energy_low) {
        // Too tired to act
        std::cout << "⚠️  Energy too low: " << state.total_graph_energy 
                  << " < " << energy_low << " (evolved)" << std::endl;
        return false;
    }
    
    // -----------------------------------------------------------------------
    // CHECK 5: Are we in exploration phase?
    // -----------------------------------------------------------------------
    float exploration_threshold = get_threshold("mode_exploration_threshold", 0.4f);
    float exploration_confidence = get_threshold("mode_exploration_confidence", 0.5f);
    if (state.learning_phase == "EXPLORATION" && state.exploration_rate > exploration_threshold) {
        // High exploration = try random actions even without perfect plan
        if (state.plan_confidence > exploration_confidence) {
            std::cout << "🔬 EXPLORATION MODE: Acting with lower confidence (thresholds evolved)" << std::endl;
            return true;
        }
    }
    
    // -----------------------------------------------------------------------
    // ALL CHECKS PASSED - READY TO ACT!
    // -----------------------------------------------------------------------
    std::cout << "✅ Ready to act!" << std::endl;
    std::cout << "   Goal: " << state.current_goal << std::endl;
    std::cout << "   Confidence: " << (state.plan_confidence * 100) << "%" << std::endl;
    std::cout << "   Energy: " << state.total_graph_energy << std::endl;
    
    return true;
}

// ============================================================================
// SHOULD_THINK - Decide if we should do internal processing
// ============================================================================

bool ModeController::should_think(const SystemState& state) {
    // -----------------------------------------------------------------------
    // REASON 1: Have goal but no plan (need to plan!)
    // -----------------------------------------------------------------------
    if (state.has_active_goal && !state.plan_exists) {
        std::cout << "💭 Need to plan for goal: " << state.current_goal << std::endl;
        return true;
    }
    
    // -----------------------------------------------------------------------
    // REASON 2: Have goal but confidence too low (need more thinking!)
    // -----------------------------------------------------------------------
    float confidence_threshold = get_threshold("mode_confidence_threshold", 0.7f);
    if (state.has_active_goal && state.plan_confidence < confidence_threshold) {
        std::cout << "💭 Plan confidence too low, need to think more... (threshold: " << confidence_threshold << " evolved)" << std::endl;
        return true;
    }
    
    // -----------------------------------------------------------------------
    // REASON 3: Consolidation due (offline learning)
    // -----------------------------------------------------------------------
    float consolidation_interval = get_threshold("mode_consolidation_interval", 300.0f);
    if (state.time_since_last_consolidation > consolidation_interval) {
        std::cout << "🧠 Consolidation due (idle: " 
                  << state.time_since_last_consolidation << "s, threshold: " << consolidation_interval << "s evolved)" << std::endl;
        return true;
    }
    
    // -----------------------------------------------------------------------
    // REASON 4: Evolution cycle due
    // -----------------------------------------------------------------------
    float evolution_interval = get_threshold("mode_evolution_interval", 600.0f);
    if (state.time_since_last_evolution > evolution_interval) {
        std::cout << "🧬 Evolution cycle due (last: " 
                  << state.time_since_last_evolution << "s ago, threshold: " << evolution_interval << "s evolved)" << std::endl;
        return true;
    }
    
    // -----------------------------------------------------------------------
    // REASON 5: High energy but no input (use energy for thinking)
    // -----------------------------------------------------------------------
    float energy_high = get_threshold("mode_energy_high_threshold", 50.0f);
    if (state.total_graph_energy > energy_high && 
        state.energy_input_rate < 1.0f) {
        std::cout << "⚡ High internal energy (" << state.total_graph_energy << " > " << energy_high << " evolved), low input → Think!" << std::endl;
        return true;
    }
    
    // -----------------------------------------------------------------------
    // REASON 6: Recent prediction errors (need to figure out what went wrong)
    // -----------------------------------------------------------------------
    if (!state.recent_errors.empty()) {
        float recent_error = state.recent_errors.back();
        if (recent_error > 0.5f) {
            std::cout << "💭 Recent error high (" << (recent_error * 100) 
                      << "%), analyzing..." << std::endl;
            return true;
        }
    }
    
    // -----------------------------------------------------------------------
    // REASON 7: Idle for too long (default to thinking)
    // -----------------------------------------------------------------------
    float idle_timeout = get_threshold("mode_idle_timeout", 60.0f);
    if (state.time_since_last_input > idle_timeout && 
        state.time_since_last_action > idle_timeout) {
        std::cout << "💤 Idle too long (" << state.time_since_last_input << "s > " << idle_timeout << "s evolved), time to consolidate..." << std::endl;
        return true;
    }
    
    return false;
}

// ============================================================================
// SHOULD_LISTEN - Decide if we should be in learning mode
// ============================================================================

bool ModeController::should_listen(const SystemState& state) {
    // -----------------------------------------------------------------------
    // REASON 1: Input actively available (default behavior)
    // -----------------------------------------------------------------------
    if (state.visual_input_available || 
        state.audio_input_available || 
        state.text_input_available) {
        
        // But don't interrupt acting!
        if (current_mode_ == ACTING) {
            return false;  // Keep acting, sensors used for feedback
        }
        
        std::cout << "👂 Input available → Listening mode" << std::endl;
        return true;
    }
    
    // -----------------------------------------------------------------------
    // REASON 2: Knowledge insufficient (need to learn more)
    // -----------------------------------------------------------------------
    float min_knowledge = get_threshold("mode_min_knowledge_for_action", 0.3f);
    if (state.knowledge_completeness < min_knowledge) {
        std::cout << "📚 Insufficient knowledge (" 
                  << (state.knowledge_completeness * 100) 
                  << "% < " << (min_knowledge * 100) << "% evolved), need to learn more" << std::endl;
        return true;
    }
    
    // -----------------------------------------------------------------------
    // REASON 3: Exploration phase with low knowledge
    // -----------------------------------------------------------------------
    float min_nodes_threshold = get_threshold("mode_min_nodes_for_action", 1000.0f);
    if (state.learning_phase == "EXPLORATION" && 
        state.total_nodes < min_nodes_threshold) {
        std::cout << "🔬 Exploration phase + low knowledge (" << state.total_nodes << " < " << min_nodes_threshold << " evolved) → Keep learning" << std::endl;
        return true;
    }
    
    // -----------------------------------------------------------------------
    // REASON 4: No goal set (default to learning)
    // -----------------------------------------------------------------------
    if (!state.has_active_goal) {
        if (state.visual_input_available || state.audio_input_available) {
            std::cout << "🎯 No goal + input available → Listen & learn" << std::endl;
            return true;
        }
    }
    
    // -----------------------------------------------------------------------
    // REASON 5: Recent action failed (observe more before trying again)
    // -----------------------------------------------------------------------
    float min_success_rate = get_threshold("mode_min_success_rate", 0.3f);
    if (state.recent_success_rate < min_success_rate && state.recent_errors.size() > 5) {
        std::cout << "⚠️  Low success rate (" << (state.recent_success_rate * 100) 
                  << "% < " << (min_success_rate * 100) << "% evolved), switching to observation" << std::endl;
        return true;
    }
    
    return false;
}

// ============================================================================
// STATE UPDATE
// ============================================================================

void ModeController::update_state(const SystemState& new_state) {
    current_state_ = new_state;
    
    // Update time in current mode
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - mode_start_time_).count();
    time_in_mode_[current_mode_] += elapsed;
    mode_start_time_ = now;
}

// ============================================================================
// FORCE MODE (For manual override or emergencies)
// ============================================================================

void ModeController::force_mode(OperationalMode mode, const std::string& reason) {
    std::cout << "🚨 FORCING MODE: " << get_mode_name(current_mode_) 
              << " → " << get_mode_name(mode) 
              << " (Reason: " << reason << ")" << std::endl;
    
    current_mode_ = mode;
    mode_switch_count_++;
    mode_start_time_ = std::chrono::steady_clock::now();
}

// ============================================================================
// LOGGING
// ============================================================================

void ModeController::log_transition(OperationalMode from, OperationalMode to, 
                                    TransitionReason reason) {
    std::cout << "\n🔄 MODE TRANSITION: " 
              << get_mode_name(from) << " → " << get_mode_name(to) << std::endl;
    
    // Log reason
    std::string reason_str;
    switch (reason) {
        case SAFETY_OVERRIDE: reason_str = "Safety override"; break;
        case GOAL_ACHIEVED: reason_str = "Goal achieved"; break;
        case GOAL_SET: reason_str = "Goal set"; break;
        case CONFIDENCE_HIGH: reason_str = "Confidence high"; break;
        case CONFIDENCE_LOW: reason_str = "Confidence low"; break;
        case INPUT_AVAILABLE: reason_str = "Input available"; break;
        case CONSOLIDATION_DUE: reason_str = "Consolidation due"; break;
        case EVOLUTION_DUE: reason_str = "Evolution due"; break;
        case ENERGY_HIGH: reason_str = "Energy high"; break;
        case ENERGY_LOW: reason_str = "Energy low"; break;
        case EXPLORATION_DRIVE: reason_str = "Exploration drive"; break;
        case IDLE_TIMEOUT: reason_str = "Idle timeout"; break;
        case PREDICTION_ERROR_HIGH: reason_str = "Prediction error high"; break;
        case PLAN_READY: reason_str = "Plan ready"; break;
        default: reason_str = "Unknown"; break;
    }
    
    std::cout << "   Reason: " << reason_str << std::endl;
    std::cout << std::endl;
    
    // Update tracking
    current_mode_ = to;
    last_transition_reason_ = reason;
    mode_switch_count_++;
    
    // Update timing
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - mode_start_time_).count();
    time_in_mode_[from] += elapsed;
    mode_start_time_ = now;
    
    // Add to history
    mode_history_.push_back({to, elapsed});
    if (mode_history_.size() > 100) {
        mode_history_.pop_front();
    }
}

// ============================================================================
// STATISTICS
// ============================================================================

float ModeController::get_time_in_mode(OperationalMode mode) const {
    auto it = time_in_mode_.find(mode);
    if (it != time_in_mode_.end()) {
        return it->second;
    }
    return 0.0f;
}

} // namespace control
} // namespace melvin

