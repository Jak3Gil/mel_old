#ifndef POPULATION_EVOLUTION_H
#define POPULATION_EVOLUTION_H

#include "../evolution/genome.h"
#include <vector>
#include <deque>
#include <string>
#include <random>

namespace melvin {
namespace cognitive_field {

/**
 * Population-Based Evolution
 * 
 * Replaces per-cycle mutation with:
 * 1. Rolling fitness over N cycles (temporal smoothing)
 * 2. Population bank of 5-10 genome variants
 * 3. Crossover between successful genomes
 * 4. Selective mutation (only parameters correlated with fitness variance)
 * 
 * This prevents chaotic drift and enables stable adaptation.
 */

class PopulationEvolution {
public:
    PopulationEvolution(size_t population_size = 10, size_t fitness_window = 100);
    
    // ========================================================================
    // Population Management
    // ========================================================================
    
    /**
     * Initialize population with default genome + variations
     */
    void initialize_population();
    
    /**
     * Get current active genome
     */
    evolution::Genome& get_active_genome();
    const evolution::Genome& get_active_genome() const;
    
    /**
     * Get all genomes in population
     */
    std::vector<evolution::Genome>& get_population();
    
    // ========================================================================
    // Fitness Tracking (Rolling Window)
    // ========================================================================
    
    struct FitnessSnapshot {
        std::chrono::high_resolution_clock::time_point timestamp;
        float prediction_accuracy;
        float energy_efficiency;
        float learning_speed;
        float combined_fitness;
    };
    
    /**
     * Record fitness for current cycle
     */
    void record_fitness(float prediction_accuracy,
                       float energy_efficiency,
                       float learning_speed);
    
    /**
     * Get rolling fitness (averaged over window)
     */
    float get_rolling_fitness() const;
    
    /**
     * Get fitness trend (improving / declining)
     */
    float get_fitness_trend() const;
    
    // ========================================================================
    // Evolution Triggers
    // ========================================================================
    
    /**
     * Check if evolution should occur
     * Requires: enough samples + high fitness variance
     */
    bool should_evolve() const;
    
    /**
     * Perform one evolution cycle
     * 1. Evaluate all genomes
     * 2. Select top performers
     * 3. Crossover
     * 4. Selective mutation
     * 5. Replace weakest genomes
     */
    void evolve();
    
    // ========================================================================
    // Selective Mutation
    // ========================================================================
    
    /**
     * Identify which parameters correlate with fitness variance
     * Only mutate these parameters
     */
    std::vector<std::string> identify_mutation_targets() const;
    
    /**
     * Mutate a genome selectively
     */
    void selective_mutate(evolution::Genome& genome,
                         const std::vector<std::string>& target_params);
    
    // ========================================================================
    // Crossover (Genetic Recombination)
    // ========================================================================
    
    /**
     * Create child genome from two parents
     * Randomly selects genes from each parent
     */
    evolution::Genome crossover(const evolution::Genome& parent_a,
                               const evolution::Genome& parent_b);
    
    // ========================================================================
    // Selection
    // ========================================================================
    
    /**
     * Select top K genomes by rolling fitness
     */
    std::vector<size_t> select_top_genomes(size_t k) const;
    
    /**
     * Select bottom K genomes (for replacement)
     */
    std::vector<size_t> select_bottom_genomes(size_t k) const;
    
    /**
     * Tournament selection (for crossover parent selection)
     */
    size_t tournament_select(size_t tournament_size = 3) const;
    
    // ========================================================================
    // Genome Evaluation
    // ========================================================================
    
    /**
     * Evaluate all genomes (assign fitness scores)
     */
    void evaluate_all_genomes();
    
    /**
     * Get fitness scores for all genomes
     */
    std::vector<float> get_all_fitness_scores() const;
    
    // ========================================================================
    // Persistence
    // ========================================================================
    
    void save_population(const std::string& directory) const;
    void load_population(const std::string& directory);
    
    // ========================================================================
    // Statistics
    // ========================================================================
    
    struct Stats {
        size_t generation;
        size_t population_size;
        float best_fitness;
        float avg_fitness;
        float worst_fitness;
        float fitness_variance;
        size_t total_evolutions;
        size_t samples_since_last_evolution;
    };
    
    Stats get_stats() const;
    
private:
    size_t population_size_;
    size_t fitness_window_;
    
    // Population
    std::vector<evolution::Genome> population_;
    std::vector<float> genome_fitness_scores_;
    size_t active_genome_index_ = 0;
    
    // Fitness history (rolling window)
    std::deque<FitnessSnapshot> fitness_history_;
    
    // Parameter tracking (for selective mutation)
    // Maps parameter name -> recent fitness correlation
    std::unordered_map<std::string, std::deque<float>> parameter_fitness_history_;
    
    // Evolution statistics
    size_t generation_ = 0;
    size_t total_evolutions_ = 0;
    size_t samples_since_evolution_ = 0;
    
    // Evolution thresholds
    size_t min_samples_for_evolution_ = 100;
    float min_fitness_variance_for_evolution_ = 0.05f;
    
    // Random number generator
    std::mt19937 rng_;
    
    // Helper functions
    float compute_fitness_variance() const;
    float compute_parameter_correlation(const std::string& param_name) const;
    void update_parameter_histories();
};

} // namespace cognitive_field
} // namespace melvin

#endif // POPULATION_EVOLUTION_H

