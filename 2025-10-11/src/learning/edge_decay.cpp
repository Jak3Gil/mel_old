#include "edge_decay.h"
#include <chrono>
#include <cmath>
#include <algorithm>

// Edge structure (stub for compilation)
struct Edge {
    uint64_t u, v;
    float weight = 1.0f;
    std::string rel;
    uint32_t count = 0;
    uint32_t last_access_time = 0;  // Unix timestamp (seconds)
    
    void update_frequency(uint64_t) {}  // Stub
};

namespace melvin {
namespace learning {

// RealClock implementation
double RealClock::now_seconds() const {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto duration = now.time_since_epoch();
    return static_cast<double>(duration_cast<seconds>(duration).count());
}

// EdgeDecay implementation
EdgeDecay::EdgeDecay(const DecayConfig& config, Clock* clock)
    : config_(config), clock_(clock), last_decay_time_(0.0) {
    if (clock_) {
        last_decay_time_ = clock_->now_seconds();
    }
}

DecayStats EdgeDecay::apply_decay(std::vector<Edge>& edges) {
    DecayStats stats;
    
    if (!config_.enabled || !clock_) {
        return stats;
    }
    
    double current_time = clock_->now_seconds();
    
    // Check if enough time has passed since last decay
    double hours_since_last = (current_time - last_decay_time_) / 3600.0;
    if (hours_since_last < config_.check_interval_hours) {
        return stats;  // Not time yet
    }
    
    // Compute statistics before decay
    double total_weight_before = 0.0;
    for (const auto& edge : edges) {
        total_weight_before += edge.weight;
    }
    stats.avg_weight_before = edges.empty() ? 0.0 : total_weight_before / edges.size();
    
    // Apply decay to each edge
    stats.edges_checked = static_cast<uint32_t>(edges.size());
    
    for (auto& edge : edges) {
        double weight_before = edge.weight;
        
        decay_edge(edge, current_time);
        
        double weight_after = edge.weight;
        
        // Track statistics
        if (weight_after < weight_before) {
            stats.edges_decayed++;
            stats.total_weight_lost += (weight_before - weight_after);
        }
        
        if (weight_after <= config_.floor) {
            stats.edges_at_floor++;
        }
    }
    
    // Compute statistics after decay
    double total_weight_after = 0.0;
    for (const auto& edge : edges) {
        total_weight_after += edge.weight;
    }
    stats.avg_weight_after = edges.empty() ? 0.0 : total_weight_after / edges.size();
    
    last_decay_time_ = current_time;
    
    return stats;
}

void EdgeDecay::reinforce_edge(Edge& edge, double amount) {
    if (!config_.enabled || !clock_) {
        return;
    }
    
    // Increase weight (clamped to max)
    edge.weight += static_cast<float>(amount);
    edge.weight = std::min(edge.weight, static_cast<float>(config_.max));
    
    // Reset decay timer (this edge is fresh)
    edge.last_access_time = static_cast<uint32_t>(clock_->now_seconds());
    
    // Increment usage count
    edge.count++;
}

void EdgeDecay::decay_edge(Edge& edge, double current_time) {
    // If edge has never been accessed, initialize to current time
    if (edge.last_access_time == 0) {
        edge.last_access_time = static_cast<uint32_t>(current_time);
        return;
    }
    
    // Compute time since last access
    double seconds_since_access = current_time - edge.last_access_time;
    double days_since_access = seconds_to_days(seconds_since_access);
    
    // Apply Ebbinghaus decay
    double decayed_weight = compute_decayed_weight(edge.weight, days_since_access);
    
    // Update edge weight (clamped to floor)
    edge.weight = static_cast<float>(std::max(decayed_weight, config_.floor));
}

double EdgeDecay::compute_decayed_weight(
    double weight,
    double time_since_update_days
) const {
    // Ebbinghaus forgetting curve:
    // w(t) = floor + (w0 - floor) * 0.5^(t / T_half)
    //
    // Where:
    // - w0 = initial weight
    // - t = time since last access (days)
    // - T_half = half-life (days)
    // - floor = minimum weight
    
    if (time_since_update_days <= 0.0) {
        return weight;  // No time passed
    }
    
    double decay_factor = std::pow(0.5, time_since_update_days / config_.half_life_days);
    double decayed = config_.floor + (weight - config_.floor) * decay_factor;
    
    // Clamp to valid range
    return std::max(config_.floor, std::min(decayed, config_.max));
}

} // namespace learning
} // namespace melvin
