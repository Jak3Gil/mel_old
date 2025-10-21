#pragma once

#include "visual_tokenization.h"
#include "../../9-other/core/types.h"
#include "../../9-other/core/optimized_storage.h"
#include "../../5-learning-chemistry/adaptive_window.h"
#include <opencv2/opencv.hpp>
#include <memory>
#include <vector>

namespace melvin {
namespace vision {

/**
 * VisionPipeline - Complete Camera → Graph Integration
 * 
 * Pipeline:
 * 1. Capture frame from camera
 * 2. Compute attention scores (S + G + C + D)
 * 3. Tokenize into visual patches
 * 4. Cluster patches into objects (Gestalt)
 * 5. Create scene node
 * 6. Add to graph with adaptive EXACT window
 * 
 * Uses:
 * - OpenCV for camera capture
 * - Custom attention algorithm
 * - VisualTokenizer for object formation
 * - AdaptiveWindowManager for temporal connections
 * - OptimizedStorage for graph persistence
 */
class VisionPipeline {
public:
    struct Config {
        // Camera settings
        int camera_id;
        int frame_width;
        int frame_height;
        int fps;
        
        // Attention settings
        int patch_size;
        float attention_threshold;
        
        // Clustering settings
        float similarity_threshold;
        int min_cluster_size;
        int max_cluster_size;
        
        // Adaptive window settings
        adaptive::AdaptiveWindowConfig adaptive_config;
        
        // Visualization
        bool show_window;
        bool verbose;
        
        Config() : camera_id(0), frame_width(640), frame_height(480), fps(30),
                   patch_size(32), attention_threshold(0.6f),
                   similarity_threshold(0.7f), min_cluster_size(3), max_cluster_size(50),
                   show_window(true), verbose(false) {}
    };
    
    VisionPipeline(optimized::OptimizedStorage* storage, const Config& config = Config());
    ~VisionPipeline();
    
    /**
     * Initialize camera and vision systems
     */
    bool initialize();
    
    /**
     * Process one frame from camera
     * 
     * @return Scene node ID (0 if failed)
     */
    NodeID process_frame();
    
    /**
     * Run continuous vision loop
     * 
     * @param max_frames Maximum frames to process (0 = infinite)
     */
    void run(int max_frames = 0);
    
    /**
     * Compute attention scores for frame
     * Returns vector of focus scores per patch
     */
    std::vector<float> compute_attention(const cv::Mat& frame);
    
    /**
     * Statistics
     */
    struct Stats {
        uint64_t frames_processed = 0;
        uint64_t patches_created = 0;
        uint64_t objects_formed = 0;
        uint64_t scenes_created = 0;
        uint64_t nodes_in_graph = 0;
        uint64_t edges_in_graph = 0;
        
        float avg_patches_per_frame = 0.0f;
        float avg_objects_per_frame = 0.0f;
        float avg_fps = 0.0f;
    };
    
    Stats get_stats() const;
    void print_stats() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * AttentionScores - Per-patch attention breakdown
 */
struct AttentionScores {
    float saliency = 0.0f;     // Contrast
    float goal = 0.0f;         // Motion
    float curiosity = 0.0f;    // Edge density
    float diversity = 0.0f;    // Novelty bonus
    float focus = 0.0f;        // Total: S + G + C + D
    
    int patch_x = 0;
    int patch_y = 0;
};

}} // namespace melvin::vision

