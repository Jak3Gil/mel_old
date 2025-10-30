#include "three_channel_feedback.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <random>

namespace melvin {
namespace feedback {

// ============================================================================
// SensoryFeedback Implementation
// ============================================================================

void SensoryFeedback::add_event(const SensoryEvent& event) {
    recent_events.push_back(event);
    if (recent_events.size() > max_history) {
        recent_events.erase(recent_events.begin());
    }
}

void SensoryFeedback::process_sensory_loop(fields::UnifiedActivationField& field) {
    // Re-inject recent sensory consequences back into the field
    for (const auto& event : recent_events) {
        // Convert sensory data to field energy
        float energy = event.reward * 10.0f;  // Scale reward to energy
        
        // Inject into field (node ID based on modality)
        int node_id = static_cast<int>(event.modality) + 1000000;  // Sensory node range
        field.inject_energy(node_id, energy, event.observation);
    }
}

std::unordered_map<int, float> SensoryFeedback::compute_action_values(
    const std::vector<int>& action_nodes) {
    
    std::unordered_map<int, float> values;
    
    // For each action, compute average reward when it was active
    for (int action_node : action_nodes) {
        float total_reward = 0.0f;
        int count = 0;
        
        for (const auto& event : recent_events) {
            // TODO: Check if action_node was active at event time
            // For now, simple temporal correlation
            total_reward += event.reward;
            count++;
        }
        
        values[action_node] = (count > 0) ? (total_reward / count) : 0.0f;
    }
    
    return values;
}

// ============================================================================
// CognitiveFeedback Implementation
// ============================================================================

void CognitiveFeedback::add_error(const PredictionError& error) {
    error_history.push_back(error);
    if (error_history.size() > max_history) {
        error_history.erase(error_history.begin());
    }
}

void CognitiveFeedback::update_attention_bias() {
    // Nodes that are frequently mispredicted should get more attention
    std::unordered_map<int, float> error_counts;
    std::unordered_map<int, float> total_errors;
    
    for (const auto& err : error_history) {
        error_counts[err.predicted_node]++;
        error_counts[err.actual_node]++;
        total_errors[err.predicted_node] += err.error_magnitude;
        total_errors[err.actual_node] += err.error_magnitude;
    }
    
    // Update bias based on error frequency and magnitude
    for (const auto& [node_id, count] : error_counts) {
        float avg_error = total_errors[node_id] / count;
        // High error → high attention bias
        attention_bias[node_id] = std::tanh(avg_error * 2.0f);
    }
}

void CognitiveFeedback::adjust_edge_weights(fields::UnifiedActivationField& field) {
    // Hebbian-like learning: strengthen edges that led to correct predictions
    // weaken edges that led to errors
    
    for (const auto& err : error_history) {
        if (err.predicted_node == err.actual_node) {
            // Correct prediction - strengthen the path
            // TODO: Trace back which edges were used and strengthen them
        } else {
            // Incorrect prediction - weaken the path
            // TODO: Trace back which edges were used and weaken them
        }
    }
}

float CognitiveFeedback::compute_surprise(const std::vector<float>& predicted_dist,
                                          const std::vector<float>& actual_dist) {
    // KL divergence: D_KL(actual || predicted)
    float kl = 0.0f;
    for (size_t i = 0; i < std::min(predicted_dist.size(), actual_dist.size()); ++i) {
        if (actual_dist[i] > 0.0f && predicted_dist[i] > 0.0f) {
            kl += actual_dist[i] * std::log(actual_dist[i] / predicted_dist[i]);
        }
    }
    return kl;
}

float CognitiveFeedback::adaptive_learning_rate(float base_rate) {
    if (error_history.size() < 10) return base_rate;
    
    // Compute error trend (increasing or decreasing)
    float recent_error = 0.0f, older_error = 0.0f;
    size_t mid = error_history.size() / 2;
    
    for (size_t i = mid; i < error_history.size(); ++i) {
        recent_error += error_history[i].error_magnitude;
    }
    for (size_t i = 0; i < mid; ++i) {
        older_error += error_history[i].error_magnitude;
    }
    
    recent_error /= (error_history.size() - mid);
    older_error /= mid;
    
    // If error is decreasing, keep rate; if increasing, boost it
    if (recent_error > older_error) {
        return base_rate * 1.5f;  // Learning not working, increase rate
    } else {
        return base_rate * 0.8f;  // Learning working, decrease rate
    }
}

// ============================================================================
// EvolutionaryFeedback Implementation
// ============================================================================

float EvolutionaryFeedback::FitnessMetrics::compute_fitness(
    const evolution::Genome& genome) const {
    
    // Weighted combination of metrics
    float w_accuracy = genome.get("fitness_weight_accuracy");
    float w_efficiency = genome.get("fitness_weight_efficiency");
    float w_speed = genome.get("fitness_weight_learning_speed");
    float w_novelty = genome.get("fitness_weight_novelty");
    float w_coherence = genome.get("fitness_weight_coherence");
    
    return (w_accuracy * prediction_accuracy +
            w_efficiency * energy_efficiency +
            w_speed * learning_speed +
            w_novelty * novelty_seeking +
            w_coherence * coherence) /
           (w_accuracy + w_efficiency + w_speed + w_novelty + w_coherence);
}

void EvolutionaryFeedback::initialize_population() {
    population.clear();
    fitness_history.clear();
    
    // Create base genome
    evolution::Genome base;
    
    // Add fitness weight genes
    base.add_gene("fitness_weight_accuracy", 0.4f, 0.0f, 1.0f);
    base.add_gene("fitness_weight_efficiency", 0.2f, 0.0f, 1.0f);
    base.add_gene("fitness_weight_learning_speed", 0.2f, 0.0f, 1.0f);
    base.add_gene("fitness_weight_novelty", 0.1f, 0.0f, 1.0f);
    base.add_gene("fitness_weight_coherence", 0.1f, 0.0f, 1.0f);
    
    // Create population with variations
    std::random_device rd;
    std::mt19937 gen(rd());
    
    for (size_t i = 0; i < population_size; ++i) {
        evolution::Genome variant = base;
        if (i > 0) {
            variant.mutate(gen);  // Mutate all but first (keep one pure base)
        }
        population.push_back(variant);
        fitness_history.push_back(std::vector<float>());
    }
}

void EvolutionaryFeedback::evaluate_fitness(const FitnessMetrics& metrics, int genome_index) {
    if (genome_index < 0 || genome_index >= (int)population.size()) return;
    
    recent_metrics.push_back(metrics);
    if (recent_metrics.size() > smoothing_window) {
        recent_metrics.erase(recent_metrics.begin());
    }
    
    float fitness = metrics.compute_fitness(population[genome_index]);
    fitness_history[genome_index].push_back(fitness);
}

float EvolutionaryFeedback::get_smoothed_fitness(int genome_index) const {
    if (genome_index < 0 || genome_index >= (int)fitness_history.size()) return 0.0f;
    
    const auto& history = fitness_history[genome_index];
    if (history.empty()) return 0.0f;
    
    // Use last N values
    size_t start = (history.size() > smoothing_window) ? 
                   (history.size() - smoothing_window) : 0;
    
    float sum = 0.0f;
    for (size_t i = start; i < history.size(); ++i) {
        sum += history[i];
    }
    return sum / (history.size() - start);
}

int EvolutionaryFeedback::select_best_genome() const {
    int best_idx = 0;
    float best_fitness = get_smoothed_fitness(0);
    
    for (size_t i = 1; i < population.size(); ++i) {
        float fitness = get_smoothed_fitness(i);
        if (fitness > best_fitness) {
            best_fitness = fitness;
            best_idx = i;
        }
    }
    
    return best_idx;
}

evolution::Genome EvolutionaryFeedback::crossover(
    const evolution::Genome& parent1,
    const evolution::Genome& parent2) {
    
    evolution::Genome child;
    const auto& genes1 = parent1.get_genes();
    const auto& genes2 = parent2.get_genes();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // For each gene, randomly pick from parent1 or parent2
    for (const auto& [name, gene1] : genes1) {
        auto it2 = genes2.find(name);
        if (it2 != genes2.end()) {
            const auto& gene2 = it2->second;
            float value = (dist(gen) < 0.5f) ? gene1.value : gene2.value;
            child.add_gene(name, value, gene1.min_val, gene1.max_val,
                          gene1.mutation_rate, gene1.mutation_strength, gene1.is_integer);
        }
    }
    
    return child;
}

void EvolutionaryFeedback::selective_mutation(
    evolution::Genome& genome,
    const std::vector<std::string>& gene_names) {
    
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Only mutate specified genes
    auto& all_genes = const_cast<std::unordered_map<std::string, evolution::Gene>&>(
        genome.get_genes());
    
    for (const auto& gene_name : gene_names) {
        auto it = all_genes.find(gene_name);
        if (it != all_genes.end()) {
            it->second.mutate(gen);
        }
    }
}

std::vector<std::string> EvolutionaryFeedback::find_correlated_genes(
    const evolution::Genome& genome,
    const std::vector<float>& fitness_values) {
    
    // Simple correlation: track gene value changes vs fitness changes
    // TODO: Implement proper correlation analysis
    
    std::vector<std::string> correlated;
    const auto& genes = genome.get_genes();
    
    // For now, return all genes (naive implementation)
    for (const auto& [name, gene] : genes) {
        correlated.push_back(name);
    }
    
    return correlated;
}

void EvolutionaryFeedback::evolution_step(int current_cycle) {
    if (population.empty()) {
        initialize_population();
        return;
    }
    
    // Select best two genomes
    int best1 = select_best_genome();
    std::vector<float> fitness_scores;
    for (size_t i = 0; i < population.size(); ++i) {
        fitness_scores.push_back(get_smoothed_fitness(i));
    }
    
    // Find second best
    int best2 = 0;
    for (size_t i = 0; i < population.size(); ++i) {
        if ((int)i != best1 && fitness_scores[i] > fitness_scores[best2]) {
            best2 = i;
        }
    }
    
    // Create new generation
    std::vector<evolution::Genome> new_population;
    new_population.push_back(population[best1]);  // Elitism: keep best
    
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Generate rest of population through crossover and mutation
    while (new_population.size() < population_size) {
        evolution::Genome child = crossover(population[best1], population[best2]);
        
        // Find correlated genes for selective mutation
        auto correlated = find_correlated_genes(population[best1], fitness_scores);
        selective_mutation(child, correlated);
        
        new_population.push_back(child);
    }
    
    population = new_population;
    
    // Reset fitness history for new generation
    fitness_history.clear();
    for (size_t i = 0; i < population.size(); ++i) {
        fitness_history.push_back(std::vector<float>());
    }
}

evolution::Genome& EvolutionaryFeedback::get_current_genome() {
    if (population.empty()) {
        initialize_population();
    }
    return population[select_best_genome()];
}

// ============================================================================
// FeedbackCoordinator Implementation
// ============================================================================

FeedbackCoordinator::FeedbackCoordinator(fields::UnifiedActivationField& field)
    : field_(field),
      last_sensory_update_(std::chrono::high_resolution_clock::now()),
      last_cognitive_update_(std::chrono::high_resolution_clock::now()),
      last_evolutionary_update_(std::chrono::high_resolution_clock::now()) {
    
    evolutionary_.initialize_population();
}

void FeedbackCoordinator::process_feedback(float dt) {
    auto now = std::chrono::high_resolution_clock::now();
    
    // Channel 1: Sensory Feedback (high frequency - 30 Hz)
    float sensory_dt = std::chrono::duration<float>(now - last_sensory_update_).count();
    if (sensory_dt >= 1.0f / sensory_update_rate_) {
        sensory_.process_sensory_loop(field_);
        last_sensory_update_ = now;
    }
    
    // Channel 2: Cognitive Feedback (medium frequency - 10 Hz)
    float cognitive_dt = std::chrono::duration<float>(now - last_cognitive_update_).count();
    if (cognitive_dt >= 1.0f / cognitive_update_rate_) {
        cognitive_.update_attention_bias();
        cognitive_.adjust_edge_weights(field_);
        last_cognitive_update_ = now;
    }
    
    // Channel 3: Evolutionary Feedback (low frequency - every 100 cycles)
    cycle_count_++;
    if (cycle_count_ % 100 == 0) {
        evolutionary_.evolution_step(cycle_count_);
        last_evolutionary_update_ = now;
    }
}

FeedbackCoordinator::Stats FeedbackCoordinator::get_stats() const {
    Stats stats;
    
    // Compute avg prediction error
    if (!cognitive_.error_history.empty()) {
        float sum = 0.0f;
        for (const auto& err : cognitive_.error_history) {
            sum += err.error_magnitude;
        }
        stats.avg_prediction_error = sum / cognitive_.error_history.size();
    } else {
        stats.avg_prediction_error = 0.0f;
    }
    
    // Compute avg sensory reward
    if (!sensory_.recent_events.empty()) {
        float sum = 0.0f;
        for (const auto& event : sensory_.recent_events) {
            sum += event.reward;
        }
        stats.avg_sensory_reward = sum / sensory_.recent_events.size();
    } else {
        stats.avg_sensory_reward = 0.0f;
    }
    
    // Current fitness
    int best_idx = evolutionary_.select_best_genome();
    stats.current_fitness = evolutionary_.get_smoothed_fitness(best_idx);
    
    // Evolution generation
    stats.evolution_generation = evolutionary_.population[best_idx].get_generation();
    
    // Attention stability
    float bias_variance = 0.0f;
    if (!cognitive_.attention_bias.empty()) {
        float mean = 0.0f;
        for (const auto& [node, bias] : cognitive_.attention_bias) {
            mean += bias;
        }
        mean /= cognitive_.attention_bias.size();
        
        for (const auto& [node, bias] : cognitive_.attention_bias) {
            float diff = bias - mean;
            bias_variance += diff * diff;
        }
        bias_variance /= cognitive_.attention_bias.size();
    }
    stats.attention_stability = 1.0f / (1.0f + bias_variance);
    
    return stats;
}

} // namespace feedback
} // namespace melvin

