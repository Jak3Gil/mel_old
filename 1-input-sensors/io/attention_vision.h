#pragma once

#include "../core/atomic_graph.h"
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <unordered_map>

namespace melvin {
namespace vision {

// ============================================================================
// ATTENTION-BASED VISUAL PERCEPTION (C++ Optimized)
// ============================================================================

/**
 * Human-like attention vision system
 * - ONE focus point at a time (like real human vision)
 * - Focuses on detected objects (not random noise)
 * - Weighted edges (co-occurrence tracking)
 * - Direct Storage integration (nodes.melvin/edges.melvin)
 * - 20+ FPS performance
 */
class AttentionVision {
public:
    struct Config {
        // Camera settings
        int camera_index;
        int frame_width;
        int frame_height;
        
        // YOLO settings
        std::string yolo_script;
        std::string yolo_model;
        float confidence_threshold;
        
        // Attention settings
        int focus_size;
        float iou_threshold;
        int max_missing_frames;
        
        // Performance
        bool show_display;
        bool verbose;
        
        Config()
            : camera_index(0), frame_width(640), frame_height(480),
              yolo_script("melvin/io/detect_objects.py"), yolo_model("yolov8n.pt"),
              confidence_threshold(0.3f), focus_size(300), iou_threshold(0.3f),
              max_missing_frames(5), show_display(true), verbose(false) {}
    };
    
    explicit AttentionVision(AtomicGraph* graph, const Config& config = Config());
    ~AttentionVision();
    
    // ========================================================================
    // CONTROL
    // ========================================================================
    
    /**
     * Start attention vision (blocking, runs until stopped)
     */
    void run();
    
    /**
     * Stop vision system
     */
    void stop();
    
    /**
     * Check if running
     */
    bool is_running() const;
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct Stats {
        uint64_t frames_processed = 0;
        uint64_t attention_shifts = 0;
        uint64_t objects_detected = 0;
        uint64_t concepts_created = 0;
        uint64_t concepts_reinforced = 0;
        uint64_t unique_edges = 0;
        uint64_t total_edge_weight = 0;  // Sum of all edge co-occurrences
        float current_fps = 0.0f;
    };
    
    Stats get_stats() const;
    void reset_stats();
    void print_stats() const;
    
    void set_config(const Config& config);
    const Config& get_config() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace vision
} // namespace melvin

