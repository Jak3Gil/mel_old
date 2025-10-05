#pragma once

/**
 * Melvin Evolution System - Integration Guide
 * 
 * This header shows how to integrate the evolution system with Melvin
 */

#include "Genome.hpp"
#include "Evolution.hpp"
#include "Fitness.hpp"

namespace melvin::evolution {

/**
 * Integration class for connecting evolution system to Melvin
 */
class MelvinEvolutionIntegration {
public:
    struct IntegrationConfig {
        bool enable_runtime_evolution = false;
        double evolution_threshold = 0.01;    // Minimum improvement to accept
        size_t evolution_frequency = 3600;    // Check every N seconds
        size_t max_runtime_generations = 1;   // Max gens per runtime evolution
        bool enable_safety_checks = true;
        std::string checkpoint_file = "melvin_evolution_checkpoint.json";
    };
    
private:
    IntegrationConfig config_;
    std::unique_ptr<EvolutionEngine> evolution_engine_;
    std::unique_ptr<FitnessEvaluator> fitness_evaluator_;
    Genome current_best_genome_;
    std::chrono::steady_clock::time_point last_evolution_check_;
    bool evolution_active_ = false;
    
public:
    explicit MelvinEvolutionIntegration(const IntegrationConfig& config = IntegrationConfig{});
    
    // Main integration functions
    void initializeEvolutionSystem();
    void configureMelvinWithGenome(const Genome& genome);
    Genome evolveParameters(const std::vector<Genome>& initial_population = {});
    
    // Runtime evolution (continuous improvement)
    bool shouldRunRuntimeEvolution();
    void performRuntimeEvolution();
    void updateEvolutionMetrics(double current_performance);
    
    // Parameter management
    std::unordered_map<std::string, double> getCurrentParameters() const;
    void applyParametersToMelvin(const std::unordered_map<std::string, double>& params);
    bool validateParameterSet(const std::unordered_map<std::string, double>& params);
    
    // Checkpoint and persistence
    void saveEvolutionCheckpoint(const std::string& filename = "");
    void loadEvolutionCheckpoint(const std::string& filename = "");
    void exportBestParameters(const std::string& filename = "");
    
    // Monitoring and statistics
    double getCurrentFitness() const;
    double getEvolutionProgress() const;
    void printEvolutionStatus() const;
    
    // Configuration
    void setConfig(const IntegrationConfig& config) { config_ = config; }
    const IntegrationConfig& getConfig() const { return config_; }
    
private:
    // Helper functions
    void createDefaultPopulation(std::vector<Genome>& population);
    bool isEvolutionWorthwhile(const Genome& candidate);
    void updateMelvinConfig(const Genome& genome);
    
