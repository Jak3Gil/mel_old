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
#include <random>

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

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// CONTINUOUS EVOLUTION IMPLEMENTATION
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

std::vector<std::pair<std::string, float*>> DynamicGenome::get_all_gene_ptrs() {
    return {
        // Activation field genes
        {"global_decay_rate", &reasoning_params_.global_decay_rate},
        {"activation_threshold", &reasoning_params_.activation_threshold},
        {"spreading_factor", &reasoning_params_.spreading_factor},
        {"max_activation", &reasoning_params_.max_activation},
        {"kwta_sparsity", &reasoning_params_.kwta_sparsity},
        
        // Hebbian learning genes
        {"hebbian_learning_rate", &reasoning_params_.hebbian_learning_rate},
        {"anti_hebbian_rate", &reasoning_params_.anti_hebbian_rate},
        {"min_edge_weight", &reasoning_params_.min_edge_weight},
        {"max_edge_weight", &reasoning_params_.max_edge_weight},
        {"weight_decay", &reasoning_params_.weight_decay},
        
        // Scoring genes
        {"activation_weight", &reasoning_params_.activation_weight},
        {"semantic_bias_weight", &reasoning_params_.semantic_bias_weight},
        {"coherence_weight", &reasoning_params_.coherence_weight},
        {"recency_weight", &reasoning_params_.recency_weight},
        {"novelty_weight", &reasoning_params_.novelty_weight},
        
        // Meta-learning genes
        {"learning_rate", &reasoning_params_.learning_rate},
        {"adaptation_rate", &reasoning_params_.adaptation_rate},
        {"confidence_decay", &reasoning_params_.confidence_decay},
        {"mutation_rate", &reasoning_params_.mutation_rate},
        {"mutation_magnitude", &reasoning_params_.mutation_magnitude},
        
        // Traversal genes
        {"temperature", &reasoning_params_.temperature},
        {"confidence_threshold", &reasoning_params_.confidence_threshold},
        {"semantic_threshold", &reasoning_params_.semantic_threshold},
        {"hop_decay", &reasoning_params_.hop_decay},
        
        // Mode switching genes
        {"exploratory_threshold", &reasoning_params_.exploratory_threshold},
        {"exploitative_threshold", &reasoning_params_.exploitative_threshold},
        {"deep_reasoning_threshold", &reasoning_params_.deep_reasoning_threshold},
        {"mode_switching_hysteresis", &reasoning_params_.mode_switching_hysteresis},
        
        // Working memory genes
        {"base_decay_rate", &reasoning_params_.base_decay_rate},
        {"confidence_decay_factor", &reasoning_params_.confidence_decay_factor},
        {"salience_threshold", &reasoning_params_.salience_threshold},
        {"wm_refresh_boost", &reasoning_params_.wm_refresh_boost},
        
        // Multi-modal genes
        {"text_modality_weight", &reasoning_params_.text_modality_weight},
        {"vision_modality_weight", &reasoning_params_.vision_modality_weight},
        {"audio_modality_weight", &reasoning_params_.audio_modality_weight},
        
        // Conversational genes
        {"theta_frequency", &reasoning_params_.theta_frequency},
        {"speech_threshold", &reasoning_params_.speech_threshold},
        {"listen_threshold", &reasoning_params_.listen_threshold},
        {"energy_threshold", &reasoning_params_.energy_threshold},
        {"min_state_duration", &reasoning_params_.min_state_duration},
        
        // Emotional genes
        {"base_tempo", &reasoning_params_.base_tempo},
        {"novelty_tempo_scale", &reasoning_params_.novelty_tempo_scale},
        {"confidence_tempo_scale", &reasoning_params_.confidence_tempo_scale},
        {"arousal_pitch_scale", &reasoning_params_.arousal_pitch_scale},
        {"hedge_confidence_threshold", &reasoning_params_.hedge_confidence_threshold},
        
        // Goal stack genes
        {"goal_decay_rate", &reasoning_params_.goal_decay_rate},
        {"goal_reactivation_boost", &reasoning_params_.goal_reactivation_boost},
        {"goal_overlap_threshold", &reasoning_params_.goal_overlap_threshold},
        {"min_goal_importance", &reasoning_params_.min_goal_importance},
        
        // Baseline activity genes
        {"baseline_activity_min", &reasoning_params_.baseline_activity_min},
        {"baseline_activity_max", &reasoning_params_.baseline_activity_max},
        {"baseline_adaptation_rate", &reasoning_params_.baseline_adaptation_rate},
        {"curiosity_baseline_scale", &reasoning_params_.curiosity_baseline_scale},
        {"boredom_baseline_scale", &reasoning_params_.boredom_baseline_scale},
        {"baseline_decay_multiplier", &reasoning_params_.baseline_decay_multiplier},
        {"baseline_power_budget", &reasoning_params_.baseline_power_budget},
        {"dmn_cycle_period", &reasoning_params_.dmn_cycle_period},
        {"introspection_bias", &reasoning_params_.introspection_bias},
        {"novelty_exploration_weight", &reasoning_params_.novelty_exploration_weight},
    };
}

