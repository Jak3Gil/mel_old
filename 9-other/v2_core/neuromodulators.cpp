#include "neuromodulators.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace melvin::v2 {

// ============================================================================
// NEUROMODULATORS IMPLEMENTATION
// ============================================================================

Neuromodulators::Neuromodulators(const evolution::Genome& genome)
    : config_(extract_neuromod_config(genome)),
      sum_da_(0.0f), sum_ne_(0.0f), sum_ach_(0.0f), sum_5ht_(0.0f),
      sample_count_(0) {
    
    // Initialize state to baselines
    state_.dopamine = config_.da_baseline;
    state_.norepinephrine = config_.ne_baseline;
    state_.acetylcholine = config_.ach_baseline;
    state_.serotonin = config_.serotonin_baseline;
    state_.last_update = get_timestamp_ns();
    
    stats_ = Stats{};
}

Neuromodulators::Neuromodulators(const Config& config)
    : config_(config),
      sum_da_(0.0f), sum_ne_(0.0f), sum_ach_(0.0f), sum_5ht_(0.0f),
      sample_count_(0) {
    
    state_.dopamine = config_.da_baseline;
    state_.norepinephrine = config_.ne_baseline;
    state_.acetylcholine = config_.ach_baseline;
    state_.serotonin = config_.serotonin_baseline;
    state_.last_update = get_timestamp_ns();
    
    stats_ = Stats{};
}

Neuromodulators::~Neuromodulators() = default;

// ========================================================================
// EVENT-DRIVEN UPDATES
// ========================================================================

void Neuromodulators::on_prediction_error(float error) {
    // Dopamine encodes reward prediction error (RPE)
    // Positive error → DA burst (unexpected reward)
    // Negative error → DA dip (unexpected punishment)
    float da_change = config_.da_gain * error;
    state_.dopamine += da_change;
    
    // Large absolute error → unexpected uncertainty → NE burst
    float abs_error = std::abs(error);
    if (abs_error > 0.5f) {
        float ne_change = config_.ne_gain * (abs_error - 0.5f);
        state_.norepinephrine += ne_change;
    }
    
    clamp_all();
    
    stats_.total_prediction_errors += abs_error;
    state_.last_update = get_timestamp_ns();
}

void Neuromodulators::on_unexpected_event(float surprise_magnitude) {
    // Norepinephrine signals unexpected uncertainty
    // Triggers exploration, attention reset
    float ne_change = config_.ne_gain * surprise_magnitude;
    state_.norepinephrine += ne_change;
    
    clamp_all();
    
    stats_.total_unexpected_events += surprise_magnitude;
    state_.last_update = get_timestamp_ns();
}

void Neuromodulators::on_expected_uncertainty(float ambiguity) {
    // Acetylcholine signals expected uncertainty
    // Sharpens bottom-up attention to resolve ambiguity
    float ach_change = config_.ach_gain * ambiguity;
    state_.acetylcholine += ach_change;
    
    clamp_all();
    state_.last_update = get_timestamp_ns();
}

void Neuromodulators::on_goal_progress(float progress_delta) {
    // Positive progress → DA + 5-HT boost (reward + stability)
    // Negative progress → 5-HT dip (less stable, try new strategies)
    
    if (progress_delta > 0.0f) {
        state_.dopamine += config_.da_gain * progress_delta * 0.5f;
        state_.serotonin += config_.serotonin_gain * progress_delta * 0.3f;
    } else {
        state_.serotonin += config_.serotonin_gain * progress_delta * 0.5f;
    }
    
    clamp_all();
    state_.last_update = get_timestamp_ns();
}

void Neuromodulators::on_conflict(float conflict_magnitude) {
    // High conflict → reduce serotonin (less patient/stable)
    // Also slight NE boost (uncertainty about what to do)
    state_.serotonin -= config_.serotonin_gain * conflict_magnitude * 0.3f;
    state_.norepinephrine += config_.ne_gain * conflict_magnitude * 0.2f;
    
    clamp_all();
    
    stats_.total_conflicts += conflict_magnitude;
    state_.last_update = get_timestamp_ns();
}

// ========================================================================
// STATE ACCESS
// ========================================================================

NeuromodState Neuromodulators::get_state() const {
    return state_;
}

// ========================================================================
// COMPUTED EFFECTS
// ========================================================================

float Neuromodulators::get_plasticity_rate() const {
    // Dopamine gates plasticity
    // High DA → learn quickly from rewards
    // Low DA → slower learning / active forgetting
    
    // Map [0, 1] DA to [0.1, 2.0] plasticity multiplier
    float plasticity_mult = 0.1f + state_.dopamine * 1.9f;
    return plasticity_mult;
}

float Neuromodulators::get_exploration_bias() const {
    // Norepinephrine controls exploration
    // High NE → explore more, widen attention
    // Low NE → exploit, narrow focus
    
    // Map [0, 1] NE to [-0.5, 0.5] exploration bias
    float exploration = (state_.norepinephrine - 0.5f);
    return exploration;
}

float Neuromodulators::get_attention_gain() const {
    // Acetylcholine boosts bottom-up salience
    // High ACh → sharper sensory processing
    // Low ACh → dampen sensory input
    
    // Map [0, 1] ACh to [0.5, 1.5] attention multiplier
    float attention_mult = 0.5f + state_.acetylcholine;
    return attention_mult;
}

float Neuromodulators::get_stability_bias() const {
    // Serotonin suppresses impulsive switching
    // High 5-HT → stable, patient
    // Low 5-HT → impulsive, willing to switch
    
    // Map [0, 1] 5-HT to [0.5, 2.0] inertia multiplier
    float inertia_mult = 0.5f + state_.serotonin * 1.5f;
    return inertia_mult;
}

