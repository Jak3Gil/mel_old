/*
 * MELVIN TINYCHAT INGESTION
 * 
 * Downloads and ingests TinyChat dataset, runs reasoning loop, shows stats
 * All prediction data stored in graph (storage.cpp) - no separate files
 */

#include "include/melvin_core.h"
#include "src/util/telemetry.h"
#include "src/util/config.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>

using namespace melvin_core;
using namespace melvin_storage;
using namespace melvin_telemetry;
using namespace melvin_config;

// ==================== DATA INGESTION ====================

int ingest_file(UnifiedBrain& brain, const std::string& filename, int limit = 0) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "❌ Cannot open file: " << filename << "\n";
        return 0;
    }
    
    int count = 0;
    std::string line;
    
    while (std::getline(file, line)) {
        if (limit > 0 && count >= limit) break;
        
        // Skip empty lines
        if (line.empty() || line.length() < 10) continue;
        
        // Feed to brain (builds graph with edges that encode transition probabilities)
        brain.learn(line);
        
        count++;
        if (count % 100 == 0) {
            std::cout << "\r  Processed " << count << " conversations..." << std::flush;
        }
    }
    
    std::cout << "\n";
    return count;
}

// ==================== MAIN ====================

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN TINYCHAT INGESTION                                   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    // Parse args
    std::string data_file = "tinychat_data.txt";
    int limit = 0;  // 0 = no limit
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--file" && i + 1 < argc) {
            data_file = argv[++i];
        } else if (arg == "--limit" && i + 1 < argc) {
            limit = std::atoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: melvin_ingest_tinychat [OPTIONS]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --file FILE    Data file (default: tinychat_data.txt)\n";
            std::cout << "  --limit N      Process only N lines (0 = all)\n";
            std::cout << "  --help         Show this help\n\n";
            std::cout << "First run:\n";
            std::cout << "  python3 ingest_tinychat.py --limit 10000\n";
            std::cout << "  ./melvin_ingest_tinychat\n\n";
            return 0;
        }
    }
    
    // Check if data file exists
    std::ifstream check(data_file);
    if (!check) {
        std::cout << "❌ Data file not found: " << data_file << "\n";
        std::cout << "   Run first: python3 ingest_tinychat.py --limit 10000\n\n";
        return 1;
    }
    check.close();
    
    // Load config
    load_config();
    enable_telemetry(false);  // Disable for speed
    
    // Create brain
    std::cout << "🧠 Initializing brain...\n";
    UnifiedBrain brain("tinychat_brain.bin");
    
    // Initial stats
    int nodes_before = nodes.size();
    int edges_before = edges.size();
    
    std::cout << "\n📊 Before ingestion:\n";
    std::cout << "   Nodes: " << nodes_before << "\n";
    std::cout << "   Edges: " << edges_before << "\n\n";
    
    // Ingest data (graph edges encode all prediction statistics)
    std::cout << "📥 Ingesting data from " << data_file << "...\n";
    std::cout << "   (Building graph - edges encode transition probabilities)\n";
    auto start = std::chrono::high_resolution_clock::now();
    
    int processed = ingest_file(brain, data_file, limit);
    
    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();
    
    std::cout << "✅ Processed " << processed << " conversations in " 
              << std::fixed << std::setprecision(1) << elapsed << "s\n\n";
    
    // Final stats
    int nodes_after = nodes.size();
    int edges_after = edges.size();
    
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  STORAGE.CPP STATISTICS                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "📊 Total Nodes: " << nodes_after << "\n";
    std::cout << "📊 Total Connections: " << edges_after << "\n";
    std::cout << "   (All prediction data encoded in edge counts/weights)\n\n";
    
    // Save brain (includes all prediction data)
    brain.save();
    
    return 0;
}

