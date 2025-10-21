#include "arbitration.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace melvin::v2::attention {

// ============================================================================
// ATTENTION ARBITRATION IMPLEMENTATION
// ============================================================================

AttentionArbitration::AttentionArbitration(const evolution::Genome& genome)
    : config_(extract_attention_config(genome)),
      current_focus_(0), current_focus_score_(0.0f), focus_start_time_(0) {
    stats_ = Stats{};
}

AttentionArbitration::AttentionArbitration(const Config& config)
    : config_(config), current_focus_(0), current_focus_score_(0.0f), focus_start_time_(0) {
    stats_ = Stats{};
}

NodeID AttentionArbitration::select_focus(
    const std::vector<PerceivedObject>& candidates,
    const std::vector<float>& saliency,
    const std::vector<float>& relevance,
    const NeuromodState& neuromod) {
    
    if (candidates.empty()) {
        return 0;
    }
    
    if (saliency.size() != candidates.size() || relevance.size() != candidates.size()) {
        return 0;  // Mismatched sizes
    }
    
    Timestamp current_time = get_timestamp_ns();
    
    // Compute scores for all candidates
    std::vector<float> scores;
    
    for (size_t i = 0; i < candidates.size(); ++i) {
        float score = compute_score(candidates[i], saliency[i], relevance[i], neuromod);
        scores.push_back(score);
    }
    
    // Apply inertia (if already focused)
    if (current_focus_ != 0) {
        // Find current focus in candidates
        int current_idx = -1;
        for (size_t i = 0; i < candidates.size(); ++i) {
            if (candidates[i].object_id == current_focus_) {
                current_idx = static_cast<int>(i);
                break;
            }
        }
        
        if (current_idx >= 0) {
            // Apply inertia: new candidate must score > current × threshold
            float threshold = current_focus_score_ * config_.inertia_threshold;
            threshold *= neuromod.serotonin;  // 5-HT modulates stability
            
            bool any_better = false;
            for (size_t i = 0; i < scores.size(); ++i) {
                if (static_cast<int>(i) != current_idx && scores[i] > threshold) {
                    any_better = true;
                    break;
                }
            }
            
            if (!any_better) {
                // No candidate beats threshold - maintain current focus
                stats_.inertia_holds++;
                return current_focus_;
            }
        }
    }
    
    // Select via softmax sampling
    int selected_idx = static_cast<int>(softmax_sample(scores));
    
    if (selected_idx < 0 || selected_idx >= static_cast<int>(candidates.size())) {
        return 0;
    }
    
    NodeID selected = candidates[selected_idx].object_id;
    float selected_score = scores[selected_idx];
    
    // Check if switching focus
    if (selected != current_focus_) {
        // Switching!
        float duration = current_focus_ != 0 
            ? timestamp_to_seconds(current_time - focus_start_time_) 
            : 0.0f;
        
        update_history(current_focus_, duration);
        
        current_focus_ = selected;
        current_focus_score_ = selected_score;
        focus_start_time_ = current_time;
        
        stats_.switches++;
    }
    
    stats_.total_selections++;
    
    return selected;
}

float AttentionArbitration::get_focus_duration() const {
    if (current_focus_ == 0 || focus_start_time_ == 0) {
        return 0.0f;
    }
    
    return timestamp_to_seconds(get_timestamp_ns() - focus_start_time_);
}

void AttentionArbitration::force_switch(NodeID new_focus) {
    if (current_focus_ != 0) {
        float duration = get_focus_duration();
        update_history(current_focus_, duration);
    }
    
    current_focus_ = new_focus;
    current_focus_score_ = 1.0f;
    focus_start_time_ = get_timestamp_ns();
    stats_.switches++;
}

void AttentionArbitration::reset() {
    current_focus_ = 0;
    current_focus_score_ = 0.0f;
    focus_start_time_ = 0;
    history_.clear();
}

void AttentionArbitration::set_config(const Config& config) {
    config_ = config;
}

AttentionArbitration::Config AttentionArbitration::get_config() const {
    return config_;
}

// ========================================================================
// STATISTICS
// ========================================================================

AttentionArbitration::Stats AttentionArbitration::get_stats() const {
    Stats stats = stats_;
    
    // Compute averages from history
    if (!history_.empty()) {
        float total_duration = 0.0f;
        for (const auto& h : history_) {
            total_duration += h.duration;
        }
        stats.avg_focus_duration = total_duration / history_.size();
    }
    
    if (stats_.switches > 0) {
        // Estimate based on total selections
        stats.avg_switch_interval = static_cast<float>(stats_.total_selections) / stats_.switches;
    }
    
    return stats;
}

