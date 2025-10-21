/*
 * Fast Visual Perception Implementation
 * Optimized for 20+ FPS real-time performance
 */

#include "fast_visual_perception.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>
#include <queue>
#include <condition_variable>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

// OpenCV (if available)
#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#endif

namespace melvin {
namespace vision {

// ============================================================================
// DETECTION RESULT STRUCT
// ============================================================================

struct Detection {
    std::string label;
    float confidence;
    int x1, y1, x2, y2;
    int frame_number;
    
    int area() const { return (x2 - x1) * (y2 - y1); }
};

struct FrameDetections {
    int frame_number;
    uint64_t timestamp;
    std::vector<Detection> detections;
};

// ============================================================================
// YOLO DETECTOR (Calls Python via subprocess - optimized)
// ============================================================================

class YOLODetector {
public:
    explicit YOLODetector(const std::string& model, float confidence)
        : model_(model), confidence_(confidence) {}
    
    std::vector<Detection> detect_from_temp_file(const std::string& temp_path, int frame_num) {
        // This is a placeholder - in production, use ONNX Runtime or direct PyTorch C++
        std::vector<Detection> results;
        
        // For now, we'll use a fast subprocess call
        std::string cmd = "python3 melvin/io/detect_objects.py " + temp_path + 
                         " " + model_ + " " + std::to_string(confidence_) + " 2>/dev/null";
        
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return results;
        
        // Quick JSON parse (simplified for speed)
        std::string output;
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            output += buffer;
        }
        pclose(pipe);
        
        // Parse detections (simple string parsing for speed)
        size_t pos = 0;
        while ((pos = output.find("\"label\"", pos)) != std::string::npos) {
            Detection det;
            det.frame_number = frame_num;
            
            // Extract label
            size_t label_start = output.find("\"", pos + 8);
            size_t label_end = output.find("\"", label_start + 1);
            if (label_start != std::string::npos && label_end != std::string::npos) {
                det.label = output.substr(label_start + 1, label_end - label_start - 1);
            }
            
            // Extract confidence
            size_t conf_pos = output.find("\"confidence\"", pos);
            if (conf_pos != std::string::npos) {
                size_t val_start = output.find(":", conf_pos);
                det.confidence = std::stof(output.substr(val_start + 1));
            }
            
            // Extract bbox
            auto extract_int = [&](const std::string& key) -> int {
                size_t key_pos = output.find("\"" + key + "\"", pos);
                if (key_pos != std::string::npos) {
                    size_t val_start = output.find(":", key_pos);
                    return std::stoi(output.substr(val_start + 1));
                }
                return 0;
            };
            
            det.x1 = extract_int("x1");
            det.y1 = extract_int("y1");
            det.x2 = extract_int("x2");
            det.y2 = extract_int("y2");
            
            results.push_back(det);
            pos = output.find("}", pos) + 1;
        }
        
        return results;
    }
    
private:
    std::string model_;
    float confidence_;
};

// ============================================================================
// IMPLEMENTATION
// ============================================================================

class FastVisualPerception::Impl {
public:
    Storage* storage;
    Config config;
    Stats stats;
    
    // Threading
    std::atomic<bool> running{false};
    std::thread capture_thread;
    std::thread processing_thread;
    
    // Frame queue (capture → processing)
    std::queue<FrameDetections> detection_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    const size_t MAX_QUEUE_SIZE = 5;  // Don't let queue grow too large
    
    // Batched graph updates
    struct GraphUpdate {
        std::vector<NodeID> nodes;
        std::vector<std::pair<NodeID, NodeID>> exact_edges;
        std::vector<std::pair<NodeID, NodeID>> leap_edges;
    };
    std::queue<GraphUpdate> graph_queue;
    std::mutex graph_mutex;
    
    // Frame history for temporal edges
    struct FrameHistory {
        int frame_num;
        std::vector<NodeID> node_ids;
        std::vector<std::string> labels;
    };
    std::vector<FrameHistory> frame_history;
    
    // YOLO detector
    std::unique_ptr<YOLODetector> detector;
    
    // Timing
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point last_stats_print;
    
    explicit Impl(Storage* stor, const Config& cfg)
        : storage(stor), config(cfg) {
        detector = std::make_unique<YOLODetector>(config.yolo_model, config.confidence_threshold);
    }
    
