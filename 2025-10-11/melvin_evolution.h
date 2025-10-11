/*
 * MELVIN CONTINUOUS LEARNING + EVOLUTION PIPELINE
 * 
 * Evolution engine for mutating and optimizing Melvin's internal parameters
 * based on cognitive performance metrics.
 */

#pragma once

#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <string>
#include "melvin_types.h"
#include "predictive_sampler.h"
#include "melvin_metrics.h"

namespace melvin {

// ==================== PARAMETER GENOME ====================

struct ParameterGenome {
    // Core predictive parameters
    float alpha = 0.7f;        // experience vs semantic balance (0.5-0.9)
    float beta = 8.0f;         // softmax sharpness (4-12)
    float gamma = 1.0f;        // relation prior scale (0.5-2.0)
    float eta = 0.002f;        // reinforcement rate (0.001-0.01)
    float delta = 0.15f;       // n-gram continuation bonus (0.05-0.3)
    float epsilon = 0.3f;      // entropy stabilization (0.1-0.5)
    
    // Additional evolution parameters
    float learning_rate = 0.01f;    // How fast to adapt (0.001-0.1)
    float exploration_rate = 0.1f;  // Random exploration factor (0.01-0.5)
    float memory_decay = 0.001f;    // Edge decay rate (0.0001-0.01)
    
    // Fitness and metadata
    float fitness = 0.0f;
    uint64_t generation = 0;
    std::chrono::system_clock::time_point created_at;
    
    ParameterGenome() : created_at(std::chrono::system_clock::now()) {}
    
    // Convert to PredictiveConfig
    PredictiveConfig to_config() const {
        PredictiveConfig cfg;
        cfg.alpha = alpha;
        cfg.beta = beta;
        cfg.gamma = gamma;
        cfg.epsilon = epsilon;
        cfg.ngram_bonus = delta;
        return cfg;
    }
    
    // Validate parameter ranges
    bool is_valid() const {
        return alpha >= 0.5f && alpha <= 0.9f &&
               beta >= 4.0f && beta <= 12.0f &&
               gamma >= 0.5f && gamma <= 2.0f &&
               eta >= 0.001f && eta <= 0.01f &&
               delta >= 0.05f && delta <= 0.3f &&
               epsilon >= 0.1f && epsilon <= 0.5f;
    }
    
    // Clamp parameters to valid ranges
    void clamp() {
        alpha = std::max(0.5f, std::min(0.9f, alpha));
        beta = std::max(4.0f, std::min(12.0f, beta));
        gamma = std::max(0.5f, std::min(2.0f, gamma));
        eta = std::max(0.001f, std::min(0.01f, eta));
        delta = std::max(0.05f, std::min(0.3f, delta));
        epsilon = std::max(0.1f, std::min(0.5f, epsilon));
        learning_rate = std::max(0.001f, std::min(0.1f, learning_rate));
        exploration_rate = std::max(0.01f, std::min(0.5f, exploration_rate));
        memory_decay = std::max(0.0001f, std::min(0.01f, memory_decay));
    }
    
    // Get parameter vector for mutation
    std::vector<float> to_vector() const {
        return {alpha, beta, gamma, eta, delta, epsilon, learning_rate, exploration_rate, memory_decay};
    }
    
    // Set from parameter vector
    void from_vector(const std::vector<float>& params) {
        if (params.size() >= 9) {
            alpha = params[0];
            beta = params[1];
            gamma = params[2];
            eta = params[3];
            delta = params[4];
            epsilon = params[5];
            learning_rate = params[6];
            exploration_rate = params[7];
            memory_decay = params[8];
            clamp();
        }
    }
    
    // Get summary string
    std::string get_summary() const {
        std::stringstream ss;
        ss << "Genome[gen=" << generation 
           << ", fitness=" << std::fixed << std::setprecision(3) << fitness
           << ", α=" << alpha << ", β=" << beta << ", γ=" << gamma
           << ", η=" << eta << ", δ=" << delta << ", ε=" << epsilon << "]";
        return ss.str();
    }
};

// ==================== EVOLUTION ENGINE ====================

class EvolutionEngine {
private:
    std::mt19937 rng_;
    ParameterGenome current_genome_;
    std::vector<ParameterGenome> evolution_history_;
    size_t max_history_size_ = 1000;
    
