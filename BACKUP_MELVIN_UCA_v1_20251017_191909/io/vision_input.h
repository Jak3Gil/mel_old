#pragma once

#include "../core/storage.h"
#include <string>
#include <vector>
#include <memory>

namespace melvin {
namespace io {

/**
 * 👁️ Vision Input - Live Camera Integration for Melvin
 * 
 * Captures frames from camera and converts them into nodes in Melvin's brain.
 * Uses visual embeddings to represent what Melvin "sees".
 * 
 * [Live Vision Extension]
 */

// ============================================================================
// VISION NODE
// ============================================================================

struct VisionNode {
    std::vector<float> embedding;  // Visual embedding (from CLIP or pixel encoding)
    std::string label;             // "frame_timestamp" or detected object
    double timestamp;              // When captured
    int frame_number;              // Sequential frame ID
    
    VisionNode() : timestamp(0.0), frame_number(0) {}
};

// ============================================================================
// VISION ENCODER
// ============================================================================

class VisionEncoder {
public:
    struct Config {
        int embedding_dim = 256;        // Size of visual embedding
        int downsample_width = 16;      // Downsample frames to 16x16
        int downsample_height = 16;
        bool normalize = true;          // Normalize embeddings
        bool use_grayscale = false;     // Convert to grayscale first
        
        Config() {}
    };
    
    explicit VisionEncoder(const Config& config = Config());
    ~VisionEncoder();
    
    // Encode a frame into embedding
    std::vector<float> encode_frame(const void* frame_data, int width, int height, int channels);
    
    // Encode with OpenCV Mat (if available)
    #ifdef HAVE_OPENCV
    std::vector<float> encode_mat(const cv::Mat& frame);
    #endif
    
    void set_config(const Config& config);
    const Config& get_config() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// LIVE VISION STREAM
// ============================================================================

class LiveVisionStream {
public:
    struct Config {
        int camera_index = 0;           // Which camera to use
        int fps = 10;                    // Frames per second to process
        int embedding_dim = 256;         // Visual embedding size
        float node_merge_threshold = 0.9f; // Similarity to merge nodes
        bool create_temporal_edges = true;  // Link consecutive frames
        bool link_to_context = true;     // Link to active reasoning context
        float context_link_weight = 0.5f; // Weight for context edges
        bool verbose = false;            // Print frame processing logs
        
        Config() {}
    };
    
    explicit LiveVisionStream(Storage* storage, const Config& config = Config());
    ~LiveVisionStream();
    
    // ========================================================================
    // STREAMING CONTROL
    // ========================================================================
    
    // Start capturing (runs until stopped)
    void start();
    
    // Stop capturing
    void stop();
    
    // Check if running
    bool is_running() const;
    
    // Process one frame manually
    NodeID process_frame(const void* frame_data, int width, int height, int channels);
    
    // ========================================================================
    // NODE CREATION
    // ========================================================================
    
    // Create or merge visual node from frame
    NodeID create_visual_node(const VisionNode& vision_node);
    
    // Link frame to temporal sequence
    void create_temporal_link(NodeID prev_frame, NodeID curr_frame);
    
    // Link frame to active reasoning context
    void link_to_context(NodeID frame_node);
    
    // ========================================================================
    // CONTEXT MANAGEMENT
    // ========================================================================
    
    // Set current active context node (for linking)
    void set_active_context(NodeID context_node);
    
    // Get current active context
    NodeID get_active_context() const;
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct Stats {
        uint64_t frames_processed = 0;
        uint64_t nodes_created = 0;
        uint64_t nodes_merged = 0;
        uint64_t temporal_edges = 0;
        uint64_t context_edges = 0;
        float avg_processing_time_ms = 0.0f;
    };
    
    Stats get_stats() const;
    void reset_stats();
    void print_stats() const;
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void set_config(const Config& config);
    const Config& get_config() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// STANDALONE HELPER FUNCTIONS
// ============================================================================

// Check if OpenCV is available
bool is_opencv_available();

// Start simple vision test (displays camera feed)
void test_camera(int camera_index = 0);

} // namespace io
} // namespace melvin