void DynamicGenome::set_gene(const std::string& name, float value) {
    auto genes = get_all_gene_ptrs();
    for (auto& [gene_name, gene_ptr] : genes) {
        if (gene_name == name) {
            *gene_ptr = value;
            return;
        }
    }
}

float DynamicGenome::get_gene(const std::string& name) const {
    // Copy to avoid const issues
    auto self = const_cast<DynamicGenome*>(this);
    auto genes = self->get_all_gene_ptrs();
    for (auto& [gene_name, gene_ptr] : genes) {
        if (gene_name == name) {
            return *gene_ptr;
        }
    }
    return 0.0f;
}

void DynamicGenome::mutate_random_genes(int count) {
    auto genes = get_all_gene_ptrs();
    
    // Seed random once
    static std::random_device rd;
    static std::mt19937 rng(rd());
    
    // Select random genes
    std::uniform_int_distribution<int> gene_dist(0, genes.size() - 1);
    std::uniform_real_distribution<float> mutation_dist(-1.0f, 1.0f);
    
    for (int i = 0; i < count; i++) {
        int idx = gene_dist(rng);
        auto& [name, ptr] = genes[idx];
        
        // Mutate by mutation_magnitude
        float delta = mutation_dist(rng) * reasoning_params_.mutation_magnitude;
        *ptr += delta;
        
        // Clamp to reasonable bounds (0.001 to 10.0 for most params)
        *ptr = std::max(0.001f, std::min(10.0f, *ptr));
    }
    
    // Re-normalize weights
    reasoning_params_.normalize_weights();
    reasoning_params_.normalize_modality_weights();
}

void DynamicGenome::evolve_towards_intelligence(float dt) {
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // CONTINUOUS SELF-IMPROVEMENT (no external prompt)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    // Goal: Increase processing efficiency and learning capacity
    
    // 1. Increase Hebbian learning rate (learn faster connections)
    reasoning_params_.hebbian_learning_rate += 0.0001f * dt;
    reasoning_params_.hebbian_learning_rate = std::min(0.05f, reasoning_params_.hebbian_learning_rate);
    
    // 2. Increase spreading factor (better activation propagation)
    reasoning_params_.spreading_factor += 0.0001f * dt;
    reasoning_params_.spreading_factor = std::min(0.95f, reasoning_params_.spreading_factor);
    
    // 3. Decrease global decay (retain information longer)
    reasoning_params_.global_decay_rate *= (1.0f - 0.0001f * dt);
    reasoning_params_.global_decay_rate = std::max(0.01f, reasoning_params_.global_decay_rate);
    
    // 4. Increase novelty weight (seek new information)
    reasoning_params_.novelty_weight += 0.0001f * dt;
    reasoning_params_.novelty_weight = std::min(0.5f, reasoning_params_.novelty_weight);
    
    // 5. Increase working memory slots (more capacity)
    if (dt > 10.0f) {  // Every 10 seconds
        reasoning_params_.working_memory_slots = std::min(7, reasoning_params_.working_memory_slots + 1);
    }
    
    // 6. Increase max active nodes (richer representations)
    reasoning_params_.max_active_nodes += static_cast<int>(0.1f * dt);
    reasoning_params_.max_active_nodes = std::min(2000, reasoning_params_.max_active_nodes);
    
    // 7. Random exploration (mutation)
    static float mutation_timer = 0.0f;
    mutation_timer += dt;
    if (mutation_timer > 30.0f) {  // Every 30 seconds, try random mutations
        mutate_random_genes(2);
        mutation_timer = 0.0f;
    }
    
    // Re-normalize weights
    reasoning_params_.normalize_weights();
}

} // namespace evolution
} // namespace melvin

