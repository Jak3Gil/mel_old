/**
 * @file turn_taking_controller.h
 * @brief Basal ganglia-inspired turn-taking controller
 * 
 * Uses theta/gamma rhythm coupling to coordinate listening/speaking transitions.
 * Mimics natural conversation timing through oscillatory synchronization.
 */

#ifndef MELVIN_TURN_TAKING_CONTROLLER_H
#define MELVIN_TURN_TAKING_CONTROLLER_H

#include <cmath>

namespace melvin {
namespace cognitive {

/**
 * @brief Turn-taking controller using theta/gamma oscillations
 * 
 * Brain regions modeled:
 * - Basal ganglia: Turn initiation/inhibition
 * - Theta oscillations (4-8 Hz): Turn timing coordination
 * - Gamma oscillations (30-80 Hz): Active processing power
 */
class TurnTakingController {
public:
    enum class State {
        LISTENING,      // Processing external input, low output
        SPEAKING,       // Generating output, reduced input
        TRANSITIONING   // Switching between states
    };
    
    TurnTakingController();
    ~TurnTakingController() = default;
    
    /**
     * @brief Update turn-taking state
     * 
     * @param dt Time since last update (seconds)
     * @param field_energy Current field energy level
     * @param gamma_power Processing intensity (energy variance)
     * @return Current conversational state
     */
    State update(float dt, float field_energy, float gamma_power);
    
    /**
     * @brief Check if should be speaking
     */
    bool should_speak() const { return current_state_ == State::SPEAKING; }
    
    /**
     * @brief Check if should be listening
     */
    bool should_listen() const { return current_state_ == State::LISTENING; }
    
    /**
     * @brief Get current theta phase (for debugging)
     */
    float get_theta_phase() const { return theta_phase_; }
    
    /**
     * @brief Get current gamma power
     */
    float get_gamma_power() const { return gamma_power_; }
    
    /**
     * @brief Force a state (for external control)
     */
    void force_state(State state) { 
        current_state_ = state; 
        transition_cooldown_ = 0.5f;  // 500ms cooldown
    }
    
    /**
     * @brief Set theta frequency (default 6 Hz)
     */
    void set_theta_frequency(float hz) { theta_frequency_ = hz; }
    
    /**
     * @brief Set transition thresholds
     */
    void set_speech_threshold(float t) { speech_threshold_ = t; }
    void set_listen_threshold(float t) { listen_threshold_ = t; }
    void set_energy_threshold(float t) { energy_threshold_ = t; }
    void set_min_state_duration(float d) { min_state_duration_ = d; }
    
    State get_state() const { return current_state_; }
    
private:
    State current_state_;
    State previous_state_;
    
    // Theta oscillation (4-8 Hz for turn-taking rhythm)
    float theta_phase_;           // Radians (0 to 2π)
    float theta_frequency_;       // Hz
    
    // Gamma power (30-80 Hz, represents processing intensity)
    float gamma_power_;           // 0.0 to 1.0
    
    // Thresholds for state transitions
    float speech_threshold_;      // Theta value to start speaking
    float listen_threshold_;      // Theta value to start listening
    float energy_threshold_;      // Minimum field energy to speak
    
    // Transition cooldown (prevent rapid switching)
    float transition_cooldown_;
    float min_state_duration_;    // Minimum time in a state (seconds)
    float time_in_current_state_;
    
    // Helper methods
    float compute_theta_value() const;
    bool should_transition_to_speaking(float theta_val, float field_energy);
    bool should_transition_to_listening(float theta_val);
};

} // namespace cognitive
} // namespace melvin

#endif // MELVIN_TURN_TAKING_CONTROLLER_H

