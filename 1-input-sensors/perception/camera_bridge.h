#pragma once

#include "../core/types_v2.h"
#include "../evolution/genome.h"
#include <vector>
#include <memory>

namespace melvin::v2::perception {

// ============================================================================
// CAMERA BRIDGE - Real Camera with Genome-Controlled Vision
// ============================================================================

/**
 * CameraBridge processes live camera frames with genome-controlled parameters.
 * 
 * Vision genes control:
 * - Edge detection thresholds
 * - Motion sensitivity
 * - Color processing weights
 * - Object formation parameters
 * 
 * Different genomes → different visual processing → different behaviors!
 */
class CameraBridge {
public:
    struct VisionConfig {
        // Edge detection (V1 cortex)
        float edge_threshold;        // 0.1-0.9 (from genome)
        float edge_weight;           // 0.0-1.0 (saliency contribution)
        
        // Motion detection (MT cortex)
        float motion_sensitivity;    // 0.1-2.0 (from genome)
        float motion_weight;         // 0.0-1.0 (saliency contribution)
        
        // Color processing (V4 cortex)
        float color_variance_threshold; // 0.05-0.8 (from genome)
        float color_weight;          // 0.0-1.0 (saliency contribution)
        
        // Object formation (IT cortex)
        float patch_size;            // 16-64 pixels (from genome)
        float min_object_size;       // 25-500 pixels (from genome)
        float grouping_threshold;    // 0.1-0.9 (from genome)
        
        // Novelty detection
        float novelty_threshold;     // 0.1-0.8 (from genome)
        
        VisionConfig()
            : edge_threshold(0.3f), edge_weight(0.3f),
              motion_sensitivity(0.5f), motion_weight(0.4f),
              color_variance_threshold(0.2f), color_weight(0.3f),
              patch_size(32.0f), min_object_size(100.0f), grouping_threshold(0.5f),
              novelty_threshold(0.3f) {}
    };
    
    CameraBridge(const evolution::Genome& genome);
    explicit CameraBridge(const VisionConfig& config);
    ~CameraBridge();
    
    /**
     * Process one camera frame
     * - Returns perceived objects with genome-computed features
     */
    std::vector<PerceivedObject> process_frame(const uint8_t* rgb_data, 
                                                int width, int height);
    
    /**
     * Compute saliency for detected object (genome-weighted)
     */
    float compute_saliency(const PerceptualFeatures& features) const;
    
    /**
     * Get vision config
     */
    VisionConfig get_config() const { return config_; }
    
    /**
     * Statistics
     */
    struct Stats {
        size_t frames_processed;
        size_t objects_detected;
        float avg_objects_per_frame;
        float avg_saliency;
    };
    
    Stats get_stats() const;
    void reset_stats();
    
private:
    VisionConfig config_;
    Stats stats_;
    
    // Previous frame for motion detection
    std::vector<uint8_t> prev_frame_;
    int prev_width_;
    int prev_height_;
    
    // Simple object detection (mock for now - would use v1 vision)
    std::vector<PerceivedObject> detect_objects(const uint8_t* rgb, int width, int height);
    
    // Feature extraction
    float compute_edge_score(const uint8_t* rgb, int x, int y, int width, int height);
    float compute_motion_score(int obj_id);
    float compute_color_variance(const uint8_t* rgb, int x, int y, int width, int height);
    
    // Helpers
    bool is_different_from_prev(const uint8_t* rgb, int width, int height);
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Extract vision config from genome
 */
CameraBridge::VisionConfig extract_vision_config(const evolution::Genome& genome);

} // namespace melvin::v2::perception