    ~Impl() {
        if (running) {
            running = false;
            queue_cv.notify_all();
            if (capture_thread.joinable()) capture_thread.join();
            if (processing_thread.joinable()) processing_thread.join();
        }
    }
    
    // Capture thread: Get frames from camera and run YOLO
    void capture_loop(int camera_index) {
#ifdef HAVE_OPENCV
        cv::VideoCapture cap(camera_index);
        
        if (!cap.isOpened()) {
            std::cerr << "[Vision] ERROR: Cannot open camera " << camera_index << "\n";
            running = false;
            return;
        }
        
        // Set camera properties for speed
        cap.set(cv::CAP_PROP_FRAME_WIDTH, config.frame_width);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, config.frame_height);
        cap.set(cv::CAP_PROP_FPS, config.target_fps);
        
        std::cout << "[Vision] ✓ Camera opened: " << config.frame_width << "x" 
                  << config.frame_height << " @ " << config.target_fps << " FPS\n";
        
        int frame_counter = 0;
        auto last_process_time = std::chrono::steady_clock::now();
        
        while (running) {
            auto frame_start = std::chrono::steady_clock::now();
            
            cv::Mat frame;
            cap >> frame;
            
            if (frame.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            
            frame_counter++;
            stats.frames_captured++;
            
            // Frame skipping for target FPS
            if (frame_counter % config.process_every_n_frames != 0) {
                stats.frames_skipped++;
                
                // Display only
                if (config.show_display) {
                    cv::imshow("Melvin Vision (Fast)", frame);
                    if (cv::waitKey(1) == 'q') {
                        running = false;
                        break;
                    }
                }
                continue;
            }
            
            // Save frame to temp file for YOLO
            std::string temp_path = "/tmp/melvin_frame_" + std::to_string(frame_counter) + ".jpg";
            cv::imwrite(temp_path, frame);
            
            // Run YOLO detection (this is the bottleneck)
            auto detect_start = std::chrono::steady_clock::now();
            auto detections = detector->detect_from_temp_file(temp_path, frame_counter);
            auto detect_end = std::chrono::steady_clock::now();
            
            float detect_ms = std::chrono::duration<float, std::milli>(detect_end - detect_start).count();
            
            // Remove temp file
            std::remove(temp_path.c_str());
            
            // Filter detections
            std::vector<Detection> filtered;
            for (const auto& det : detections) {
                if (det.confidence >= config.min_confidence && det.area() >= config.min_box_area) {
                    filtered.push_back(det);
                    if (filtered.size() >= static_cast<size_t>(config.max_objects_per_frame)) {
                        break;
                    }
                }
            }
            
            stats.objects_detected += filtered.size();
            
            // Display with bounding boxes
            if (config.show_display) {
                for (const auto& det : filtered) {
                    cv::rectangle(frame, cv::Point(det.x1, det.y1), 
                                cv::Point(det.x2, det.y2), cv::Scalar(0, 255, 0), 2);
                    std::string label = det.label + " " + std::to_string(static_cast<int>(det.confidence * 100)) + "%";
                    cv::putText(frame, label, cv::Point(det.x1, det.y1 - 5),
                              cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
                }
                
                // FPS overlay
                std::string fps_text = "FPS: " + std::to_string(static_cast<int>(stats.current_fps.load())) + 
                                      " | Objects: " + std::to_string(filtered.size()) + 
                                      " | Nodes: " + std::to_string(stats.nodes_created.load());
                cv::putText(frame, fps_text, cv::Point(10, 30),
                          cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
                
                cv::imshow("Melvin Vision (Fast)", frame);
                if (cv::waitKey(1) == 'q') {
                    running = false;
                    break;
                }
            }
            
            // Add to processing queue
            FrameDetections frame_data;
            frame_data.frame_number = frame_counter;
            frame_data.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
            frame_data.detections = filtered;
            
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                if (detection_queue.size() < MAX_QUEUE_SIZE) {
                    detection_queue.push(frame_data);
                    queue_cv.notify_one();
                }
            }
            
            stats.frames_processed++;
            
            // Update FPS
            auto now = std::chrono::steady_clock::now();
            float elapsed = std::chrono::duration<float>(now - last_process_time).count();
            if (elapsed > 0) {
                stats.current_fps = 1.0f / elapsed;
            }
            last_process_time = now;
            
            // Maintain target FPS
            auto frame_end = std::chrono::steady_clock::now();
            auto frame_duration = std::chrono::duration<float, std::milli>(frame_end - frame_start).count();
            float target_frame_time = 1000.0f / config.target_fps;
            
            if (frame_duration < target_frame_time) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(static_cast<int>(target_frame_time - frame_duration))
                );
            }
        }
        
