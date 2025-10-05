#pragma once

#include "Genome.hpp"
#include <vector>
#include <random>
#include <memory>

namespace melvin::evolution {

/**
 * Melvin's Evolution System - Core Evolution Algorithms
 * 
 * Implements self-adaptive mutation, crossover, and selection
 */

class EvolutionEngine {
public:
    // Evolution parameters
    struct Config {
        size_t population_size = 40;
        size_t max_generations = 50;
        double selection_rate = 0.25;        // Top 25%
        size_t elitism_count = 2;            // Copy best N genomes untouched
        double crossover_rate = 0.8;
        
        // Mutation parameters
        double initial_sigma = 0.15;         // Initial step size
        double global_eta = 1.0;             // Global step multiplier
        double tau_prime_factor = 1.0;       // Shared noise scaling
        double tau_factor = 1.0;             // Per-gene noise scaling
        double heavy_tail_prob = 0.1;        // Cauchy jump probability
        double heavy_tail_kappa = 1.0;       // Cauchy scale factor
        double one_fifth_a = 1.2;            // Success rate scaling
        double one_fifth_b = 1.2;            // Failure rate scaling
        double one_fifth_threshold = 0.2;    // Success rate threshold
        
        // Two-scale move parameters (optional)
        bool use_two_scale = true;
        double sigma_small = 0.02;
        double sigma_med = 0.10;
        
        // Success rate tracking
        size_t success_window = 100;         // Window for 1/5 rule
        
        // Safety and convergence
        double fitness_threshold = 0.01;     // Minimum improvement to accept
        size_t stagnation_limit = 10;        // Generations without improvement
        bool enable_safety_checks = true;
    };
    
private:
    Config config_;
    std::vector<Genome> population_;
    std::vector<double> success_history_;    // Track success rate for 1/5 rule
    size_t current_generation_;
    size_t stagnation_count_;
    double best_fitness_ever_;
    
    // Random number generation
    mutable std::random_device rd_;
    mutable std::mt19937 gen_;
    mutable std::normal_distribution<double> normal_dist_;
    mutable std::cauchy_distribution<double> cauchy_dist_;
    mutable std::uniform_real_distribution<double> uniform_dist_;
    
    // Evolution statistics
    double current_success_rate_;
    double current_eta_;
    
public:
    explicit EvolutionEngine(const Config& config);
    
    // Main evolution loop
    void evolve();
    
    // Individual operations
    Genome mutate(const Genome& parent) const;
    Genome crossover(const Genome& parent1, const Genome& parent2) const;
    std::vector<Genome> select(const std::vector<Genome>& population) const;
    
    // Success rate tracking for 1/5 rule
    void updateSuccessRate(bool success);
    void updateGlobalStepSize();
    
    // Population management
    void initializePopulation();
    void evaluateGeneration();
    void replacePopulation(const std::vector<Genome>& new_generation);
    
    // Statistics and monitoring
    const Genome& getBestGenome() const;
    double getCurrentSuccessRate() const { return current_success_rate_; }
    double getCurrentEta() const { return current_eta_; }
    size_t getCurrentGeneration() const { return current_generation_; }
    double getBestFitnessEver() const { return best_fitness_ever_; }
    
    // Convergence detection
    bool hasConverged() const;
    bool shouldTerminate() const;
    
    // Export/Import
    void savePopulation(const std::string& filename) const;
    void loadPopulation(const std::string& filename);
    
    // Utility functions
    void printGenerationSummary() const;
    void printPopulationStats() const;
    
private:
    // Helper functions for mutation
    void updateSigmas(Genome& genome) const;
    void mutateGenes(Genome& genome) const;
    void applyTwoScaleMove(Genome& genome) const;
    
    // Helper functions for crossover
    void blendGenes(Genome& child, const Genome& parent1, const Genome& parent2) const;
    void blendSigmas(Genome& child, const Genome& parent1, const Genome& parent2) const;
    
    // Validation and safety
    bool passesSafetyChecks(const Genome& genome) const;
    void validateGenome(Genome& genome) const;
    
    // Statistics calculation
    void calculateGenerationStats();
    void updateBestFitness(const Genome& genome);
};

/**
 * Evolution utilities and helper functions
 */
namespace EvolutionUtils {
    
    // Distance metrics for diversity
    double euclideanDistance(const Genome& g1, const Genome& g2);
    double phenotypeDistance(const Genome& g1, const Genome& g2);
    
    // Population diversity measures
    double calculatePopulationDiversity(const std::vector<Genome>& population);
    double calculatePhenotypeDiversity(const std::vector<Genome>& population);
    
    // Selection strategies
    std::vector<Genome> tournamentSelection(const std::vector<Genome>& population, 
                                           size_t tournament_size = 3);
    std::vector<Genome> rankSelection(const std::vector<Genome>& population);
    
    // Crossover strategies
    Genome arithmeticCrossover(const Genome& parent1, const Genome& parent2);
    Genome uniformCrossover(const Genome& parent1, const Genome& parent2);
    
    // Mutation strategies
    Genome gaussianMutation(const Genome& parent, double mutation_strength = 1.0);
    Genome polynomialMutation(const Genome& parent, double eta = 20.0);
    
    // Convergence detection
    bool checkConvergence(const std::vector<Genome>& population, 
                         double threshold = 1e-6);
    bool checkStagnation(const std::vector<double>& fitness_history, 
                        size_t window = 10);
    
    // Parameter optimization
    EvolutionEngine::Config optimizeConfig(const std::vector<Genome>& initial_population);
    
} // namespace EvolutionUtils

} // namespace melvin::evolution
