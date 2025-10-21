/*
 * OpenCV Attention Vision - Minimal Implementation
 * Direct AtomicGraph integration
 */

#include "opencv_attention.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <cmath>
#include <cstdio>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

namespace melvin {
namespace vision {

// Tracked object for persistence
struct Tracked {
    uint64_t node_id;
    std::string label;
    int x, y, w, h;
    int last_frame;
    
    Tracked(uint64_t id, const std::string& l, int bx, int by, int bw, int bh, int f)
        : node_id(id), label(l), x(bx), y(by), w(bw), h(bh), last_frame(f) {}
};

// Compute IoU
static float iou(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    int ix1 = std::max(x1, x2);
    int iy1 = std::max(y1, y2);
    int ix2 = std::min(x1+w1, x2+w2);
    int iy2 = std::min(y1+h1, y2+h2);
    
    if (ix2 < ix1 || iy2 < iy1) return 0.0f;
    
    int inter = (ix2-ix1) * (iy2-iy1);
    int u = w1*h1 + w2*h2 - inter;
    return u > 0 ? float(inter)/u : 0.0f;
}

// Simple JSON parser for YOLO output
struct Detection {
    std::string label;
    float conf;
    int x, y, w, h;
};

static std::vector<Detection> parse_yolo_json(const std::string& json) {
    std::vector<Detection> dets;
    size_t pos = 0;
    
    while (true) {
        pos = json.find("{", pos);
        if (pos == std::string::npos) break;
        
        size_t end = json.find("}", pos);
        if (end == std::string::npos) break;
        
        std::string obj = json.substr(pos, end - pos + 1);
        Detection d;
        
        // Parse label
        size_t lp = obj.find("\"label\"");
        if (lp != std::string::npos) {
            size_t s = obj.find("\"", lp + 8);
            size_t e = obj.find("\"", s + 1);
            if (s != std::string::npos && e != std::string::npos)
                d.label = obj.substr(s+1, e-s-1);
        }
        
        // Parse confidence
        size_t cp = obj.find("\"confidence\"");
        if (cp != std::string::npos) {
            size_t s = obj.find(":", cp);
            if (s != std::string::npos)
                d.conf = std::stof(obj.substr(s+1));
        }
        
        // Parse bbox
        auto parse_int = [&](const std::string& key) {
            size_t kp = obj.find("\"" + key + "\"");
            if (kp != std::string::npos) {
                size_t s = obj.find(":", kp);
                if (s != std::string::npos)
                    return std::stoi(obj.substr(s+1));
            }
            return 0;
        };
        
        int x1 = parse_int("x1"), y1 = parse_int("y1");
        int x2 = parse_int("x2"), y2 = parse_int("y2");
        d.x = x1; d.y = y1; d.w = x2-x1; d.h = y2-y1;
        
        dets.push_back(d);
        pos = end + 1;
    }
    
    return dets;
}

// Call YOLO via Python
static std::vector<Detection> run_yolo(const std::string& img_path, const std::string& script) {
    std::string cmd = "python3 " + script + " " + img_path + " 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return {};
    
    std::string output;
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) output += buf;
    pclose(pipe);
    
    return parse_yolo_json(output);
}

// Implementation
class OpenCVAttention::Impl {
public:
    AtomicGraph* graph;
    Config config;
    Stats stats;
    std::vector<Tracked> tracked;
    std::atomic<bool> running{false};
    int frame_count = 0;
    
    Impl(AtomicGraph* g, const Config& c) : graph(g), config(c) {
        stats.frames = 0;
        stats.detections = 0;
        stats.concepts_created = 0;
        stats.concepts_reused = 0;
        stats.fps = 0.0f;
    }
    
    uint64_t match_or_create(const Detection& det) {
        // Try match
        for (auto& t : tracked) {
            if (t.label != det.label) continue;
            if (frame_count - t.last_frame > config.max_missing_frames) continue;
            
            float overlap = iou(det.x, det.y, det.w, det.h, t.x, t.y, t.w, t.h);
            if (overlap > config.iou_threshold) {
                // Reuse
                t.x = det.x; t.y = det.y; t.w = det.w; t.h = det.h;
                t.last_frame = frame_count;
                stats.concepts_reused++;
                return t.node_id;
            }
        }
        
        // Create new
        std::string node_label = "object:" + det.label + ":id" + std::to_string(stats.concepts_created);
        uint64_t id = graph->get_or_create_node(node_label, 1);  // type=1 (instance)
        
        tracked.emplace_back(id, det.label, det.x, det.y, det.w, det.h, frame_count);
        stats.concepts_created++;
        
        return id;
    }
    