        cap.release();
        cv::destroyAllWindows();
        
#else
        std::cerr << "[Vision] ERROR: OpenCV not available\n";
        running = false;
#endif
    }
    
    // Processing thread: Convert detections to graph nodes/edges
    void processing_loop() {
        while (running || !detection_queue.empty()) {
            FrameDetections frame_data;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                queue_cv.wait_for(lock, std::chrono::milliseconds(100), 
                                 [this] { return !detection_queue.empty() || !running; });
                
                if (detection_queue.empty()) continue;
                
                frame_data = detection_queue.front();
                detection_queue.pop();
            }
            
            process_frame_to_graph(frame_data);
        }
        
        // Flush remaining graph updates
        flush_graph_updates();
    }
    
    void process_frame_to_graph(const FrameDetections& frame_data) {
        if (frame_data.detections.empty()) return;
        
        std::vector<NodeID> frame_nodes;
        std::vector<std::string> frame_labels;
        
        // Create nodes for each detection
        for (size_t i = 0; i < frame_data.detections.size(); ++i) {
            const auto& det = frame_data.detections[i];
            
            // Node content: "object:<label>:<frame>:<idx>"
            std::string content = "object:" + det.label + ":" + 
                                std::to_string(frame_data.frame_number) + ":" + 
                                std::to_string(i);
            
            NodeID node_id = storage->create_node(content, NodeType::SENSORY);
            
            // Set activation to confidence
            Node node;
            if (storage->get_node(node_id, node)) {
                node.activation = det.confidence;
                storage->update_node(node);
            }
            
            frame_nodes.push_back(node_id);
            frame_labels.push_back(det.label);
            stats.nodes_created++;
        }
        
        // Create EXACT edges (intra-frame)
        if (config.create_intra_frame_edges && frame_nodes.size() > 1) {
            for (size_t i = 0; i < frame_nodes.size(); ++i) {
                for (size_t j = i + 1; j < frame_nodes.size(); ++j) {
                    storage->create_edge(frame_nodes[i], frame_nodes[j], 
                                       RelationType::EXACT, config.intra_weight);
                    storage->create_edge(frame_nodes[j], frame_nodes[i], 
                                       RelationType::EXACT, config.intra_weight);
                    stats.exact_edges += 2;
                }
            }
        }
        
        // Create LEAP edges (inter-frame)
        if (config.create_inter_frame_edges && !frame_history.empty()) {
            for (const auto& prev_frame : frame_history) {
                for (size_t i = 0; i < frame_nodes.size(); ++i) {
                    for (size_t j = 0; j < prev_frame.node_ids.size(); ++j) {
                        // Link same object types
                        if (frame_labels[i] == prev_frame.labels[j]) {
                            storage->create_edge(prev_frame.node_ids[j], frame_nodes[i],
                                               RelationType::LEAP, config.inter_weight);
                            stats.leap_edges++;
                        }
                    }
                }
            }
        }
        
        // Add to history
        FrameHistory hist;
        hist.frame_num = frame_data.frame_number;
        hist.node_ids = frame_nodes;
        hist.labels = frame_labels;
        
        frame_history.push_back(hist);
        if (frame_history.size() > static_cast<size_t>(config.temporal_window)) {
            frame_history.erase(frame_history.begin());
        }
    }
    
    void flush_graph_updates() {
        // Save storage to disk
        storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
        stats.graph_flushes++;
    }
};

// ============================================================================
// PUBLIC API
// ============================================================================

FastVisualPerception::FastVisualPerception(Storage* storage, const Config& config)
    : impl_(std::make_unique<Impl>(storage, config)) {}

