/*
 * Live Vision Input Implementation
 * [Live Vision Extension]
 * 
 * Captures camera frames and converts them into Melvin's knowledge graph
 */

#include "vision_input.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <thread>
#include <atomic>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

namespace melvin {
namespace io {

// ============================================================================
// VISION ENCODER IMPLEMENTATION
// ============================================================================

class VisionEncoder::Impl {
public:
    Config config;
    
    // Simple pixel-based encoding (placeholder for CLIP)
    std::vector<float> encode_pixels(const unsigned char* data, int width, int height, int channels) {
        std::vector<float> embedding;
        
        // Downsample to config size
        int target_w = config.downsample_width;
        int target_h = config.downsample_height;
        
        embedding.resize(target_w * target_h * (config.use_grayscale ? 1 : channels), 0.0f);
        
        // Simple average pooling
        int step_x = width / target_w;
        int step_y = height / target_h;
        
        for (int ty = 0; ty < target_h; ++ty) {
            for (int tx = 0; tx < target_w; ++tx) {
                int sx = tx * step_x;
                int sy = ty * step_y;
                
                if (sx < width && sy < height) {
                    int idx = (sy * width + sx) * channels;
                    
                    if (config.use_grayscale) {
                        // Average RGB to grayscale
                        float gray = 0.0f;
                        for (int c = 0; c < channels && c < 3; ++c) {
                            gray += data[idx + c];
                        }
                        embedding[ty * target_w + tx] = gray / (channels * 255.0f);
                    } else {
                        // Keep all channels
                        for (int c = 0; c < channels; ++c) {
                            embedding[(ty * target_w + tx) * channels + c] = data[idx + c] / 255.0f;
                        }
                    }
                }
            }
        }
        
        // Normalize if requested
        if (config.normalize) {
            float norm = 0.0f;
            for (float v : embedding) norm += v * v;
            norm = std::sqrt(norm);
            if (norm > 1e-8f) {
                for (float& v : embedding) v /= norm;
            }
        }
        
        return embedding;
    }
};

VisionEncoder::VisionEncoder(const Config& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
}

VisionEncoder::~VisionEncoder() = default;

std::vector<float> VisionEncoder::encode_frame(const void* frame_data, int width, int height, int channels) {
    return impl_->encode_pixels(static_cast<const unsigned char*>(frame_data), width, height, channels);
}

void VisionEncoder::set_config(const Config& config) {
    impl_->config = config;
}

const VisionEncoder::Config& VisionEncoder::get_config() const {
    return impl_->config;
}

// ============================================================================
// LIVE VISION STREAM IMPLEMENTATION
// ============================================================================

class LiveVisionStream::Impl {
public:
    Storage* storage;
    Config config;
    Stats stats;
    
    std::unique_ptr<VisionEncoder> encoder;
    std::atomic<bool> running{false};
    std::thread capture_thread;
    
    NodeID active_context_node = 0;
    NodeID last_frame_node = 0;
    int frame_counter = 0;
    
    Impl(Storage* stor) : storage(stor) {
        VisionEncoder::Config enc_config;
        encoder = std::make_unique<VisionEncoder>(enc_config);
    }
    
