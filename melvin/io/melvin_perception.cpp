/*
 * Melvin Visual Perception Engine (YOLO Integration)
 * 
 * Converts YOLO object detections into Melvin's knowledge graph:
 * - Each bounding box → SENSORY node
 * - Same frame objects → EXACT edges (spatial co-occurrence)
 * - Different frame objects → LEAP edges (temporal inference)
 */

#include "melvin_perception.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <algorithm>
#include <cmath>

// JSON parsing (simple manual parser for detection results)
#include <cstring>

namespace melvin {
namespace perception {

// ============================================================================
// DETECTED OBJECT HELPERS
// ============================================================================

float DetectedObject::iou(const DetectedObject& other) const {
    // Compute intersection
    int ix1 = std::max(x1, other.x1);
    int iy1 = std::max(y1, other.y1);
    int ix2 = std::min(x2, other.x2);
    int iy2 = std::min(y2, other.y2);
    
    if (ix2 < ix1 || iy2 < iy1) {
        return 0.0f;  // No intersection
    }
    
    int intersection = (ix2 - ix1) * (iy2 - iy1);
    int union_area = area() + other.area() - intersection;
    
    return union_area > 0 ? static_cast<float>(intersection) / union_area : 0.0f;
}

// ============================================================================
// SIMPLE JSON PARSER FOR YOLO OUTPUT
// ============================================================================

namespace {
    // Parse YOLO detection JSON output
    std::vector<DetectedObject> parse_yolo_json(const std::string& json_str) {
        std::vector<DetectedObject> objects;
        
        // Very simple JSON parser for detection format:
        // [{"label": "person", "confidence": 0.95, "x1": 100, "y1": 200, "x2": 300, "y2": 400}, ...]
        
        size_t pos = 0;
        while (true) {
            // Find next object
            size_t obj_start = json_str.find("{", pos);
            if (obj_start == std::string::npos) break;
            
            size_t obj_end = json_str.find("}", obj_start);
            if (obj_end == std::string::npos) break;
            
            std::string obj_str = json_str.substr(obj_start, obj_end - obj_start + 1);
            
            DetectedObject obj;
            
            // Parse label
            size_t label_pos = obj_str.find("\"label\"");
            if (label_pos != std::string::npos) {
                size_t val_start = obj_str.find("\"", label_pos + 7);
                size_t val_end = obj_str.find("\"", val_start + 1);
                if (val_start != std::string::npos && val_end != std::string::npos) {
                    obj.label = obj_str.substr(val_start + 1, val_end - val_start - 1);
                }
            }
            
            // Parse confidence
            size_t conf_pos = obj_str.find("\"confidence\"");
            if (conf_pos != std::string::npos) {
                size_t val_start = obj_str.find(":", conf_pos);
                if (val_start != std::string::npos) {
                    obj.confidence = std::stof(obj_str.substr(val_start + 1));
                }
            }
            
            // Parse bounding box
            auto parse_int = [&](const std::string& key) -> int {
                size_t key_pos = obj_str.find("\"" + key + "\"");
                if (key_pos != std::string::npos) {
                    size_t val_start = obj_str.find(":", key_pos);
                    if (val_start != std::string::npos) {
                        return std::stoi(obj_str.substr(val_start + 1));
                    }
                }
                return 0;
            };
            
            obj.x1 = parse_int("x1");
            obj.y1 = parse_int("y1");
            obj.x2 = parse_int("x2");
            obj.y2 = parse_int("y2");
            
            objects.push_back(obj);
            pos = obj_end + 1;
        }
        
        return objects;
    }
}

// ============================================================================
// YOLO DETECTOR IMPLEMENTATION
// ============================================================================

class YOLODetector::Impl {
public:
    Config config;
    
    Impl(const Config& cfg) : config(cfg) {}
    