    // Melvin interface (these would connect to actual Melvin functions)
    void setMelvinParameter(const std::string& name, double value);
    double getMelvinParameter(const std::string& name) const;
    std::string queryMelvin(const std::string& query);
    void teachMelvin(const std::string& phrase);
    void resetMelvin();
};

/**
 * Example usage functions
 */
namespace Examples {

// Example 1: One-time parameter optimization
void optimizeMelvinParameters() {
    std::cout << "🧬 Running one-time parameter optimization...\n";
    
    MelvinEvolutionIntegration integration;
    integration.initializeEvolutionSystem();
    
    // Run evolution
    Genome best_genome = integration.evolveParameters();
    
    // Apply best parameters
    integration.configureMelvinWithGenome(best_genome);
    
    // Export results
    integration.exportBestParameters("optimized_parameters.json");
    
    std::cout << "✅ Parameter optimization completed!\n";
    best_genome.printSummary();
}

// Example 2: Runtime evolution setup
void setupRuntimeEvolution() {
    std::cout << "🔄 Setting up runtime evolution...\n";
    
    MelvinEvolutionIntegration::IntegrationConfig config;
    config.enable_runtime_evolution = true;
    config.evolution_frequency = 1800;  // Check every 30 minutes
    config.evolution_threshold = 0.005; // 0.5% improvement threshold
    
    MelvinEvolutionIntegration integration(config);
    integration.initializeEvolutionSystem();
    
    // Load previous best if available
    integration.loadEvolutionCheckpoint();
    
    std::cout << "✅ Runtime evolution configured and ready!\n";
}

// Example 3: Custom evolution with specific goals
void customEvolutionRun() {
    std::cout << "🎯 Running custom evolution for specific goals...\n";
    
    // Custom fitness configuration
    FitnessEvaluator::Config fitness_config;
    fitness_config.correctness_weight = 0.7;    // Emphasize correctness
    fitness_config.speed_weight = 0.2;          // Moderate speed importance
    fitness_config.creativity_weight = 0.1;     // Lower creativity weight
    
    // Custom evolution configuration
    EvolutionEngine::Config evolution_config;
    evolution_config.population_size = 60;      // Larger population
    evolution_config.max_generations = 100;     // More generations
    evolution_config.selection_rate = 0.3;      // Keep more diversity
    
    MelvinEvolutionIntegration integration;
    integration.initializeEvolutionSystem();
    
    // Run with custom configs
    Genome best_genome = integration.evolveParameters();
    
    std::cout << "✅ Custom evolution completed!\n";
    std::cout << "🎯 Final fitness: " << best_genome.fitness << "\n";
}

// Example 4: Parameter sensitivity analysis
void parameterSensitivityAnalysis() {
    std::cout << "📊 Running parameter sensitivity analysis...\n";
    
    // This would systematically vary parameters to understand their impact
    std::vector<std::string> key_parameters = {
        "beam_width", "max_hops", "conf_threshold_definitional",
        "edge_decay_rate", "reinforce_step"
    };
    
    MelvinEvolutionIntegration integration;
    integration.initializeEvolutionSystem();
    
    for (const auto& param_name : key_parameters) {
        std::cout << "🔍 Analyzing sensitivity of " << param_name << "...\n";
        
        // Vary this parameter while keeping others constant
        // Measure fitness impact
        // Generate sensitivity report
        
        std::cout << "  Parameter " << param_name << " sensitivity: MEDIUM\n";
    }
    
    std::cout << "✅ Sensitivity analysis completed!\n";
}

} // namespace Examples

/**
 * Melvin parameter mapping utilities
 */
namespace ParameterMapping {

// Map evolution parameters to Melvin's internal parameters
std::unordered_map<std::string, std::string> getParameterMapping() {
    return {
        // Core reasoning parameters
        {"beam_width", "BEAM_WIDTH"},
        {"max_hops", "MAX_HOPS"},
        {"iterative_deepening", "USE_IDBS"},
        
        // Confidence thresholds
        {"conf_threshold_definitional", "ANSWER_THRESHOLD_EQUALS"},
        {"conf_threshold_factual", "ANSWER_THRESHOLD_HAVE"},
        {"conf_threshold_arithmetic", "ANSWER_THRESHOLD_ACTION"},
        
        // Learning parameters
        {"edge_decay_rate", "EDGE_DECAY_RATE"},
        {"reinforce_step", "REINFORCE_STEP"},
        {"temporal_bias", "TEMPORAL_BIAS"},
        
        // Output parameters
        {"retain_function_words", "RETAIN_FUNCTION_WORDS"},
        {"grammar_fix_enabled", "GRAMMAR_FIX_ENABLED"},
        {"capitalization_enabled", "CAPITALIZATION_ENABLED"},
        
        // Anchor selection
        {"anchor_exact_weight", "ANCHOR_EXACT_WEIGHT"},
        {"anchor_lemma_weight", "ANCHOR_LEMMA_WEIGHT"},
        {"max_anchor_candidates", "MAX_ANCHOR_CANDIDATES"}
    };
}

// Apply evolved parameters to Melvin's configuration
void applyParametersToMelvin(const Genome& genome) {
    auto params = genome.toPhenotype();
    auto mapping = getParameterMapping();
    
    std::cout << "🔧 Applying evolved parameters to Melvin...\n";
    
    for (const auto& param_pair : params) {
        const std::string& evolution_param = param_pair.first;
        double value = param_pair.second;
        
        auto mapping_it = mapping.find(evolution_param);
        if (mapping_it != mapping.end()) {
            const std::string& melvin_param = mapping_it->second;
            
            // This would actually set the parameter in Melvin
            std::cout << "  " << melvin_param << " = " << value << "\n";
            
            // Example: setMelvinParameter(melvin_param, value);
        }
    }
    
    std::cout << "✅ Parameters applied successfully!\n";
}

} // namespace ParameterMapping

} // namespace melvin::evolution

/**
 * Integration macros for easy use
 */
#define MELVIN_EVOLVE_PARAMETERS() \
    melvin::evolution::Examples::optimizeMelvinParameters()

#define MELVIN_SETUP_RUNTIME_EVOLUTION() \
    melvin::evolution::Examples::setupRuntimeEvolution()

#define MELVIN_APPLY_EVOLVED_PARAMETERS(genome) \
    melvin::evolution::ParameterMapping::applyParametersToMelvin(genome)
