/*
 * VisionPipeline - Complete C++ Camera → Graph Integration
 * 
 * Combines:
 * - Camera capture (OpenCV)
 * - Attention computation (S + G + C + D)
 * - Visual tokenization (patches)
 * - Gestalt clustering (objects)
 * - Adaptive EXACT window (temporal connections)
 * - Graph persistence (OptimizedStorage)
 */

#include "vision_pipeline.h"
#include <iostream>
#include <chrono>
#include <deque>
#include <unordered_map>

namespace melvin {
namespace vision {

// ============================================================================
// IMPLEMENTATION CLASS
// ============================================================================

class VisionPipeline::Impl {
public:
    Config config;
    Stats stats;
    
    // Core systems
    optimized::OptimizedStorage* storage = nullptr;
    std::unique_ptr<VisualTokenizer> tokenizer;
    std::unique_ptr<adaptive::AdaptiveWindowManager> adaptive_window;
    
    // Camera
    cv::VideoCapture camera;
    cv::Mat current_frame;
    cv::Mat prev_frame_gray;
    
    // Temporal tracking
    std::deque<std::vector<AttentionScores>> attention_history;
    std::unordered_map<std::string, int> color_history;  // For diversity
    
    uint64_t frame_counter = 0;
    
    Impl(optimized::OptimizedStorage* stor, const Config& cfg)
        : config(cfg), storage(stor) {
        
        // Create visual tokenizer
        VisualTokenizer::Config tok_config;
        tok_config.patch_size = cfg.patch_size;
        tok_config.attention_threshold = cfg.attention_threshold;
        tok_config.similarity_threshold = cfg.similarity_threshold;
        tok_config.min_cluster_size = cfg.min_cluster_size;
        tok_config.max_cluster_size = cfg.max_cluster_size;
        tok_config.verbose = cfg.verbose;
        
        tokenizer = std::make_unique<VisualTokenizer>(tok_config);
        
        // Create adaptive window manager
        if (storage) {
            adaptive_window = std::make_unique<adaptive::AdaptiveWindowManager>(
                storage, cfg.adaptive_config
            );
        }
    }
    
    // Classify patch color
    std::string classify_color(const cv::Mat& patch) {
        cv::Scalar mean_color = cv::mean(patch);
        float b = mean_color[0];
        float g = mean_color[1];
        float r = mean_color[2];
        
        if (r > 150 && r > g + 30 && r > b + 30) return "red";
        if (b > 150 && b > r + 30 && b > g + 30) return "blue";
        if (g > 150 && g > r + 30 && g > b + 30) return "green";
        if (r > 200 && g > 200 && b > 200) return "bright";
        if (r < 80 && g < 80 && b < 80) return "dark";
        return "neutral";
    }
    
    // Compute diversity score
    float compute_diversity(const std::string& color) {
        float diversity = 0.0f;
        
        if (color_history.size() > 10) {
            int total_recent = 0;
            for (const auto& pair : color_history) {
                total_recent += pair.second;
            }
            
            int color_count = color_history[color];
            float ratio = static_cast<float>(color_count) / total_recent;
            
            if (ratio > 0.6f) {
                diversity -= 0.25f;  // Suppress saturated colors
            } else if (ratio < 0.2f) {
                diversity += 0.25f;  // Boost novel colors
            }
        }
        
        return std::max(-0.4f, std::min(0.4f, diversity));
    }
    
