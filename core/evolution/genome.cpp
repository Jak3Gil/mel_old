#include "genome.h"
#include <algorithm>
#include <cmath>

namespace melvin {
namespace evolution {

Genome::Genome() : current_generation_(0), rng_(std::random_device{}()) {
    initialize_default_genome();
}

void Genome::initialize_default_genome() {
    // Energy system genes
    genes_["base_input_energy"] = {"base_input_energy", 10.0f, 1.0f, 100.0f, 0.05f, 2.0f, false, 0.0f, 0};
    genes_["energy_decay_rate"] = {"energy_decay_rate", 0.9f, 0.5f, 0.999f, 0.05f, 0.05f, true, 0.0f, 0};
    genes_["energy_spread_rate"] = {"energy_spread_rate", 0.3f, 0.01f, 0.9f, 0.05f, 0.1f, false, 0.0f, 0};
    genes_["min_activation_threshold"] = {"min_activation_threshold", 0.01f, 0.001f, 0.5f, 0.03f, 0.01f, true, 0.0f, 0};
    genes_["novelty_bonus_multiplier"] = {"novelty_bonus_multiplier", 2.0f, 1.0f, 10.0f, 0.08f, 0.5f, false, 0.0f, 0};
    
    // Learning system genes
    genes_["base_learning_rate"] = {"base_learning_rate", 0.1f, 0.001f, 0.5f, 0.05f, 0.02f, true, 0.0f, 0};
    genes_["exploration_learning_rate"] = {"exploration_learning_rate", 0.3f, 0.05f, 0.8f, 0.05f, 0.05f, false, 0.0f, 0};
    genes_["exploitation_learning_rate"] = {"exploitation_learning_rate", 0.03f, 0.001f, 0.2f, 0.05f, 0.01f, false, 0.0f, 0};
    genes_["eligibility_trace_decay"] = {"eligibility_trace_decay", 0.95f, 0.5f, 0.999f, 0.05f, 0.02f, true, 0.0f, 0};
    
    // Attention system genes
    genes_["base_attention_weight"] = {"base_attention_weight", 1.0f, 0.1f, 5.0f, 0.05f, 0.2f, false, 0.0f, 0};
    genes_["goal_relevance_weight"] = {"goal_relevance_weight", 1.0f, 0.0f, 5.0f, 0.05f, 0.3f, false, 0.0f, 0};
    genes_["surprise_bonus_weight"] = {"surprise_bonus_weight", 1.0f, 0.0f, 3.0f, 0.05f, 0.2f, false, 0.0f, 0};
    
    // Prediction system genes
    genes_["prediction_confidence_threshold"] = {"prediction_confidence_threshold", 0.5f, 0.1f, 0.95f, 0.05f, 0.1f, false, 0.0f, 0};
    genes_["prediction_temperature"] = {"prediction_temperature", 1.0f, 0.1f, 5.0f, 0.05f, 0.3f, false, 0.0f, 0};
    
    // Consolidation system genes
    genes_["consolidation_replay_strength"] = {"consolidation_replay_strength", 0.05f, 0.001f, 0.5f, 0.05f, 0.02f, false, 0.0f, 0};
    genes_["consolidation_pruning_threshold"] = {"consolidation_pruning_threshold", 0.1f, 0.01f, 0.5f, 0.05f, 0.05f, true, 0.0f, 0};
    genes_["consolidation_merge_threshold"] = {"consolidation_merge_threshold", 0.85f, 0.5f, 0.99f, 0.05f, 0.05f, false, 0.0f, 0};
    
    // Meta-learning system genes
    genes_["meta_success_threshold_explore"] = {"meta_success_threshold_explore", 0.3f, 0.1f, 0.5f, 0.05f, 0.05f, false, 0.0f, 0};
    genes_["meta_success_threshold_exploit"] = {"meta_success_threshold_exploit", 0.7f, 0.5f, 0.95f, 0.05f, 0.05f, false, 0.0f, 0};
    
    // Temporal reasoning genes
    genes_["temporal_window_ms"] = {"temporal_window_ms", 200.0f, 50.0f, 1000.0f, 0.05f, 50.0f, false, 0.0f, 0};
    
    // Edge dynamics genes
    genes_["edge_strengthening_rate"] = {"edge_strengthening_rate", 0.05f, 0.001f, 0.5f, 0.05f, 0.02f, false, 0.0f, 0};
    genes_["edge_weakening_rate"] = {"edge_weakening_rate", 0.1f, 0.001f, 0.5f, 0.05f, 0.05f, false, 0.0f, 0};
    
    // Output generation genes (COGNITIVE SYSTEM)
    genes_["output_length_min"] = {"output_length_min", 20.0f, 5.0f, 50.0f, 0.1f, 5.0f, false, 0.0f, 0};
    genes_["output_length_max"] = {"output_length_max", 100.0f, 20.0f, 500.0f, 0.1f, 20.0f, false, 0.0f, 0};
    genes_["output_length_variance"] = {"output_length_variance", 30.0f, 0.0f, 100.0f, 0.1f, 10.0f, false, 0.0f, 0};
    genes_["quality_threshold"] = {"quality_threshold", 0.3f, 0.1f, 0.7f, 0.1f, 0.05f, false, 0.0f, 0};
    genes_["boredom_threshold"] = {"boredom_threshold", 3.0f, 1.0f, 10.0f, 0.1f, 1.0f, false, 0.0f, 0};
    genes_["exploration_rate"] = {"exploration_rate", 0.3f, 0.05f, 0.8f, 0.1f, 0.05f, false, 0.0f, 0};
    genes_["goal_duration_min"] = {"goal_duration_min", 5.0f, 1.0f, 20.0f, 0.1f, 2.0f, false, 0.0f, 0};
    genes_["goal_duration_max"] = {"goal_duration_max", 15.0f, 5.0f, 50.0f, 0.1f, 5.0f, false, 0.0f, 0};
    genes_["consolidation_interval"] = {"consolidation_interval", 5.0f, 1.0f, 20.0f, 0.1f, 2.0f, false, 0.0f, 0};
    genes_["temperature_min"] = {"temperature_min", 0.6f, 0.1f, 1.5f, 0.1f, 0.1f, false, 0.0f, 0};
    genes_["temperature_max"] = {"temperature_max", 1.4f, 0.5f, 3.0f, 0.1f, 0.2f, false, 0.0f, 0};
    
    // Mode Control System genes (Autonomous Mode Selection)
    genes_["mode_confidence_threshold"] = {"mode_confidence_threshold", 0.7f, 0.5f, 0.95f, 0.1f, 0.05f, false, 0.0f, 0};
    genes_["mode_min_knowledge_for_action"] = {"mode_min_knowledge_for_action", 0.3f, 0.1f, 0.7f, 0.1f, 0.05f, false, 0.0f, 0};
    genes_["mode_energy_low_threshold"] = {"mode_energy_low_threshold", 5.0f, 1.0f, 10.0f, 0.1f, 0.5f, false, 0.0f, 0};
    genes_["mode_energy_high_threshold"] = {"mode_energy_high_threshold", 50.0f, 20.0f, 100.0f, 0.1f, 5.0f, false, 0.0f, 0};
    genes_["mode_consolidation_interval"] = {"mode_consolidation_interval", 300.0f, 60.0f, 600.0f, 0.1f, 30.0f, false, 0.0f, 0};
    genes_["mode_evolution_interval"] = {"mode_evolution_interval", 600.0f, 120.0f, 1200.0f, 0.1f, 60.0f, false, 0.0f, 0};
    genes_["mode_idle_timeout"] = {"mode_idle_timeout", 60.0f, 10.0f, 300.0f, 0.1f, 10.0f, false, 0.0f, 0};
    genes_["mode_min_nodes_for_action"] = {"mode_min_nodes_for_action", 1000.0f, 100.0f, 10000.0f, 0.1f, 500.0f, false, 0.0f, 0};
    genes_["mode_min_safe_distance"] = {"mode_min_safe_distance", 0.3f, 0.1f, 1.0f, 0.1f, 0.05f, false, 0.0f, 0};
    genes_["mode_max_error_rate"] = {"mode_max_error_rate", 0.7f, 0.3f, 0.9f, 0.1f, 0.05f, false, 0.0f, 0};
    genes_["mode_exploration_threshold"] = {"mode_exploration_threshold", 0.4f, 0.2f, 0.8f, 0.1f, 0.05f, false, 0.0f, 0};
    genes_["mode_exploration_confidence"] = {"mode_exploration_confidence", 0.5f, 0.3f, 0.8f, 0.1f, 0.05f, false, 0.0f, 0};
    genes_["mode_min_success_rate"] = {"mode_min_success_rate", 0.3f, 0.1f, 0.7f, 0.1f, 0.05f, false, 0.0f, 0};
    
    std::cout << "🧬 Genome initialized with " << genes_.size() << " genes" << std::endl;
}

float Genome::get(const std::string& name) const {
    auto it = genes_.find(name);
    if (it != genes_.end()) {
        return it->second.value;
    }
    std::cerr << "⚠️  Gene '" << name << "' not found! Using default 0.0" << std::endl;
    return 0.0f;
}

void Genome::set(const std::string& name, float value) {
    auto it = genes_.find(name);
    if (it != genes_.end()) {
        value = std::max(it->second.min_value, std::min(it->second.max_value, value));
        it->second.value = value;
    } else {
        std::cerr << "⚠️  Gene '" << name << "' not found! Cannot set." << std::endl;
    }
}

bool Genome::has(const std::string& name) const {
    return genes_.find(name) != genes_.end();
}

void Genome::mutate() {
    std::uniform_real_distribution<float> prob_dist(0.0f, 1.0f);
    int mutations = 0;
    
    for (auto& pair : genes_) {
        Gene& gene = pair.second;
        
        if (prob_dist(rng_) < gene.mutation_rate) {
            float magnitude = gene.mutation_magnitude;
            if (gene.is_critical) {
                magnitude *= 0.2f;
            }
            
            std::normal_distribution<float> mut_dist(0.0f, magnitude);
            float delta = mut_dist(rng_);
            float old_value = gene.value;
            gene.value += delta;
            gene.value = std::max(gene.min_value, std::min(gene.max_value, gene.value));
            
            if (std::abs(gene.value - old_value) > 0.001f) {
                mutations++;
            }
        }
    }
    
    if (mutations > 0) {
        std::cout << "🧬 Mutation: " << mutations << " genes mutated (generation " 
                  << current_generation_ << ")" << std::endl;
    }
    current_generation_++;
}

void Genome::evaluate_fitness(const std::unordered_map<std::string, float>& metrics) {
    float prediction_acc = metrics.count("prediction_accuracy") ? metrics.at("prediction_accuracy") : 0.0f;
    float energy_eff = metrics.count("energy_efficiency") ? metrics.at("energy_efficiency") : 0.0f;
    float learning_speed = metrics.count("learning_speed") ? metrics.at("learning_speed") : 0.0f;
    
    float overall_fitness = prediction_acc * 0.5f + energy_eff * 0.3f + learning_speed * 0.2f;
    
    for (auto& pair : genes_) {
        Gene& gene = pair.second;
        float mid_value = 0.5f * (gene.min_value + gene.max_value);
        float divergence = std::abs(gene.value - mid_value);
        float normalized_div = divergence / (gene.max_value - gene.min_value);
        gene.fitness_contribution = overall_fitness * normalized_div;
    }
}

void Genome::natural_selection() {
    std::vector<std::pair<std::string, Gene>> all_genes(genes_.begin(), genes_.end());
    std::sort(all_genes.begin(), all_genes.end(),
              [](const auto& a, const auto& b) {
                  return a.second.fitness_contribution > b.second.fitness_contribution;
              });
    
    int top_count = std::max(3, static_cast<int>(all_genes.size() * 0.1));
    if (all_genes.size() >= static_cast<size_t>(top_count)) {
        std::cout << "\n📊 Top " << top_count << " genes:\n";
        for (int i = 0; i < top_count; ++i) {
            std::cout << "  ✅ " << all_genes[i].first << " = " << all_genes[i].second.value << "\n";
        }
    }
}

void Genome::save(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "❌ Cannot save genome to " << filename << std::endl;
        return;
    }
    
