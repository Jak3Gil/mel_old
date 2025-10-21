/*
 * Attention-Based Visual Perception (C++ Implementation)
 * 
 * Human-like attention: ONE focus at a time, rapid shifting
 * Direct Storage integration, weighted edges
 */

#include "attention_vision.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <unordered_set>
#include <algorithm>
#include <cmath>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

namespace melvin {
namespace vision {

// ============================================================================
// TRACKED CONCEPT
// ============================================================================

struct TrackedConcept {
    NodeID node_id;
    std::string label;
    int x1, y1, x2, y2;  // Bounding box
    int first_seen_frame;
    int last_seen_frame;
    uint64_t observation_count;
    
    TrackedConcept(NodeID id, const std::string& lbl, int bx1, int by1, int bx2, int by2, int frame)
        : node_id(id), label(lbl), x1(bx1), y1(by1), x2(bx2), y2(by2),
          first_seen_frame(frame), last_seen_frame(frame), observation_count(1) {}
    
    void update(int bx1, int by1, int bx2, int by2, int frame) {
        x1 = bx1; y1 = by1; x2 = bx2; y2 = by2;
        last_seen_frame = frame;
        observation_count++;
    }
    
    int center_x() const { return (x1 + x2) / 2; }
    int center_y() const { return (y1 + y2) / 2; }
    int area() const { return (x2 - x1) * (y2 - y1); }
};

// ============================================================================
// EDGE TRACKING
// ============================================================================

struct EdgeKey {
    NodeID from;
    NodeID to;
    
    EdgeKey(NodeID a, NodeID b) {
        // Canonical order (smaller first)
        if (a < b) { from = a; to = b; }
        else { from = b; to = a; }
    }
    
    bool operator==(const EdgeKey& other) const {
        return from == other.from && to == other.to;
    }
};

struct EdgeKeyHash {
    std::size_t operator()(const EdgeKey& k) const {
        return std::hash<uint16_t>()(k.from) ^ (std::hash<uint16_t>()(k.to) << 1);
    }
};

struct WeightedEdge {
    NodeID from;
    NodeID to;
    RelationType type;
    uint64_t weight;  // Co-occurrence count
    
    WeightedEdge(NodeID f, NodeID t, RelationType rt)
        : from(f), to(t), type(rt), weight(1) {}
};

// ============================================================================
// IMPLEMENTATION
// ============================================================================

class AttentionVision::Impl {
public:
    AtomicGraph* graph;
    Config config;
    Stats stats;
    
    // Tracked concepts (persistent objects)
    std::vector<TrackedConcept> tracked_concepts;
    
    // Edge tracking
    std::unordered_map<EdgeKey, WeightedEdge, EdgeKeyHash> edges;
    
    // Attention state
    int attention_index = 0;
    int prev_focus_x = -1;
    int prev_focus_y = -1;
    
    // Control
    std::atomic<bool> running{false};
    
    Impl(AtomicGraph* g, const Config& cfg)
        : graph(g), config(cfg) {}
    
    // Find IoU between two boxes
    float compute_iou(int x1a, int y1a, int x2a, int y2a,
                      int x1b, int y1b, int x2b, int y2b) {
        int inter_x1 = std::max(x1a, x1b);
        int inter_y1 = std::max(y1a, y1b);
        int inter_x2 = std::min(x2a, x2b);
        int inter_y2 = std::min(y2a, y2b);
        
        if (inter_x2 < inter_x1 || inter_y2 < inter_y1) {
            return 0.0f;
        }
        
        int inter_area = (inter_x2 - inter_x1) * (inter_y2 - inter_y1);
        int area_a = (x2a - x1a) * (y2a - y1a);
        int area_b = (x2b - x1b) * (y2b - y1b);
        int union_area = area_a + area_b - inter_area;
        
        return static_cast<float>(inter_area) / union_area;
    }
    
