/**
 * @file visual_context.h
 * @brief Rich visual context that understands WHAT is being seen
 * 
 * Problem: Context field tracks node IDs but doesn't understand visual features
 * Solution: Build semantic understanding of visual content
 * 
 * Key insight: If Melvin knows he's looking at "round red object",
 * he can intentionally seek "blue square object" for diversity!
 */

#pragma once

#include "melvin_types.h"
#include "melvin_graph.h"
#include "context_field.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <deque>

namespace melvin {

/**
 * Visual features extracted from patches
 */
struct VisualFeatures {
    // Color features
    float red, green, blue;       // Dominant color [0,1]
    float hue, saturation, value; // HSV color space
    
    // Shape features  
    float roundness;              // 0=angular, 1=round
    float edginess;               // Edge density
    float complexity;             // Pattern complexity
    
    // Motion features
    float motion_magnitude;       // Speed of movement
    float motion_direction;       // Direction in radians
    
    // Texture features
    float smoothness;             // 0=rough, 1=smooth
    float contrast;               // Local contrast
    
    VisualFeatures() : red(0), green(0), blue(0), hue(0), saturation(0), value(0),
                      roundness(0), edginess(0), complexity(0),
                      motion_magnitude(0), motion_direction(0),
                      smoothness(0), contrast(0) {}
};

/**
 * Semantic understanding of what's in view
 */
struct VisualConcept {
    std::string feature_type;     // "color_red", "shape_round", "motion_fast"
    float activation;             // How strongly present [0,1]
    uint64_t concept_node;        // Link to graph concept
};

/**
 * VisualContext - Semantic understanding of visual field
 * 
 * Goes beyond "looking at node 12345" to understand:
 * "I'm looking at a round red moving object"
 * 
 * This enables:
 * - Diversity seeking (if seeing red, look for blue!)
 * - Feature-based inhibition (tired of round objects? look for angular!)
 * - Semantic saturation detection (too much of one type?)
 * - Intelligent exploration (seek visual variety)
 */
class VisualContext {
public:
    VisualContext(AtomicGraph& graph, ContextField& context);
    
    // ========================================================================
    // FEATURE EXTRACTION & SEMANTIC UNDERSTANDING
    // ========================================================================
    
    /**
     * Extract visual features from patch
     * @param patch_data RGB data for 32x32 or 64x64 patch
     * @param patch_size Size of patch
     * @return Extracted features
     */
    VisualFeatures extract_features(const uint8_t* patch_data, int patch_size);
    
    /**
     * Convert features to semantic concepts
     * "red + round + moving" → concepts: color_red, shape_round, motion_detected
     */
    std::vector<VisualConcept> features_to_concepts(const VisualFeatures& features);
    
    /**
     * Update context from visual features
     * Injects semantic concepts into context field
     */
    void update_from_visual(uint64_t patch_id, const VisualFeatures& features);
    
    // ========================================================================
    // DIVERSITY SEEKING (Anti-Repetition)
    // ========================================================================
    
    /**
     * Compute diversity score for patch
     * Returns HIGH score if patch is DIFFERENT from recent focuses
     * Returns LOW score if patch is SIMILAR to what we've been looking at
     * 
     * @param features Features of candidate patch
     * @return Diversity bonus [-0.3, +0.3]
     */
    float compute_diversity_score(const VisualFeatures& features);
    
    /**
     * Check if visual context is too repetitive
     * "I've been looking at red round things for 50 frames!"
     */
    bool is_visually_saturated() const;
    
    /**
     * Get feature contrast score
     * How different is this from what we've been seeing?
     */
    float get_feature_contrast(const VisualFeatures& features);
    
    // ========================================================================
    // SEMANTIC BIAS COMPUTATION
    // ========================================================================
    
    /**
     * Get bias for specific visual feature
     * 
     * If context shows "red" is over-represented:
     *   red patches: bias = 0.5 (suppress!)
     *   blue patches: bias = 1.5 (boost!)
     * 
     * Encourages visual diversity!
     */
    float get_feature_bias(const std::string& feature_name) const;
    
    /**
     * Compute attention weight adjustments based on visual saturation
     * 
     * If too much saliency-driven focus (all on bright things):
     *   Boost γ (curiosity) to break pattern
     * 
     * @return Adjusted (α, β, γ)
     */
    ContextField::AttentionWeights compute_diversity_adjusted_weights(
        const ContextField::AttentionWeights& base_weights
    );
    
    // ========================================================================
    // HISTORY TRACKING
    // ========================================================================
    
    /**
     * Record what was just focused on
     */
    void record_focus(uint64_t patch_id, const VisualFeatures& features);
    
    /**
     * Get recent feature history
     */
    std::vector<VisualFeatures> get_recent_features(int count = 10) const;
    
    /**
     * Detect repetitive patterns
     * "Looking at red things for last 30 frames"
     */
    struct RepetitionPattern {
        std::string feature_type;  // "color_red", "shape_round"
        int consecutive_frames;
        float saturation_level;    // [0,1]
    };
    std::vector<RepetitionPattern> detect_repetition() const;
    
    // ========================================================================
    // VISUALIZATION
    // ========================================================================
    
    /**
     * Show visual context summary
     */
    void print_visual_summary() const;
    
    /**
     * Get dominant features in recent history
     */
    std::string get_dominant_features_summary() const;
    
private:
    AtomicGraph& graph_;
    ContextField& context_;
    
    // Feature history (last N focuses)
    std::deque<VisualFeatures> feature_history_;
    static const int MAX_HISTORY = 50;
    
    // Feature frequency tracking
    struct FeatureStats {
        int count;
        float avg_activation;
        int frames_since_last;
    };
    std::unordered_map<std::string, FeatureStats> feature_stats_;
    
    // Saturation detection
    int frames_with_similar_features_;
    VisualFeatures last_dominant_features_;
    
    // Helper: compute feature similarity
    float feature_similarity(const VisualFeatures& a, const VisualFeatures& b) const;
    
    // Helper: detect dominant feature type
    std::string get_dominant_feature_type() const;
};

} // namespace melvin