    void cleanup() {
        tracked.erase(std::remove_if(tracked.begin(), tracked.end(),
            [this](const Tracked& t) { return frame_count - t.last_frame > config.max_missing_frames; }),
            tracked.end());
    }
};

OpenCVAttention::OpenCVAttention(AtomicGraph* graph, const Config& config)
    : impl_(new Impl(graph, config)) {}

OpenCVAttention::~OpenCVAttention() {
    if (impl_->running) stop();
    delete impl_;
}

void OpenCVAttention::run() {
#ifdef HAVE_OPENCV
    impl_->running = true;
    
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🧠 OpenCV Attention Vision + AtomicGraph                      ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    cv::VideoCapture cap(impl_->config.camera_index);
    if (!cap.isOpened()) {
        std::cerr << "ERROR: Cannot open camera " << impl_->config.camera_index << "\n";
        impl_->running = false;
        return;
    }
    
    cap.set(cv::CAP_PROP_FRAME_WIDTH, impl_->config.width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, impl_->config.height);
    
    std::cout << "✓ Camera opened: " << impl_->config.width << "x" << impl_->config.height << "\n";
    std::cout << "🧠 ONE focus point, rapid shifting\n";
    std::cout << "Press 'q' to quit\n\n";
    
    auto start = std::chrono::steady_clock::now();
    std::vector<uint64_t> prev_nodes;
    
    while (impl_->running) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        impl_->frame_count++;
        impl_->stats.frames++;
        
        // Save frame temporarily
        std::string tmp = "/tmp/melvin_frame.jpg";
        cv::imwrite(tmp, frame);
        
        // Run YOLO
        auto dets = run_yolo(tmp, impl_->config.yolo_script);
        
        std::vector<uint64_t> frame_nodes;
        
        for (const auto& det : dets) {
            if (det.conf < impl_->config.confidence) continue;
            if (det.w < 20 || det.h < 20) continue;
            
            impl_->stats.detections++;
            
            // Match or create concept
            uint64_t nid = impl_->match_or_create(det);
            frame_nodes.push_back(nid);
            
            // Draw
            bool is_new = (impl_->stats.concepts_reused == 0 || 
                          std::find(prev_nodes.begin(), prev_nodes.end(), nid) == prev_nodes.end());
            
            cv::Scalar color = is_new ? cv::Scalar(0,255,0) : cv::Scalar(0,255,255);
            cv::rectangle(frame, cv::Rect(det.x, det.y, det.w, det.h), color, 2);
            
            std::string text = det.label;
            cv::putText(frame, text, cv::Point(det.x, det.y-5),
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 2);
        }
        
        // EXACT edges (same frame co-occurrence)
        for (size_t i = 0; i < frame_nodes.size(); ++i) {
            for (size_t j = i+1; j < frame_nodes.size(); ++j) {
                impl_->graph->add_edge(frame_nodes[i], frame_nodes[j], CO_OCCURS_WITH, 1.0f);
            }
        }
        
        // LEAP edges (temporal)
        if (!prev_nodes.empty() && !frame_nodes.empty()) {
            for (auto p : prev_nodes) {
                for (auto c : frame_nodes) {
                    if (p != c)
                        impl_->graph->add_edge(p, c, TEMPORAL_NEXT, 0.5f);
                }
            }
        }
        
        prev_nodes = frame_nodes;
        impl_->cleanup();
        
        // Stats overlay
        auto now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - start).count();
        impl_->stats.fps = impl_->stats.frames / elapsed;
        
        std::string stat = "Frames:" + std::to_string(impl_->stats.frames) +
                          " Concepts:" + std::to_string(impl_->graph->node_count()) +
                          " Edges:" + std::to_string(impl_->graph->edge_count()) +
                          " FPS:" + std::to_string(int(impl_->stats.fps));
        
        cv::putText(frame, stat, cv::Point(10, 30),
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0,255,255), 2);
        
        if (impl_->config.show_display) {
            cv::imshow("Melvin Vision", frame);
            int key = cv::waitKey(30);
            if (key == 'q' || key == 27) break;
        }
        
        // Periodic save
        if (impl_->stats.frames % 50 == 0) {
            impl_->graph->save("melvin/data/atomic_nodes.bin", "melvin/data/atomic_edges.bin");
        }
    }
    
    cap.release();
    cv::destroyAllWindows();
    
    // Final save
    impl_->graph->save("melvin/data/atomic_nodes.bin", "melvin/data/atomic_edges.bin");
    print_stats();
    
#else
    std::cerr << "ERROR: OpenCV not available\n";
#endif
}

void OpenCVAttention::stop() {
    impl_->running = false;
}

bool OpenCVAttention::is_running() const {
    return impl_->running;
}

OpenCVAttention::Stats OpenCVAttention::get_stats() const {
    return impl_->stats;
}

void OpenCVAttention::print_stats() const {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  📊 Vision Session Complete                                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Frames:            " << impl_->stats.frames << "\n";
    std::cout << "Detections:        " << impl_->stats.detections << "\n";
    std::cout << "Concepts created:  " << impl_->stats.concepts_created << "\n";
    std::cout << "Concepts reused:   " << impl_->stats.concepts_reused << "\n";
    std::cout << "Reuse rate:        " << (impl_->stats.detections > 0 ? 
        100.0f * impl_->stats.concepts_reused / impl_->stats.detections : 0) << "%\n";
    std::cout << "FPS:               " << impl_->stats.fps << "\n\n";
    
    std::cout << "🧠 AtomicGraph:\n";
    impl_->graph->print_stats();
    
    std::cout << "💾 Saved to: melvin/data/atomic_nodes.bin / atomic_edges.bin\n\n";
}

} // namespace vision
} // namespace melvin

