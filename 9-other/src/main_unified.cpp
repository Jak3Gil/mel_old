/*
 * Melvin UCA v1 - Main Application
 * Complete unified cognitive loop
 */

#include "unified_mind.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <csignal>

using namespace melvin;

volatile bool g_running = true;

void signal_handler(int signal) {
    std::cout << "\n🛑 Received signal " << signal << ", shutting down...\n";
    g_running = false;
}

// Simple test pattern generator
std::vector<uint8_t> generate_frame(int w, int h, int frame_num) {
    std::vector<uint8_t> rgb(w * h * 3, 128);
    
    // Moving bright region
    int center_x = (frame_num * 5) % w;
    int center_y = h / 2;
    int radius = 20;
    
    for (int y = center_y - radius; y < center_y + radius; ++y) {
        for (int x = center_x - radius; x < center_x + radius; ++x) {
            if (x >= 0 && x < w && y >= 0 && y < h) {
                int idx = (y * w + x) * 3;
                rgb[idx + 0] = 255;
                rgb[idx + 1] = 200;
                rgb[idx + 2] = 150;
            }
        }
    }
    
    return rgb;
}

int main(int argc, char** argv) {
    // Setup signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🧠 MELVIN UCA v1 - Unified Cognitive Architecture                 ║\n";
    std::cout << "║  Real-time Perception → Reasoning → Action Loop                   ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";
    
    // Parse arguments
    int num_cycles = 100;
    if (argc > 1) {
        num_cycles = std::atoi(argv[1]);
    }
    
    std::cout << "Running " << num_cycles << " cognitive cycles...\n";
    std::cout << "(Press Ctrl+C to stop)\n\n";
    
    // Create unified mind
    UnifiedMind mind;
    
    // Run cognitive loop
    for (int i = 0; i < num_cycles && g_running; ++i) {
        // Generate synthetic frame
        FrameMeta fm;
        fm.id = i + 1;
        fm.w = 128;
        fm.h = 128;
        fm.ts = i * 50000; // 50ms intervals (20 Hz)
        fm.cam = 0;
        
        auto image = generate_frame(128, 128, i);
        
        // Run one cognitive cycle
        mind.tick(fm, image);
    }
    
    // Print final statistics
    std::cout << "\n";
    mind.print_stats();
    
    // Save knowledge
    std::cout << "\n💾 Saving knowledge...\n";
    mind.get_graph().save("melvin_nodes.bin", "melvin_edges.bin");
    std::cout << "✅ Saved to melvin_nodes.bin and melvin_edges.bin\n";
    
    std::cout << "\n✅ Melvin UCA session complete\n\n";
    
    return 0;
}


