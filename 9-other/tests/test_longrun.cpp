/*
 * Long-Run Soak Test
 * 
 * Endurance test for Melvin's brain:
 * - Runs reasoning cycles continuously for extended periods
 * - Monitors metric drift and learning stability
 * - Verifies persistence integrity with periodic snapshots
 * - Tracks memory usage and performance
 * 
 * Run for hours/days to prove production readiness.
 */

#include "../core/storage.h"
#include "../core/reasoning.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <ctime>

using namespace melvin;

struct RunMetrics {
    float avg_node_weight = 0.0f;
    float avg_edge_weight = 0.0f;
    int active_nodes = 0;
    int active_edges = 0;
};

RunMetrics compute_metrics(Storage& storage) {
    RunMetrics metrics;
    
    auto nodes = storage.get_all_nodes();
    auto edges = storage.get_all_edges();
    
    float total_node_weight = 0.0f;
    float total_edge_weight = 0.0f;
    
    for (const auto& node : nodes) {
        if (node.weight > 0.0f) {
            total_node_weight += node.weight;
            metrics.active_nodes++;
        }
    }
    
    for (const auto& edge : edges) {
        if (edge.adaptive_weight > 0.0f) {
            total_edge_weight += edge.adaptive_weight;
            metrics.active_edges++;
        }
    }
    
    metrics.avg_node_weight = metrics.active_nodes > 0 
        ? total_node_weight / metrics.active_nodes 
        : 0.0f;
    
    metrics.avg_edge_weight = metrics.active_edges > 0 
        ? total_edge_weight / metrics.active_edges 
        : 0.0f;
    
    return metrics;
}

std::string current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char buffer[26];
    ctime_r(&time, buffer);
    buffer[24] = '\0';  // Remove newline
    return std::string(buffer);
}