    // Update color history
    void update_color_history(const std::string& color) {
        color_history[color]++;
        
        // Keep recent window only
        if (color_history.size() > 50) {
            // Decay old entries
            for (auto& pair : color_history) {
                pair.second = static_cast<int>(pair.second * 0.95f);
            }
        }
    }
};

// ============================================================================
// PUBLIC API
// ============================================================================

VisionPipeline::VisionPipeline(optimized::OptimizedStorage* storage, const Config& config)
    : impl_(std::make_unique<Impl>(storage, config)) {}

VisionPipeline::~VisionPipeline() {
    if (impl_->camera.isOpened()) {
        impl_->camera.release();
    }
    cv::destroyAllWindows();
}

bool VisionPipeline::initialize() {
    std::cout << "🔍 Opening camera " << impl_->config.camera_id << "...\n";
    
    impl_->camera.open(impl_->config.camera_id);
    
    if (!impl_->camera.isOpened()) {
        std::cerr << "❌ Cannot open camera " << impl_->config.camera_id << "\n";
        return false;
    }
    
    // Set camera properties
    impl_->camera.set(cv::CAP_PROP_FRAME_WIDTH, impl_->config.frame_width);
    impl_->camera.set(cv::CAP_PROP_FRAME_HEIGHT, impl_->config.frame_height);
    impl_->camera.set(cv::CAP_PROP_FPS, impl_->config.fps);
    
    // Test frame
    cv::Mat test_frame;
    impl_->camera >> test_frame;
    
    if (test_frame.empty()) {
        std::cerr << "❌ Cannot read from camera\n";
        return false;
    }
    
    std::cout << "✅ Camera opened: " << test_frame.cols << "×" << test_frame.rows << "\n";
    std::cout << "   Frame size: " << test_frame.total() * test_frame.elemSize() / 1024 << " KB\n";
    
    return true;
}

std::vector<float> VisionPipeline::compute_attention(const cv::Mat& frame) {
    std::vector<float> attention_scores;
    std::vector<AttentionScores> detailed_scores;
    
    int patch_size = impl_->config.patch_size;
    int grid_h = frame.rows / patch_size;
    int grid_w = frame.cols / patch_size;
    
    // Convert to grayscale for processing
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    
    // Motion map
    cv::Mat motion_map;
    bool has_motion = false;
    
    if (!impl_->prev_frame_gray.empty()) {
        cv::Mat diff;
        cv::absdiff(gray, impl_->prev_frame_gray, diff);
        cv::threshold(diff, motion_map, 25, 255, cv::THRESH_BINARY);
        has_motion = true;
    }
    
    impl_->prev_frame_gray = gray.clone();
    
    // Process each patch
    for (int py = 0; py < grid_h; ++py) {
        for (int px = 0; px < grid_w; ++px) {
            int x = px * patch_size;
            int y = py * patch_size;
            
            if (x + patch_size > frame.cols || y + patch_size > frame.rows) {
                attention_scores.push_back(0.0f);
                continue;
            }
            
            cv::Rect roi(x, y, patch_size, patch_size);
            cv::Mat patch = frame(roi);
            cv::Mat gray_patch = gray(roi);
            
            AttentionScores score;
            score.patch_x = px;
            score.patch_y = py;
            
            // SALIENCY: Contrast
            cv::Scalar mean, stddev;
            cv::meanStdDev(gray_patch, mean, stddev);
            score.saliency = static_cast<float>(stddev[0]) / 128.0f;
            
            // GOAL: Motion
            if (has_motion) {
                cv::Mat motion_patch = motion_map(roi);
                int motion_pixels = cv::countNonZero(motion_patch);
                score.goal = (static_cast<float>(motion_pixels) / (patch_size * patch_size)) * 2.0f;
            }
            
            // CURIOSITY: Edge density
            cv::Mat edges;
            cv::Canny(gray_patch, edges, 50, 150);
            int edge_pixels = cv::countNonZero(edges);
            score.curiosity = static_cast<float>(edge_pixels) / (patch_size * patch_size);
            
            // DIVERSITY: Color novelty
            std::string color = impl_->classify_color(patch);
            score.diversity = impl_->compute_diversity(color);
            impl_->update_color_history(color);
            
            // TOTAL FOCUS
            score.focus = score.saliency + score.goal + score.curiosity + score.diversity;
            
            attention_scores.push_back(score.focus);
            detailed_scores.push_back(score);
        }
    }
    
    // Store for visualization
    impl_->attention_history.push_back(detailed_scores);
    if (impl_->attention_history.size() > 10) {
        impl_->attention_history.pop_front();
    }
    
    return attention_scores;
}

NodeID VisionPipeline::process_frame() {
    if (!impl_->camera.isOpened()) {
        return 0;
    }
    
    // Capture frame
    impl_->camera >> impl_->current_frame;
    
    if (impl_->current_frame.empty()) {
        std::cerr << "❌ Empty frame\n";
        return 0;
    }
    
    impl_->frame_counter++;
    impl_->stats.frames_processed++;
    
    // Compute attention
    std::vector<float> attention = compute_attention(impl_->current_frame);
    
    // Tokenize and cluster
    NodeID scene_id = impl_->tokenizer->process_frame(
        impl_->current_frame.data,
        impl_->current_frame.cols,
        impl_->current_frame.rows,
        attention
    );
    
    if (scene_id == 0) {
        return 0;  // No scene created (low attention)
    }
    
    // Add scene to graph with adaptive window
    if (impl_->adaptive_window && impl_->storage) {
        auto edges = impl_->adaptive_window->add_node_to_buffer(scene_id);
        
        // Create edges in graph
        for (const auto& [from, to, rel, weight] : edges) {
            impl_->storage->create_edge(from, to, rel, weight);
        }
    }
    
    // Update stats
    auto tok_stats = impl_->tokenizer->get_stats();
    impl_->stats.patches_created = tok_stats.patches_created;
    impl_->stats.objects_formed = tok_stats.objects_formed;
    impl_->stats.scenes_created = tok_stats.scenes_recorded;
    
    if (impl_->storage) {
        impl_->stats.nodes_in_graph = impl_->storage->node_count();
        impl_->stats.edges_in_graph = impl_->storage->edge_count();
    }
    
    // Visualization (TODO: implement cv::imshow for real-time display)
    // if (impl_->config.show_window) {
    //     draw_visualization();
    // }
    
    return scene_id;
}

void VisionPipeline::run(int max_frames) {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🧠 MELVIN VISION PIPELINE - C++ Implementation          ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Camera → Attention → Patches → Objects → Scene → Graph\n";
    std::cout << "Using Adaptive EXACT Window + Gestalt Clustering\n\n";
    
    std::cout << "Controls:\n";
    std::cout << "  'q' - Quit\n";
    std::cout << "  'p' - Print stats\n";
    std::cout << "  's' - Save graph\n";
    std::cout << "  SPACE - Pause/Resume\n\n";
    
    auto start_time = std::chrono::steady_clock::now();
    bool paused = false;
    int frames_processed = 0;
    
    while (max_frames == 0 || frames_processed < max_frames) {
        if (!paused) {
            NodeID scene_id = process_frame();
            
            if (scene_id == 0) {
                std::cerr << "⚠️  Frame processing failed\n";
                continue;
            }
            
            frames_processed++;
            
            // Print progress periodically
            if (frames_processed % 60 == 0) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
                impl_->stats.avg_fps = static_cast<float>(frames_processed) / elapsed;
                
                std::cout << "📊 Frame " << frames_processed 
                          << " | Nodes: " << impl_->stats.nodes_in_graph
                          << " | Edges: " << impl_->stats.edges_in_graph
                          << " | FPS: " << impl_->stats.avg_fps << "\n";
            }
        }
        
        // Handle keyboard
        int key = cv::waitKey(1);
        if (key == 'q' || key == 27) {  // q or ESC
            break;
        } else if (key == 'p') {
            print_stats();
        } else if (key == 's') {
            if (impl_->storage) {
                impl_->storage->save("data/vision_nodes_cpp.bin", "data/vision_edges_cpp.bin");
                std::cout << "💾 Graph saved\n";
            }
        } else if (key == ' ') {
            paused = !paused;
            std::cout << (paused ? "⏸️  PAUSED" : "▶️  RESUMED") << "\n";
        }
    }
    
