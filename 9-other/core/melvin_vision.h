#pragma once

#include "atomic_graph.h"
#include <vector>
#include <string>
#include <cstdint>

namespace melvin {
namespace uca {

// ============================================================================
// BIOLOGICAL ANALOG: V1 → V2 → V4 → IT (Ventral "What" Pathway)
// ============================================================================

/**
 * FrameNode: Raw sensory input (Retina → LGN → V1)
 * Represents a single visual patch at the lowest level
 */
struct FrameNode {
    int x, y, w, h;              // Spatial location
    float luminance;             // Average brightness
    float contrast;              // Local contrast
    float edge_density;          // Edge content (V1 simple cells)
    float color[3];              // RGB channels
    float motion_dx, motion_dy;  // Optical flow (MT/V5)
    
    uint64_t graph_id;           // Corresponding node in AtomicGraph
    double timestamp;
    
    FrameNode() : x(0), y(0), w(0), h(0), luminance(0), contrast(0), 
                  edge_density(0), motion_dx(0), motion_dy(0), 
                  graph_id(0), timestamp(0) {
        color[0] = color[1] = color[2] = 0;
    }
};

/**
 * VisualNode: Mid-level object representation (V2 → V4)
 * Gestalt grouping: continuity, proximity, similarity, closure
 */
struct VisualNode {
    std::string object_id;       // "object_0", "object_1", etc.
    std::vector<FrameNode*> patches; // Constituent patches
    
    // Spatial properties
    int center_x, center_y;
    int bbox_x1, bbox_y1, bbox_x2, bbox_y2;
    float area;
    
    // Feature properties (V4 complex features)
    float avg_luminance;
    float avg_contrast;
    float dominant_color[3];
    float shape_complexity;      // Perimeter²/Area
    float motion_magnitude;
    float motion_direction;      // Radians
    
    // Attention properties
    float saliency;              // Bottom-up salience
    float relevance;             // Top-down goal match
    float curiosity;             // Prediction error
    float focus_score;           // Combined F = αS + βR + γC
    
    // Temporal tracking
    int frames_visible;
    int frames_since_last_seen;
    uint64_t persistent_id;      // Tracking across frames
    
    // Graph integration
    uint64_t graph_id;           // Node in AtomicGraph
    std::vector<uint64_t> concept_links; // Links to concepts
    
    VisualNode() : center_x(0), center_y(0), 
                   bbox_x1(0), bbox_y1(0), bbox_x2(0), bbox_y2(0),
                   area(0), avg_luminance(0), avg_contrast(0),
                   shape_complexity(0), motion_magnitude(0), motion_direction(0),
                   saliency(0), relevance(0), curiosity(0), focus_score(0),
                   frames_visible(0), frames_since_last_seen(0),
                   persistent_id(0), graph_id(0) {
        dominant_color[0] = dominant_color[1] = dominant_color[2] = 0;
    }
};

/**
 * ConceptNode: High-level semantic representation (IT cortex)
 * Links visual objects to learned categories
 */
struct ConceptNode {
    std::string label;           // "person", "fire", "tool", etc.
    float confidence;            // Recognition confidence
    uint64_t graph_id;           // Concept node in graph
    
    std::vector<uint64_t> instance_ids; // Objects that exemplify this
    
    ConceptNode() : confidence(0), graph_id(0) {}
};

// ============================================================================
// VISION PIPELINE - Hierarchical Processing
// ============================================================================

/**
 * MelvinVision: Complete visual processing system
 * 
 * Pipeline:
 * 1. Low-level feature extraction (V1: edges, orientation, color)
 * 2. Gestalt grouping (V2: continuity, proximity, similarity)
 * 3. Object formation (V4: complex shapes, surfaces)
 * 4. Attention computation (Parietal: saliency map)
 * 5. Concept linking (IT: semantic categories)
 * 6. Graph integration (store in AtomicGraph)
 * 
 * Biological mapping:
 * - extract_patches() → V1 simple/complex cells
 * - group_patches() → V2 boundary detection
 * - form_objects() → V4 shape/surface processing
 * - compute_saliency() → Posterior parietal cortex
 * - link_concepts() → Inferior temporal cortex
 */
class MelvinVision {
public:
    struct Config {
        int patch_size;           // Grid resolution (default: 32px)
        int min_patches_per_object; // Minimum for grouping (default: 4)
        float proximity_threshold;  // Max distance for grouping (default: 64px)
        float similarity_threshold; // Max feature diff for grouping (default: 0.3)
        float motion_threshold;     // Motion detection sensitivity (default: 2.0px)
        
        // Attention weights (FEF/SC analog)
        float alpha_saliency;     // Bottom-up weight (default: 0.4)
        float beta_relevance;     // Top-down weight (default: 0.3)
        float gamma_curiosity;    // Prediction error (default: 0.3)
        
        bool enable_tracking;     // Object persistence across frames
        bool enable_prediction;   // Predictive coding (forward model)
        
