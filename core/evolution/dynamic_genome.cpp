/**
 * @file dynamic_genome.cpp
 * @brief Implementation of dynamic genome with adaptive parameters
 */

#include "dynamic_genome.h"
#include "../cognitive/turn_taking_controller.h"
#include "../cognitive/emotional_modulator.h"
#include "../cognitive/conversation_goal_stack.h"
#include <fstream>
#include <algorithm>
#include <cmath>

namespace melvin {
namespace evolution {

DynamicGenome::DynamicGenome() {
    reasoning_params_.normalize_weights();
    reasoning_params_.normalize_modality_weights();
}

void DynamicGenome::log_reasoning_episode(
    const std::string& query_type,
    float confidence,
    float coherence,
    bool success
) {
    // Update profile for this query type
    auto& profile = profiles_[query_type];
    profile.query_type = query_type;
    profile.update_from_episode(confidence, coherence, success);
    
    // If this was successful, save current params as best for this type
    if (success && confidence > profile.avg_confidence) {
        profile.best_params = reasoning_params_;
    }
    
    // Update recent history
    recent_confidences_.push_back(confidence);
    recent_successes_.push_back(success);
    
    if (recent_confidences_.size() > HISTORY_SIZE) {
        prune_history();
    }
}

const ReasoningProfile* DynamicGenome::get_profile(const std::string& query_type) const {
    auto it = profiles_.find(query_type);
    if (it != profiles_.end()) {
        return &it->second;
    }
    return nullptr;
}

void DynamicGenome::apply_profile(const std::string& query_type) {
    auto* profile = get_profile(query_type);
    if (profile && profile->num_attempts > 5) {
        // Use learned best parameters for this query type
        reasoning_params_ = profile->best_params;
    }
}

void DynamicGenome::tune_from_feedback(
    float confidence,
    float coherence,
    bool success
) {
    float rate = reasoning_params_.adaptation_rate;
    
    if (success) {
        // Reinforce current parameter balance
        if (confidence > 0.7f) {
            // High confidence success - current weights are good
            // Slightly increase whatever was highest
            float max_weight = std::max({
                reasoning_params_.activation_weight,
                reasoning_params_.semantic_bias_weight,
                reasoning_params_.coherence_weight
            });
            
            if (max_weight == reasoning_params_.semantic_bias_weight) {
                reasoning_params_.semantic_bias_weight += rate;
            } else if (max_weight == reasoning_params_.activation_weight) {
                reasoning_params_.activation_weight += rate;
            } else {
                reasoning_params_.coherence_weight += rate;
            }
        }
        
        // Increase learning rate slightly (we're learning well)
        reasoning_params_.learning_rate *= 1.01f;
        reasoning_params_.learning_rate = std::min(reasoning_params_.learning_rate, 0.1f);
    } else {
        // Failure - adjust strategy
        if (confidence < 0.3f) {
            // Low confidence failure - need more exploration
            reasoning_params_.temperature *= 1.1f;
            reasoning_params_.semantic_bias_weight += rate;  // Try semantic search
        } else {
            // High confidence failure - wrong reasoning path
            reasoning_params_.coherence_weight += rate;  // Focus on path quality
            reasoning_params_.activation_weight -= rate;  // Less trust in raw activation
        }
        
        // Decrease learning rate (unstable)
        reasoning_params_.learning_rate *= 0.99f;
        reasoning_params_.learning_rate = std::max(reasoning_params_.learning_rate, 0.001f);
    }
    
    reasoning_params_.normalize_weights();
}

void DynamicGenome::adapt_temperature(float current_confidence) {
    float target_temp;
    
    if (current_confidence < reasoning_params_.exploratory_threshold) {
        // Low confidence - explore more
        target_temp = 1.5f;
    } else if (current_confidence > reasoning_params_.exploitative_threshold) {
        // High confidence - exploit
        target_temp = 0.5f;
    } else {
        // Moderate confidence - balanced
        target_temp = 1.0f;
    }
    
    // Smooth adaptation
    reasoning_params_.temperature += 
        reasoning_params_.adaptation_rate * (target_temp - reasoning_params_.temperature);
    
    // Clamp
    reasoning_params_.temperature = std::max(0.1f, std::min(2.0f, reasoning_params_.temperature));
}

void DynamicGenome::adapt_thresholds(float avg_path_length) {
    // If paths are getting very long, we might be wandering
    if (avg_path_length > 10.0f) {
        reasoning_params_.confidence_threshold += 0.01f;  // Be more selective
        reasoning_params_.semantic_threshold += 0.01f;
    }
    // If paths are very short, we might be too restrictive
    else if (avg_path_length < 2.0f) {
        reasoning_params_.confidence_threshold -= 0.01f;  // Be more permissive
        reasoning_params_.semantic_threshold -= 0.01f;
    }
    
    // Clamp thresholds
    reasoning_params_.confidence_threshold = 
        std::max(0.1f, std::min(0.9f, reasoning_params_.confidence_threshold));
    reasoning_params_.semantic_threshold = 
        std::max(0.1f, std::min(0.8f, reasoning_params_.semantic_threshold));
}

void DynamicGenome::save(const std::string& filepath) const {
    std::ofstream file(filepath, std::ios::binary);
    if (!file) return;
    
    // Write params
    file.write(reinterpret_cast<const char*>(&reasoning_params_), sizeof(DynamicReasoningParams));
    
    // Write profiles
    size_t num_profiles = profiles_.size();
    file.write(reinterpret_cast<const char*>(&num_profiles), sizeof(size_t));
    
    for (const auto& [type, profile] : profiles_) {
        size_t type_len = type.length();
        file.write(reinterpret_cast<const char*>(&type_len), sizeof(size_t));
        file.write(type.c_str(), type_len);
        file.write(reinterpret_cast<const char*>(&profile), sizeof(ReasoningProfile));
    }
    
    file.close();
}

void DynamicGenome::load(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return;
    
    // Read params
    file.read(reinterpret_cast<char*>(&reasoning_params_), sizeof(DynamicReasoningParams));
    
    // Read profiles
    size_t num_profiles;
    file.read(reinterpret_cast<char*>(&num_profiles), sizeof(size_t));
    
    profiles_.clear();
    for (size_t i = 0; i < num_profiles; i++) {
        size_t type_len;
        file.read(reinterpret_cast<char*>(&type_len), sizeof(size_t));
        
        std::string type(type_len, '\0');
        file.read(&type[0], type_len);
        
        ReasoningProfile profile;
        file.read(reinterpret_cast<char*>(&profile), sizeof(ReasoningProfile));
        
        profiles_[type] = profile;
    }
    
    file.close();
}

void DynamicGenome::prune_history() {
    // Keep only most recent entries
    size_t to_remove = recent_confidences_.size() - HISTORY_SIZE;
    recent_confidences_.erase(recent_confidences_.begin(), 
                             recent_confidences_.begin() + to_remove);
    recent_successes_.erase(recent_successes_.begin(),
                           recent_successes_.begin() + to_remove);
}

float DynamicGenome::compute_recent_success_rate() const {
    if (recent_successes_.empty()) return 0.5f;
    
    int successes = 0;
    for (bool success : recent_successes_) {
        if (success) successes++;
    }
    
    return static_cast<float>(successes) / recent_successes_.size();
}

// ====================================================================
// CONVERSATIONAL GENOME APPLICATION
// ====================================================================

void DynamicReasoningParams::apply_to_turn_taking(void* controller_ptr) const {
    auto* controller = static_cast<melvin::cognitive::TurnTakingController*>(controller_ptr);
    
    // Apply turn-taking parameters from genome
    controller->set_theta_frequency(theta_frequency);
    controller->set_speech_threshold(speech_threshold);
    controller->set_listen_threshold(listen_threshold);
    controller->set_energy_threshold(energy_threshold);
    controller->set_min_state_duration(min_state_duration);
}

void DynamicReasoningParams::apply_to_emotional_modulator(void* modulator_ptr) const {
    auto* modulator = static_cast<melvin::cognitive::EmotionalModulator*>(modulator_ptr);
    
    // Apply emotional modulation parameters from genome
    modulator->set_base_tempo(base_tempo);
    modulator->set_novelty_tempo_scale(novelty_tempo_scale);
    modulator->set_confidence_tempo_scale(confidence_tempo_scale);
    modulator->set_arousal_pitch_scale(arousal_pitch_scale);
    modulator->set_hedge_confidence_threshold(hedge_confidence_threshold);
}

void DynamicReasoningParams::apply_to_goal_stack(void* goal_stack_ptr) const {
    auto* goal_stack = static_cast<melvin::cognitive::ConversationGoalStack*>(goal_stack_ptr);
    
    // Apply goal stack parameters from genome
    goal_stack->set_decay_rate(goal_decay_rate);
    goal_stack->set_reactivation_boost(goal_reactivation_boost);
    goal_stack->set_overlap_threshold(goal_overlap_threshold);
    goal_stack->set_max_turns_inactive(max_turns_inactive);
    goal_stack->set_min_importance(min_goal_importance);
    goal_stack->set_max_context_nodes(max_context_nodes);
}

} // namespace evolution
} // namespace melvin

