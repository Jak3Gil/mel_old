/*
 * Test Melvin C++ Vision Pipeline
 * 
 * Demonstrates:
 * - Camera capture
 * - Attention computation
 * - Visual tokenization
 * - Gestalt object clustering
 * - Adaptive EXACT window
 * - Graph building
 */

#include "../vision/vision_pipeline.h"
#include "../core/optimized_storage.h"
#include <iostream>

using namespace melvin;

int main(int argc, char** argv) {
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🧠 MELVIN C++ VISION PIPELINE TEST                         ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    // Parse args
    int camera_id = (argc > 1) ? std::atoi(argv[1]) : 0;
    int max_frames = (argc > 2) ? std::atoi(argv[2]) : 0;  // 0 = infinite
    
    // Create storage
    std::cout << "📦 Initializing graph storage...\n";
    optimized::OptimizedStorage storage;
    
    // Try to load existing graph
    if (storage.load("data/vision_nodes_cpp.bin", "data/vision_edges_cpp.bin")) {
        std::cout << "✅ Loaded existing graph:\n";
        std::cout << "   Nodes: " << storage.node_count() << "\n";
        std::cout << "   Edges: " << storage.edge_count() << "\n\n";
    } else {
        std::cout << "📝 Starting with empty graph\n\n";
    }
    
    // Configure vision pipeline
    vision::VisionPipeline::Config config;
    config.camera_id = camera_id;
    config.frame_width = 640;   // Lower res for speed
    config.frame_height = 480;
    config.patch_size = 32;
    config.attention_threshold = 0.6f;
    config.show_window = true;
    config.verbose = false;
    
    // Adaptive window configuration
    config.adaptive_config.N_min = 5;
    config.adaptive_config.N_max = 50;
    config.adaptive_config.temporal_decay_lambda = 0.1f;
    config.adaptive_config.enable_stats = true;
    
    std::cout << "⚙️  Configuration:\n";
    std::cout << "   Camera: " << camera_id << "\n";
    std::cout << "   Resolution: " << config.frame_width << "×" << config.frame_height << "\n";
    std::cout << "   Patch size: " << config.patch_size << "×" << config.patch_size << "\n";
    std::cout << "   Adaptive window: " << config.adaptive_config.N_min 
              << "-" << config.adaptive_config.N_max << " frames\n\n";
    
    // Create pipeline
    vision::VisionPipeline pipeline(&storage, config);
    
    // Initialize
    if (!pipeline.initialize()) {
        std::cerr << "❌ Failed to initialize vision pipeline\n";
        return 1;
    }
    
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Starting Vision Learning...                                ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Pipeline:\n";
    std::cout << "  Camera → Attention (S+G+C+D)\n";
    std::cout << "         → Visual Patches\n";
    std::cout << "         → Gestalt Clustering\n";
    std::cout << "         → Object Nodes\n";
    std::cout << "         → Scene Nodes\n";
    std::cout << "         → Graph (Adaptive EXACT + LEAP)\n\n";
    
    // Run pipeline
    try {
        pipeline.run(max_frames);
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ VISION PIPELINE TEST COMPLETE                           ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    return 0;
}

