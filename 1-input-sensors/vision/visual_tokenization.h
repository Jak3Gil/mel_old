#pragma once

#include "../../9-other/core/types.h"
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <memory>

namespace melvin {
namespace vision {

/**
 * VisualFeatures - Low-level features extracted from a patch
 * (Retina-equivalent representation)
 */
struct VisualFeatures {
    // Color information
    float avg_red = 0.0f;
    float avg_green = 0.0f;
    float avg_blue = 0.0f;
    float brightness = 0.0f;
    
    // Texture information
    float texture_entropy = 0.0f;  // Complexity/randomness
    float edge_density = 0.0f;     // Amount of edges
    
    // Motion information
    float motion_magnitude = 0.0f;
    float motion_direction = 0.0f; // Angle in radians
    
    // Spatial location
    int patch_x = 0;
    int patch_y = 0;
    uint64_t frame_id = 0;
    
    // Similarity measure
    float similarity(const VisualFeatures& other) const {
        float color_dist = std::abs(avg_red - other.avg_red) +
                          std::abs(avg_green - other.avg_green) +
                          std::abs(avg_blue - other.avg_blue);
        color_dist /= 3.0f * 255.0f;  // Normalize
        
        float texture_dist = std::abs(texture_entropy - other.texture_entropy);
        float motion_dist = std::abs(motion_magnitude - other.motion_magnitude);
        
        // Combined similarity (higher = more similar)
        return 1.0f - ((color_dist * 0.5f + texture_dist * 0.3f + motion_dist * 0.2f));
    }
    
    // Check if spatially adjacent
    bool is_adjacent(const VisualFeatures& other) const {
        int dx = std::abs(patch_x - other.patch_x);
        int dy = std::abs(patch_y - other.patch_y);
        return (dx <= 1 && dy <= 1) && (dx + dy > 0);  // Adjacent, not same
    }
};

/**
 * VisualPatch - A single tokenized patch with graph node
 */
struct VisualPatch {
    NodeID node_id = 0;
    VisualFeatures features;
    float attention_score = 0.0f;  // From attention system
};

/**
 * ObjectCluster - Grouped patches forming a proto-object
 */
struct ObjectCluster {
    NodeID object_node_id = 0;
    std::vector<NodeID> patch_ids;  // Member patches
    
    // Aggregate features
    VisualFeatures centroid;        // Average features
    float coherence = 0.0f;         // How well patches group
    int size = 0;                   // Number of patches
    
    // Spatial bounds
    int min_x = 0, max_x = 0;
    int min_y = 0, max_y = 0;
    
    float area() const {
        return (max_x - min_x + 1) * (max_y - min_y + 1);
    }
};

/**
 * Scene - Collection of objects at a moment in time
 */
struct Scene {
    NodeID scene_node_id = 0;
    std::vector<NodeID> object_ids;
    uint64_t frame_id = 0;
    uint64_t timestamp = 0;
};

/**
 * VisualTokenizer - Converts raw frames into graph nodes
 * 
 * Pipeline:
 * Frame → Patches → VisualNodes → ObjectClusters → SceneNode → Graph
 */
class VisualTokenizer {
public:
    struct Config {
        int patch_size;            // Patch dimension (32×32 pixels)
        float attention_threshold;  // Min attention to process patch
        
        // Gestalt grouping parameters
        float similarity_threshold;   // Min similarity to cluster
        int min_cluster_size;            // Min patches for object
        int max_cluster_size;           // Max patches for object
        
        // Temporal parameters
        int scene_window;  // Remember last N scenes
        
        bool verbose;
        
        Config() : patch_size(32), attention_threshold(0.5f),
                   similarity_threshold(0.7f), min_cluster_size(3),
                   max_cluster_size(50), scene_window(10), verbose(false) {}
    };
    
    VisualTokenizer(const Config& config = Config());
    ~VisualTokenizer();
    
    /**
     * Process a frame: tokenize → cluster → create scene
     * 
     * @param frame_data Raw frame pixels (RGB)
     * @param width Frame width
     * @param height Frame height
     * @param attention_map Attention scores per patch
     * @return Scene node ID
     */
    NodeID process_frame(
        const uint8_t* frame_data,
        int width,
        int height,
        const std::vector<float>& attention_map
    );
    
    /**
     * Extract visual features from a patch
     */
    VisualFeatures extract_patch_features(
        const uint8_t* frame_data,
        int frame_width,
        int patch_x,
        int patch_y,
        int patch_size
    );
    
    /**
     * Group patches into object clusters using Gestalt principles
     */
    std::vector<ObjectCluster> cluster_patches(
        const std::vector<VisualPatch>& patches
    );
    
    /**
     * Create scene node from objects
     */
    NodeID create_scene_node(
        const std::vector<ObjectCluster>& objects,
        uint64_t frame_id
    );
    
    /**
     * Get statistics
     */
    struct Stats {
        uint64_t frames_processed = 0;
        uint64_t patches_created = 0;
        uint64_t objects_formed = 0;
        uint64_t scenes_recorded = 0;
        
        float avg_patches_per_frame = 0.0f;
        float avg_objects_per_scene = 0.0f;
        float avg_cluster_coherence = 0.0f;
    };
    
    Stats get_stats() const;
    void print_stats() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}} // namespace melvin::vision

