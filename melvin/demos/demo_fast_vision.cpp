/*
 * Fast Visual Perception Demo
 * Optimized C++ real-time vision system
 */

#include "../io/fast_visual_perception.h"
#include "../core/storage.h"
#include <iostream>
#include <csignal>
#include <atomic>

using namespace melvin;
using namespace melvin::vision;

// Global pointer for signal handler
static FastVisualPerception* g_vision_ptr = nullptr;
static std::atomic<bool> g_interrupted{false};

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[Signal] Caught interrupt signal, stopping...\n";
        g_interrupted = true;
        if (g_vision_ptr) {
            g_vision_ptr->stop();
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                                ║\n";
    std::cout << "║   🚀 MELVIN FAST VISUAL PERCEPTION (C++ Optimized)             ║\n";
    std::cout << "║                                                                ║\n";
    std::cout << "║   Target: 20+ FPS Real-Time Performance                       ║\n";
    std::cout << "║                                                                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    // Parse arguments
    int camera_index = 0;
    int target_fps = 20;
    int duration = 0;  // 0 = run until Ctrl+C
    
    if (argc > 1) {
        camera_index = std::atoi(argv[1]);
    }
    if (argc > 2) {
        target_fps = std::atoi(argv[2]);
    }
    if (argc > 3) {
        duration = std::atoi(argv[3]);
    }
    
    std::cout << "Configuration:\n";
    std::cout << "  Camera index:     " << camera_index << "\n";
    std::cout << "  Target FPS:       " << target_fps << "\n";
    std::cout << "  Duration:         " << (duration > 0 ? std::to_string(duration) + "s" : "unlimited") << "\n";
    std::cout << "\n";
    
    // Test camera access
    std::cout << "Testing camera access...\n";
    if (!test_camera_access(camera_index)) {
        std::cerr << "ERROR: Cannot access camera " << camera_index << "\n";
        std::cerr << "\nTroubleshooting:\n";
        std::cerr << "  1. Check System Settings → Privacy & Security → Camera\n";
        std::cerr << "  2. Grant Terminal camera access\n";
        std::cerr << "  3. Try different camera: ./demo_fast_vision 0\n";
        return 1;
    }
    std::cout << "✓ Camera accessible\n\n";
    
    // Create storage (uses nodes.melvin/edges.melvin)
    Storage storage;
    
    // Load existing graph if available
    if (storage.load("melvin/data/nodes.melvin", "melvin/data/edges.melvin")) {
        std::cout << "✓ Loaded existing knowledge graph\n";
        storage.print_stats();
        std::cout << "\n";
    } else {
        std::cout << "✓ Starting fresh knowledge graph\n\n";
    }
    
    // Configure vision system
    FastVisualPerception::Config config;
    config.camera_index = camera_index;
    config.target_fps = target_fps;
    config.frame_width = 640;
    config.frame_height = 480;
    config.process_every_n_frames = 1;  // Process every frame for max accuracy
    config.yolo_model = "yolov8n.pt";   // Fastest model
    config.confidence_threshold = 0.3f;
    config.min_confidence = 0.3f;
    config.min_box_area = 100;
    config.max_objects_per_frame = 50;
    config.use_threading = true;
    config.create_intra_frame_edges = true;
    config.create_inter_frame_edges = true;
    config.temporal_window = 3;
    config.intra_weight = 1.0f;
    config.inter_weight = 0.5f;
    config.show_display = true;
    config.verbose = false;
    
    // Create vision system
    FastVisualPerception vision(&storage, config);
    g_vision_ptr = &vision;
    
    // Setup signal handler
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    // Start vision system
    vision.start();
    
    // Run for specified duration or until interrupted
    if (duration > 0) {
        for (int i = 0; i < duration && !g_interrupted; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Print stats every 10 seconds
            if ((i + 1) % 10 == 0) {
                vision.print_stats();
            }
        }
        
        if (!g_interrupted) {
            vision.stop();
        }
    } else {
        std::cout << "Running indefinitely. Press Ctrl+C to stop, or 'q' in video window.\n\n";
        
        // Wait until stopped
        vision.wait_until_stopped();
    }
    
    // Final statistics
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ SESSION COMPLETE                                           ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    
    vision.print_stats();
    
    // Show final graph state
    std::cout << "Final Knowledge Graph:\n";
    storage.print_stats();
    
    std::cout << "\n💾 Graph saved to:\n";
    std::cout << "   melvin/data/nodes.melvin\n";
    std::cout << "   melvin/data/edges.melvin\n";
    std::cout << "\n✨ Visual knowledge integrated into Melvin's brain!\n\n";
    
    return 0;
}

