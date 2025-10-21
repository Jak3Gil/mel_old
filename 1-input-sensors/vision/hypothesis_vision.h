/*
 * Melvin Spatial Hypothesis Graph Vision System
 * 
 * Self-organizing perceptual tokenization:
 * Patch → Hypothesis → Object → Scene → Concept
 * 
 * No labels. No datasets. Pure unsupervised structure discovery.
 */

#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>

namespace melvin {
namespace vision {

using NodeID = uint32_t;

// Edge types for spatial hypothesis graph
enum class EdgeType {
    SPATIALLY_NEAR,    // Adjacent grid cells
    PART_OF,           // Multi-scale: small → large
    OBSERVES,          // Hypothesis → Patch
    NEAR,              // Hypothesis overlap
    PREDICTS,          // Hypothesis → Object
    INSTANCE_OF,       // Object → Concept
    TEMPORAL_NEXT,     // Frame-to-frame tracking
    CO_OCCURS_WITH,    // Objects in same scene
    ABOVE, BELOW,      // Spatial relations
    LEFT_OF, RIGHT_OF,
    INSIDE, CONTAINS,
    EXACT, LEAP        // Temporal (from existing system)
};

// Visual patch: lowest-level feature extraction
struct VisualPatch {
    NodeID id;
    int x, y;           // Grid position
    int scale;          // Resolution level (0=finest)
    
    // Features
    cv::Scalar color_mean;
    float color_std;
    float brightness;
    float edge_density;
    float motion;
    float texture_entropy;
    
    // Attention
    float saliency;
    float focus_score;
    
    cv::Mat embedding;  // Small feature vector
};

// Hypothesis: candidate region/object boundary
struct Hypothesis {
    NodeID id;
    std::vector<NodeID> patch_ids;  // Observed patches
    
    // Shape properties
    cv::Rect bbox;
    cv::RotatedRect oriented_bbox;
    std::vector<cv::Point> contour;
    
    // Confidence & tracking
    float confidence;
    float stability;    // How long it's been tracked
    int age_frames;
    
    // Predicted object
    NodeID predicted_object_id;
    
    // Visual embedding (averaged from patches)
    cv::Mat embedding;
};

// Object: merged, confirmed hypothesis
struct ObjectNode {
    NodeID id;
    std::vector<NodeID> hypothesis_ids;
    
    // Properties
    cv::Rect bbox;
    cv::Point2f center;
    cv::Point2f velocity;
    
    // Belief & persistence
    float belief_score;
    int frames_tracked;
    int frames_since_seen;
    
    // Concept link
    NodeID concept_id;  // 0 if no concept yet
    
    // Features
    cv::Scalar dominant_color;
    std::string shape_type;  // "blob", "edge", "corner", etc.
    cv::Mat embedding;
};

// Scene: spatial arrangement of objects
struct SceneNode {
    NodeID id;
    int frame_number;
    std::vector<NodeID> object_ids;
    
    // Spatial relationships stored as edges
    // e.g., obj1 ABOVE obj2, obj3 INSIDE obj1
    
    NodeID prev_scene_id;  // Temporal link
};

// Concept: abstract category discovered from patterns
struct ConceptNode {
    NodeID id;
    std::string label;  // Generated name
    
    int instance_count;
    float prototype_distance_threshold;
    
    cv::Mat prototype_embedding;
};

// Edge in the hypothesis graph
struct GraphEdge {
    NodeID from;
    NodeID to;
    EdgeType type;
    float weight;
    
    // Optional metadata
    std::unordered_map<std::string, float> attributes;
};

// ============================================================================
// MAIN HYPOTHESIS VISION SYSTEM
// ============================================================================

class HypothesisVision {
public:
    struct Config {
        // Grid configuration
        std::vector<int> scales;  // e.g., {16, 32, 64, 128}
        
        // Hypothesis generation
        int max_hypotheses_per_scale;
        float hypothesis_confidence_threshold;
        float merge_overlap_threshold;
        float merge_similarity_threshold;
        
        // Object tracking
        float object_match_threshold;
        int object_max_missing_frames;
        
        // Concept formation
        int concept_min_instances;
        float concept_similarity_threshold;
        
        // Prediction
        bool enable_prediction;
        float prediction_learning_rate;
        
        Config();
    };
    
    HypothesisVision(const Config& config = Config());
    ~HypothesisVision();
    
    // Main processing pipeline
    SceneNode process_frame(const cv::Mat& frame);
    
    // Multi-scale grid formation
    std::vector<std::vector<VisualPatch>> create_spatial_grid(const cv::Mat& frame);
    
    // Hypothesis generation & refinement
    std::vector<Hypothesis> generate_hypotheses(
        const std::vector<std::vector<VisualPatch>>& grids
    );
    
    void refine_hypotheses(
        std::vector<Hypothesis>& hypotheses,
        const cv::Mat& frame
    );
    
    // Object formation from hypotheses
    std::vector<ObjectNode> merge_into_objects(
        const std::vector<Hypothesis>& hypotheses
    );
    
    // Temporal tracking
    void track_objects_temporal(
        std::vector<ObjectNode>& current_objects
    );
    
    // Scene construction
    SceneNode construct_scene(
        const std::vector<ObjectNode>& objects
    );
    
    // Concept emergence
    void update_concepts(
        const std::vector<ObjectNode>& objects
    );
    
    // Predictive feedback
    void predict_next_frame();
    void compare_prediction_to_reality(const cv::Mat& frame);
    
    // Visualization
    cv::Mat visualize_graph(const cv::Mat& frame) const;
    
    // Graph access
    const std::vector<VisualPatch>& get_patches() const;
    const std::vector<Hypothesis>& get_hypotheses() const;
    const std::vector<ObjectNode>& get_objects() const;
    const std::vector<SceneNode>& get_scenes() const;
    const std::vector<ConceptNode>& get_concepts() const;
    const std::vector<GraphEdge>& get_edges() const;
    
    // Statistics
    struct Stats {
        int frames_processed;
        int total_patches;
        int total_hypotheses;
        int total_objects;
        int total_scenes;
        int total_concepts;
        int total_edges;
        
        float avg_hypotheses_per_frame;
        float avg_objects_per_frame;
        float prediction_accuracy;
    };
    
    Stats get_stats() const;
    void print_stats() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}} // namespace melvin::vision

