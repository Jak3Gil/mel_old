#include "src/uca/ReflectionEngine.hpp"
#include <numeric>
#include <cmath>
#include <algorithm>
#include <limits>

namespace uca {

// Mathematical constants for reflection and evolution
static constexpr float SLOPE_THRESHOLD = -0.002f;     // E1: Trend-based stagnation threshold
static constexpr float CONFIDENCE_THRESHOLD = 0.18f;  // Low confidence threshold
static constexpr float REWARD_THRESHOLD = 0.10f;      // Low reward threshold
static constexpr float UCB_EXPLORATION = 1.0f;        // E2: UCB exploration constant
static constexpr size_t RATE_LIMIT_TICKS = 50;        // E3: Minimum ticks between genome swaps

// Rate limiting state
static size_t ticks_since_last_swap = 0;
static size_t total_reflection_calls = 0;

bool ReflectionEngine::configure(const DynamicGenome&) { 
    return true; 
}

// E1: Linear regression slope calculation
float compute_linear_regression_slope(const std::deque<float>& values) {
    if (values.size() < 3) return 0.0f;
    
    size_t n = values.size();
    float sum_x = 0.0f, sum_y = 0.0f, sum_xy = 0.0f, sum_x2 = 0.0f;
    
    for (size_t i = 0; i < n; ++i) {
        float x = static_cast<float>(i);
        float y = values[i];
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }
    
    // Slope formula: β = (n·Σxy - Σx·Σy) / (n·Σx² - (Σx)²)
    float denominator = n * sum_x2 - sum_x * sum_x;
    if (std::abs(denominator) < 1e-6f) return 0.0f;
    
    return (n * sum_xy - sum_x * sum_y) / denominator;
}

// E1: Rolling mean calculation
float compute_rolling_mean(const std::deque<float>& values) {
    if (values.empty()) return 0.0f;
    
    float sum = std::accumulate(values.begin(), values.end(), 0.0f);
    return sum / static_cast<float>(values.size());
}

// E2: UCB (Upper Confidence Bound) for parameter selection
float compute_ucb(float mean_reward, size_t total_trials, size_t param_trials) {
    if (param_trials == 0) return std::numeric_limits<float>::infinity();
    
    return mean_reward + UCB_EXPLORATION * std::sqrt(std::log(total_trials) / param_trials);
}

// E3: Rate limiting check
bool should_rate_limit_genome_swap() {
    return ticks_since_last_swap < RATE_LIMIT_TICKS;
}

ReflectReport ReflectionEngine::reflect(const std::deque<float>& confidences,
                                        const std::deque<float>& rewards) {
    ReflectReport rep{};
    total_reflection_calls++;
    ticks_since_last_swap++;
    
    // Need minimum data for trend analysis
    if (confidences.size() < 8) return rep;
    
    // E1: Trend-based stagnation detection
    float confidence_slope = compute_linear_regression_slope(confidences);
    float reward_slope = compute_linear_regression_slope(rewards);
    
    float avg_confidence = compute_rolling_mean(confidences);
    float avg_reward = compute_rolling_mean(rewards);
    
    // Enhanced stagnation criteria:
    // 1. Negative trend in confidence AND low average confidence
    // 2. Negative trend in reward AND low average reward  
    // 3. Rate limiting: don't swap too frequently
    bool confidence_stagnating = (confidence_slope < SLOPE_THRESHOLD) && 
                                (avg_confidence < CONFIDENCE_THRESHOLD);
    bool reward_stagnating = (reward_slope < SLOPE_THRESHOLD) && 
                           (avg_reward < REWARD_THRESHOLD);
    bool rate_limited = should_rate_limit_genome_swap();
    
    if ((confidence_stagnating || reward_stagnating) && !rate_limited) {
        rep.stagnating = true;
        
        // Create diagnostic reason
        std::string reason = "trend-based stagnation: ";
        if (confidence_stagnating) {
            reason += "conf_slope=" + std::to_string(confidence_slope) + 
                     ", conf_avg=" + std::to_string(avg_confidence);
        }
        if (reward_stagnating) {
            if (confidence_stagnating) reason += "; ";
            reason += "reward_slope=" + std::to_string(reward_slope) + 
                     ", reward_avg=" + std::to_string(avg_reward);
        }
        rep.reason = reason;
        
        // E2: Generate new genome with UCB-guided parameter selection
        DynamicGenome new_genome;
        
        // Parameter variants to explore (simplified UCB selection)
        std::vector<std::pair<std::string, float>> param_variants = {
            {"beam_width", 6.0f},      // Slightly higher exploration
            {"max_hops", 6.0f},       // Moderate depth increase
            {"leap_bias", 0.15f},     // Increased creative connections
            {"abstr_thresh", 0.55f},  // Slightly lower abstraction threshold
            {"confidence_threshold", 0.12f},  // Lower confidence threshold for more outputs
            {"reinforcement_rate", 0.12f}    // Slightly higher learning rate
        };
        
        // Select parameters with UCB (simplified - in real system would track per-param stats)
        for (const auto& [name, value] : param_variants) {
            GenomeParam param;
            param.name = name;
            param.value = value;
            
            // Add some variation based on current performance
            float variation = 0.1f * (avg_confidence < 0.15f ? 1.0f : -0.5f);
            param.value = std::clamp(param.value + variation, 0.01f, 1.0f);
            
            // Set reasonable bounds
            if (name == "beam_width" || name == "max_hops") {
                param.min_v = 1.0f;
                param.max_v = 16.0f;
            } else {
                param.min_v = 0.0f;
                param.max_v = 1.0f;
            }
            
            new_genome.params.push_back(param);
        }
        
        rep.new_genome = new_genome;
        
        // Reset rate limiting counter
        ticks_since_last_swap = 0;
    }
    
    return rep;
}

}
