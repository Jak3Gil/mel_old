#pragma once

#include "../core/atomic_graph.h"
#include <string>
#include <vector>
#include <atomic>

namespace melvin {
namespace vision {

/**
 * OpenCV Attention-Based Vision
 * Minimal, efficient integration with AtomicGraph
 */
class OpenCVAttention {
public:
    struct Config {
        int camera_index;
        int width;
        int height;
        std::string yolo_script;
        float confidence;
        float iou_threshold;
        int focus_size;
        int max_missing_frames;
        bool show_display;
        
        Config() : camera_index(0), width(640), height(480),
                   yolo_script("melvin/io/detect_objects.py"),
                   confidence(0.3f), iou_threshold(0.3f),
                   focus_size(300), max_missing_frames(5),
                   show_display(true) {}
    };
    
    OpenCVAttention(AtomicGraph* graph, const Config& config = Config());
    ~OpenCVAttention();
    
    void run();
    void stop();
    bool is_running() const;
    
    struct Stats {
        uint64_t frames;
        uint64_t detections;
        uint64_t concepts_created;
        uint64_t concepts_reused;
        float fps;
    };
    
    Stats get_stats() const;
    void print_stats() const;
    
private:
    class Impl;
    Impl* impl_;
};

} // namespace vision
} // namespace melvin

