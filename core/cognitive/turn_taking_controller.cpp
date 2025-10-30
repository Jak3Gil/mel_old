/**
 * @file turn_taking_controller.cpp
 * @brief Implementation of turn-taking controller
 */

#include "turn_taking_controller.h"
#include <iostream>

namespace melvin {
namespace cognitive {

TurnTakingController::TurnTakingController()
    : current_state_(State::LISTENING)
    , previous_state_(State::LISTENING)
    , theta_phase_(0.0f)
    , theta_frequency_(6.0f)  // 6 Hz theta
    , gamma_power_(0.0f)
    , speech_threshold_(0.65f)
    , listen_threshold_(-0.3f)
    , energy_threshold_(0.5f)
    , transition_cooldown_(0.0f)
    , min_state_duration_(0.8f)  // At least 800ms per state
    , time_in_current_state_(0.0f)
{
}

TurnTakingController::State TurnTakingController::update(
    float dt, 
    float field_energy, 
    float gamma_power
) {
    // Update timers
    time_in_current_state_ += dt;
    if (transition_cooldown_ > 0.0f) {
        transition_cooldown_ -= dt;
    }
    
    // Update theta oscillation (rhythmic turn-taking)
    theta_phase_ += dt * theta_frequency_ * 2.0f * M_PI;
    if (theta_phase_ > 2.0f * M_PI) {
        theta_phase_ -= 2.0f * M_PI;
    }
    
    // Update gamma power (processing intensity)
    gamma_power_ = 0.9f * gamma_power_ + 0.1f * gamma_power;
    
    // Get theta value (-1 to 1)
    float theta_val = compute_theta_value();
    
    // Can't transition if in cooldown or min duration not met
    if (transition_cooldown_ > 0.0f || time_in_current_state_ < min_state_duration_) {
        return current_state_;
    }
    
    previous_state_ = current_state_;
    
    // State machine
    switch (current_state_) {
        case State::LISTENING:
            // Transition to speaking when theta peaks AND we have ideas
            if (should_transition_to_speaking(theta_val, field_energy)) {
                current_state_ = State::TRANSITIONING;
                std::cout << "🎤 [Turn-Taking] Taking turn to speak (θ=" 
                         << theta_val << ", E=" << field_energy << ")\n";
                time_in_current_state_ = 0.0f;
            }
            break;
            
        case State::SPEAKING:
            // Transition to listening when theta troughs OR energy depleted
            if (should_transition_to_listening(theta_val) || field_energy < 0.2f) {
                current_state_ = State::TRANSITIONING;
                std::cout << "👂 [Turn-Taking] Yielding turn to listen (θ=" 
                         << theta_val << ", E=" << field_energy << ")\n";
                time_in_current_state_ = 0.0f;
            }
            break;
            
        case State::TRANSITIONING:
            // Complete transition when theta crosses zero
            if (std::abs(theta_val) < 0.15f) {
                // Determine new state based on previous
                if (previous_state_ == State::LISTENING) {
                    current_state_ = State::SPEAKING;
                    std::cout << "   ✅ Transition complete → SPEAKING\n";
                } else {
                    current_state_ = State::LISTENING;
                    std::cout << "   ✅ Transition complete → LISTENING\n";
                }
                transition_cooldown_ = 0.5f;  // 500ms cooldown
                time_in_current_state_ = 0.0f;
            }
            break;
    }
    
    return current_state_;
}

float TurnTakingController::compute_theta_value() const {
    return std::sin(theta_phase_);
}

bool TurnTakingController::should_transition_to_speaking(
    float theta_val, 
    float field_energy
) {
    // Need: theta peak + sufficient field energy + sufficient gamma power
    return theta_val > speech_threshold_ && 
           field_energy > energy_threshold_ &&
           gamma_power_ > 0.4f;
}

bool TurnTakingController::should_transition_to_listening(float theta_val) {
    // Transition when theta troughs OR gamma power drops
    return theta_val < listen_threshold_ || gamma_power_ < 0.15f;
}

} // namespace cognitive
} // namespace melvin