    file.write(reinterpret_cast<const char*>(&current_generation_), sizeof(int));
    int gene_count = genes_.size();
    file.write(reinterpret_cast<const char*>(&gene_count), sizeof(int));
    
    for (const auto& pair : genes_) {
        const Gene& gene = pair.second;
        int name_len = gene.name.size();
        file.write(reinterpret_cast<const char*>(&name_len), sizeof(int));
        file.write(gene.name.c_str(), name_len);
        file.write(reinterpret_cast<const char*>(&gene.value), sizeof(float));
        file.write(reinterpret_cast<const char*>(&gene.min_value), sizeof(float));
        file.write(reinterpret_cast<const char*>(&gene.max_value), sizeof(float));
        file.write(reinterpret_cast<const char*>(&gene.mutation_rate), sizeof(float));
        file.write(reinterpret_cast<const char*>(&gene.mutation_magnitude), sizeof(float));
        file.write(reinterpret_cast<const char*>(&gene.is_critical), sizeof(bool));
        file.write(reinterpret_cast<const char*>(&gene.fitness_contribution), sizeof(float));
        file.write(reinterpret_cast<const char*>(&gene.generation_created), sizeof(int));
    }
    
    file.close();
    std::cout << "💾 Genome saved (generation " << current_generation_ << ")" << std::endl;
}

void Genome::load(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cout << "💾 No saved genome found, using defaults" << std::endl;
        return;
    }
    
