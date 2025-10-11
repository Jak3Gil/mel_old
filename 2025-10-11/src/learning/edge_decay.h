#pragma once

#include <cstdint>
#include <vector>
#include <functional>

// Forward declaration
struct Edge;

namespace melvin {
namespace learning {

// Decay configuration
struct DecayConfig {
    double half_life_days = 7.0;      // Time for weight to decay 50%
    double floor = 0.05;              // Minimum weight (never decay below)
    double max = 4.0;                 // Maximum weight (cap reinforcement)
    bool enabled = true;              // Master switch
    double check_interval_hours = 24.0; // How often to run decay pass
};

// Statistics from decay pass
struct DecayStats {
    uint32_t edges_checked = 0;
    uint32_t edges_decayed = 0;
    uint32_t edges_at_floor = 0;
    double avg_weight_before = 0.0;
    double avg_weight_after = 0.0;
    double total_weight_lost = 0.0;
};

// Clock interface for time (allows simulation in tests)
class Clock {
public:
    virtual ~Clock() = default;
    virtual double now_seconds() const = 0;
};

// Real-time clock (production)
class RealClock : public Clock {
public:
    double now_seconds() const override;
};

// Simulated clock (testing)
class SimulatedClock : public Clock {
public:
    explicit SimulatedClock(double start_time = 0.0) : time_(start_time) {}
    
    double now_seconds() const override { return time_; }
    void advance(double seconds) { time_ += seconds; }
    void set_time(double seconds) { time_ = seconds; }
    
private:
    double time_;
};

// Edge decay engine
class EdgeDecay {
public:
    explicit EdgeDecay(const DecayConfig& config, Clock* clock);
    
    // Apply decay to all edges (called periodically)
    DecayStats apply_decay(std::vector<Edge>& edges);
    
    // Reinforce an edge (resets decay timer, increases weight)
    void reinforce_edge(Edge& edge, double amount);
    
    // Get current configuration
    const DecayConfig& config() const { return config_; }
    
    // Update configuration
    void set_config(const DecayConfig& config) { config_ = config; }
    
    // Get last decay time
    double last_decay_time() const { return last_decay_time_; }
    
private:
    DecayConfig config_;
    Clock* clock_;
    double last_decay_time_;
    
    // Apply decay to single edge
    void decay_edge(Edge& edge, double current_time);
    
    // Compute decayed weight using Ebbinghaus curve
    double compute_decayed_weight(
        double weight,
        double time_since_update_days
    ) const;
};

// Utility: Convert seconds to days
inline double seconds_to_days(double seconds) {
    return seconds / 86400.0;
}

// Utility: Convert days to seconds
inline double days_to_seconds(double days) {
    return days * 86400.0;
}

} // namespace learning
} // namespace melvin
