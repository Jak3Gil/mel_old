#include "src/uca/FeedbackBus.hpp"
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <string>

namespace uca {

// Mathematical constants for curiosity and feedback
static constexpr float CURIOSITY_ALPHA = 0.7f;        // D1: Confidence vs curiosity blend
static constexpr float PREDICTION_ERROR_THRESHOLD = 0.1f;  // Minimum error for curiosity
static constexpr float SURPRISAL_BASE = 2.0f;         // Base for surprisal calculation

// Mock prediction storage (in real implementation, this would track predictions)
static std::unordered_map<std::string, float> mock_predictions;
static std::unordered_map<std::string, float> mock_observation_probabilities;

// Forward declarations
float compute_prediction_error_curiosity(const ReasoningResult& r);
float estimate_actual_reward(const ReasoningResult& r);

Feedback FeedbackBus::collect(const ReasoningResult& r) {
    Feedback fb;
    
    // D1: Prediction-error curiosity
    float curiosity = compute_prediction_error_curiosity(r);
    
    // Blend confidence with curiosity: reward = α·confidence + (1-α)·curiosity
    if (!r.text.empty()) {
        float confidence_component = CURIOSITY_ALPHA * r.confidence;
        float curiosity_component = (1.0f - CURIOSITY_ALPHA) * curiosity;
        fb.reward = confidence_component + curiosity_component;
        
        // Add diagnostic note
        fb.note = "confidence=" + std::to_string(r.confidence) + 
                 ", curiosity=" + std::to_string(curiosity);
    } else {
        // Low reward for empty outputs
        fb.reward = -0.1f;
        fb.note = "empty_output";
    }
    
    return fb;
}

// D1: Prediction-error curiosity calculation
float compute_prediction_error_curiosity(const ReasoningResult& r) {
    if (r.text.empty()) return 0.0f;
    
    // Generate a key for this reasoning result
    std::string result_key = r.text + "_" + std::to_string(r.confidence);
    
    // Get previous prediction (if any)
    auto pred_it = mock_predictions.find(result_key);
    float predicted_reward = (pred_it != mock_predictions.end()) ? pred_it->second : 0.5f;
    
    // Estimate actual reward based on confidence and output quality
    float actual_reward = estimate_actual_reward(r);
    
    // Prediction error: |predicted - actual|
    float prediction_error = std::abs(predicted_reward - actual_reward);
    
    // Only reward curiosity if error is above threshold
    if (prediction_error < PREDICTION_ERROR_THRESHOLD) {
        return 0.0f;
    }
    
    // Update prediction for next time
    mock_predictions[result_key] = actual_reward;
    
    // Return normalized curiosity (0 to 1)
    return std::min(1.0f, prediction_error);
}

// Estimate actual reward based on reasoning result properties
float estimate_actual_reward(const ReasoningResult& r) {
    float reward = 0.0f;
    
    // Base reward from confidence
    reward += r.confidence * 0.5f;
    
    // Bonus for non-empty, coherent output
    if (!r.text.empty() && r.text.length() > 3) {
        reward += 0.3f;
    }
    
    // Bonus for multiple reasoning paths (more thorough reasoning)
    if (r.used_paths.size() > 1) {
        reward += 0.2f;
    }
    
    // Penalty for very low confidence
    if (r.confidence < 0.2f) {
        reward -= 0.3f;
    }
    
    // Clamp to [-1, 1] range
    return std::clamp(reward, -1.0f, 1.0f);
}

}