    ~Impl() {
        if (running) {
            running = false;
            if (capture_thread.joinable()) {
                capture_thread.join();
            }
        }
    }
};

LiveVisionStream::LiveVisionStream(Storage* storage, const Config& config)
    : impl_(std::make_unique<Impl>(storage)) {
    impl_->config = config;
}

LiveVisionStream::~LiveVisionStream() = default;

// ========================================================================
// STREAMING CONTROL
// ========================================================================

void LiveVisionStream::start() {
    #ifdef HAVE_OPENCV
    if (impl_->running) {
        std::cout << "[Vision] Already running\n";
        return;
    }
    
    impl_->running = true;
    
    impl_->capture_thread = std::thread([this]() {
        cv::VideoCapture cap(impl_->config.camera_index);
        
        if (!cap.isOpened()) {
            std::cerr << "[Vision] ERROR: Could not open camera " 
                      << impl_->config.camera_index << "\n";
            impl_->running = false;
            return;
        }
        
        std::cout << "[Vision] 👁️  Camera opened! Streaming...\n";
        
        int frame_delay_ms = 1000 / impl_->config.fps;
        
        while (impl_->running) {
            auto start_time = std::chrono::steady_clock::now();
            
            cv::Mat frame;
            cap >> frame;
            
            if (frame.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            
            // Process frame
            impl_->frame_counter++;
            
            VisionNode vision_node;
            vision_node.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
            vision_node.frame_number = impl_->frame_counter;
            vision_node.label = "vision_frame_" + std::to_string(impl_->frame_counter);
            vision_node.embedding = impl_->encoder->encode_frame(
                frame.data, frame.cols, frame.rows, frame.channels()
            );
            
            // Create node in graph
            NodeID frame_node = create_visual_node(vision_node);
            
            // Link temporally
            if (impl_->config.create_temporal_edges && impl_->last_frame_node != 0) {
                create_temporal_link(impl_->last_frame_node, frame_node);
            }
            
            // Link to context
            if (impl_->config.link_to_context && impl_->active_context_node != 0) {
                link_to_context(frame_node);
            }
            
            impl_->last_frame_node = frame_node;
            
            if (impl_->config.verbose) {
                std::cout << "[Vision] Frame " << impl_->frame_counter 
                          << " → Node " << frame_node << "\n";
            }
            
            // Display (optional)
            cv::imshow("Melvin Vision", frame);
            if (cv::waitKey(1) == 27) { // ESC to stop
                impl_->running = false;
                break;
            }
            
            // Maintain FPS
            auto end_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            impl_->stats.avg_processing_time_ms = 
                (impl_->stats.avg_processing_time_ms * (impl_->stats.frames_processed - 1) + elapsed) / 
                impl_->stats.frames_processed;
            
            int sleep_time = std::max(1, frame_delay_ms - static_cast<int>(elapsed));
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        }
        
        cap.release();
        cv::destroyAllWindows();
        
        std::cout << "[Vision] 👁️  Camera closed\n";
    });
    #else
    std::cerr << "[Vision] ERROR: OpenCV not available. Compile with HAVE_OPENCV=1\n";
    #endif
}

void LiveVisionStream::stop() {
    if (impl_->running) {
        impl_->running = false;
        if (impl_->capture_thread.joinable()) {
            impl_->capture_thread.join();
        }
    }
}

bool LiveVisionStream::is_running() const {
    return impl_->running;
}

NodeID LiveVisionStream::process_frame(const void* frame_data, int width, int height, int channels) {
    auto start_time = std::chrono::steady_clock::now();
    
    impl_->frame_counter++;
    impl_->stats.frames_processed++;
    
    VisionNode vision_node;
    vision_node.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    vision_node.frame_number = impl_->frame_counter;
    vision_node.label = "vision_frame_" + std::to_string(impl_->frame_counter);
    vision_node.embedding = impl_->encoder->encode_frame(frame_data, width, height, channels);
    
    NodeID frame_node = create_visual_node(vision_node);
    
    // Link temporally
    if (impl_->config.create_temporal_edges && impl_->last_frame_node != 0) {
        create_temporal_link(impl_->last_frame_node, frame_node);
    }
    
    // Link to context
    if (impl_->config.link_to_context && impl_->active_context_node != 0) {
        link_to_context(frame_node);
    }
    
    impl_->last_frame_node = frame_node;
    
    return frame_node;
}

// ========================================================================
// NODE CREATION
// ========================================================================

NodeID LiveVisionStream::create_visual_node(const VisionNode& vision_node) {
    // Create SENSORY node (visual type)
    NodeID node_id = impl_->storage->create_node(vision_node.label, NodeType::SENSORY);
    
    // Get the node and set embedding
    Node node;
    if (impl_->storage->get_node(node_id, node)) {
        node.embedding = vision_node.embedding;
        node.creation_timestamp = static_cast<uint64_t>(vision_node.timestamp);
        impl_->storage->update_node(node);
    }
    
    impl_->stats.nodes_created++;
    
    return node_id;
}

void LiveVisionStream::create_temporal_link(NodeID prev_frame, NodeID curr_frame) {
    // Create EXACT edge: previous_frame → current_frame
    // This represents temporal flow
    impl_->storage->create_edge(prev_frame, curr_frame, RelationType::EXACT, 1.0f);
    impl_->stats.temporal_edges++;
}

void LiveVisionStream::link_to_context(NodeID frame_node) {
    if (impl_->active_context_node == 0) return;
    
    // Create bidirectional link: frame <-> context
    impl_->storage->create_edge(
        frame_node, 
        impl_->active_context_node, 
        RelationType::LEAP,  // LEAP for cross-modal
        impl_->config.context_link_weight
    );
    
    impl_->storage->create_edge(
        impl_->active_context_node,
        frame_node,
        RelationType::LEAP,
        impl_->config.context_link_weight
    );
    
    impl_->stats.context_edges += 2;
}

// ========================================================================
// CONTEXT MANAGEMENT
// ========================================================================

void LiveVisionStream::set_active_context(NodeID context_node) {
    impl_->active_context_node = context_node;
}

NodeID LiveVisionStream::get_active_context() const {
    return impl_->active_context_node;
}

// ========================================================================
// STATISTICS
// ========================================================================

LiveVisionStream::Stats LiveVisionStream::get_stats() const {
    return impl_->stats;
}

void LiveVisionStream::reset_stats() {
    impl_->stats = Stats();
}

void LiveVisionStream::print_stats() const {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  👁️  VISION STREAM STATISTICS                                  ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Frames processed:      " << impl_->stats.frames_processed << "\n";
    std::cout << "Visual nodes created:  " << impl_->stats.nodes_created << "\n";
    std::cout << "Nodes merged:          " << impl_->stats.nodes_merged << "\n";
    std::cout << "Temporal edges:        " << impl_->stats.temporal_edges << "\n";
    std::cout << "Context edges:         " << impl_->stats.context_edges << "\n";
    std::cout << "Avg processing time:   " << std::fixed << std::setprecision(2) 
              << impl_->stats.avg_processing_time_ms << " ms/frame\n";
    
    if (impl_->stats.frames_processed > 0) {
        std::cout << "\nEffective FPS:         " 
                  << (1000.0f / impl_->stats.avg_processing_time_ms) << "\n";
    }
    
    std::cout << "\n";
}

void LiveVisionStream::set_config(const Config& config) {
    impl_->config = config;
}

const LiveVisionStream::Config& LiveVisionStream::get_config() const {
    return impl_->config;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

bool is_opencv_available() {
    #ifdef HAVE_OPENCV
    return true;
    #else
    return false;
    #endif
}

void test_camera(int camera_index) {
    #ifdef HAVE_OPENCV
    cv::VideoCapture cap(camera_index);
    
    if (!cap.isOpened()) {
        std::cerr << "[Vision] ERROR: Cannot open camera " << camera_index << "\n";
        return;
    }
    
    std::cout << "[Vision] Camera opened! Press ESC to quit.\n";
    
    while (true) {
        cv::Mat frame;
        cap >> frame;
        
        if (frame.empty()) {
            std::cout << "[Vision] Empty frame\n";
            break;
        }
        
        cv::imshow("Melvin Vision Test", frame);
        
        if (cv::waitKey(30) == 27) { // ESC
            break;
        }
    }
    
    cap.release();
    cv::destroyAllWindows();
    #else
    std::cout << "[Vision] OpenCV not available. Install OpenCV and recompile with -DHAVE_OPENCV\n";
    #endif
}

} // namespace io
} // namespace melvin