    // Match detection to existing concept or create new
    NodeID match_or_create_concept(const std::string& label, int x1, int y1, int x2, int y2, int frame_num) {
        // Try to match existing tracked concept
        TrackedConcept* best_match = nullptr;
        float best_iou = 0.0f;
        
        for (auto& concept : tracked_concepts) {
            if (concept.label != label) continue;
            if (frame_num - concept.last_seen_frame > config.max_missing_frames) continue;
            
            float iou = compute_iou(x1, y1, x2, y2, 
                                   concept.x1, concept.y1, concept.x2, concept.y2);
            
            if (iou > best_iou) {
                best_iou = iou;
                best_match = &concept;
            }
        }
        
        if (best_match && best_iou > config.iou_threshold) {
            // REINFORCE existing concept
            best_match->update(x1, y1, x2, y2, frame_num);
            stats.concepts_reinforced++;
            return best_match->node_id;
        }
        
        // CREATE new concept
        std::stringstream node_content;
        node_content << "object:" << label << ":#" << frame_num;
        
        uint64_t node_id = graph->get_or_create_node(node_content.str(), 1);  // type=1 (instance)
        
        tracked_concepts.emplace_back(node_id, label, x1, y1, x2, y2, frame_num);
        stats.concepts_created++;
        
        return node_id;
    }
    
    // Add or reinforce edge (now uses AtomicGraph!)
    void add_edge(uint64_t from, uint64_t to, Relation rel_type) {
        if (from == to) return;  // No self-loops
        
        // Map RelationType to Relation
        uint8_t rel = (rel_type == RelationType::EXACT) ? CO_OCCURS_WITH : TEMPORAL_NEXT;
        
        graph->add_edge(from, to, rel, 1.0f);
        
        stats.unique_edges++;
        stats.total_edge_weight++;
    }
    
    // Cleanup old concepts
    void cleanup_old_concepts(int current_frame) {
        tracked_concepts.erase(
            std::remove_if(tracked_concepts.begin(), tracked_concepts.end(),
                [this, current_frame](const TrackedConcept& c) {
                    return current_frame - c.last_seen_frame > config.max_missing_frames;
                }),
            tracked_concepts.end()
        );
    }
    