FastVisualPerception::~FastVisualPerception() = default;

void FastVisualPerception::start() {
    if (impl_->running) {
        std::cout << "[Vision] Already running\n";
        return;
    }
    
    impl_->running = true;
    impl_->start_time = std::chrono::steady_clock::now();
    impl_->last_stats_print = impl_->start_time;
    
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🚀 FAST VISUAL PERCEPTION - STARTING                          ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    std::cout << "Target FPS: " << impl_->config.target_fps << "\n";
    std::cout << "Frame size: " << impl_->config.frame_width << "x" << impl_->config.frame_height << "\n";
    std::cout << "Process every: " << impl_->config.process_every_n_frames << " frame(s)\n";
    std::cout << "Press 'q' to quit\n\n";
    
    // Start threads
    if (impl_->config.use_threading) {
        impl_->processing_thread = std::thread(&Impl::processing_loop, impl_.get());
    }
    
    impl_->capture_thread = std::thread(&Impl::capture_loop, impl_.get(), impl_->config.camera_index);
}

void FastVisualPerception::stop() {
    if (!impl_->running) return;
    
    std::cout << "\n[Vision] Stopping...\n";
    impl_->running = false;
    
    impl_->queue_cv.notify_all();
    
    if (impl_->capture_thread.joinable()) {
        impl_->capture_thread.join();
    }
    
    if (impl_->processing_thread.joinable()) {
        impl_->processing_thread.join();
    }
    
    // Final flush
    impl_->flush_graph_updates();
    
    std::cout << "[Vision] ✓ Stopped\n\n";
    print_stats();
}

bool FastVisualPerception::is_running() const {
    return impl_->running;
}

void FastVisualPerception::wait_until_stopped() {
    if (impl_->capture_thread.joinable()) {
        impl_->capture_thread.join();
    }
    if (impl_->processing_thread.joinable()) {
        impl_->processing_thread.join();
    }
}

FastVisualPerception::Stats FastVisualPerception::get_stats() const {
    return impl_->stats;
}

void FastVisualPerception::reset_stats() {
    impl_->stats = Stats();
}

void FastVisualPerception::print_stats() const {
    auto elapsed = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - impl_->start_time
    ).count();
    
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  📊 FAST VISUAL PERCEPTION STATISTICS                          ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Duration:              " << std::fixed << std::setprecision(1) << elapsed << " seconds\n";
    std::cout << "Frames captured:       " << impl_->stats.frames_captured.load() << "\n";
    std::cout << "Frames processed:      " << impl_->stats.frames_processed.load() << "\n";
    std::cout << "Frames skipped:        " << impl_->stats.frames_skipped.load() << "\n";
    std::cout << "Objects detected:      " << impl_->stats.objects_detected.load() << "\n";
    std::cout << "Nodes created:         " << impl_->stats.nodes_created.load() << "\n";
    std::cout << "EXACT edges (intra):   " << impl_->stats.exact_edges.load() << "\n";
    std::cout << "LEAP edges (inter):    " << impl_->stats.leap_edges.load() << "\n";
    std::cout << "Current FPS:           " << std::fixed << std::setprecision(1) 
              << impl_->stats.current_fps.load() << "\n";
    std::cout << "Graph saves:           " << impl_->stats.graph_flushes.load() << "\n";
    std::cout << "\n";
}

void FastVisualPerception::set_config(const Config& config) {
    impl_->config = config;
}

const FastVisualPerception::Config& FastVisualPerception::get_config() const {
    return impl_->config;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

bool test_camera_access(int camera_index) {
#ifdef HAVE_OPENCV
    cv::VideoCapture cap(camera_index);
    bool opened = cap.isOpened();
    cap.release();
    return opened;
#else
    return false;
#endif
}

void benchmark_vision_system(int duration_seconds) {
    Storage storage;
    
    FastVisualPerception::Config config;
    config.target_fps = 30;
    config.process_every_n_frames = 1;
    config.show_display = true;
    config.verbose = false;
    
    FastVisualPerception vision(&storage, config);
    
    std::cout << "Running benchmark for " << duration_seconds << " seconds...\n";
    
    vision.start();
    std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
    vision.stop();
}

} // namespace vision
} // namespace melvin

