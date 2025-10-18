/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  DEMO: Live Vision Input for Melvin                                      ║
 * ║  Shows how Melvin can "see" and integrate visual data into his brain     ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "melvin/core/storage.h"
#include "melvin/io/vision_input.h"
#include <iostream>
#include <iomanip>
#include <csignal>
#include <atomic>

using namespace melvin;
using namespace melvin::io;

std::atomic<bool> keep_running{true};

void signal_handler(int) {
    std::cout << "\n[Vision] Stopping gracefully...\n";
    keep_running = false;
}

void print_header(const std::string& title) {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  " << std::left << std::setw(60) << title << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
}

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  👁️  MELVIN LIVE VISION DEMO                                   ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    
    // Setup signal handler
    std::signal(SIGINT, signal_handler);
    
    // ========================================================================
    // CHECK OPENCV
    // ========================================================================
    
    print_header("CHECKING OPENCV AVAILABILITY");
    
    if (!is_opencv_available()) {
        std::cout << "❌ OpenCV not available\n\n";
        std::cout << "To enable vision:\n";
        std::cout << "  1. Install OpenCV: brew install opencv (macOS)\n";
        std::cout << "  2. Recompile with: make HAVE_OPENCV=1\n\n";
        std::cout << "Running in TEST MODE (no actual camera)...\n\n";
        
        // Test mode without OpenCV
        auto storage = std::make_unique<Storage>();
        storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
        
        std::cout << "Simulating vision input...\n";
        
        // Create some fake visual nodes
        for (int i = 0; i < 5; ++i) {
            std::string label = "vision_frame_" + std::to_string(i);
            NodeID node = storage->create_node(label, NodeType::SENSORY);
            std::cout << "  Created visual node: " << label << " (ID: " << node << ")\n";
        }
        
        storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
        std::cout << "\n✅ Test mode complete\n";
        
        return 0;
    }
    
    std::cout << "✅ OpenCV is available!\n";
    
    // ========================================================================
    // LOAD KNOWLEDGE BASE
    // ========================================================================
    
    print_header("LOADING MELVIN'S BRAIN");
    
    auto storage = std::make_unique<Storage>();
    bool loaded = storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    
    if (loaded) {
        std::cout << "✓ Loaded existing knowledge:\n";
        std::cout << "  Nodes: " << storage->node_count() << "\n";
        std::cout << "  Edges: " << storage->edge_count() << "\n";
    } else {
        std::cout << "ℹ️  Starting fresh brain\n";
    }
    
    // ========================================================================
    // SETUP VISION STREAM
    // ========================================================================
    
    print_header("INITIALIZING VISION SYSTEM");
    
    LiveVisionStream::Config vision_config;
    vision_config.camera_index = 0;
    vision_config.fps = 10;  // 10 frames per second
    vision_config.embedding_dim = 256;
    vision_config.create_temporal_edges = true;   // Link consecutive frames
    vision_config.link_to_context = true;         // Link to reasoning context
    vision_config.context_link_weight = 0.5f;
    vision_config.verbose = true;
    
    auto vision = std::make_unique<LiveVisionStream>(storage.get(), vision_config);
    
    std::cout << "Configuration:\n";
    std::cout << "  Camera index:      " << vision_config.camera_index << "\n";
    std::cout << "  FPS:               " << vision_config.fps << "\n";
    std::cout << "  Embedding dim:     " << vision_config.embedding_dim << "\n";
    std::cout << "  Temporal linking:  " << (vision_config.create_temporal_edges ? "Yes" : "No") << "\n";
    std::cout << "  Context linking:   " << (vision_config.link_to_context ? "Yes" : "No") << "\n";
    
    // ========================================================================
    // SET ACTIVE CONTEXT (OPTIONAL)
    // ========================================================================
    
    print_header("SETTING REASONING CONTEXT");
    
    // Create or find a context node to link vision to
    NodeID context_node = storage->create_node("current_observation", NodeType::ABSTRACT);
    vision->set_active_context(context_node);
    
    std::cout << "✓ Active context: 'current_observation' (ID: " << context_node << ")\n";
    std::cout << "  Visual frames will link to this concept\n";
    
    // ========================================================================
    // START VISION STREAM
    // ========================================================================
    
    print_header("STARTING LIVE VISION STREAM");
    
    std::cout << "🎥 Opening camera...\n";
    std::cout << "⚠️  Press ESC in the video window to stop\n";
    std::cout << "⚠️  Or press Ctrl+C in this terminal\n\n";
    
    // Start streaming
    vision->start();
    
    // Wait for user to stop
    while (keep_running && vision->is_running()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    vision->stop();
    
    // ========================================================================
    // SHOW RESULTS
    // ========================================================================
    
    print_header("VISION STREAM RESULTS");
    
    vision->print_stats();
    
    std::cout << "Knowledge base growth:\n";
    std::cout << "  Nodes: " << storage->node_count() << "\n";
    std::cout << "  Edges: " << storage->edge_count() << "\n";
    
    // ========================================================================
    // SAVE
    // ========================================================================
    
    print_header("SAVING RESULTS");
    
    std::cout << "Saving visual memories to disk...\n";
    if (storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin")) {
        std::cout << "✅ Saved!\n";
    } else {
        std::cerr << "❌ Failed to save\n";
    }
    
    // ========================================================================
    // SUMMARY
    // ========================================================================
    
    print_header("SUMMARY");
    
    std::cout << "✅ Melvin can now SEE!\n\n";
    std::cout << "What happened:\n";
    std::cout << "  ✓ Camera frames captured\n";
    std::cout << "  ✓ Converted to visual embeddings\n";
    std::cout << "  ✓ Created as SENSORY nodes in brain\n";
    std::cout << "  ✓ Linked temporally (frame sequence)\n";
    std::cout << "  ✓ Linked to reasoning context\n";
    std::cout << "  ✓ Saved to persistent storage\n\n";
    
    std::cout << "Next steps:\n";
    std::cout << "  • Integrate into reasoning loop\n";
    std::cout << "  • Add object detection (YOLO, etc.)\n";
    std::cout << "  • Use CLIP for semantic embeddings\n";
    std::cout << "  • Link visual + textual reasoning\n\n";
    
    return 0;
}




