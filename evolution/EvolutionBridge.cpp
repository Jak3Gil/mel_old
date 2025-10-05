#include "EvolutionBridge.hpp"
#include <iostream>
#include <algorithm>
#include <random>
#include <chrono>

namespace melvin {

EvolutionBridge::EvolutionBridge()
    : evolution_count_(0), best_fitness_(0.0f), rng_(std::random_device{}()) {
    // Simplified initialization without evolution system dependencies
}

void EvolutionBridge::run_full_evolution(const std::string& data_dir, const std::string& config_dir) {
    std::cout << "[EvolutionBridge] Running full evolution with data_dir=" << data_dir 
              << ", config_dir=" << config_dir << std::endl;
    
    // This would integrate with the existing evolution system
    // For now, we'll just simulate the process
    evolution_count_++;
    
    std::cout << "[EvolutionBridge] Full evolution completed." << std::endl;
}

void EvolutionBridge::run_micro_evolution(MelvinParams& params, size_t population_size, size_t generations) {
    std::cout << "[EvolutionBridge] Running micro-evolution: pop_size=" << population_size 
              << ", generations=" << generations << std::endl;
    
    // Create initial population from current parameters
    std::vector<DynamicGenome> population = create_initial_population(params, population_size);
    
    // Run evolution loop
    for (size_t gen = 0; gen < generations; ++gen) {
        std::cout << "[EvolutionBridge] Generation " << gen + 1 << "/" << generations << std::endl;
        
        // Evaluate fitness for each genome
        for (auto& genome : population) {
            float fitness = evaluate_fitness(genome);
            if (fitness > best_fitness_.load()) {
                best_fitness_ = fitness;
            }
        }
        
        // Simple selection: keep top 50%
        std::sort(population.begin(), population.end(), 
                 [this](const DynamicGenome& a, const DynamicGenome& b) {
                     return evaluate_fitness(a) > evaluate_fitness(b);
                 });
        
        size_t keep_count = population_size / 2;
        population.resize(keep_count);
        
        // Generate new population through mutation
        while (population.size() < population_size) {
            DynamicGenome parent = population[rng_() % keep_count];
            DynamicGenome mutated = mutate_genome(parent);
            population.push_back(std::move(mutated));
        }
    }
    
    // Adopt best genome
    if (!population.empty()) {
        MelvinParams best_params = genome_to_params(population[0]);
        params = best_params;
        std::cout << "[EvolutionBridge] Adopted best genome with fitness: " << evaluate_fitness(population[0]) << std::endl;
    }
    
    evolution_count_++;
}

DynamicGenome EvolutionBridge::params_to_genome(const MelvinParams& params) {
    DynamicGenome genome;
    genome.id = "micro_genome_" + std::to_string(evolution_count_.load());
    genome.seed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
    
    map_params_to_genome_values(params, genome);
    return genome;
}

MelvinParams EvolutionBridge::genome_to_params(const DynamicGenome& genome) {
    MelvinParams params;
    map_genome_values_to_params(genome, params);
    return params;
}

float EvolutionBridge::evaluate_fitness(const DynamicGenome& genome) {
    // Use custom fitness function if available
    if (custom_fitness_func_) {
        return custom_fitness_func_(genome);
    }
    
    // Simple fitness evaluation based on parameter values
    // In a real system, this would run actual cognitive tasks
    
    float fitness = 0.0f;
    
    // Reward balanced confidence thresholds
    if (genome.values.count("confidence_threshold")) {
        float conf = genome.values.at("confidence_threshold");
        fitness += 1.0f - std::abs(conf - 0.5f); // Peak at 0.5
    }
    
    // Reward reasonable depth settings
    if (genome.values.count("max_depth")) {
        float depth = genome.values.at("max_depth");
        fitness += 1.0f - std::abs(depth - 6.0f) / 6.0f; // Peak at 6
    }
    
    // Reward moderate reinforcement rates
    if (genome.values.count("reinforcement_rate")) {
        float rate = genome.values.at("reinforcement_rate");
        fitness += 1.0f - std::abs(rate - 0.1f) / 0.1f; // Peak at 0.1
    }
    
    return fitness;
}

void EvolutionBridge::set_fitness_function(std::function<float(const DynamicGenome&)> fitness_func) {
    custom_fitness_func_ = fitness_func;
}

size_t EvolutionBridge::get_evolution_count() const {
    return evolution_count_.load();
}

float EvolutionBridge::get_best_fitness() const {
    return best_fitness_.load();
}

void EvolutionBridge::reset_statistics() {
    evolution_count_.store(0);
    best_fitness_.store(0.0f);
}

void EvolutionBridge::map_params_to_genome_values(const MelvinParams& params, DynamicGenome& genome) {
    genome.values["confidence_threshold"] = params.confidence_threshold;
    genome.values["max_depth"] = static_cast<float>(params.max_depth);
    genome.values["beam_width"] = static_cast<float>(params.beam_width);
    genome.values["reinforcement_rate"] = params.reinforcement_rate;
    genome.values["decay_rate"] = params.decay_rate;
    genome.values["stagnation_threshold"] = params.stagnation_threshold;
}

void EvolutionBridge::map_genome_values_to_params(const DynamicGenome& genome, MelvinParams& params) {
    if (genome.values.count("confidence_threshold")) {
        params.confidence_threshold = genome.values.at("confidence_threshold");
    }
    if (genome.values.count("max_depth")) {
        params.max_depth = static_cast<int>(genome.values.at("max_depth"));
    }
    if (genome.values.count("beam_width")) {
        params.beam_width = static_cast<int>(genome.values.at("beam_width"));
    }
    if (genome.values.count("reinforcement_rate")) {
        params.reinforcement_rate = genome.values.at("reinforcement_rate");
    }
    if (genome.values.count("decay_rate")) {
        params.decay_rate = genome.values.at("decay_rate");
    }
    if (genome.values.count("stagnation_threshold")) {
        params.stagnation_threshold = genome.values.at("stagnation_threshold");
    }
}

DynamicGenome EvolutionBridge::mutate_genome(const DynamicGenome& genome) {
    DynamicGenome mutated;
    mutated.id = genome.id + "_mutated";
    mutated.seed = genome.seed + 1;
    mutated.values = genome.values;
    
    std::uniform_real_distribution<float> dist(-0.1f, 0.1f);
    
    for (auto& pair : mutated.values) {
        float current_val = pair.second;
        float new_val = current_val + dist(rng_);
        
        // Clamp values to reasonable ranges
        if (pair.first == "confidence_threshold") {
            new_val = std::clamp(new_val, 0.1f, 0.9f);
        } else if (pair.first == "max_depth") {
            new_val = std::clamp(new_val, 2.0f, 10.0f);
        } else if (pair.first == "beam_width") {
            new_val = std::clamp(new_val, 2.0f, 20.0f);
        } else if (pair.first == "reinforcement_rate") {
            new_val = std::clamp(new_val, 0.01f, 0.5f);
        } else if (pair.first == "decay_rate") {
            new_val = std::clamp(new_val, 0.001f, 0.1f);
        } else if (pair.first == "stagnation_threshold") {
            new_val = std::clamp(new_val, 0.1f, 0.5f);
        }
        
        pair.second = new_val;
    }
    
    return mutated;
}

std::vector<DynamicGenome> EvolutionBridge::create_initial_population(const MelvinParams& base_params, size_t size) {
    std::vector<DynamicGenome> population;
    population.reserve(size);
    
    // Add base genome
    population.push_back(params_to_genome(base_params));
    
    // Generate mutated variants
    for (size_t i = 1; i < size; ++i) {
        DynamicGenome genome = mutate_genome(population[0]);
        population.push_back(std::move(genome));
    }
    
    return population;
}

} // namespace melvin
