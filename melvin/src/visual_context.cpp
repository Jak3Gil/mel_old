#include "visual_context.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iostream>

namespace melvin {

VisualContext::VisualContext(AtomicGraph& graph, ContextField& context)
    : graph_(graph)
    , context_(context)
    , frames_with_similar_features_(0)
{}

// ============================================================================
// FEATURE EXTRACTION
// ============================================================================

VisualFeatures VisualContext::extract_features(const uint8_t* patch_data, int patch_size) {
    VisualFeatures feat;
    
    int pixel_count = patch_size * patch_size;
    float r_sum = 0, g_sum = 0, b_sum = 0;
    float variance = 0;
    
    // Compute color statistics
    for (int i = 0; i < pixel_count; ++i) {
        int idx = i * 3;
        r_sum += patch_data[idx + 0];
        g_sum += patch_data[idx + 1];
        b_sum += patch_data[idx + 2];
    }
    
    feat.red = (r_sum / pixel_count) / 255.0f;
    feat.green = (g_sum / pixel_count) / 255.0f;
    feat.blue = (b_sum / pixel_count) / 255.0f;
    
    // HSV conversion (simplified)
    float r = feat.red, g = feat.green, b = feat.blue;
    float max_val = std::max({r, g, b});
    float min_val = std::min({r, g, b});
    float delta = max_val - min_val;
    
    feat.value = max_val;
    feat.saturation = (max_val > 0) ? (delta / max_val) : 0;
    
    // Hue calculation
    if (delta > 0) {
        if (max_val == r) {
            feat.hue = 60.0f * fmod(((g - b) / delta), 6.0f);
        } else if (max_val == g) {
            feat.hue = 60.0f * (((b - r) / delta) + 2.0f);
        } else {
            feat.hue = 60.0f * (((r - g) / delta) + 4.0f);
        }
    }
    if (feat.hue < 0) feat.hue += 360.0f;
    
    // Edge density (simplified)
    float edge_count = 0;
    for (int y = 1; y < patch_size - 1; ++y) {
        for (int x = 1; x < patch_size - 1; ++x) {
            int idx = (y * patch_size + x) * 3;
            int idx_right = (y * patch_size + (x+1)) * 3;
            int idx_down = ((y+1) * patch_size + x) * 3;
            
            float dx = std::abs(patch_data[idx] - patch_data[idx_right]);
            float dy = std::abs(patch_data[idx] - patch_data[idx_down]);
            
            if (dx > 30 || dy > 30) {
                edge_count += 1.0f;
            }
        }
    }
    feat.edginess = edge_count / pixel_count;
    
    // Other features (simplified)
    feat.contrast = feat.saturation;
    feat.complexity = feat.edginess;
    feat.roundness = 0.5f;  // Would need shape analysis
    feat.smoothness = 1.0f - feat.edginess;
    
    return feat;
}

std::vector<VisualConcept> VisualContext::features_to_concepts(const VisualFeatures& features) {
    std::vector<VisualConcept> concepts;
    
    // Color concepts
    if (features.red > 0.6f) {
        concepts.push_back({"color_red", features.red, 
                           graph_.get_or_create_concept("color_red")});
    }
    if (features.green > 0.6f) {
        concepts.push_back({"color_green", features.green,
                           graph_.get_or_create_concept("color_green")});
    }
    if (features.blue > 0.6f) {
        concepts.push_back({"color_blue", features.blue,
                           graph_.get_or_create_concept("color_blue")});
    }
    
    // Shape concepts
    if (features.edginess > 0.5f) {
        concepts.push_back({"shape_edgy", features.edginess,
                           graph_.get_or_create_concept("shape_edgy")});
    }
    if (features.edginess < 0.2f) {
        concepts.push_back({"shape_smooth", 1.0f - features.edginess,
                           graph_.get_or_create_concept("shape_smooth")});
    }
    
    // Motion concepts
    if (features.motion_magnitude > 0.3f) {
        concepts.push_back({"motion_detected", features.motion_magnitude,
                           graph_.get_or_create_concept("motion_detected")});
    }
    
    // Brightness concepts
    if (features.value > 0.7f) {
        concepts.push_back({"bright", features.value,
                           graph_.get_or_create_concept("bright")});
    } else if (features.value < 0.3f) {
        concepts.push_back({"dark", 1.0f - features.value,
                           graph_.get_or_create_concept("dark")});
    }
    
    return concepts;
}

void VisualContext::update_from_visual(uint64_t, const VisualFeatures& features) {
    // Convert features to semantic concepts
    std::vector<VisualConcept> concepts = features_to_concepts(features);
    
    // Inject into context field - track feature statistics
    for (size_t i = 0; i < concepts.size(); ++i) {
        const VisualConcept& vis_concept = concepts[i];
        
        // Track feature statistics
        FeatureStats& stats = feature_stats_[vis_concept.feature_type];
        stats.count++;
        stats.avg_activation = stats.avg_activation * 0.9f + vis_concept.activation * 0.1f;
        stats.frames_since_last = 0;
    }
    
    // Update all feature stats - increment frames since last
    for (auto it = feature_stats_.begin(); it != feature_stats_.end(); ++it) {
        it->second.frames_since_last++;
    }
}

// ============================================================================
// DIVERSITY SEEKING
// ============================================================================

float VisualContext::compute_diversity_score(const VisualFeatures& features) {
    /**
     * KEY ANTI-STICKING MECHANISM!
     * 
     * If Melvin has been looking at RED objects:
     *   red patches: diversity = -0.3 (suppress!)
     *   blue patches: diversity = +0.3 (boost!)
     * 
     * Forces visual variety!
     */
    
    if (feature_history_.empty()) {
        return 0.0f;  // No history, no bias
    }
    
    float diversity = 0.0f;
    
    // Check color diversity
    float avg_red = 0, avg_blue = 0;
    int recent_count = std::min(10, (int)feature_history_.size());
    
    for (int i = 0; i < recent_count; ++i) {
        const auto& hist = feature_history_[feature_history_.size() - 1 - i];
        avg_red += hist.red;
        avg_blue += hist.blue;
    }
    avg_red /= recent_count;
    avg_blue /= recent_count;
    
    // If been looking at lots of red, penalize more red
    if (avg_red > 0.6f && features.red > 0.6f) {
        diversity -= 0.2f;  // Too much red!
    }
    // If been looking at red, boost blue
    if (avg_red > 0.6f && features.blue > 0.6f) {
        diversity += 0.25f;  // Nice contrast!
    }
    
    // Check shape diversity
    float avg_edginess = 0;
    for (int i = 0; i < recent_count; ++i) {
        const auto& hist = feature_history_[feature_history_.size() - 1 - i];
        avg_edginess += hist.edginess;
    }
    avg_edginess /= recent_count;
    
    // If been looking at edgy things, prefer smooth
    float edge_diff = std::abs(features.edginess - avg_edginess);
    if (edge_diff > 0.3f) {
        diversity += 0.15f;  // Different shape type!
    }
    
    return std::max(-0.3f, std::min(0.3f, diversity));
}

bool VisualContext::is_visually_saturated() const {
    // Check if same features for many frames
    return frames_with_similar_features_ > 20;
}

float VisualContext::get_feature_contrast(const VisualFeatures& features) {
    if (feature_history_.empty()) {
        return 0.5f;  // Unknown, moderate
    }
    
    // Compare to recent history
    float total_diff = 0;
    int count = std::min(5, (int)feature_history_.size());
    
    for (int i = 0; i < count; ++i) {
        const auto& hist = feature_history_[feature_history_.size() - 1 - i];
        total_diff += feature_similarity(features, hist);
    }
    
    float avg_similarity = total_diff / count;
    return 1.0f - avg_similarity;  // High contrast = low similarity
}

// ============================================================================
// SEMANTIC BIAS
// ============================================================================

float VisualContext::get_feature_bias(const std::string& feature_name) const {
    auto it = feature_stats_.find(feature_name);
    if (it == feature_stats_.end()) {
        return 1.0f;  // No history, no bias
    }
    
    const auto& stats = it->second;
    
    // If this feature is over-represented (seen a lot recently)
    if (stats.count > 15 && stats.frames_since_last < 5) {
        return 0.7f;  // Suppress (seen too much!)
    }
    
    // If this feature is under-represented (not seen in a while)
    if (stats.frames_since_last > 20) {
        return 1.3f;  // Boost (seek novelty!)
    }
    
    return 1.0f;  // Neutral
}

ContextField::AttentionWeights VisualContext::compute_diversity_adjusted_weights(
    const ContextField::AttentionWeights& base_weights)
{
    ContextField::AttentionWeights adjusted = base_weights;
    
    // Detect repetitive patterns
    auto patterns = detect_repetition();
    
    for (const auto& pattern : patterns) {
        if (pattern.saturation_level > 0.7f) {
            // High saturation of one feature type
            // BOOST curiosity to break out!
            adjusted.gamma += 0.15f * pattern.saturation_level;
            adjusted.alpha -= 0.10f * pattern.saturation_level;
            
            adjusted.reason = "Diversity-seeking (visual saturation detected)";
        }
    }
    
    // Renormalize
    float total = adjusted.alpha + adjusted.beta + adjusted.gamma;
    if (total > 0) {
        adjusted.alpha /= total;
        adjusted.beta /= total;
        adjusted.gamma /= total;
    }
    
    return adjusted;
}

// ============================================================================
// HISTORY TRACKING
// ============================================================================

void VisualContext::record_focus(uint64_t patch_id, const VisualFeatures& features) {
    // Add to history
    feature_history_.push_back(features);
    if (feature_history_.size() > MAX_HISTORY) {
        feature_history_.pop_front();
    }
    
    // Check similarity to recent focuses
    if (feature_history_.size() > 1) {
        const auto& prev = feature_history_[feature_history_.size() - 2];
        float sim = feature_similarity(features, prev);
        
        if (sim > 0.7f) {
            frames_with_similar_features_++;
        } else {
            frames_with_similar_features_ = 0;
        }
    }
    
    last_dominant_features_ = features;
}

std::vector<VisualFeatures> VisualContext::get_recent_features(int count) const {
    std::vector<VisualFeatures> recent;
    int start = std::max(0, (int)feature_history_.size() - count);
    
    for (size_t i = start; i < feature_history_.size(); ++i) {
        recent.push_back(feature_history_[i]);
    }
    
    return recent;
}

std::vector<VisualContext::RepetitionPattern> VisualContext::detect_repetition() const {
    std::vector<RepetitionPattern> patterns;
    
    if (feature_history_.size() < 10) {
        return patterns;  // Not enough data
    }
    
    // Check for color repetition
    float red_count = 0, blue_count = 0, green_count = 0;
    int recent = std::min(20, (int)feature_history_.size());
    
    for (int i = 0; i < recent; ++i) {
        const auto& f = feature_history_[feature_history_.size() - 1 - i];
        if (f.red > 0.5f) red_count++;
        if (f.blue > 0.5f) blue_count++;
        if (f.green > 0.5f) green_count++;
    }
    
    if (red_count / recent > 0.6f) {
        RepetitionPattern p;
        p.feature_type = "color_red";
        p.consecutive_frames = red_count;
        p.saturation_level = red_count / recent;
        patterns.push_back(p);
    }
    
    // Check for shape repetition
    float edgy_count = 0, smooth_count = 0;
    for (int i = 0; i < recent; ++i) {
        const auto& f = feature_history_[feature_history_.size() - 1 - i];
        if (f.edginess > 0.5f) edgy_count++;
        if (f.edginess < 0.2f) smooth_count++;
    }
    
    if (edgy_count / recent > 0.6f) {
        RepetitionPattern p;
        p.feature_type = "shape_edgy";
        p.consecutive_frames = edgy_count;
        p.saturation_level = edgy_count / recent;
        patterns.push_back(p);
    }
    
    return patterns;
}

// ============================================================================
// VISUALIZATION
// ============================================================================

void VisualContext::print_visual_summary() const {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Visual Context Summary                                  ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    
    std::cout << "  Recent focus history: " << feature_history_.size() << " frames\n";
    std::cout << "  Similar features: " << frames_with_similar_features_ << " consecutive\n";
    
    if (!feature_history_.empty()) {
        const auto& latest = feature_history_.back();
        std::cout << "  Latest features:\n";
        std::cout << "    Color: R=" << latest.red << " G=" << latest.green << " B=" << latest.blue << "\n";
        std::cout << "    Edginess: " << latest.edginess << "\n";
        std::cout << "    Contrast: " << latest.contrast << "\n";
    }
    
    auto patterns = detect_repetition();
    if (!patterns.empty()) {
        std::cout << "  Repetition detected:\n";
        for (const auto& p : patterns) {
            std::cout << "    " << p.feature_type 
                     << " (saturation=" << p.saturation_level << ")\n";
        }
    }
    
    std::cout << std::endl;
}

std::string VisualContext::get_dominant_features_summary() const {
    if (feature_history_.empty()) {
        return "No visual history";
    }
    
    const auto& latest = feature_history_.back();
    std::ostringstream oss;
    
    if (latest.red > 0.6f) oss << "red ";
    if (latest.blue > 0.6f) oss << "blue ";
    if (latest.green > 0.6f) oss << "green ";
    if (latest.edginess > 0.5f) oss << "edgy ";
    if (latest.edginess < 0.2f) oss << "smooth ";
    
    std::string result = oss.str();
    return result.empty() ? "neutral" : result;
}

// ============================================================================
// HELPERS
// ============================================================================

float VisualContext::feature_similarity(const VisualFeatures& a, const VisualFeatures& b) const {
    // Color similarity
    float color_diff = std::abs(a.red - b.red) + 
                      std::abs(a.green - b.green) +
                      std::abs(a.blue - b.blue);
    float color_sim = 1.0f - (color_diff / 3.0f);
    
    // Shape similarity
    float shape_diff = std::abs(a.edginess - b.edginess);
    float shape_sim = 1.0f - shape_diff;
    
    // Overall similarity
    return (color_sim * 0.6f + shape_sim * 0.4f);
}

std::string VisualContext::get_dominant_feature_type() const {
    if (feature_stats_.empty()) {
        return "none";
    }
    
    std::string dominant;
    int max_count = 0;
    
    for (const auto& [name, stats] : feature_stats_) {
        if (stats.count > max_count && stats.frames_since_last < 10) {
            max_count = stats.count;
            dominant = name;
        }
    }
    
    return dominant.empty() ? "mixed" : dominant;
}

} // namespace melvin