    std::cout << "\n✅ Vision pipeline complete!\n";
    print_stats();
    
    // Auto-save
    if (impl_->storage) {
        std::cout << "💾 Saving final graph...\n";
        impl_->storage->save("data/vision_nodes_cpp.bin", "data/vision_edges_cpp.bin");
    }
}

VisionPipeline::Stats VisionPipeline::get_stats() const {
    return impl_->stats;
}

void VisionPipeline::print_stats() const {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  📊 VISION PIPELINE STATISTICS                           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "  Frames processed: " << impl_->stats.frames_processed << "\n";
    std::cout << "  Average FPS: " << impl_->stats.avg_fps << "\n\n";
    
    std::cout << "  Visual patches: " << impl_->stats.patches_created << "\n";
    std::cout << "  Objects formed: " << impl_->stats.objects_formed << "\n";
    std::cout << "  Scenes created: " << impl_->stats.scenes_created << "\n\n";
    
    std::cout << "  Graph nodes: " << impl_->stats.nodes_in_graph << "\n";
    std::cout << "  Graph edges: " << impl_->stats.edges_in_graph << "\n\n";
    
    std::cout << "  Avg patches/frame: " << impl_->stats.avg_patches_per_frame << "\n";
    std::cout << "  Avg objects/frame: " << impl_->stats.avg_objects_per_frame << "\n\n";
    
    // Print tokenizer stats
    impl_->tokenizer->print_stats();
    
    // Print adaptive window stats
    if (impl_->adaptive_window) {
        impl_->adaptive_window->print_stats();
    }
}

}} // namespace melvin::vision

