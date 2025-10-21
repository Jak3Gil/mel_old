/*
 * Melvin Hypothesis Vision Demo
 * 
 * Demonstrates self-organizing perceptual hierarchy:
 * Patch → Hypothesis → Object → Scene → Concept
 */

#include "hypothesis_vision.h"
#include <iostream>

using namespace melvin::vision;
using namespace cv;

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🧠 MELVIN HYPOTHESIS VISION SYSTEM                          ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Self-Organizing Perceptual Hierarchy:\n";
    std::cout << "  Patch → Hypothesis → Object → Scene → Concept\n\n";
    
    // Configure system
    HypothesisVision::Config config;
    config.scales = {30, 60, 120};  // 3-level pyramid
    config.max_hypotheses_per_scale = 50;
    config.hypothesis_confidence_threshold = 0.4f;
    config.enable_prediction = true;
    
    std::cout << "Configuration:\n";
    std::cout << "  Scales: 30×30, 60×60, 120×120 pixels\n";
    std::cout << "  Multi-scale pyramid: 3 levels\n";
    std::cout << "  Hypothesis confidence: " << config.hypothesis_confidence_threshold << "\n\n";
    
    HypothesisVision vision(config);
    
    // Open camera
    VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "❌ Cannot open camera\n";
        return 1;
    }
    
    std::cout << "✅ Camera opened\n\n";
    
    std::cout << "Graph Structure:\n";
    std::cout << "  • SPATIALLY_NEAR: Adjacent patches\n";
    std::cout << "  • PART_OF: Fine → coarse scale\n";
    std::cout << "  • OBSERVES: Hypothesis → patch\n";
    std::cout << "  • PREDICTS: Hypothesis → object\n";
    std::cout << "  • TEMPORAL_NEXT: Frame-to-frame\n";
    std::cout << "  • INSTANCE_OF: Object → concept\n";
    std::cout << "  • Spatial: ABOVE, BELOW, LEFT_OF, RIGHT_OF, INSIDE, CONTAINS\n\n";
    
    std::cout << "Controls:\n";
    std::cout << "  'q' - Quit\n";
    std::cout << "  'p' - Print stats\n";
    std::cout << "  SPACE - Pause\n\n";
    
    std::cout << "Processing...\n\n";
    
    Mat frame;
    bool paused = false;
    int total_frames = 0;
    
    while (true) {
        if (!paused) {
            cap >> frame;
            if (frame.empty()) break;
            
            // Process through hypothesis system
            SceneNode scene = vision.process_frame(frame);
            
            // Visualize
            Mat vis = vision.visualize_graph(frame);
            
            // Add stats overlay
            auto stats = vision.get_stats();
            std::string info = "Frame: " + std::to_string(stats.frames_processed);
            putText(vis, info, Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2);
            
            info = "Patches: " + std::to_string(stats.total_patches) + 
                   " | Hypotheses: " + std::to_string(stats.total_hypotheses);
            putText(vis, info, Point(10, 60), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 2);
            
            info = "Objects: " + std::to_string(stats.total_objects) + 
                   " | Concepts: " + std::to_string(stats.total_concepts);
            putText(vis, info, Point(10, 85), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 2);
            
            info = "Edges: " + std::to_string(stats.total_edges);
            putText(vis, info, Point(10, 110), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(200, 200, 200), 2);
            
            imshow("🧠 Melvin Hypothesis Vision", vis);
            
            total_frames++;
            
            if (total_frames % 60 == 0) {
                std::cout << "📊 Frame " << total_frames 
                          << " | Objects: " << stats.total_objects
                          << " | Concepts: " << stats.total_concepts
                          << " | Edges: " << stats.total_edges << "\n";
            }
        }
        
        int key = waitKey(1);
        if (key == 'q' || key == 27) break;
        if (key == ' ') paused = !paused;
        if (key == 'p') vision.print_stats();
    }
    
    std::cout << "\n";
    vision.print_stats();
    
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ HYPOTHESIS VISION DEMO COMPLETE                          ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Graph hierarchy built:\n";
    std::cout << "  " << vision.get_patches().size() << " patches (multi-scale)\n";
    std::cout << "  " << vision.get_hypotheses().size() << " hypotheses (last frame)\n";
    std::cout << "  " << vision.get_objects().size() << " objects (tracked)\n";
    std::cout << "  " << vision.get_scenes().size() << " scenes (temporal)\n";
    std::cout << "  " << vision.get_concepts().size() << " concepts (emerged)\n";
    std::cout << "  " << vision.get_edges().size() << " edges (relationships)\n\n";
    
    return 0;
}