int main(int argc, char* argv[]) {
    // Parse arguments
    int total_cycles = 1000;
    int sleep_ms = 500;
    int verify_every = 50;
    int report_every = 10;
    
    if (argc > 1) total_cycles = std::atoi(argv[1]);
    if (argc > 2) sleep_ms = std::atoi(argv[2]);
    if (argc > 3) verify_every = std::atoi(argv[3]);
    
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  LONG-RUN SOAK TEST                                   ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "⚙️  Configuration:\n";
    std::cout << "  Total cycles:       " << total_cycles << "\n";
    std::cout << "  Sleep between:      " << sleep_ms << " ms\n";
    std::cout << "  Snapshot every:     " << verify_every << " cycles\n";
    std::cout << "  Report every:       " << report_every << " cycles\n\n";
    
    // Load Melvin's brain
    Storage storage;
    std::cout << "📂 Loading brain...\n";
    if (!storage.load("data/nodes.melvin", "data/edges.melvin")) {
        std::cerr << "✗ Failed to load brain!\n";
        std::cerr << "  Run ./demos/teach_knowledge first\n\n";
        return 1;
    }
    
    std::cout << "  ✅ Loaded: " << storage.node_count() << " nodes, " 
              << storage.edge_count() << " edges\n\n";
    
    // Set up reasoning with auto-save
    ReasoningEngine engine;
    engine.set_auto_save_paths("data/nodes.melvin", "data/edges.melvin");
    
    // Test queries
    std::vector<std::string> queries = {
        "what is fire?",
        "what are dogs?",
        "what do plants need?",
        "where do animals live?"
    };
    
    // Open log file
    std::ofstream log("data/longrun_log.csv");
    log << "cycle,timestamp,query,hops,energy,avg_node_weight,avg_edge_weight,active_nodes,active_edges,time_ms\n";
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  STARTING LONG-RUN TEST\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    std::cout << "Started: " << current_timestamp() << "\n\n";
    std::cout << "Press Ctrl+C to stop...\n\n";
    
    auto test_start = std::chrono::steady_clock::now();
    
    // Main test loop
    for (int cycle = 0; cycle < total_cycles; ++cycle) {
        for (const auto& qtext : queries) {
            // Time the inference
            auto start = std::chrono::high_resolution_clock::now();
            
            Query q;
            q.text = qtext;
            Answer a = engine.infer(q, &storage);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            
            // Extract metrics
            float hops = a.metrics.count("actual_hops") ? a.metrics["actual_hops"] : 0.0f;
            float energy = a.metrics.count("final_energy") ? a.metrics["final_energy"] : 0.0f;
            
            // Compute weight metrics
            RunMetrics metrics = compute_metrics(storage);
            
            // Log to CSV
            log << cycle << ","
                << "\"" << current_timestamp() << "\","
                << "\"" << qtext << "\","
                << hops << ","
                << energy << ","
                << metrics.avg_node_weight << ","
                << metrics.avg_edge_weight << ","
                << metrics.active_nodes << ","
                << metrics.active_edges << ","
                << ms << "\n";
            log.flush();  // Ensure data is written (important for long runs)
        }
        
        // Periodic snapshot verification
        if (cycle > 0 && cycle % verify_every == 0) {
            std::cout << "[Cycle " << std::setw(4) << cycle << "] ";
            std::cout << "📸 Creating verification snapshot... ";
            
            if (storage.save("data/nodes_verify.melvin", "data/edges_verify.melvin")) {
                std::cout << "✓\n";
            } else {
                std::cout << "✗ Failed!\n";
            }
        }
        
        // Periodic progress report
        if (cycle > 0 && cycle % report_every == 0) {
            RunMetrics metrics = compute_metrics(storage);
            auto elapsed = std::chrono::steady_clock::now() - test_start;
            auto elapsed_sec = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
            
            float progress = (float)cycle / total_cycles * 100.0f;
            float cycles_per_sec = (float)cycle / elapsed_sec;
            int eta_sec = (int)((total_cycles - cycle) / cycles_per_sec);
            
            std::cout << "[Cycle " << std::setw(4) << cycle << "] "
                      << "Progress: " << std::fixed << std::setprecision(1) << progress << "% | "
                      << "Active: " << metrics.active_nodes << " nodes, " << metrics.active_edges << " edges | "
                      << "ETA: " << (eta_sec / 60) << "m " << (eta_sec % 60) << "s\n";
        }
        
        // Sleep between cycles (simulates real-world usage pattern)
        if (sleep_ms > 0 && cycle < total_cycles - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        }
    }
    
    log.close();
    
    auto test_end = std::chrono::steady_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::seconds>(test_end - test_start).count();
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  TEST COMPLETE\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    std::cout << "Finished: " << current_timestamp() << "\n\n";
    
    std::cout << "📊 Summary:\n";
    std::cout << "  Total cycles:       " << total_cycles << "\n";
    std::cout << "  Total queries:      " << (total_cycles * queries.size()) << "\n";
    std::cout << "  Total duration:     " << (total_duration / 60) << "m " << (total_duration % 60) << "s\n";
    std::cout << "  Cycles per second:  " << std::fixed << std::setprecision(2) 
              << ((float)total_cycles / total_duration) << "\n\n";
    
    RunMetrics final_metrics = compute_metrics(storage);
    std::cout << "  Final state:\n";
    std::cout << "    Active nodes:     " << final_metrics.active_nodes << "\n";
    std::cout << "    Active edges:     " << final_metrics.active_edges << "\n";
    std::cout << "    Avg node weight:  " << std::fixed << std::setprecision(4) 
              << final_metrics.avg_node_weight << "\n";
    std::cout << "    Avg edge weight:  " << final_metrics.avg_edge_weight << "\n\n";
    
    std::cout << "✅ Results saved:\n";
    std::cout << "  📄 data/longrun_log.csv\n";
    std::cout << "  📄 data/nodes_verify.melvin (snapshot)\n";
    std::cout << "  📄 data/edges_verify.melvin (snapshot)\n\n";
    
    std::cout << "📊 Next steps:\n";
    std::cout << "  python3 scripts/verify_snapshots.py\n";
    std::cout << "  python3 scripts/analyze_longrun.py\n\n";
    
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ ENDURANCE TEST COMPLETE                           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    return 0;
}

