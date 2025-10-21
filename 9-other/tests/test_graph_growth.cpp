/*
 * Graph Growth Tracker
 * 
 * Monitors and visualizes Melvin's brain expansion:
 * - Node count over time
 * - Edge count over time
 * - EXACT vs LEAP connection breakdown
 * - Growth rate analysis
 * 
 * Watch Melvin's brain grow in real-time!
 */

#include "../core/storage.h"
#include "../core/reasoning.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <thread>

using namespace melvin;

void display_progress_bar(int current, int total, int bar_width = 40) {
    float progress = (float)current / total;
    int pos = bar_width * progress;
    
    std::cout << "[";
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) std::cout << "█";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << (progress * 100.0f) << "%\r";
    std::cout.flush();
}

int main(int argc, char* argv[]) {
    // Parse arguments
    int total_cycles = 500;
    int sleep_ms = 100;
    int snapshot_every = 50;
    bool verbose = false;
    
    if (argc > 1) total_cycles = std::atoi(argv[1]);
    if (argc > 2) sleep_ms = std::atoi(argv[2]);
    if (argc > 3) snapshot_every = std::atoi(argv[3]);
    if (argc > 4) verbose = (std::string(argv[4]) == "true");
    
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  GRAPH GROWTH TRACKER                                 ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "⚙️  Configuration:\n";
    std::cout << "  Total cycles:       " << total_cycles << "\n";
    std::cout << "  Sleep between:      " << sleep_ms << " ms\n";
    std::cout << "  Snapshot every:     " << snapshot_every << " cycles\n";
    std::cout << "  Verbose:            " << (verbose ? "yes" : "no") << "\n\n";
    
    // Load brain
    Storage storage;
    std::cout << "📂 Loading brain...\n";
    if (!storage.load("data/nodes.melvin", "data/edges.melvin")) {
        std::cerr << "✗ Failed to load brain!\n";
        std::cerr << "  Run ./demos/teach_knowledge first\n\n";
        return 1;
    }
    
    size_t initial_nodes = storage.node_count();
    size_t initial_edges = storage.edge_count();
    
    std::cout << "  ✅ Initial state: " << initial_nodes << " nodes, " 
              << initial_edges << " edges\n\n";
    
    // Set up reasoning
    ReasoningEngine engine;
    engine.set_auto_save_paths("data/nodes.melvin", "data/edges.melvin");
    
    // Test queries
    std::vector<std::string> queries = {
        "what is fire?",
        "what are dogs?",
        "what do plants need?",
        "where do animals live?"
    };
    
    // Open log
    std::ofstream log("data/graph_growth.csv");
    log << "cycle,nodes,edges,exact,leap,time_ms\n";
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  TRACKING GROWTH\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    auto test_start = std::chrono::steady_clock::now();
    
    // Main tracking loop
    for (int cycle = 0; cycle < total_cycles; ++cycle) {
        auto cycle_start = std::chrono::high_resolution_clock::now();
        
        // Run queries
        for (const auto& qtext : queries) {
            Query q;
            q.text = qtext;
            engine.infer(q, &storage);
        }
        
        auto cycle_end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(cycle_end - cycle_start).count();
        
        // Get counts
        size_t node_count = storage.node_count();
        size_t edge_count = storage.edge_count();
        size_t exact_count = storage.edge_count_by_type(RelationType::EXACT);
        size_t leap_count = storage.edge_count_by_type(RelationType::LEAP);
        
        // Log to CSV
        log << cycle << ","
            << node_count << ","
            << edge_count << ","
            << exact_count << ","
            << leap_count << ","
            << ms << "\n";
        log.flush();
        
        // Display progress
        if (verbose) {
            std::cout << "[Cycle " << std::setw(4) << cycle << "] "
                      << "Nodes: " << std::setw(4) << node_count << " "
                      << "Edges: " << std::setw(4) << edge_count << " "
                      << "(EXACT: " << exact_count << ", LEAP: " << leap_count << ") "
                      << ms << "ms\n";
        } else if (cycle % 10 == 0) {
            display_progress_bar(cycle, total_cycles);
        }
        
        // Periodic snapshots
        if (cycle > 0 && cycle % snapshot_every == 0) {
            storage.save("data/nodes_growth_snapshot.melvin", "data/edges_growth_snapshot.melvin");
            
            if (verbose) {
                std::cout << "  📸 Snapshot saved\n";
            }
        }
        
        // Sleep between cycles
        if (sleep_ms > 0 && cycle < total_cycles - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        }
    }
    
    if (!verbose) {
        display_progress_bar(total_cycles, total_cycles);
        std::cout << "\n";
    }
    
    log.close();
    
    auto test_end = std::chrono::steady_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::seconds>(test_end - test_start).count();
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  GROWTH COMPLETE\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    size_t final_nodes = storage.node_count();
    size_t final_edges = storage.edge_count();
    size_t final_exact = storage.edge_count_by_type(RelationType::EXACT);
    size_t final_leap = storage.edge_count_by_type(RelationType::LEAP);
    
    std::cout << "📊 Growth Summary:\n";
    std::cout << "  Duration:           " << (total_duration / 60) << "m " << (total_duration % 60) << "s\n";
    std::cout << "  Total cycles:       " << total_cycles << "\n";
    std::cout << "  Total queries:      " << (total_cycles * queries.size()) << "\n\n";
    
    std::cout << "  Initial state:\n";
    std::cout << "    Nodes:            " << initial_nodes << "\n";
    std::cout << "    Edges:            " << initial_edges << "\n\n";
    
    std::cout << "  Final state:\n";
    std::cout << "    Nodes:            " << final_nodes << " (+" << (final_nodes - initial_nodes) << ")\n";
    std::cout << "    Edges:            " << final_edges << " (+" << (final_edges - initial_edges) << ")\n";
    std::cout << "    EXACT:            " << final_exact << "\n";
    std::cout << "    LEAP:             " << final_leap << "\n\n";
    
    float node_growth_rate = ((float)(final_nodes - initial_nodes) / total_cycles);
    float edge_growth_rate = ((float)(final_edges - initial_edges) / total_cycles);
    
    std::cout << "  Growth rates:\n";
    std::cout << "    Nodes/cycle:      " << std::fixed << std::setprecision(3) << node_growth_rate << "\n";
    std::cout << "    Edges/cycle:      " << edge_growth_rate << "\n\n";
    
    std::cout << "✅ Results saved:\n";
    std::cout << "  📄 data/graph_growth.csv\n";
    std::cout << "  📄 data/nodes_growth_snapshot.melvin\n";
    std::cout << "  📄 data/edges_growth_snapshot.melvin\n\n";
    
    std::cout << "📊 Next steps:\n";
    std::cout << "  python3 scripts/analyze_graph_growth.py\n";
    std::cout << "  Or: make growth (runs both)\n\n";
    
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ GROWTH TRACKING COMPLETE                          ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    return 0;
}