float Neuromodulators::get_effective_learning_rate(float base_rate) const {
    // Combine plasticity rate with base learning rate
    return base_rate * get_plasticity_rate();
}

// ========================================================================
// TIME-BASED UPDATES
// ========================================================================

void Neuromodulators::tick(float dt) {
    // Apply homeostatic decay toward baselines
    apply_decay(dt);
    
    // Update statistics
    sum_da_ += state_.dopamine;
    sum_ne_ += state_.norepinephrine;
    sum_ach_ += state_.acetylcholine;
    sum_5ht_ += state_.serotonin;
    sample_count_++;
    
    stats_.avg_dopamine = sum_da_ / sample_count_;
    stats_.avg_norepinephrine = sum_ne_ / sample_count_;
    stats_.avg_acetylcholine = sum_ach_ / sample_count_;
    stats_.avg_serotonin = sum_5ht_ / sample_count_;
    stats_.tick_count++;
}

void Neuromodulators::reset() {
    state_.dopamine = config_.da_baseline;
    state_.norepinephrine = config_.ne_baseline;
    state_.acetylcholine = config_.ach_baseline;
    state_.serotonin = config_.serotonin_baseline;
    state_.last_update = get_timestamp_ns();
}

// ========================================================================
// CONFIGURATION
// ========================================================================

void Neuromodulators::set_config(const Config& config) {
    config_ = config;
    clamp_all();
}

Neuromodulators::Config Neuromodulators::get_config() const {
    return config_;
}

// ========================================================================
// STATISTICS
// ========================================================================

Neuromodulators::Stats Neuromodulators::get_stats() const {
    return stats_;
}

void Neuromodulators::reset_stats() {
    stats_ = Stats{};
    sum_da_ = 0.0f;
    sum_ne_ = 0.0f;
    sum_ach_ = 0.0f;
    sum_5ht_ = 0.0f;
    sample_count_ = 0;
}

// ========================================================================
// PRIVATE HELPERS
// ========================================================================

void Neuromodulators::clamp_all() {
    state_.dopamine = std::clamp(state_.dopamine, config_.min_level, config_.max_level);
    state_.norepinephrine = std::clamp(state_.norepinephrine, config_.min_level, config_.max_level);
    state_.acetylcholine = std::clamp(state_.acetylcholine, config_.min_level, config_.max_level);
    state_.serotonin = std::clamp(state_.serotonin, config_.min_level, config_.max_level);
}

void Neuromodulators::apply_decay(float dt) {
    // Exponential decay toward baseline
    // x(t+dt) = x(t) + (baseline - x(t)) * (1 - decay^dt)
    
    float da_decay_factor = std::pow(config_.da_decay, dt);
    float ne_decay_factor = std::pow(config_.ne_decay, dt);
    float ach_decay_factor = std::pow(config_.ach_decay, dt);
    float serotonin_decay_factor = std::pow(config_.serotonin_decay, dt);
    
    state_.dopamine = config_.da_baseline + (state_.dopamine - config_.da_baseline) * da_decay_factor;
    state_.norepinephrine = config_.ne_baseline + (state_.norepinephrine - config_.ne_baseline) * ne_decay_factor;
    state_.acetylcholine = config_.ach_baseline + (state_.acetylcholine - config_.ach_baseline) * ach_decay_factor;
    state_.serotonin = config_.serotonin_baseline + (state_.serotonin - config_.serotonin_baseline) * serotonin_decay_factor;
    
    clamp_all();
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

Neuromodulators::Config extract_neuromod_config(const evolution::Genome& genome) {
    Neuromodulators::Config config;
    
    // Extract from genome
    auto module = genome.get_module("neuromodulators");
    if (!module) {
        // Return defaults if no neuromod module
        return config;
    }
    
    // Baselines
    if (auto gene = module->get_gene("da_baseline"))
        config.da_baseline = gene->value;
    if (auto gene = module->get_gene("ne_baseline"))
        config.ne_baseline = gene->value;
    if (auto gene = module->get_gene("ach_baseline"))
        config.ach_baseline = gene->value;
    if (auto gene = module->get_gene("serotonin_baseline"))
        config.serotonin_baseline = gene->value;
    
    // Gains
    if (auto gene = module->get_gene("da_gain"))
        config.da_gain = gene->value;
    if (auto gene = module->get_gene("ne_gain"))
        config.ne_gain = gene->value;
    if (auto gene = module->get_gene("ach_gain"))
        config.ach_gain = gene->value;
    if (auto gene = module->get_gene("serotonin_gain"))
        config.serotonin_gain = gene->value;
    
    // Decays
    if (auto gene = module->get_gene("da_decay"))
        config.da_decay = gene->value;
    if (auto gene = module->get_gene("ne_decay"))
        config.ne_decay = gene->value;
    if (auto gene = module->get_gene("ach_decay"))
        config.ach_decay = gene->value;
    if (auto gene = module->get_gene("serotonin_decay"))
        config.serotonin_decay = gene->value;
    
    return config;
}

std::string neuromod_state_to_string(const NeuromodState& state) {
    std::ostringstream oss;
    oss << "Neuromod[";
    oss << "DA=" << std::fixed << std::setprecision(2) << state.dopamine;
    oss << " NE=" << state.norepinephrine;
    oss << " ACh=" << state.acetylcholine;
    oss << " 5-HT=" << state.serotonin;
    oss << "]";
    return oss.str();
}

} // namespace melvin::v2

