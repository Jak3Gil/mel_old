#pragma once

#include "../core/types.h"
#include "../core/storage.h"
#include <string>
#include <vector>
#include <memory>

namespace melvin {
namespace perception {

// ============================================================================
// DETECTION STRUCTURES
// ============================================================================

/**
 * Single detected object with bounding box
 */
struct DetectedObject {
    std::string label;          // Object class label (e.g., "person", "car")
    float confidence;           // Detection confidence [0.0, 1.0]
    int x1, y1, x2, y2;        // Bounding box coordinates (top-left, bottom-right)
    
    DetectedObject() : confidence(0.0f), x1(0), y1(0), x2(0), y2(0) {}
    
    DetectedObject(const std::string& lbl, float conf, int bx1, int by1, int bx2, int by2)
        : label(lbl), confidence(conf), x1(bx1), y1(by1), x2(bx2), y2(by2) {}
    
    // Helper: compute bounding box area
    int area() const {
        return (x2 - x1) * (y2 - y1);
    }
    
    // Helper: compute IoU (Intersection over Union) with another box
    float iou(const DetectedObject& other) const;
};

/**
 * Single frame containing multiple detected objects
 */
struct SceneFrame {
    std::vector<DetectedObject> objects;  // All detected objects in this frame
    long long timestamp;                   // Timestamp (milliseconds since epoch)
    int frame_number;                      // Sequential frame number
    
    SceneFrame() : timestamp(0), frame_number(0) {}
    
    bool empty() const { return objects.empty(); }
    size_t object_count() const { return objects.size(); }
};

// ============================================================================
// YOLO DETECTOR WRAPPER
// ============================================================================

/**
 * Wrapper for Python YOLO detector script
 * Calls detect_objects.py and parses JSON output
 */
class YOLODetector {
public:
    struct Config {
        std::string python_script_path = "melvin/io/detect_objects.py";
        std::string model_name = "yolov8n.pt";  // YOLO model (n=nano, s=small, m=medium)
        float confidence_threshold = 0.25f;      // Minimum confidence
        bool verbose = false;
    };
    
    explicit YOLODetector(const Config& config = Config());
    ~YOLODetector();
    
    // Detect objects from image file
    std::vector<DetectedObject> detect_from_file(const std::string& image_path);
    
    // Detect objects from saved frame
    SceneFrame detect_frame(const std::string& frame_path, long long timestamp = 0, int frame_num = 0);
    
    void set_config(const Config& config);
    const Config& get_config() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// VISUAL PERCEPTION ENGINE
// ============================================================================

/**
 * Main perception engine that integrates YOLO detections into Melvin's graph
 * 
 * Rules:
 * - Each detected object → SENSORY node in graph
 * - Objects in SAME frame → EXACT edges (spatial co-occurrence)
 * - Objects in DIFFERENT frames → LEAP edges (temporal inference)
 */
class VisualPerception {
public:
    struct Config {
        // Node creation
        bool create_object_nodes = true;          // Create nodes for detected objects
        bool create_frame_summary_node = false;   // Create summary node per frame
        
        // Edge creation
        bool create_intra_frame_edges = true;     // EXACT edges between objects in same frame
        bool create_inter_frame_edges = true;     // LEAP edges between objects across frames
        float intra_frame_weight = 1.0f;          // Weight for same-frame edges
        float inter_frame_weight = 0.5f;          // Weight for cross-frame edges
        
        // Temporal tracking
        int temporal_window = 5;                  // How many previous frames to link
        bool track_object_persistence = true;     // Track same object across frames
        float iou_threshold = 0.5f;               // IoU threshold for object tracking
        
        // Filtering
        float min_confidence = 0.25f;             // Minimum detection confidence
        int min_box_area = 100;                   // Minimum bounding box area (pixels^2)
        
        // Debug
        bool verbose = false;
    };
    
    explicit VisualPerception(Storage* storage, const Config& config = Config());
    ~VisualPerception();
    
    // ========================================================================
    // CORE PROCESSING API
    // ========================================================================
    
    /**
     * Process a single scene frame and add to knowledge graph
     * Returns number of nodes created
     */
    int process_scene(const SceneFrame& scene);
    
    /**
     * Detect objects from image file and process into graph
     */
    int process_image_file(const std::string& image_path, YOLODetector& detector);
    
    /**
     * Start continuous visual processing loop from camera
     * Blocks until stopped (call stop() from another thread)
     */
    void start_visual_loop(int camera_index = 0);
    
    /**
     * Stop visual loop
     */
    void stop();
    
    /**
     * Check if visual loop is running
     */
    bool is_running() const;
    
    // ========================================================================
    // NODE/EDGE CREATION
    // ========================================================================
    
    /**
     * Create a SENSORY node for a detected object
     * Node content format: "object:<label>:<frame_num>:<object_idx>"
     */
    NodeID create_object_node(const DetectedObject& obj, int frame_num, int obj_idx);
    
    /**
     * Create EXACT edges between all objects in the same frame
     * Represents spatial co-occurrence
     */
    void create_intra_frame_edges(const std::vector<NodeID>& object_nodes, int frame_num);
    
    /**
     * Create LEAP edges between objects in different frames
     * Represents temporal inference and object persistence
     */
    void create_inter_frame_edges(const std::vector<NodeID>& curr_nodes, int frame_num);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct Stats {
        uint64_t frames_processed = 0;
        uint64_t objects_detected = 0;
        uint64_t nodes_created = 0;
        uint64_t exact_edges_created = 0;    // Intra-frame edges
        uint64_t leap_edges_created = 0;     // Inter-frame edges
        float avg_objects_per_frame = 0.0f;
        float avg_processing_time_ms = 0.0f;
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
// STANDALONE HELPER FUNCTIONS
// ============================================================================

/**
 * Detect objects from image file using YOLO
 * Convenience function that wraps YOLODetector
 */
SceneFrame detect_objects_from_frame(const std::string& frame_path);

/**
 * Test camera and display detections
 */
void test_perception_camera(int camera_index = 0);

} // namespace perception
} // namespace melvin


