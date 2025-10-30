#ifndef GENOME_H
#define GENOME_H

#include <unordered_map>
#include <string>
#include <vector>
#include <random>
#include <fstream>
#include <iostream>

namespace melvin {
namespace evolution {

// Every gene is a variable (parameter) in the system
struct Gene {
    std::string name;
    float value;
    float min_value;
    float max_value;
    float mutation_rate;
    float mutation_magnitude;
    bool is_critical;
    float fitness_contribution;
    int generation_created;
};

// ADAPTIVE INTELLIGENCE: Meta-learning phases
enum class AdaptationPhase {
    EXPLORE,     // High plasticity, high exploration, low confidence
    REFINE,      // Medium plasticity, balanced exploration
    EXPLOIT      // Low plasticity, low exploration, high confidence
};

// The complete genome: ALL variables that control MELVIN
class Genome {
public:
    Genome();
    
    // Variable access (used by ALL systems)
    float get(const std::string& name) const;
    void set(const std::string& name, float value);
    bool has(const std::string& name) const;
    
    // Evolution operations
    void mutate();
    void evaluate_fitness(const std::unordered_map<std::string, float>& metrics);
    void natural_selection();
    
    // Persistence
    void save(const std::string& filename) const;
    void load(const std::string& filename);
    
    // Inspection
    std::vector<Gene> get_all_genes() const;
    std::vector<Gene> get_top_genes(int n) const;
    std::vector<Gene> get_worst_genes(int n) const;
    int get_generation() const { return current_generation_; }
    
    // ADAPTIVE INTELLIGENCE: Meta-learning phase control
    AdaptationPhase get_phase() const { return current_phase_; }
    const char* get_phase_name() const;
    void update_phase(float recent_success_rate, float recent_surprise_rate);
    void apply_phase_parameters();  // Adjust learning rates, exploration based on phase
    
    // HUMAN-LEVEL v4.0: Affective Neuromodulation
    struct NeuromodulatorLevels {
        float dopamine = 0.0f;         // Reward/success signal
        float serotonin = 0.0f;        // Stability/confidence signal
        float noradrenaline = 0.0f;    // Novelty/arousal signal
        float acetylcholine = 0.0f;    // Attention/focus signal
    };
    
    NeuromodulatorLevels& get_neuromodulators() { return neuromodulators_; }
    const NeuromodulatorLevels& get_neuromodulators() const { return neuromodulators_; }
    
    // Apply neuromodulation to parameters
    void apply_affective_modulation(float success_rate, float stability, float novelty);
    
private:
    std::unordered_map<std::string, Gene> genes_;
    int current_generation_;
    std::mt19937 rng_;
    
    // ADAPTIVE INTELLIGENCE: Phase tracking
    AdaptationPhase current_phase_ = AdaptationPhase::EXPLORE;
    float recent_success_rate_ = 0.5f;
    float recent_surprise_rate_ = 0.5f;
    int cycles_in_current_phase_ = 0;
    
    // HUMAN-LEVEL v4.0: Neuromodulator levels
    NeuromodulatorLevels neuromodulators_;
    
    void initialize_default_genome();
};

} // namespace evolution
} // namespace melvin

#endif // GENOME_H

