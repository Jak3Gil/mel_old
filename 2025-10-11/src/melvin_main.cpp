/*
 * MELVIN UNIFIED BRAIN - MAIN ENTRY POINT
 * 
 * Demonstrates the complete unified cognitive loop:
 * - Sensory input (text, audio, image placeholders)
 * - Reasoning engine (context field, traversal)
 * - Output generation (multimodal)
 * - Learning (reinforcement, evolution)
 * - Persistence (automatic save/load)
 */

#include "../include/melvin_core.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN UNIFIED BRAIN v5.0                                 ║\n";
    std::cout << "║  Complete cognitive loop: Input → Reason → Output → Learn ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    if (argc > 1) {
        std::string mode = argv[1];
        
        if (mode == "basic") {
            melvin_core::run_basic_demo();
        } 
        else if (mode == "multimodal") {
            melvin_core::run_multimodal_demo();
        }
        else if (mode == "continuous") {
            int cycles = argc > 2 ? std::atoi(argv[2]) : 100;
            melvin_core::run_continuous_demo(cycles);
        }
        else {
            std::cout << "Usage:\n";
            std::cout << "  " << argv[0] << " basic          - Run basic demo\n";
            std::cout << "  " << argv[0] << " multimodal     - Run multimodal demo\n";
            std::cout << "  " << argv[0] << " continuous [N] - Run continuous learning (N cycles)\n";
            return 1;
        }
    } else {
        // Default: run basic demo
        melvin_core::run_basic_demo();
    }
    
    return 0;
}

