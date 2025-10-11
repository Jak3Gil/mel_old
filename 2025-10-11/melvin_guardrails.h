/*
 * MELVIN LLM TRAINING GUARDRAILS
 * 
 * Rolling windows, alert thresholds, and champion-challenger system
 * for rock-solid parameter optimization and auto-recovery.
 */

#pragma once

#include <deque>
#include <vector>
#include <string>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <iomanip>

namespace melvin {

// ==================== ROLLING WINDOWS ====================

template<typename T>
class RollingWindow {
private:
    std::deque<T> data_;
    size_t max_size_;
    
public:
    RollingWindow(size_t max_size) : max_size_(max_size) {}
    
    void add(const T& value) {
        data_.push_back(value);
        if (data_.size() > max_size_) {
            data_.pop_front();
        }
    }
    
    T average() const {
        if (data_.empty()) return T{};
        
        T sum = T{};
        for (const auto& val : data_) {
            sum += val;
        }
        return sum / static_cast<T>(data_.size());
    }
    
    T min() const {
        if (data_.empty()) return T{};
        
        T min_val = data_[0];
        for (const auto& val : data_) {
            if (val < min_val) min_val = val;
        }
        return min_val;
    }
    
    T max() const {
        if (data_.empty()) return T{};
        
        T max_val = data_[0];
        for (const auto& val : data_) {
            if (val > max_val) max_val = val;
        }
        return max_val;
    }
    
    size_t size() const { return data_.size(); }
    bool is_full() const { return data_.size() >= max_size_; }
    void clear() { data_.clear(); }
};

// ==================== ALERT THRESHOLDS ====================

struct GuardrailThresholds {
    // Rolling window sizes
    size_t W_short = 1000;    // 1k steps
    size_t W_mid = 10000;     // 10k steps  
    size_t W_long = 100000;   // 100k steps
    
    // Alert thresholds (starter values)
    struct EntropyThresholds {
        float too_random = 1.55f;    // > 1.55 ⇒ too random
        float too_rigid = 0.65f;     // < 0.65 ⇒ too rigid
    } entropy;
    
    struct MarginThresholds {
        float ambiguity = 0.12f;     // < 0.12 ⇒ ambiguity
    } margin;
    
    struct SuccessThresholds {
        float degrade = 0.62f;       // < 0.62 ⇒ degrade
    } success;
    
    struct DriftThresholds {
        float concept_drift = 0.28f; // > 0.28 ⇒ concept space drifting
    } drift;
    
    struct ThoughtReplayThresholds {
        float memory_instability = 0.75f; // < 0.75 ⇒ memory instability
    } thought_replay;
    
    // Champion-Challenger thresholds
    float fitness_improvement_min = 0.03f; // ≥3% fitness improvement required
    float post_adoption_degrade_max = 0.03f; // >3% degradation triggers rollback
};

// ==================== METRICS TRACKING ====================

struct MetricSnapshot {
    float entropy_short = 0.0f;
    float entropy_mid = 0.0f;
    float entropy_long = 0.0f;
    float top2_margin_mid = 0.0f;
    float success_short = 0.0f;
    float success_mid = 0.0f;
    float drift_long = 0.0f;
    float thought_replay_mid = 0.0f;
    
    uint64_t timestamp = 0;
    
    // Fitness calculation: 0.35*(1−entropy) + 0.25*top2_margin + 0.25*success − 0.15*drift
    float compute_fitness() const {
        return 0.35f * (1.0f - entropy_mid) + 
               0.25f * top2_margin_mid + 
               0.25f * success_mid - 
               0.15f * drift_long;
    }
};

// ==================== CHAMPION-CHALLENGER SYSTEM ====================

struct GenomeSnapshot {
    float alpha = 0.7f;
    float beta = 8.0f;
    float gamma = 1.0f;
    float eta = 0.002f;
    float delta = 0.15f;
    float epsilon = 0.3f;
    
    float fitness = 0.0f;
    uint64_t generation = 0;
    std::string hash = "";  // Genome hash for versioning
    
    // Validation bounds
    static constexpr float ALPHA_MIN = 0.55f, ALPHA_MAX = 0.9f;
    static constexpr float BETA_MIN = 5.0f, BETA_MAX = 12.0f;
    static constexpr float GAMMA_MIN = 0.6f, GAMMA_MAX = 1.8f;
    static constexpr float ETA_MIN = 0.001f, ETA_MAX = 0.01f;
    static constexpr float DELTA_MIN = 0.08f, DELTA_MAX = 0.28f;
    static constexpr float EPSILON_MIN = 0.1f, EPSILON_MAX = 0.5f;
    