    file.read(reinterpret_cast<char*>(&current_generation_), sizeof(int));
    int gene_count;
    file.read(reinterpret_cast<char*>(&gene_count), sizeof(int));
    
    for (int i = 0; i < gene_count; ++i) {
        Gene gene;
        int name_len;
        file.read(reinterpret_cast<char*>(&name_len), sizeof(int));
        gene.name.resize(name_len);
        file.read(&gene.name[0], name_len);
        file.read(reinterpret_cast<char*>(&gene.value), sizeof(float));
        file.read(reinterpret_cast<char*>(&gene.min_value), sizeof(float));
        file.read(reinterpret_cast<char*>(&gene.max_value), sizeof(float));
        file.read(reinterpret_cast<char*>(&gene.mutation_rate), sizeof(float));
        file.read(reinterpret_cast<char*>(&gene.mutation_magnitude), sizeof(float));
        file.read(reinterpret_cast<char*>(&gene.is_critical), sizeof(bool));
        file.read(reinterpret_cast<char*>(&gene.fitness_contribution), sizeof(float));
        file.read(reinterpret_cast<char*>(&gene.generation_created), sizeof(int));
        genes_[gene.name] = gene;
    }
    
    file.close();
    std::cout << "💾 Genome loaded (generation " << current_generation_ << ")" << std::endl;
}

