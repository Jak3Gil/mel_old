/*
 * OpenCV Attention Vision - Main Demo
 */

#include "../vision/opencv_attention.h"
#include "../core/atomic_graph.h"
#include <iostream>
#include <csignal>

using namespace melvin;
using namespace melvin::vision;

static OpenCVAttention* g_vision = nullptr;

void sigint_handler(int) {
    if (g_vision) g_vision->stop();
}

int main(int argc, char* argv[]) {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🧠 Melvin OpenCV Attention Vision                             ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    int cam = (argc > 1) ? std::atoi(argv[1]) : 0;
    
    // Load or create graph
    AtomicGraph graph;
    graph.load("melvin/data/atomic_nodes.bin", "melvin/data/atomic_edges.bin");
    
    if (graph.node_count() > 0) {
        std::cout << "✓ Loaded existing graph\n";
        graph.print_stats();
    } else {
        std::cout << "✓ Starting fresh\n\n";
    }
    
    // Configure
    OpenCVAttention::Config config;
    config.camera_index = cam;
    config.width = 640;
    config.height = 480;
    config.confidence = 0.3f;
    config.iou_threshold = 0.3f;
    config.show_display = true;
    
    std::cout << "Camera: " << cam << "\n";
    std::cout << "Resolution: " << config.width << "x" << config.height << "\n";
    std::cout << "Confidence: " << config.confidence << "\n\n";
    
    // Run
    OpenCVAttention vision(&graph, config);
    g_vision = &vision;
    std::signal(SIGINT, sigint_handler);
    
    vision.run();
    
    std::cout << "✨ Session complete! Graph saved to atomic_*.bin\n\n";
    return 0;
}