    // Save graph to disk
    void save_graph() {
        graph->save("melvin/data/atomic_nodes.bin", "melvin/data/atomic_edges.bin");
        
        if (config.verbose) {
            std::cout << "[Vision] Saved graph to atomic_*.bin\n";
        }
    }
};

// ============================================================================
// PUBLIC API
// ============================================================================

AttentionVision::AttentionVision(AtomicGraph* graph, const Config& config)
    : impl_(std::make_unique<Impl>(graph, config)) {}

AttentionVision::~AttentionVision() {
    if (impl_->running) {
        stop();
    }
}

void AttentionVision::run() {
#ifdef HAVE_OPENCV
    impl_->running = true;
    
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🧠 MELVIN ATTENTION VISION (C++ Optimized)                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    std::cout << "🎯 Human-like attention: ONE focus at a time\n";
    std::cout << "📊 Direct Storage integration (nodes.melvin/edges.melvin)\n\n";
    
    cv::VideoCapture cap(impl_->config.camera_index);
    
    if (!cap.isOpened()) {
        std::cerr << "[Vision] ERROR: Cannot open camera " << impl_->config.camera_index << "\n";
        impl_->running = false;
        return;
    }
    
    cap.set(cv::CAP_PROP_FRAME_WIDTH, impl_->config.frame_width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, impl_->config.frame_height);
    
    std::cout << "[Vision] ✓ Camera opened: " << impl_->config.frame_width 
              << "x" << impl_->config.frame_height << "\n";
    std::cout << "[Vision] 🧠 Attention: ONE focus point per frame\n";
    std::cout << "[Vision] Press 'q' to quit\n\n";
    
    auto start_time = std::chrono::steady_clock::now();
    auto last_fps_time = start_time;
    int fps_frame_count = 0;
    
    std::vector<NodeID> prev_frame_nodes;
    
    while (impl_->running) {
        cv::Mat frame;
        cap >> frame;
        
        if (frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        impl_->stats.frames_processed++;
        fps_frame_count++;
        
        // TODO: Implement YOLO detection via Python subprocess
        // For now, placeholder that would call detect_objects.py
        
        std::vector<NodeID> frame_nodes;
        
        // Example detection result (in real implementation, call YOLO)
        // This is where we'd integrate with detect_objects.py
        
        // Create edges for co-occurring objects
        if (frame_nodes.size() > 1) {
            for (size_t i = 0; i < frame_nodes.size(); ++i) {
                for (size_t j = i + 1; j < frame_nodes.size(); ++j) {
                    impl_->add_edge(frame_nodes[i], frame_nodes[j], RelationType::EXACT);
                }
            }
        }
        
        // Temporal edges
        if (!prev_frame_nodes.empty() && !frame_nodes.empty()) {
            for (auto prev_id : prev_frame_nodes) {
                for (auto curr_id : frame_nodes) {
                    if (prev_id != curr_id) {
                        impl_->add_edge(prev_id, curr_id, RelationType::LEAP);
                    }
                }
            }
        }
        
        prev_frame_nodes = frame_nodes;
        
        // Cleanup old concepts
        impl_->cleanup_old_concepts(impl_->stats.frames_processed);
        
        // Calculate FPS
        auto now = std::chrono::steady_clock::now();
        auto fps_elapsed = std::chrono::duration<float>(now - last_fps_time).count();
        if (fps_elapsed >= 1.0f) {
            impl_->stats.current_fps = fps_frame_count / fps_elapsed;
            fps_frame_count = 0;
            last_fps_time = now;
        }
        
        // Display
        if (impl_->config.show_display) {
            cv::imshow("Melvin Attention Vision", frame);
            
            int key = cv::waitKey(30);
            if (key == 'q' || key == 27) {  // 'q' or ESC
                impl_->running = false;
                break;
            }
        }
        
        // Periodic save
        if (impl_->stats.frames_processed % 100 == 0) {
            impl_->save_graph();
        }
    }
    
    cap.release();
    cv::destroyAllWindows();
    
    // Final save
    impl_->save_graph();
    
    print_stats();
    
#else
    std::cerr << "[Vision] ERROR: OpenCV not available. Compile with -DHAVE_OPENCV\n";
#endif
}

void AttentionVision::stop() {
    impl_->running = false;
}

bool AttentionVision::is_running() const {
    return impl_->running;
}

AttentionVision::Stats AttentionVision::get_stats() const {
    return impl_->stats;
}

void AttentionVision::reset_stats() {
    impl_->stats = Stats();
}

void AttentionVision::print_stats() const {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  📊 ATTENTION VISION STATISTICS                                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Frames processed:      " << impl_->stats.frames_processed << "\n";
    std::cout << "Attention shifts:      " << impl_->stats.attention_shifts << "\n";
    std::cout << "Objects detected:      " << impl_->stats.objects_detected << "\n";
    std::cout << "Concepts created:      " << impl_->stats.concepts_created << "\n";
    std::cout << "Concepts reinforced:   " << impl_->stats.concepts_reinforced << "\n";
    std::cout << "Unique edges:          " << impl_->stats.unique_edges << "\n";
    std::cout << "Total edge weight:     " << impl_->stats.total_edge_weight << "\n";
    std::cout << "Current FPS:           " << std::fixed << std::setprecision(1) 
              << impl_->stats.current_fps << "\n";
    
    std::cout << "\n🧠 Knowledge Graph State:\n";
    impl_->graph->print_stats();
    
    std::cout << "\n💾 Saved to:\n";
    std::cout << "   melvin/data/atomic_nodes.bin\n";
    std::cout << "   melvin/data/atomic_edges.bin\n\n";
}

void AttentionVision::set_config(const Config& config) {
    impl_->config = config;
}

const AttentionVision::Config& AttentionVision::get_config() const {
    return impl_->config;
}

} // namespace vision
} // namespace melvin

