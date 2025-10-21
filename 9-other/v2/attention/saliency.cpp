#include "saliency.h"
#include <algorithm>
#include <cmath>

namespace melvin::v2::attention {

SaliencyComputer::SaliencyComputer(const Config& config)
    : config_(config) {}

std::vector<float> SaliencyComputer::compute_visual(const std::vector<PerceivedObject>& objects) {
    std::vector<float> saliency_scores;
    
    for (const auto& obj : objects) {
        float score = compute_object_saliency(obj.features);
        saliency_scores.push_back(score);
    }
    
    // Normalize to 0-1 range
    normalize(saliency_scores);
    
    return saliency_scores;
}

float SaliencyComputer::compute_object_saliency(const PerceptualFeatures& features) {
    // Extract individual feature dimensions
    float motion = features.visual_features.size() > 0 ? features.visual_features[0] : 0.0f;
    float edges = features.visual_features.size() > 1 ? features.visual_features[1] : 0.0f;
    float color = features.visual_features.size() > 2 ? features.visual_features[2] : 0.0f;
    float novelty = features.novelty;
    
    // Weighted combination
    float score = config_.motion_weight * motion +
                 config_.edge_weight * edges +
                 config_.color_weight * color +
                 config_.novelty_weight * novelty;
    
    // Normalize
    if (config_.normalization > 0.0f) {
        score /= config_.normalization;
    }
    
    // Clamp to [0, 1]
    return std::clamp(score, 0.0f, 1.0f);
}

void SaliencyComputer::normalize(std::vector<float>& scores) {
    if (scores.empty()) return;
    
    // Find max
    float max_score = *std::max_element(scores.begin(), scores.end());
    
    if (max_score > 1e-6f) {
        // Normalize by max
        for (float& score : scores) {
            score /= max_score;
        }
    }
}

void SaliencyComputer::set_config(const Config& config) {
    config_ = config;
}

SaliencyComputer::Config SaliencyComputer::get_config() const {
    return config_;
}

} // namespace melvin::v2::attention

