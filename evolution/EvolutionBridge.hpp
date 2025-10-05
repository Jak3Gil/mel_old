#pragma once

#include "melvin_types.h"
#include "uca_types.h" // For MelvinParams, DynamicGenome
#include <memory>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <atomic>
#include <functional>

namespace melvin {

/**
 * EvolutionBridge - Integrates existing evolution system with UCA
 * 
 * Responsibilities:
 * - Bridge between UCA parameters and evolution system
 * - Manage micro-evolution for reflection
 * - Handle full evolution cycles
 * - Convert between parameter formats
 */
class EvolutionBridge {
public:
    EvolutionBridge();
    ~EvolutionBridge() = default;

    // Core evolution methods
    void run_full_evolution(const std::string& data_dir, const std::string& config_dir);
    void run_micro_evolution(MelvinParams& params, size_t population_size = 10, size_t generations = 5);
    
    // Parameter conversion
    DynamicGenome params_to_genome(const MelvinParams& params);
    MelvinParams genome_to_params(const DynamicGenome& genome);
    
    // Evolution configuration
    void set_fitness_function(std::function<float(const DynamicGenome&)> fitness_func);
    
    // Statistics
    size_t get_evolution_count() const;
    float get_best_fitness() const;
    void reset_statistics();

private:
    // Note: We'll use simplified evolution without the full evolution system for now
    // to avoid type conflicts between different DynamicGenome definitions
    
    std::atomic<size_t> evolution_count_;
    std::atomic<float> best_fitness_;
    
    std::mt19937 rng_; // Random number generator
    
    // Internal methods
    void map_params_to_genome_values(const MelvinParams& params, DynamicGenome& genome);
    void map_genome_values_to_params(const DynamicGenome& genome, MelvinParams& params);
    DynamicGenome mutate_genome(const DynamicGenome& genome);
    std::vector<DynamicGenome> create_initial_population(const MelvinParams& base_params, size_t size);
    float evaluate_fitness(const DynamicGenome& genome);
    
    // Custom fitness function
    std::function<float(const DynamicGenome&)> custom_fitness_func_;
};

} // namespace melvin
