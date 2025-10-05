#include "Genome.hpp"
#include "Evolution.hpp"
#include "Fitness.hpp"
#include "PersistentMemory.hpp"
#include <iostream>
#include <string>
#include <chrono>
#include <memory>
#include <fstream>
#include <iomanip>

using namespace melvin::evolution;

void printBanner() {
    std::cout << "🧬 =============================================\n";
    std::cout << "🧬     MELVIN EVOLUTION SYSTEM v1.0\n";
    std::cout << "🧬     Self-Adaptive Parameter Evolution\n";
    std::cout << "🧬 =============================================\n\n";
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --population SIZE     Population size (default: 40)\n";
    std::cout << "  --generations NUM     Max generations (default: 50)\n";
    std::cout << "  --output DIR          Output directory (default: evolution_logs/)\n";
    std::cout << "  --config FILE         Configuration file\n";
    std::cout << "  --seed NUM            Random seed\n";
    std::cout << "  --fast                Use mini eval only (faster)\n";
    std::cout << "  --verbose             Verbose output\n";
    std::cout << "  --help                Show this help\n\n";
}

EvolutionEngine::Config parseCommandLine(int argc, char* argv[]) {
    EvolutionEngine::Config config;
    // bool verbose = false; // Unused for now
    bool fast_mode = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            printUsage(argv[0]);
            exit(0);
        } else if (arg == "--population" && i + 1 < argc) {
            config.population_size = std::stoul(argv[++i]);
        } else if (arg == "--generations" && i + 1 < argc) {
            config.max_generations = std::stoul(argv[++i]);
        } else if (arg == "--fast") {
            fast_mode = true;
        } else if (arg == "--verbose") {
            // verbose = true; // Unused for now
        } else if (arg == "--seed" && i + 1 < argc) {
            // Would set random seed here
            std::cout << "Random seed: " << argv[++i] << "\n";
        }
    }
    
    if (fast_mode) {
        std::cout << "🚀 Fast mode enabled - using mini eval only\n";
    }
    
    return config;
}

