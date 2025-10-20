#pragma once

#include "../core/types_v2.h"
#include <vector>

namespace melvin::v2::attention {

// ============================================================================
// SALIENCY - Bottom-Up Attention
// ============================================================================

/**
 * SaliencyComputer calculates stimulus-driven attention scores.
 * 
 * Based on visual/audio features:
 * - Motion (optic flow)
 * - Contrast (edge density)
 * - Color pop-out (variance)
 * - Loudness (audio amplitude)
 * - Novelty (different from predictions)
 * 
 * Formula (visual):
 *   S = α·motion + β·edges + γ·color_variance + δ·novelty
 */
class SaliencyComputer {
public:
    struct Config {
        float motion_weight;
        float edge_weight;
        float color_weight;
        float novelty_weight;
        float normalization;     // Divide by this to get 0-1 range
        
        Config()
            : motion_weight(0.4f), edge_weight(0.3f),
              color_weight(0.2f), novelty_weight(0.1f),
              normalization(1.0f) {}
    };
    
    explicit SaliencyComputer(const Config& config = Config());
    
    /**
     * Compute saliency for visual objects
     * - objects: perceived objects with features
     * - Returns: saliency score for each object (0-1)
     */
    std::vector<float> compute_visual(const std::vector<PerceivedObject>& objects);
    
    /**
     * Compute saliency for individual object
     */
    float compute_object_saliency(const PerceptualFeatures& features);
    
    /**
     * Normalize scores to 0-1 range
     */
    void normalize(std::vector<float>& scores);
    
    void set_config(const Config& config);
    Config get_config() const;
    
private:
    Config config_;
};

} // namespace melvin::v2::attention