    // Evolution parameters
    size_t population_size_ = 10;
    size_t elite_size_ = 3;
    float mutation_rate_ = 0.1f;
    float mutation_strength_ = 0.05f;
    float crossover_rate_ = 0.7f;
    
public:
    EvolutionEngine() : rng_(std::random_device{}()) {
        current_genome_.generation = 0;
        evolution_history_.push_back(current_genome_);
    }
    
    // Get current genome
    const ParameterGenome& get_current_genome() const {
        return current_genome_;
    }
    
    // Set current genome
    void set_current_genome(const ParameterGenome& genome) {
        current_genome_ = genome;
        current_genome_.clamp();
    }
    
    // Run evolution cycle
    ParameterGenome evolve(const CognitivePerformanceMetrics& metrics) {
        std::cout << "🧬 Starting evolution cycle (gen " << current_genome_.generation + 1 << ")\n";
        
        // Update current genome fitness
        current_genome_.fitness = metrics.compute_fitness();
        
        // Generate population of mutated genomes
        std::vector<ParameterGenome> population;
        population.reserve(population_size_);
        
        // Add current genome (elitism)
        population.push_back(current_genome_);
        
        // Generate mutations
        for (size_t i = 1; i < population_size_; ++i) {
            ParameterGenome child = current_genome_;
            mutate_genome(child);
            child.generation = current_genome_.generation + 1;
            population.push_back(child);
        }
        
        // Evaluate population (simplified - in practice would run on test data)
        evaluate_population(population, metrics);
        
        // Select elite
        std::sort(population.begin(), population.end(), 
                 [](const ParameterGenome& a, const ParameterGenome& b) {
                     return a.fitness > b.fitness;
                 });
        
        // Create new genome by blending elite
        ParameterGenome new_genome = blend_elite(population);
        new_genome.generation = current_genome_.generation + 1;
        
        // Update current genome
        current_genome_ = new_genome;
        
        // Store in history
        evolution_history_.push_back(current_genome_);
        if (evolution_history_.size() > max_history_size_) {
            evolution_history_.erase(evolution_history_.begin());
        }
        
        std::cout << "🧬 Evolution complete: " << current_genome_.get_summary() << "\n";
        
        return current_genome_;
    }
    
    // Homeostatic parameter adjustment (continuous fine-tuning)
    void homeostatic_adjust(const CognitivePerformanceMetrics& metrics) {
        float adjustment_rate = 0.001f;  // Small adjustments
        
        // Entropy-based beta adjustment
        if (metrics.predictive.avg_traversal_entropy > 1.5f) {
            current_genome_.beta += adjustment_rate * 2.0f;  // Sharpen
        } else if (metrics.predictive.avg_traversal_entropy < 0.5f) {
            current_genome_.beta -= adjustment_rate * 2.0f;  // Soften
        }
        
        // Success-based alpha adjustment
        if (metrics.predictive.success_rate < 0.6f) {
            current_genome_.alpha += adjustment_rate;  // Trust memory more
        } else if (metrics.predictive.success_rate > 0.9f) {
            current_genome_.alpha -= adjustment_rate;  // Explore more
        }
        
        // Drift-based adjustments
        if (metrics.cognitive.coherence_drift > 0.25f) {
            current_genome_.eta -= adjustment_rate * 0.5f;  // Slow learning
            current_genome_.gamma += adjustment_rate * 0.5f;  // Trust relations more
        }
        
        current_genome_.clamp();
    }
    
    // Save evolution history
    void save_history(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) return;
        
        file << "generation,fitness,alpha,beta,gamma,eta,delta,epsilon,learning_rate,exploration_rate,memory_decay\n";
        
        for (const auto& genome : evolution_history_) {
            file << genome.generation << "," << genome.fitness << ","
                 << genome.alpha << "," << genome.beta << "," << genome.gamma << ","
                 << genome.eta << "," << genome.delta << "," << genome.epsilon << ","
                 << genome.learning_rate << "," << genome.exploration_rate << ","
                 << genome.memory_decay << "\n";
        }
    }
    