    std::vector<DetectedObject> detect_from_file(const std::string& image_path) {
        // Build command to call Python script
        std::stringstream cmd;
        cmd << "python3 " << config.python_script_path 
            << " " << image_path
            << " " << config.model_name
            << " " << config.confidence_threshold;
        
        if (config.verbose) {
            std::cout << "[YOLO] Running: " << cmd.str() << "\n";
        }
        
        // Execute and capture output
        FILE* pipe = popen(cmd.str().c_str(), "r");
        if (!pipe) {
            std::cerr << "[YOLO] ERROR: Failed to run detection script\n";
            return {};
        }
        
        std::stringstream json_output;
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            json_output << buffer;
        }
        
        int status = pclose(pipe);
        
        if (status != 0) {
            std::cerr << "[YOLO] ERROR: Detection script failed with code " << status << "\n";
            std::cerr << "[YOLO] Output: " << json_output.str() << "\n";
            return {};
        }
        
        // Parse JSON output
        std::string json_str = json_output.str();
        if (config.verbose) {
            std::cout << "[YOLO] Detection output: " << json_str << "\n";
        }
        
        return parse_yolo_json(json_str);
    }
};

YOLODetector::YOLODetector(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {}

YOLODetector::~YOLODetector() = default;

std::vector<DetectedObject> YOLODetector::detect_from_file(const std::string& image_path) {
    return impl_->detect_from_file(image_path);
}

SceneFrame YOLODetector::detect_frame(const std::string& frame_path, long long timestamp, int frame_num) {
    SceneFrame frame;
    frame.objects = detect_from_file(frame_path);
    frame.timestamp = timestamp > 0 ? timestamp : 
        std::chrono::system_clock::now().time_since_epoch().count();
    frame.frame_number = frame_num;
    return frame;
}

void YOLODetector::set_config(const Config& config) {
    impl_->config = config;
}

const YOLODetector::Config& YOLODetector::get_config() const {
    return impl_->config;
}

// ============================================================================
// VISUAL PERCEPTION IMPLEMENTATION
// ============================================================================

class VisualPerception::Impl {
public:
    Storage* storage;
    Config config;
    Stats stats;
    
    // Frame history for temporal linking
    struct FrameHistory {
        int frame_num;
        std::vector<NodeID> object_nodes;
        std::vector<DetectedObject> objects;
    };
    std::vector<FrameHistory> frame_history;
    
    // Visual loop control
    std::atomic<bool> running{false};
    std::thread visual_thread;
    
    Impl(Storage* stor, const Config& cfg) 
        : storage(stor), config(cfg) {}
    
    ~Impl() {
        if (running) {
            running = false;
            if (visual_thread.joinable()) {
                visual_thread.join();
            }
        }
    }
    
    void add_to_history(int frame_num, const std::vector<NodeID>& nodes, 
                        const std::vector<DetectedObject>& objects) {
        FrameHistory hist;
        hist.frame_num = frame_num;
        hist.object_nodes = nodes;
        hist.objects = objects;
        
        frame_history.push_back(hist);
        
        // Keep only recent frames
        if (frame_history.size() > static_cast<size_t>(config.temporal_window)) {
            frame_history.erase(frame_history.begin());
        }
    }
};

VisualPerception::VisualPerception(Storage* storage, const Config& config)
    : impl_(std::make_unique<Impl>(storage, config)) {}

VisualPerception::~VisualPerception() = default;

// ========================================================================
// CORE PROCESSING
// ========================================================================

int VisualPerception::process_scene(const SceneFrame& scene) {
    auto start_time = std::chrono::steady_clock::now();
    
    if (scene.empty()) {
        if (impl_->config.verbose) {
            std::cout << "[Perception] Frame " << scene.frame_number << ": No objects detected\n";
        }
        return 0;
    }
    
    std::vector<NodeID> object_nodes;
    std::vector<DetectedObject> filtered_objects;
    
    int obj_idx = 0;
    for (const auto& obj : scene.objects) {
        // Filter by confidence and area
        if (obj.confidence < impl_->config.min_confidence ||
            obj.area() < impl_->config.min_box_area) {
            continue;
        }
        
        // Create node for this object
        if (impl_->config.create_object_nodes) {
            NodeID node_id = create_object_node(obj, scene.frame_number, obj_idx);
            object_nodes.push_back(node_id);
            filtered_objects.push_back(obj);
            impl_->stats.nodes_created++;
        }
        
        obj_idx++;
        impl_->stats.objects_detected++;
    }
    
    // Create intra-frame edges (EXACT - same frame co-occurrence)
    if (impl_->config.create_intra_frame_edges && object_nodes.size() > 1) {
        create_intra_frame_edges(object_nodes, scene.frame_number);
    }
    
    // Create inter-frame edges (LEAP - temporal connections)
    if (impl_->config.create_inter_frame_edges && !impl_->frame_history.empty()) {
        create_inter_frame_edges(object_nodes, scene.frame_number);
    }
    
    // Add to history
    impl_->add_to_history(scene.frame_number, object_nodes, filtered_objects);
    
    // Update stats
    impl_->stats.frames_processed++;
    impl_->stats.avg_objects_per_frame = 
        static_cast<float>(impl_->stats.objects_detected) / impl_->stats.frames_processed;
    
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    impl_->stats.avg_processing_time_ms = 
        (impl_->stats.avg_processing_time_ms * (impl_->stats.frames_processed - 1) + elapsed) / 
        impl_->stats.frames_processed;
    
    if (impl_->config.verbose) {
        std::cout << "[Perception] Frame " << scene.frame_number 
                  << ": Detected " << filtered_objects.size() << " objects, "
                  << "created " << object_nodes.size() << " nodes\n";
    }
    
    return static_cast<int>(object_nodes.size());
}

int VisualPerception::process_image_file(const std::string& image_path, YOLODetector& detector) {
    static int frame_counter = 0;
    frame_counter++;
    
    SceneFrame frame = detector.detect_frame(image_path, 0, frame_counter);
    return process_scene(frame);
}

void VisualPerception::start_visual_loop(int camera_index) {
    if (impl_->running) {
        std::cout << "[Perception] Visual loop already running\n";
        return;
    }
    
    impl_->running = true;
    
    impl_->visual_thread = std::thread([this, camera_index]() {
        std::cout << "[Perception] Starting visual loop (camera " << camera_index << ")...\n";
        std::cout << "[Perception] Note: Requires OpenCV and YOLO setup\n";
        std::cout << "[Perception] Press Ctrl+C to stop\n";
        
        // This would require OpenCV integration
        // For now, just a placeholder
        while (impl_->running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "[Perception] Visual loop stopped\n";
    });
}

void VisualPerception::stop() {
    if (impl_->running) {
        impl_->running = false;
        if (impl_->visual_thread.joinable()) {
            impl_->visual_thread.join();
        }
    }
}

bool VisualPerception::is_running() const {
    return impl_->running;
}

// ========================================================================
// NODE/EDGE CREATION
// ========================================================================

NodeID VisualPerception::create_object_node(const DetectedObject& obj, int frame_num, int obj_idx) {
    // Node content format: "object:<label>:<frame_num>:<obj_idx>"
    std::stringstream node_content;
    node_content << "object:" << obj.label << ":" << frame_num << ":" << obj_idx;
    
    NodeID node_id = impl_->storage->create_node(node_content.str(), NodeType::SENSORY);
    
    // Get node and set metadata
    Node node;
    if (impl_->storage->get_node(node_id, node)) {
        node.activation = obj.confidence;  // Use confidence as activation
        node.weight = 1.0f;
        impl_->storage->update_node(node);
    }
    
    if (impl_->config.verbose) {
        std::cout << "[Perception] Created node " << node_id 
                  << " for " << obj.label 
                  << " (conf=" << std::fixed << std::setprecision(2) << obj.confidence << ")\n";
    }
    
    return node_id;
}

void VisualPerception::create_intra_frame_edges(const std::vector<NodeID>& object_nodes, int frame_num) {
    // Create EXACT edges between all pairs of objects in the same frame
    // This represents spatial co-occurrence
    
    for (size_t i = 0; i < object_nodes.size(); ++i) {
        for (size_t j = i + 1; j < object_nodes.size(); ++j) {
            impl_->storage->create_edge(
                object_nodes[i], 
                object_nodes[j], 
                RelationType::EXACT,
                impl_->config.intra_frame_weight
            );
            
            // Bidirectional
            impl_->storage->create_edge(
                object_nodes[j], 
                object_nodes[i], 
                RelationType::EXACT,
                impl_->config.intra_frame_weight
            );
            
            impl_->stats.exact_edges_created += 2;
        }
    }
    
    if (impl_->config.verbose && !object_nodes.empty()) {
        int edge_count = object_nodes.size() * (object_nodes.size() - 1);
        std::cout << "[Perception] Created " << edge_count 
                  << " intra-frame EXACT edges for frame " << frame_num << "\n";
    }
}

void VisualPerception::create_inter_frame_edges(const std::vector<NodeID>& curr_nodes, int frame_num) {
    // Create LEAP edges between current frame and previous frames
    // Strategy: Link objects with same label or high IoU (object tracking)
    
    const auto& curr_objects = impl_->frame_history.back().objects;
    
    for (const auto& prev_frame : impl_->frame_history) {
        if (prev_frame.frame_num >= frame_num) continue;  // Skip current frame
        
        for (size_t i = 0; i < curr_nodes.size(); ++i) {
            for (size_t j = 0; j < prev_frame.object_nodes.size(); ++j) {
                bool should_link = false;
                
                // Link if same object type
                if (curr_objects[i].label == prev_frame.objects[j].label) {
                    should_link = true;
                    
                    // If tracking enabled, check IoU
                    if (impl_->config.track_object_persistence) {
                        float iou = curr_objects[i].iou(prev_frame.objects[j]);
                        if (iou < impl_->config.iou_threshold) {
                            should_link = false;  // Different instance of same class
                        }
                    }
                }
                
                if (should_link) {
                    // Create LEAP edge (temporal inference)
                    impl_->storage->create_edge(
                        prev_frame.object_nodes[j],
                        curr_nodes[i],
                        RelationType::LEAP,
                        impl_->config.inter_frame_weight
                    );
                    
                    impl_->stats.leap_edges_created++;
                }
            }
        }
    }
}

// ========================================================================
// STATISTICS
// ========================================================================

VisualPerception::Stats VisualPerception::get_stats() const {
    return impl_->stats;
}

void VisualPerception::reset_stats() {
    impl_->stats = Stats();
}

void VisualPerception::print_stats() const {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  👁️  VISUAL PERCEPTION STATISTICS                              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Frames processed:        " << impl_->stats.frames_processed << "\n";
    std::cout << "Objects detected:        " << impl_->stats.objects_detected << "\n";
    std::cout << "Nodes created:           " << impl_->stats.nodes_created << "\n";
    std::cout << "EXACT edges (intra):     " << impl_->stats.exact_edges_created << "\n";
    std::cout << "LEAP edges (inter):      " << impl_->stats.leap_edges_created << "\n";
    std::cout << "Avg objects/frame:       " << std::fixed << std::setprecision(2) 
              << impl_->stats.avg_objects_per_frame << "\n";
    std::cout << "Avg processing time:     " << std::fixed << std::setprecision(2)
              << impl_->stats.avg_processing_time_ms << " ms/frame\n";
    
    std::cout << "\n";
}

void VisualPerception::set_config(const Config& config) {
    impl_->config = config;
}

const VisualPerception::Config& VisualPerception::get_config() const {
    return impl_->config;
}

// ============================================================================
// STANDALONE HELPER FUNCTIONS
// ============================================================================

SceneFrame detect_objects_from_frame(const std::string& frame_path) {
    YOLODetector detector;
    return detector.detect_frame(frame_path);
}

void test_perception_camera(int camera_index) {
    std::cout << "[Perception] Camera test not yet implemented\n";
    std::cout << "[Perception] Use OpenCV-based camera capture with YOLO\n";
}

} // namespace perception
} // namespace melvin