        Config() : patch_size(32), min_patches_per_object(4),
                   proximity_threshold(64.0f), similarity_threshold(0.3f),
                   motion_threshold(2.0f),
                   alpha_saliency(0.4f), beta_relevance(0.3f), gamma_curiosity(0.3f),
                   enable_tracking(true), enable_prediction(true) {}
    };
    
    MelvinVision(const Config& config = Config());
    ~MelvinVision();
    
    // ========================================================================
    // MAIN PIPELINE
    // ========================================================================
    
    /**
     * Process one frame through complete visual hierarchy
     * Returns: List of detected visual objects with attention scores
     */
    std::vector<VisualNode> process_frame(
        const uint8_t* image_data,
        int width,
        int height,
        int channels,
        double timestamp
    );
    
    /**
     * Integrate visual objects into knowledge graph
     * Creates nodes and edges for objects, relations, and concepts
     */
    void integrate_to_graph(
        const std::vector<VisualNode>& objects,
        AtomicGraph& graph
    );
    
    // ========================================================================
    // TOP-DOWN MODULATION (Prefrontal → Parietal)
    // ========================================================================
    
    /**
     * Set active goal/concept to bias attention
     * Example: If reasoning about "fire", boost relevance of hot/red objects
     */
    void set_active_concepts(const std::vector<std::string>& concepts);
    
    /**
     * Predict expected objects based on context
     * Used for predictive coding and curiosity computation
     */
    void set_predictions(const std::vector<std::string>& expected_objects);
    
    // ========================================================================
    // STATE QUERIES
    // ========================================================================
    
    /**
     * Get currently tracked objects (across frames)
     */
    const std::vector<VisualNode>& get_tracked_objects() const { return tracked_objects_; }
    
    /**
     * Get prediction errors (high curiosity candidates)
     */
    std::vector<VisualNode> get_surprising_objects() const;
    
    /**
     * Get statistics
     */
    void print_stats() const;
    
private:
    // ========================================================================
    // VISUAL HIERARCHY STAGES
    // ========================================================================
    
    /**
     * STAGE 1: V1 analog - Extract low-level features
     * Creates grid of FrameNodes with edge, color, motion info
     */
    std::vector<FrameNode> extract_patches(
        const uint8_t* image_data,
        int width, int height, int channels
    );
    
    /**
     * STAGE 2: V2 analog - Gestalt grouping
     * Groups patches by proximity, similarity, continuity
     */
    std::vector<std::vector<FrameNode*>> group_patches(
        std::vector<FrameNode>& patches
    );
    
    /**
     * STAGE 3: V4 analog - Form object representations
     * Creates VisualNodes with shape/surface properties
     */
    std::vector<VisualNode> form_objects(
        const std::vector<std::vector<FrameNode*>>& groups
    );
    
    /**
     * STAGE 4: Parietal analog - Compute saliency
     * Bottom-up attention: contrast, color pop-out, motion
     */
    void compute_saliency(std::vector<VisualNode>& objects);
    
    /**
     * STAGE 5: Prefrontal analog - Compute relevance
     * Top-down attention: goal matching, concept activation
     */
    void compute_relevance(std::vector<VisualNode>& objects);
    
    /**
     * STAGE 6: Predictive coding - Compute curiosity
     * Prediction error: mismatch between expected and observed
     */
    void compute_curiosity(std::vector<VisualNode>& objects);
    
    /**
     * STAGE 7: FEF/SC analog - Combine attention scores
     * Focus arbitration: F = αS + βR + γC
     */
    void compute_focus_scores(std::vector<VisualNode>& objects);
    
    /**
     * STAGE 8: IT analog - Link to semantic concepts
     * Match objects to learned categories
     */
    void link_concepts(std::vector<VisualNode>& objects, AtomicGraph& graph);
    
    /**
     * STAGE 9: Temporal tracking
     * Maintain object identity across frames (MT/MST)
     */
    void track_objects(std::vector<VisualNode>& new_objects);
    
    // ========================================================================
    // HELPER FUNCTIONS
    // ========================================================================
    
    float compute_feature_similarity(const FrameNode& a, const FrameNode& b);
    float compute_spatial_distance(const FrameNode& a, const FrameNode& b);
    float compute_object_similarity(const VisualNode& a, const VisualNode& b);
    
    // Predictive forward model
    VisualNode predict_object_state(const VisualNode& obj, float dt);
    float compute_prediction_error(const VisualNode& predicted, const VisualNode& observed);
    
    // ========================================================================
    // STATE
    // ========================================================================
    
    Config config_;
    
    // Previous frame data (for motion/tracking)
    std::vector<FrameNode> prev_patches_;
    std::vector<VisualNode> tracked_objects_;
    
    // Top-down modulation
    std::vector<std::string> active_concepts_;
    std::vector<std::string> predicted_objects_;
    
    // Frame counter
    uint64_t frame_count_;
    uint64_t next_object_id_;
    
    // Statistics
    uint64_t total_patches_processed_;
    uint64_t total_objects_formed_;
};

} // namespace uca
} // namespace melvin