    // Load evolution history
    bool load_history(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return false;
        
        std::string line;
        std::getline(file, line);  // Skip header
        
        evolution_history_.clear();
        
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string cell;
            std::vector<std::string> cells;
            
            while (std::getline(ss, cell, ',')) {
                cells.push_back(cell);
            }
            
            if (cells.size() >= 11) {
                ParameterGenome genome;
                genome.generation = std::stoull(cells[0]);
                genome.fitness = std::stof(cells[1]);
                genome.alpha = std::stof(cells[2]);
                genome.beta = std::stof(cells[3]);
                genome.gamma = std::stof(cells[4]);
                genome.eta = std::stof(cells[5]);
                genome.delta = std::stof(cells[6]);
                genome.epsilon = std::stof(cells[7]);
                genome.learning_rate = std::stof(cells[8]);
                genome.exploration_rate = std::stof(cells[9]);
                genome.memory_decay = std::stof(cells[10]);
                
                evolution_history_.push_back(genome);
            }
        }
        
        if (!evolution_history_.empty()) {
            current_genome_ = evolution_history_.back();
        }
        
        return true;
    }
    
    // Get evolution statistics
    struct EvolutionStats {
        size_t total_generations = 0;
        float best_fitness = 0.0f;
        float avg_fitness = 0.0f;
        float fitness_improvement = 0.0f;
        std::vector<float> fitness_trend;
    };
    
    EvolutionStats get_stats() const {
        EvolutionStats stats;
        
        if (evolution_history_.empty()) return stats;
        
        stats.total_generations = evolution_history_.size();
        stats.best_fitness = evolution_history_[0].fitness;
        
        float total_fitness = 0.0f;
        for (const auto& genome : evolution_history_) {
            total_fitness += genome.fitness;
            stats.fitness_trend.push_back(genome.fitness);
            stats.best_fitness = std::max(stats.best_fitness, genome.fitness);
        }
        stats.avg_fitness = total_fitness / evolution_history_.size();
        
        if (evolution_history_.size() > 1) {
            stats.fitness_improvement = evolution_history_.back().fitness - 
                                      evolution_history_.front().fitness;
        }
        
        return stats;
    }
    
private:
    // Mutate a genome
    void mutate_genome(ParameterGenome& genome) {
        std::uniform_real_distribution<float> mutation_dist(0.0f, 1.0f);
        std::normal_distribution<float> noise_dist(0.0f, mutation_strength_);
        
        auto params = genome.to_vector();
        
        for (size_t i = 0; i < params.size(); ++i) {
            if (mutation_dist(rng_) < mutation_rate_) {
                params[i] += noise_dist(rng_);
            }
        }
        
        genome.from_vector(params);
    }
    
    // Evaluate population fitness (simplified)
    void evaluate_population(std::vector<ParameterGenome>& population, 
                           const CognitivePerformanceMetrics& base_metrics) {
        for (auto& genome : population) {
            // Simplified fitness evaluation based on parameter constraints
            // In practice, this would run actual reasoning tests
            
            float fitness = base_metrics.compute_fitness();
            
            // Bonus for parameter balance
            float balance_bonus = 0.0f;
            if (genome.alpha > 0.6f && genome.alpha < 0.8f) balance_bonus += 0.1f;
            if (genome.beta > 6.0f && genome.beta < 10.0f) balance_bonus += 0.1f;
            if (genome.eta > 0.001f && genome.eta < 0.005f) balance_bonus += 0.1f;
            
            genome.fitness = fitness + balance_bonus;
        }
    }
    
    // Blend elite genomes
    ParameterGenome blend_elite(const std::vector<ParameterGenome>& population) {
        ParameterGenome blended;
        size_t elite_count = std::min(elite_size_, population.size());
        
        // Weighted average of elite genomes
        float total_weight = 0.0f;
        std::vector<float> blended_params(9, 0.0f);
        
        for (size_t i = 0; i < elite_count; ++i) {
            float weight = 1.0f / (i + 1);  // Higher weight for better genomes
            total_weight += weight;
            
            auto params = population[i].to_vector();
            for (size_t j = 0; j < params.size(); ++j) {
                blended_params[j] += params[j] * weight;
            }
        }
        
        // Normalize
        for (auto& param : blended_params) {
            param /= total_weight;
        }
        
        blended.from_vector(blended_params);
        return blended;
    }
};

} // namespace melvin
