#include "Evolution.hpp"
#include "Fitness.hpp"
#include <algorithm>
#include <numeric>
#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>

namespace melvin::evolution {

EvolutionEngine::EvolutionEngine(const Config& config)
    : config_(config)
    , current_generation_(0)
    , stagnation_count_(0)
    , best_fitness_ever_(0.0)
    , gen_(rd_())
    , normal_dist_(0.0, 1.0)
    , cauchy_dist_(0.0, 1.0)
    , uniform_dist_(0.0, 1.0)
    , current_success_rate_(0.0)
    , current_eta_(config.global_eta)
{
    success_history_.reserve(config.success_window);
}

void EvolutionEngine::evolve() {
    std::cout << "🧬 Starting Melvin Evolution System\n";
    std::cout << "Population: " << config_.population_size 
              << ", Generations: " << config_.max_generations << "\n\n";
    
    initializePopulation();
    
    for (current_generation_ = 0; current_generation_ < config_.max_generations; ++current_generation_) {
        std::cout << "=== Generation " << current_generation_ << " ===\n";
        
        // Evaluate current population
        evaluateGeneration();
        
        // Check for termination
        if (shouldTerminate()) {
            std::cout << "🎯 Evolution terminated early (convergence/stagnation)\n";
            break;
        }
        
        // Selection
        auto selected = select(population_);
        std::cout << "Selected " << selected.size() << " parents\n";
        
        // Generate new population
        std::vector<Genome> new_generation;
        new_generation.reserve(config_.population_size);
        
        // Elitism: copy best genomes unchanged
        for (size_t i = 0; i < config_.elitism_count && i < selected.size(); ++i) {
            new_generation.push_back(selected[i]);
            new_generation.back().generation = current_generation_ + 1;
        }
        
        // Generate offspring through crossover and mutation
        while (new_generation.size() < config_.population_size) {
            if (uniform_dist_(gen_) < config_.crossover_rate && selected.size() >= 2) {
                // Crossover
                std::uniform_int_distribution<size_t> parent_dist(0, selected.size() - 1);
                size_t p1_idx = parent_dist(gen_);
                size_t p2_idx = parent_dist(gen_);
                while (p2_idx == p1_idx && selected.size() > 1) {
                    p2_idx = parent_dist(gen_);
                }
                
                auto child = crossover(selected[p1_idx], selected[p2_idx]);
                child.generation = current_generation_ + 1;
                child.id = new_generation.size();
                
                new_generation.push_back(std::move(child));
            } else {
                // Mutation only
                std::uniform_int_distribution<size_t> parent_dist(0, selected.size() - 1);
                auto child = mutate(selected[parent_dist(gen_)]);
                child.generation = current_generation_ + 1;
                child.id = new_generation.size();
                
                new_generation.push_back(std::move(child));
            }
        }
        
        // Replace population
        replacePopulation(new_generation);
        
        // Update global step size based on success rate
        updateGlobalStepSize();
        
        // Print generation summary
        printGenerationSummary();
        
        std::cout << "\n";
    }
    
    std::cout << "🏆 Evolution completed!\n";
    std::cout << "Best fitness achieved: " << best_fitness_ever_ << "\n";
    getBestGenome().printSummary();
}

Genome EvolutionEngine::mutate(const Genome& parent) const {
    Genome child = parent;
    
    // Update sigma values using self-adaptive strategy
    updateSigmas(child);
    
    // Mutate genes
    mutateGenes(child);
    
    // Optional two-scale move
    if (config_.use_two_scale) {
        applyTwoScaleMove(child);
    }
    
    // Validate and fix any issues
    validateGenome(child);
    
    return child;
}

void EvolutionEngine::updateSigmas(Genome& genome) const {
    const double n = static_cast<double>(Genome::GENOME_SIZE);
    const double tau_prime = config_.tau_prime_factor / std::sqrt(2.0 * n);
    const double tau = config_.tau_factor / std::sqrt(2.0 * std::sqrt(n));
    
    double shared_noise = normal_dist_(gen_);
    
    for (size_t i = 0; i < Genome::GENOME_SIZE; ++i) {
        double per_gene_noise = normal_dist_(gen_);
        double sigma_update = tau_prime * shared_noise + tau * per_gene_noise;
        genome.sigmas[i] = genome.sigmas[i] * std::exp(sigma_update);
        
        // Ensure sigma doesn't get too small or too large
        genome.sigmas[i] = std::max(1e-6, std::min(genome.sigmas[i], 10.0));
    }
}

void EvolutionEngine::mutateGenes(Genome& genome) const {
    for (size_t i = 0; i < Genome::GENOME_SIZE; ++i) {
        double delta = 0.0;
        
        if (uniform_dist_(gen_) < config_.heavy_tail_prob) {
            // Heavy-tail Cauchy mutation
            delta = config_.heavy_tail_kappa * current_eta_ * genome.sigmas[i] * cauchy_dist_(gen_);
        } else {
            // Standard Gaussian mutation
            delta = current_eta_ * genome.sigmas[i] * normal_dist_(gen_);
        }
        
        genome.genes[i] += delta;
    }
}

void EvolutionEngine::applyTwoScaleMove(Genome& genome) const {
    for (size_t i = 0; i < Genome::GENOME_SIZE; ++i) {
        double small_scale = config_.sigma_small * normal_dist_(gen_);
        double med_scale = config_.sigma_med * normal_dist_(gen_);
        double two_scale_delta = 0.5 * small_scale + 0.5 * med_scale;
        
        genome.genes[i] += two_scale_delta;
    }
}

Genome EvolutionEngine::crossover(const Genome& parent1, const Genome& parent2) const {
    Genome child;
    child.genes.resize(Genome::GENOME_SIZE);
    child.sigmas.resize(Genome::GENOME_SIZE);
    
    // Blend genes
    blendGenes(child, parent1, parent2);
    
    // Blend sigmas (inherit from fitter parent or blend)
    blendSigmas(child, parent1, parent2);
    
    // Validate
    validateGenome(child);
    
    return child;
}

void EvolutionEngine::blendGenes(Genome& child, const Genome& parent1, const Genome& parent2) const {
    double beta = uniform_dist_(gen_); // Random blend factor
    
    for (size_t i = 0; i < Genome::GENOME_SIZE; ++i) {
        child.genes[i] = beta * parent1.genes[i] + (1.0 - beta) * parent2.genes[i];
    }
}

void EvolutionEngine::blendSigmas(Genome& child, const Genome& parent1, const Genome& parent2) const {
    // Inherit sigmas from fitter parent, or blend if fitness is similar
    if (std::abs(parent1.fitness - parent2.fitness) > 0.01) {
        // One parent is clearly better, inherit its sigmas
        const Genome& better_parent = (parent1.fitness > parent2.fitness) ? parent1 : parent2;
        child.sigmas = better_parent.sigmas;
    } else {
        // Similar fitness, blend sigmas
        double beta = uniform_dist_(gen_);
        for (size_t i = 0; i < Genome::GENOME_SIZE; ++i) {
            child.sigmas[i] = beta * parent1.sigmas[i] + (1.0 - beta) * parent2.sigmas[i];
        }
    }
}

std::vector<Genome> EvolutionEngine::select(const std::vector<Genome>& population) const {
    // Sort by fitness (descending)
    std::vector<Genome> sorted_pop = population;
    std::sort(sorted_pop.begin(), sorted_pop.end(), 
              [](const Genome& a, const Genome& b) { return a.fitness > b.fitness; });
    
    // Select top percentage
    size_t select_count = static_cast<size_t>(
        std::ceil(config_.selection_rate * sorted_pop.size()));
    select_count = std::max(select_count, config_.elitism_count);
    
    std::vector<Genome> selected;
    selected.reserve(select_count);
    
    for (size_t i = 0; i < select_count; ++i) {
        selected.push_back(sorted_pop[i]);
    }
    
    return selected;
}

void EvolutionEngine::initializePopulation() {
    population_.clear();
    population_.reserve(config_.population_size);
    
    for (size_t i = 0; i < config_.population_size; ++i) {
        Genome genome;
        genome.randomize(1.0, config_.initial_sigma);
        genome.generation = 0;
        genome.id = i;
        population_.push_back(std::move(genome));
    }
    
    std::cout << "Initialized population of " << population_.size() << " genomes\n";
}

void EvolutionEngine::evaluateGeneration() {
    static FitnessEvaluator::Config fitness_config;
    static FitnessEvaluator evaluator(fitness_config);
    
    for (auto& genome : population_) {
        if (!genome.evaluated) {
            evaluator.evaluate(genome);
        }
    }
    
    calculateGenerationStats();
}

void EvolutionEngine::replacePopulation(const std::vector<Genome>& new_generation) {
    population_ = new_generation;
    
    // Track success rate for 1/5 rule
    if (current_generation_ > 0) {
        // double avg_fitness_old = 0.0; // Unused for now
        double avg_fitness_new = 0.0;
        
        // Calculate average fitness of previous generation (would need to store)
        // For now, use a simplified approach
        for (const auto& genome : population_) {
            avg_fitness_new += genome.fitness;
        }
        avg_fitness_new /= population_.size();
        
        // Update success rate (simplified)
        bool improvement = avg_fitness_new > best_fitness_ever_;
        updateSuccessRate(improvement);
    }
}

void EvolutionEngine::updateSuccessRate(bool success) {
    success_history_.push_back(success ? 1.0 : 0.0);
    
    if (success_history_.size() > config_.success_window) {
        success_history_.erase(success_history_.begin());
    }
    
    if (!success_history_.empty()) {
        current_success_rate_ = std::accumulate(success_history_.begin(), 
                                               success_history_.end(), 0.0) / 
                               success_history_.size();
    }
}

void EvolutionEngine::updateGlobalStepSize() {
    if (current_success_rate_ > config_.one_fifth_threshold) {
        current_eta_ *= config_.one_fifth_a;
    } else if (current_success_rate_ < config_.one_fifth_threshold) {
        current_eta_ /= config_.one_fifth_b;
    }
    
    // Keep eta within reasonable bounds
    current_eta_ = std::max(0.1, std::min(current_eta_, 10.0));
}

const Genome& EvolutionEngine::getBestGenome() const {
    return *std::max_element(population_.begin(), population_.end(),
                            [](const Genome& a, const Genome& b) {
                                return a.fitness < b.fitness;
                            });
}

bool EvolutionEngine::hasConverged() const {
    if (population_.size() < 2) return false;
    
    // Check if all genomes are very similar
    double max_fitness = population_[0].fitness;
    double min_fitness = population_[0].fitness;
    
    for (const auto& genome : population_) {
        max_fitness = std::max(max_fitness, genome.fitness);
        min_fitness = std::min(min_fitness, genome.fitness);
    }
    
    return (max_fitness - min_fitness) < 1e-6;
}

bool EvolutionEngine::shouldTerminate() const {
    if (hasConverged()) return true;
    
    if (stagnation_count_ >= config_.stagnation_limit) return true;
    
    return false;
}

void EvolutionEngine::validateGenome(Genome& genome) const {
    // Ensure all values are finite
    for (size_t i = 0; i < Genome::GENOME_SIZE; ++i) {
        if (!std::isfinite(genome.genes[i])) {
            genome.genes[i] = normal_dist_(gen_);
        }
        if (!std::isfinite(genome.sigmas[i]) || genome.sigmas[i] <= 0.0) {
            genome.sigmas[i] = config_.initial_sigma;
        }
    }
}

void EvolutionEngine::calculateGenerationStats() {
    double total_fitness = 0.0;
    double max_fitness = 0.0;
    
    for (const auto& genome : population_) {
        total_fitness += genome.fitness;
        max_fitness = std::max(max_fitness, genome.fitness);
    }
    
    updateBestFitness(getBestGenome());
    
    double avg_fitness = total_fitness / population_.size();
    
    std::cout << "Avg fitness: " << std::fixed << std::setprecision(4) << avg_fitness
              << ", Max: " << max_fitness << ", Best ever: " << best_fitness_ever_ << "\n";
}

void EvolutionEngine::updateBestFitness(const Genome& genome) {
    if (genome.fitness > best_fitness_ever_) {
        best_fitness_ever_ = genome.fitness;
        stagnation_count_ = 0;
    } else {
        stagnation_count_++;
    }
}

void EvolutionEngine::printGenerationSummary() const {
    std::cout << "Success rate: " << std::fixed << std::setprecision(3) 
              << current_success_rate_ << ", Eta: " << current_eta_ << "\n";
    
    if (stagnation_count_ > 0) {
        std::cout << "Stagnation: " << stagnation_count_ << "/" 
                  << config_.stagnation_limit << "\n";
    }
}

void EvolutionEngine::printPopulationStats() const {
    std::cout << "=== Population Statistics ===\n";
    
    std::vector<double> fitnesses;
    for (const auto& genome : population_) {
        fitnesses.push_back(genome.fitness);
    }
    
    std::sort(fitnesses.begin(), fitnesses.end());
    
    std::cout << "Fitness distribution:\n";
    std::cout << "  Min: " << fitnesses.front() << "\n";
    std::cout << "  Q1:  " << fitnesses[fitnesses.size() / 4] << "\n";
    std::cout << "  Med: " << fitnesses[fitnesses.size() / 2] << "\n";
    std::cout << "  Q3:  " << fitnesses[3 * fitnesses.size() / 4] << "\n";
    std::cout << "  Max: " << fitnesses.back() << "\n";
    
    double sum = std::accumulate(fitnesses.begin(), fitnesses.end(), 0.0);
    double mean = sum / fitnesses.size();
    
    double variance = 0.0;
    for (double f : fitnesses) {
        variance += (f - mean) * (f - mean);
    }
    variance /= fitnesses.size();
    
    std::cout << "  Mean: " << mean << ", Std: " << std::sqrt(variance) << "\n";
}

void EvolutionEngine::savePopulation(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << filename << " for writing\n";
        return;
    }
    
    // Write header
    file << "generation,id,fitness,fitness_mini,fitness_agi,correctness,speed,creativity,eta,success_rate,";
    for (int i = 1; i <= 55; ++i) {
        file << "g" << i;
        if (i < 55) file << ",";
    }
    file << ",";
    for (int i = 1; i <= 55; ++i) {
        file << "s" << i;
        if (i < 55) file << ",";
    }
    file << ",phen_beam_width,phen_max_hops,phen_conf_def,phen_edge_decay,phen_reinforce_step\n";
    
    // Write data
    for (const auto& genome : population_) {
        file << genome.toCSV() << "\n";
    }
    
    std::cout << "Saved population to " << filename << "\n";
}

void EvolutionEngine::loadPopulation(const std::string& filename) {
    // Implementation would parse CSV and load population
    std::cout << "Load population from " << filename << " (not implemented yet)\n";
}

} // namespace melvin::evolution
