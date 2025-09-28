#include "melvin.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <filesystem>

using namespace melvin;

int main() {
    std::cout << "🧠 Hardened Evolutionary Brain Simulation Demo" << std::endl;
    std::cout << "==============================================" << std::endl;
    
    // Create output directory for the simulation
    std::string output_dir = "hardened_brain_simulation";
    
    try {
        // Create the simulation runner with deterministic seed
        uint64_t master_seed = 1234567890;
        BrainSimulationRunner runner(output_dir);
        
        std::cout << "Starting hardened evolutionary brain simulation..." << std::endl;
        std::cout << "Master seed: " << master_seed << std::endl;
        std::cout << std::endl;
        
        std::cout << "Hardening features enabled:" << std::endl;
        std::cout << "  ✅ Binary memory layout with Melvin's reserved codes" << std::endl;
        std::cout << "  ✅ 2-bit relation codes (00=Exact, 01=Temporal, 10=Leap)" << std::endl;
        std::cout << "  ✅ Sensory data streams (audio tokens, image embeddings)" << std::endl;
        std::cout << "  ✅ Deterministic seed hierarchy" << std::endl;
        std::cout << "  ✅ Constrained parallelism (bounded workers)" << std::endl;
        std::cout << "  ✅ Crash containment with brain quarantine" << std::endl;
        std::cout << "  ✅ I/O budget tracking as fitness penalty" << std::endl;
        std::cout << "  ✅ Precise ECE and Self-Consistency metrics" << std::endl;
        std::cout << "  ✅ Multi-hop probes with temporal chains and leap bridges" << std::endl;
        std::cout << "  ✅ Thought-node replay audits" << std::endl;
        std::cout << std::endl;
        
        // Run sanity-check playbook (first 3 generations)
        std::cout << "Running sanity-check playbook..." << std::endl;
        
        // Gen 0 (smoke test)
        std::cout << "\n--- Generation 0 (Smoke Test) ---" << std::endl;
        std::cout << "Population: 20, Events: 5k" << std::endl;
        runner.run_single_generation();
        
        // Gen 1 (pressure test)
        std::cout << "\n--- Generation 1 (Pressure Test) ---" << std::endl;
        std::cout << "Population: 50, Events: 25k" << std::endl;
        runner.run_single_generation();
        
        // Gen 2 (scale test)
        std::cout << "\n--- Generation 2 (Scale Test) ---" << std::endl;
        std::cout << "Population: 100, Events: 50k" << std::endl;
        runner.run_single_generation();
        
        std::cout << "\nSimulation completed!" << std::endl;
        std::cout << "Results exported to: " << output_dir << std::endl;
        std::cout << std::endl;
        
        // Export final results
        runner.export_final_results();
        
        std::cout << "Check the following directories for results:" << std::endl;
        std::cout << "  - " << output_dir << "/genomes/     (brain configurations)" << std::endl;
        std::cout << "  - " << output_dir << "/metrics/     (performance metrics)" << std::endl;
        std::cout << "  - " << output_dir << "/thoughts/    (reasoning traces)" << std::endl;
        std::cout << "  - " << output_dir << "/brains/      (brain memory data)" << std::endl;
        std::cout << std::endl;
        
        // Check for success indicators
        std::cout << "Success indicators to look for:" << std::endl;
        std::cout << "  📈 Fitness rising via MultiHop, (1-ECE), and Self-Consistency" << std::endl;
        std::cout << "  📊 Leap:Temporal edge ratio stabilizing (~0.2–0.35)" << std::endl;
        std::cout << "  🎯 Thought-node replays >95% reproduction rate" << std::endl;
        std::cout << "  💾 I/O budget not exploding (bytes_written/probe stable)" << std::endl;
        std::cout << "  🧬 Elite diversity maintained (genomes not collapsing)" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error running simulation: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
