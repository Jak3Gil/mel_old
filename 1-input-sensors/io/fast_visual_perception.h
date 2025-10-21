#pragma once

#include "../core/types.h"
#include "../core/storage.h"
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>

namespace melvin {
namespace vision {

// ============================================================================
// FAST VISUAL PERCEPTION ENGINE (Optimized C++)
// ============================================================================

/**
 * High-performance visual perception engine
 * 
 * Features:
 * - 20+ FPS real-time processing
 * - Direct integration with Storage (nodes.melvin/edges.melvin)
 * - Multi-threaded: capture thread + processing thread
 * - Frame skipping for consistent FPS
 * - Batched graph updates
 * - Zero-copy where possible
 */
class FastVisualPerception {
public:
    struct Config {
        // Camera settings
        int camera_index = 0;
        int frame_width = 640;
        int frame_height = 480;
        int target_fps = 20;
        
        // YOLO settings
        std::string yolo_model = "yolov8n.pt";
        float confidence_threshold = 0.3f;
        
        // Processing optimization
        int process_every_n_frames = 1;  // 1 = every frame, 2 = every other frame
        int max_objects_per_frame = 50;   // Limit for performance
        bool use_threading = true;
        int batch_size = 10;              // Batch graph updates
        
        // Graph integration
        bool create_intra_frame_edges = true;
        bool create_inter_frame_edges = true;
        int temporal_window = 3;          // Smaller for speed
        float intra_weight = 1.0f;
        float inter_weight = 0.5f;
        
        // Filtering
        float min_confidence = 0.3f;
        int min_box_area = 100;
        
        // Debug
        bool show_display = true;
        bool verbose = false;
    };
    
    explicit FastVisualPerception(Storage* storage, const Config& config = Config());
    ~FastVisualPerception();
    
    // ========================================================================
    // CONTROL
    // ========================================================================
    
    /**
     * Start real-time visual perception
     * Non-blocking: runs in background threads
     */
    void start();
    
    /**
     * Stop visual perception and flush all data
     */
    void stop();
    
    /**
     * Check if running
     */
    bool is_running() const;
    
    /**
     * Wait until stopped (blocking)
     */
    void wait_until_stopped();
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct Stats {
        std::atomic<uint64_t> frames_captured{0};
        std::atomic<uint64_t> frames_processed{0};
        std::atomic<uint64_t> frames_skipped{0};
        std::atomic<uint64_t> objects_detected{0};
        std::atomic<uint64_t> nodes_created{0};
        std::atomic<uint64_t> exact_edges{0};
        std::atomic<uint64_t> leap_edges{0};
        std::atomic<float> current_fps{0.0f};
        std::atomic<float> avg_processing_ms{0.0f};
        std::atomic<uint64_t> graph_flushes{0};
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

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Quick camera test
 */
bool test_camera_access(int camera_index = 0);

/**
 * Benchmark vision system performance
 */
void benchmark_vision_system(int duration_seconds = 30);

} // namespace vision
} // namespace melvin