    bool is_valid() const {
        return alpha >= ALPHA_MIN && alpha <= ALPHA_MAX &&
               beta >= BETA_MIN && beta <= BETA_MAX &&
               gamma >= GAMMA_MIN && gamma <= GAMMA_MAX &&
               eta >= ETA_MIN && eta <= ETA_MAX &&
               delta >= DELTA_MIN && delta <= DELTA_MAX &&
               epsilon >= EPSILON_MIN && epsilon <= EPSILON_MAX;
    }
    
    void clamp() {
        alpha = std::max(ALPHA_MIN, std::min(ALPHA_MAX, alpha));
        beta = std::max(BETA_MIN, std::min(BETA_MAX, beta));
        gamma = std::max(GAMMA_MIN, std::min(GAMMA_MAX, gamma));
        eta = std::max(ETA_MIN, std::min(ETA_MAX, eta));
        delta = std::max(DELTA_MIN, std::min(DELTA_MAX, delta));
        epsilon = std::max(EPSILON_MIN, std::min(EPSILON_MAX, epsilon));
    }
    
    std::string compute_hash() const {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(6)
           << alpha << "_" << beta << "_" << gamma << "_"
           << eta << "_" << delta << "_" << epsilon;
        return ss.str();
    }
    
    std::string get_summary() const {
        std::stringstream ss;
        ss << "Genome[gen=" << generation 
           << ", fitness=" << std::fixed << std::setprecision(3) << fitness
           << ", α=" << alpha << ", β=" << beta << ", γ=" << gamma
           << ", η=" << eta << ", δ=" << delta << ", ε=" << epsilon << "]";
        return ss.str();
    }
};

// ==================== GUARDRAILS MONITOR ====================

class GuardrailsMonitor {
private:
    GuardrailThresholds thresholds_;
    
    // Rolling windows
    RollingWindow<float> entropy_short_;
    RollingWindow<float> entropy_mid_;
    RollingWindow<float> entropy_long_;
    RollingWindow<float> top2_margin_mid_;
    RollingWindow<float> success_short_;
    RollingWindow<float> success_mid_;
    RollingWindow<float> drift_long_;
    RollingWindow<float> thought_replay_mid_;
    
    // Champion-Challenger state
    GenomeSnapshot champion_;
    std::vector<GenomeSnapshot> challengers_;
    std::unordered_set<std::string> blacklisted_genomes_;
    
    // Alert callbacks
    std::vector<std::function<void(const std::string&)>> alert_callbacks_;
    
public:
    GuardrailsMonitor() 
        : entropy_short_(thresholds_.W_short),
          entropy_mid_(thresholds_.W_mid),
          entropy_long_(thresholds_.W_long),
          top2_margin_mid_(thresholds_.W_mid),
          success_short_(thresholds_.W_short),
          success_mid_(thresholds_.W_mid),
          drift_long_(thresholds_.W_long),
          thought_replay_mid_(thresholds_.W_mid) {
        
        // Initialize champion with default genome
        champion_.generation = 0;
        champion_.hash = champion_.compute_hash();
    }
    
    // Record metrics
    void record_metrics(float entropy, float top2_margin, float success_rate, 
                       float drift, float thought_replay_success) {
        entropy_short_.add(entropy);
        entropy_mid_.add(entropy);
        entropy_long_.add(entropy);
        top2_margin_mid_.add(top2_margin);
        success_short_.add(success_rate);
        success_mid_.add(success_rate);
        drift_long_.add(drift);
        thought_replay_mid_.add(thought_replay_success);
        
        // Check for alert conditions
        check_alerts();
    }
    
    // Get current metric snapshot
    MetricSnapshot get_current_snapshot() const {
        MetricSnapshot snapshot;
        snapshot.entropy_short = entropy_short_.average();
        snapshot.entropy_mid = entropy_mid_.average();
        snapshot.entropy_long = entropy_long_.average();
        snapshot.top2_margin_mid = top2_margin_mid_.average();
        snapshot.success_short = success_short_.average();
        snapshot.success_mid = success_mid_.average();
        snapshot.drift_long = drift_long_.average();
        snapshot.thought_replay_mid = thought_replay_mid_.average();
        snapshot.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return snapshot;
    }
    
    // Champion-Challenger management
    const GenomeSnapshot& get_champion() const { return champion_; }
    