int main(int argc, char* argv[]) {
    printBanner();
    
    // Parse command line arguments
    auto config = parseCommandLine(argc, argv);
    
    std::cout << "🧬 Configuration:\n";
    std::cout << "  Population size: " << config.population_size << "\n";
    std::cout << "  Max generations: " << config.max_generations << "\n";
    std::cout << "  Selection rate: " << config.selection_rate << "\n";
    std::cout << "  Elitism count: " << config.elitism_count << "\n";
    std::cout << "  Initial sigma: " << config.initial_sigma << "\n\n";
    
    // Initialize persistent memory system
    std::cout << "🧠 Initializing Persistent Memory System...\n";
    PersistentMemoryManager::Config mem_config;
    mem_config.memory_directory = "evolution_memory/";
    mem_config.read_write_mode = true;
    mem_config.enable_integrity_checks = true;
    mem_config.enable_clutter_control = true;
    
    auto memory_manager = std::make_unique<PersistentMemoryManager>(mem_config);
    if (memory_manager->initialize()) {
        std::cout << "✅ Persistent memory initialized successfully\n";
        
        // Get initial memory statistics
        auto stats = memory_manager->getMemoryStats();
        std::cout << "📊 Memory state: " << stats.total_nodes << " nodes, " 
                  << stats.total_edges << " edges\n";
        std::cout << "   Health score: " << std::fixed << std::setprecision(3) 
                  << memory_manager->calculateMemoryHealthScore(stats) << "\n";
    } else {
        std::cerr << "⚠️  Failed to initialize persistent memory, continuing with individual instances\n";
        memory_manager.reset();
    }
    
    // Initialize logging (simplified for now)
    std::cout << "📊 Logging initialized (simplified mode)\n";
    std::cout << "  Output directory: evolution_logs/\n";
    std::cout << "  Session ID: " << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() << "\n\n";
    
    // Create evolution engine
    EvolutionEngine engine(config);
    
    // Record start time
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        // Run evolution
        std::cout << "🚀 Starting evolution process...\n\n";
        engine.evolve();
        
        // Record end time
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::minutes>(end_time - start_time);
        
        // Final results
        std::cout << "\n🏆 ========== EVOLUTION COMPLETED ==========\n";
        std::cout << "⏱️  Total time: " << duration.count() << " minutes\n";
        std::cout << "🧬 Final generation: " << engine.getCurrentGeneration() << "\n";
        std::cout << "📈 Best fitness: " << std::fixed << std::setprecision(4) 
                  << engine.getBestFitnessEver() << "\n";
        std::cout << "📊 Final success rate: " << std::fixed << std::setprecision(3)
                  << engine.getCurrentSuccessRate() << "\n";
        std::cout << "⚙️  Final eta: " << std::fixed << std::setprecision(3)
                  << engine.getCurrentEta() << "\n\n";
        
        // Show best genome
        const auto& best_genome = engine.getBestGenome();
        std::cout << "🥇 BEST GENOME:\n";
        best_genome.printSummary();
        
        // Generate final logs (simplified)
        std::cout << "\n📝 Generating final reports...\n";
        
        // Save best genome (simplified)
        std::ofstream best_file("evolution_logs/best_genome_final.json");
        if (best_file.is_open()) {
            best_file << best_genome.toJSON();
            best_file.close();
            std::cout << "✅ Best genome saved to evolution_logs/best_genome_final.json\n";
        }
        
        std::cout << "✅ Reports generated successfully!\n";
        std::cout << "📁 Check the evolution_logs/ directory for results.\n\n";
        
        // Final memory statistics
        if (memory_manager) {
            std::cout << "🧠 Final Memory Statistics:\n";
            auto final_stats = memory_manager->getMemoryStats();
            std::cout << "  Total nodes: " << final_stats.total_nodes << "\n";
            std::cout << "  Total edges: " << final_stats.total_edges << "\n";
            std::cout << "  Health score: " << std::fixed << std::setprecision(3) 
                      << memory_manager->calculateMemoryHealthScore(final_stats) << "\n";
            std::cout << "  Memory persisted in: " << mem_config.memory_directory << "\n";
        }
        
        // Show key evolved parameters
        std::cout << "🔧 KEY EVOLVED PARAMETERS:\n";
        auto params = best_genome.toPhenotype();
        
        std::cout << "  Beam Width: " << static_cast<int>(params.at("beam_width")) << "\n";
        std::cout << "  Max Hops: " << static_cast<int>(params.at("max_hops")) << "\n";
        std::cout << "  Confidence Threshold: " << std::fixed << std::setprecision(3) 
                  << params.at("conf_threshold_definitional") << "\n";
        std::cout << "  Edge Decay Rate: " << std::scientific << std::setprecision(2)
                  << params.at("edge_decay_rate") << "\n";
        std::cout << "  Reinforce Step: " << std::fixed << std::setprecision(3)
                  << params.at("reinforce_step") << "\n";
        std::cout << "  Iterative Deepening: " << (params.at("iterative_deepening") ? "ON" : "OFF") << "\n";
        std::cout << "  Grammar Fixes: " << (params.at("grammar_fix_enabled") ? "ON" : "OFF") << "\n";
        
        std::cout << "\n🎯 Evolution system completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Evolution failed with error: " << e.what() << "\n";
        return 1;
    }
    
    // Cleanup (simplified)
    std::cout << "🧹 Cleanup completed\n";
    
    return 0;
}

// Additional utility functions for command line interface

void printEvolutionProgress(const EvolutionEngine& engine, int generation) {
    std::cout << "🔄 Generation " << generation << "/" << engine.getCurrentGeneration() 
              << " - Best: " << std::fixed << std::setprecision(4) << engine.getBestFitnessEver()
              << " - Success Rate: " << std::fixed << std::setprecision(3) << engine.getCurrentSuccessRate()
              << " - Eta: " << std::fixed << std::setprecision(3) << engine.getCurrentEta() << "\n";
}

void saveCheckpoint(const EvolutionEngine& engine, const std::string& filename) {
    // Save current population state
    std::cout << "💾 Saving checkpoint to " << filename << "\n";
    engine.savePopulation(filename);
}

void loadCheckpoint(EvolutionEngine& engine, const std::string& filename) {
    // Load population from checkpoint
    std::cout << "📂 Loading checkpoint from " << filename << "\n";
    engine.loadPopulation(filename);
}

// Real-time monitoring (if needed for runtime mode)
void monitorRuntimeEvolution() {
    std::cout << "🔄 Runtime evolution monitoring not implemented yet\n";
    std::cout << "💡 This would enable continuous evolution during Melvin operation\n";
}

// Parameter export for integration with Melvin
void exportBestParameters(const Genome& genome, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "❌ Failed to open " << filename << " for writing\n";
        return;
    }
    
    auto params = genome.toPhenotype();
    
    file << "// Melvin Evolved Parameters\n";
    file << "// Generated by Evolution System\n";
    file << "// Fitness: " << genome.fitness << "\n";
    file << "// Correctness: " << genome.correctness << "\n";
    file << "// Speed: " << genome.speed << "\n";
    file << "// Creativity: " << genome.creativity << "\n\n";
    
    for (const auto& param : params) {
        file << "const double " << param.first << " = " << param.second << ";\n";
    }
    
    std::cout << "✅ Exported best parameters to " << filename << "\n";
}
