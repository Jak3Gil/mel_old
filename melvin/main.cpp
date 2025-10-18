/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN - Main Entry Point                                                ║
 * ║  Unified brain for all modalities                                         ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "core/melvin.h"
#include "UCAConfig.h"
#include <iostream>
#include <cstdlib>

using namespace melvin;

int main(int argc, char** argv) {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN v1.0 - Emergent Dimensional Reasoning        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    
    // Create core brain
    MelvinCore melvin;
    
    // Load configuration
    const char* config_path = std::getenv("MELVIN_CONFIG");
    if (config_path) {
        std::cout << "Loading config from: " << config_path << "\n";
        melvin.load_config(config_path);
    } else {
        std::cout << "Using default configuration\n";
    }
    
    // Load memory
    std::cout << "Loading memory...\n";
    bool memory_loaded = melvin.load_memory(
        config::STORAGE_PATH_NODES,
        config::STORAGE_PATH_EDGES
    );
    
    if (!memory_loaded) {
        std::cout << "Starting with fresh memory\n";
    }
    
    // Check for command-line arguments
    if (argc > 1) {
        // Single query mode
        std::string query;
        for (int i = 1; i < argc; ++i) {
            if (i > 1) query += " ";
            query += argv[i];
        }
        
        std::cout << "\nQuery: " << query << "\n";
        auto answer = melvin.query(query);
        std::cout << "\n" << answer.text << "\n";
        std::cout << "(confidence: " << answer.confidence << ")\n\n";
        
        // Save and exit
        melvin.save_memory();
        return 0;
    }
    
    // Interactive mode
    melvin.run_interactive();
    
    return 0;
}