    void add_challenger(const GenomeSnapshot& challenger) {
        if (challenger.is_valid() && 
            blacklisted_genomes_.find(challenger.hash) == blacklisted_genomes_.end()) {
            challengers_.push_back(challenger);
        }
    }
    
    bool evaluate_challengers() {
        if (challengers_.empty()) return false;
        
        // Sort challengers by fitness
        std::sort(challengers_.begin(), challengers_.end(),
                 [](const GenomeSnapshot& a, const GenomeSnapshot& b) {
                     return a.fitness > b.fitness;
                 });
        
        const auto& best_challenger = challengers_[0];
        
        // Check if best challenger meets promotion criteria
        float fitness_improvement = best_challenger.fitness - champion_.fitness;
        
        if (fitness_improvement >= thresholds_.fitness_improvement_min) {
            // Promote challenger to champion
            champion_ = best_challenger;
            challengers_.clear();
            
            std::cout << "🏆 Champion promoted: " << champion_.get_summary() 
                      << " (fitness improvement: " << std::fixed << std::setprecision(3) 
                      << fitness_improvement << ")\n";
            
            return true;
        }
        
        challengers_.clear();
        return false;
    }
    
    void blacklist_genome(const std::string& genome_hash) {
        blacklisted_genomes_.insert(genome_hash);
        std::cout << "🚫 Blacklisted genome: " << genome_hash << "\n";
    }
    
    // Alert system
    void add_alert_callback(std::function<void(const std::string&)> callback) {
        alert_callbacks_.push_back(callback);
    }
    
    // Check if evolution should be triggered
    bool should_trigger_evolution() const {
        auto snapshot = get_current_snapshot();
        
        // Check guardrail breaches in W_mid window
        return entropy_mid_.is_full() && 
               (snapshot.entropy_mid > thresholds_.entropy.too_random ||
                snapshot.entropy_mid < thresholds_.entropy.too_rigid ||
                snapshot.top2_margin_mid < thresholds_.margin.ambiguity ||
                snapshot.success_mid < thresholds_.success.degrade ||
                snapshot.drift_long > thresholds_.drift.concept_drift ||
                snapshot.thought_replay_mid < thresholds_.thought_replay.memory_instability);
    }
    
    // Check if fitness is stagnating
    bool is_fitness_stagnating() const {
        // Check if fitness slope ~0 for ≥50k steps
        // This is a simplified check - in practice would track fitness history
        return entropy_long_.is_full() && entropy_long_.size() >= 50000;
    }
    
    // Get current thresholds
    const GuardrailThresholds& get_thresholds() const { return thresholds_; }
    
    // Update thresholds (for dynamic adjustment)
    void update_thresholds(const GuardrailThresholds& new_thresholds) {
        thresholds_ = new_thresholds;
    }
    
private:
    void check_alerts() {
        auto snapshot = get_current_snapshot();
        
        // Check each threshold and trigger alerts
        if (entropy_mid_.is_full()) {
            if (snapshot.entropy_mid > thresholds_.entropy.too_random) {
                trigger_alert("ENTROPY_TOO_HIGH: " + std::to_string(snapshot.entropy_mid));
            } else if (snapshot.entropy_mid < thresholds_.entropy.too_rigid) {
                trigger_alert("ENTROPY_TOO_LOW: " + std::to_string(snapshot.entropy_mid));
            }
        }
        
        if (top2_margin_mid_.is_full() && 
            snapshot.top2_margin_mid < thresholds_.margin.ambiguity) {
            trigger_alert("TOP2_MARGIN_LOW: " + std::to_string(snapshot.top2_margin_mid));
        }
        
        if (success_short_.is_full() && 
            snapshot.success_short < thresholds_.success.degrade) {
            trigger_alert("SUCCESS_RATE_LOW: " + std::to_string(snapshot.success_short));
        }
        
        if (drift_long_.is_full() && 
            snapshot.drift_long > thresholds_.drift.concept_drift) {
            trigger_alert("DRIFT_HIGH: " + std::to_string(snapshot.drift_long));
        }
        
        if (thought_replay_mid_.is_full() && 
            snapshot.thought_replay_mid < thresholds_.thought_replay.memory_instability) {
            trigger_alert("THOUGHT_REPLAY_LOW: " + std::to_string(snapshot.thought_replay_mid));
        }
    }
    
    void trigger_alert(const std::string& message) {
        std::cout << "🚨 GUARDRAIL ALERT: " << message << "\n";
        
        for (const auto& callback : alert_callbacks_) {
            callback(message);
        }
    }
};

} // namespace melvin