std::vector<Gene> Genome::get_all_genes() const {
    std::vector<Gene> result;
    for (const auto& pair : genes_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<Gene> Genome::get_top_genes(int n) const {
    std::vector<std::pair<std::string, Gene>> all_genes(genes_.begin(), genes_.end());
    std::sort(all_genes.begin(), all_genes.end(),
              [](const auto& a, const auto& b) {
                  return a.second.fitness_contribution > b.second.fitness_contribution;
              });
    
    std::vector<Gene> result;
    for (int i = 0; i < n && i < static_cast<int>(all_genes.size()); ++i) {
        result.push_back(all_genes[i].second);
    }
    return result;
}

std::vector<Gene> Genome::get_worst_genes(int n) const {
    std::vector<std::pair<std::string, Gene>> all_genes(genes_.begin(), genes_.end());
    std::sort(all_genes.begin(), all_genes.end(),
              [](const auto& a, const auto& b) {
                  return a.second.fitness_contribution < b.second.fitness_contribution;
              });
    
    std::vector<Gene> result;
    for (int i = 0; i < n && i < static_cast<int>(all_genes.size()); ++i) {
        result.push_back(all_genes[i].second);
    }
    return result;
}

// ==============================================================================
// ADAPTIVE INTELLIGENCE: Meta-Learning Phase Control
// ==============================================================================

const char* Genome::get_phase_name() const {
    switch (current_phase_) {
        case AdaptationPhase::EXPLORE:  return "EXPLORE";
        case AdaptationPhase::REFINE:   return "REFINE";
        case AdaptationPhase::EXPLOIT:  return "EXPLOIT";
        default: return "UNKNOWN";
    }
}

void Genome::update_phase(float recent_success_rate, float recent_surprise_rate) {
    // Update tracking
    recent_success_rate_ = recent_success_rate;
    recent_surprise_rate_ = recent_surprise_rate;
    cycles_in_current_phase_++;
    
    // Phase transition logic
    AdaptationPhase new_phase = current_phase_;
    
    if (recent_success_rate < 0.3f || recent_surprise_rate > 0.5f) {
        // Struggling or encountering novelty → EXPLORE
        new_phase = AdaptationPhase::EXPLORE;
    } else if (recent_success_rate < 0.7f) {
        // Improving but not mastered → REFINE
        new_phase = AdaptationPhase::REFINE;
    } else {
        // Mastered → EXPLOIT
        new_phase = AdaptationPhase::EXPLOIT;
    }
    
    // Only transition if we've been in current phase for at least 10 cycles
    // (prevents rapid phase oscillation)
    if (new_phase != current_phase_ && cycles_in_current_phase_ >= 10) {
        std::cout << "🔄 Phase transition: " << get_phase_name() << " → ";
        current_phase_ = new_phase;
        cycles_in_current_phase_ = 0;
        std::cout << get_phase_name() << std::endl;
        
        // Apply phase-specific parameters
        apply_phase_parameters();
    }
}

void Genome::apply_phase_parameters() {
    // Adjust key learning parameters based on current phase
    switch (current_phase_) {
        case AdaptationPhase::EXPLORE:
            // High plasticity, high exploration
            set("base_learning_rate", 0.3f);
            set("exploration_rate", 0.5f);
            set("quality_threshold", 0.2f);  // Accept lower quality to learn more
            set("temperature_min", 0.8f);
            set("temperature_max", 1.6f);
            set("energy_spread_rate", 0.4f);  // Spread energy more
            break;
            
        case AdaptationPhase::REFINE:
            // Medium plasticity, balanced
            set("base_learning_rate", 0.1f);
            set("exploration_rate", 0.2f);
            set("quality_threshold", 0.3f);
            set("temperature_min", 0.6f);
            set("temperature_max", 1.4f);
            set("energy_spread_rate", 0.3f);
            break;
            
        case AdaptationPhase::EXPLOIT:
            // Low plasticity, consolidate knowledge
            set("base_learning_rate", 0.03f);
            set("exploration_rate", 0.05f);
            set("quality_threshold", 0.4f);  // More selective
            set("temperature_min", 0.4f);
            set("temperature_max", 1.0f);
            set("energy_spread_rate", 0.2f);  // More focused
            break;
    }
}

// ==============================================================================
// HUMAN-LEVEL ADAPTIVE INTELLIGENCE v4.0: Affective Neuromodulation
// ==============================================================================

void Genome::apply_affective_modulation(float success_rate, float stability, float novelty) {
    // Compute neuromodulator levels based on system state
    // These act like biological neurotransmitters that modulate plasticity
    
    // Dopamine: reward prediction error / success signal
    // High when succeeding, low when failing
    neuromodulators_.dopamine = std::max(-1.0f, std::min(1.0f, success_rate - 0.5f));
    
    // Serotonin: stability / confidence signal
    // High when stable and coherent, low when chaotic
    neuromodulators_.serotonin = std::max(-1.0f, std::min(1.0f, stability - 0.5f));
    
    // Noradrenaline: novelty / arousal signal
    // High when encountering new things
    neuromodulators_.noradrenaline = novelty;
    
    // Acetylcholine: attention / focus signal
    // High during prediction errors (uncertainty)
    neuromodulators_.acetylcholine = 1.0f - stability;
    
    // Apply neuromodulation to learning parameters
    // Dopamine boosts learning rate when successful
    float current_lr = get("base_learning_rate");
    float modulated_lr = current_lr * (1.0f + 0.5f * neuromodulators_.dopamine);
    set("base_learning_rate", std::max(0.001f, std::min(0.5f, modulated_lr)));
    
    // Noradrenaline boosts exploration
    float current_explore = get("exploration_rate");
    float modulated_explore = current_explore * (1.0f + 0.3f * neuromodulators_.noradrenaline);
    set("exploration_rate", std::max(0.05f, std::min(0.8f, modulated_explore)));
    
    // Serotonin affects quality threshold (more selective when confident)
    float current_threshold = get("quality_threshold");
    float modulated_threshold = current_threshold + 0.05f * (1.0f - neuromodulators_.serotonin);
    set("quality_threshold", std::max(0.1f, std::min(0.7f, modulated_threshold)));
    
    // Acetylcholine focuses attention (reduces temperature variance)
    float current_temp_max = get("temperature_max");
    float modulated_temp = current_temp_max * (1.0f - 0.2f * neuromodulators_.acetylcholine);
    set("temperature_max", std::max(0.5f, std::min(3.0f, modulated_temp)));
}

} // namespace evolution
} // namespace melvin