void AttentionArbitration::reset_stats() {
    stats_ = Stats{};
}

// ========================================================================
// PRIVATE HELPERS
// ========================================================================

float AttentionArbitration::compute_score(
    const PerceivedObject& obj,
    float saliency,
    float relevance,
    const NeuromodState& neuromod) {
    
    // Base score (genome-weighted formula)
    float score = config_.alpha_saliency * saliency +
                 config_.beta_relevance * relevance +
                 config_.gamma_curiosity * obj.features.novelty +
                 config_.delta_need * 0.0f +  // Need not implemented yet
                 config_.epsilon_persistence * get_persistence_bonus(obj.object_id);
    
    // Neuromodulator influences
    // ACh boosts bottom-up (saliency)
    float ach_boost = (neuromod.acetylcholine - 0.5f) * saliency;
    score += ach_boost;
    
    // NE adds exploration noise
    float ne_noise = (neuromod.norepinephrine - 0.5f) * 0.1f;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    score += ne_noise * dist(gen);
    
    // Apply IOR penalty
    float ior_penalty = get_ior_penalty(obj.object_id, get_timestamp_ns());
    score -= ior_penalty;
    
    return std::max(0.0f, score);
}

float AttentionArbitration::get_ior_penalty(NodeID object_id, Timestamp current_time) const {
    // Inhibition of Return: suppress recently-focused objects
    
    for (const auto& h : history_) {
        if (h.object_id != object_id) continue;
        
        float elapsed = timestamp_to_seconds(current_time - h.focused_at);
        
        if (elapsed < config_.ior_duration) {
            // Within IOR window - apply penalty
            float decay = std::exp(-elapsed / config_.ior_duration);
            return 0.3f * decay;  // Max penalty = 0.3
        }
    }
    
    return 0.0f;
}

float AttentionArbitration::get_persistence_bonus(NodeID object_id) const {
    // Bonus for objects we've focused on before (continuity)
    
    int focus_count = 0;
    for (const auto& h : history_) {
        if (h.object_id == object_id) {
            focus_count++;
        }
    }
    
    // Diminishing returns
    return std::min(0.3f, focus_count * 0.1f);
}

float AttentionArbitration::softmax_sample(const std::vector<float>& scores) {
    if (scores.empty()) return -1;
    
    // Compute softmax probabilities
    std::vector<float> probs;
    float sum = 0.0f;
    
    for (float score : scores) {
        float p = std::exp(score / config_.softmax_temperature);
        probs.push_back(p);
        sum += p;
    }
    
    // Normalize
    for (float& p : probs) {
        p /= sum;
    }
    
    // Sample from distribution
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    float r = dist(gen);
    float cumulative = 0.0f;
    
    for (size_t i = 0; i < probs.size(); ++i) {
        cumulative += probs[i];
        if (r < cumulative) {
            return static_cast<float>(i);
        }
    }
    
    // Fallback: highest score
    auto max_it = std::max_element(scores.begin(), scores.end());
    return static_cast<float>(std::distance(scores.begin(), max_it));
}

void AttentionArbitration::update_history(NodeID object_id, float duration) {
    if (object_id == 0) return;
    
    FocusHistory entry;
    entry.object_id = object_id;
    entry.focused_at = get_timestamp_ns();
    entry.duration = duration;
    
    history_.push_back(entry);
    
    // Keep limited history
    if (history_.size() > MAX_HISTORY) {
        history_.erase(history_.begin());
    }
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

AttentionArbitration::Config extract_attention_config(const evolution::Genome& genome) {
    AttentionArbitration::Config config;
    
    auto module = genome.get_module("attention");
    if (!module) {
        return config;
    }
    
    // Extract weights
    if (auto gene = module->get_gene("alpha_saliency"))
        config.alpha_saliency = gene->value;
    if (auto gene = module->get_gene("beta_goal"))
        config.beta_relevance = gene->value;
    if (auto gene = module->get_gene("gamma_curiosity"))
        config.gamma_curiosity = gene->value;
    if (auto gene = module->get_gene("delta_need"))
        config.delta_need = gene->value;
    if (auto gene = module->get_gene("epsilon_persistence"))
        config.epsilon_persistence = gene->value;
    
    // Extract dynamics
    if (auto gene = module->get_gene("inertia_threshold"))
        config.inertia_threshold = gene->value;
    if (auto gene = module->get_gene("softmax_temperature"))
        config.softmax_temperature = gene->value;
    
    return config;
}

} // namespace melvin::v2::attention

