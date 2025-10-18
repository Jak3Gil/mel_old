/*
 * Melvin Attention Vision Demo (C++)
 * Human-like visual attention with direct Storage integration
 */

#include "../io/attention_vision.h"
#include "../core/atomic_graph.h"
#include <iostream>
#include <csignal>
#include <atomic>

using namespace melvin;
using namespace melvin::vision;

static std::atomic<bool> g_interrupted{false};

void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\n[Signal] Caught Ctrl+C, stopping...\n";
        g_interrupted = true;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                                ║\n";
    std::cout << "║   🧠 MELVIN ATTENTION VISION (C++ Implementation)              ║\n";
    std::cout << "║                                                                ║\n";
    std::cout << "║   Human-like attention + Direct Storage integration           ║\n";
    std::cout << "║                                                                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    // Parse arguments
    int camera_index = 0;
    if (argc > 1) {
        camera_index = std::atoi(argv[1]);
    }
    
    // Setup signal handler
    std::signal(SIGINT, signal_handler);
    
    // Create AtomicGraph (loads existing graph if present)
    AtomicGraph graph;
    
    graph.load("melvin/data/atomic_nodes.bin", "melvin/data/atomic_edges.bin");
    
    if (graph.node_count() > 0) {
        std::cout << "[Graph] ✓ Loaded existing knowledge graph\n";
        graph.print_stats();
        std::cout << "\n";
    } else {
        std::cout << "[Graph] ✓ Starting fresh knowledge graph\n\n";
    }
    
    // Configure attention vision
    AttentionVision::Config config;
    config.camera_index = camera_index;
    config.frame_width = 640;
    config.frame_height = 480;
    config.yolo_model = "yolov8n.pt";
    config.confidence_threshold = 0.3f;
    config.focus_size = 300;
    config.iou_threshold = 0.3f;
    config.max_missing_frames = 5;
    config.show_display = true;
    config.verbose = false;
    
    std::cout << "Configuration:\n";
    std::cout << "  Camera:         " << camera_index << "\n";
    std::cout << "  Resolution:     " << config.frame_width << "x" << config.frame_height << "\n";
    std::cout << "  Focus size:     " << config.focus_size << "x" << config.focus_size << "\n";
    std::cout << "  IoU threshold:  " << config.iou_threshold << "\n\n";
    
    // Create vision system
    AttentionVision vision(&graph, config);
    
    std::cout << "🚀 Starting attention vision...\n";
    std::cout << "   Press 'q' or Ctrl+C to stop\n\n";
    
    // Run vision system
    vision.run();
    
    // Final summary
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ SESSION COMPLETE                                           ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    
    vision.print_stats();
    
    std::cout << "\n✨ Visual knowledge integrated into AtomicGraph!\n";
    std::cout << "   Saved to: melvin/data/atomic_nodes.bin\n";
    std::cout << "   Saved to: melvin/data/atomic_edges.bin\n";
    std::cout << "   (Vision + language in unified binary graph)\n\n";
    
    return 0;
}

