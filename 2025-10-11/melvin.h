#pragma once

#include "melvin_types.h"
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

// C API for Melvin
typedef struct melvin_t melvin_t;

// Core functions
melvin_t* melvin_create(const char* store_dir);
void melvin_destroy(melvin_t* melvin);

// Reasoning and learning
const char* melvin_reason(melvin_t* melvin, const char* query);
void melvin_learn(melvin_t* melvin, const char* text);
void melvin_decay_pass(melvin_t* melvin);

// Nightly consolidation
void melvin_run_nightly_consolidation(melvin_t* melvin);

// Statistics
size_t melvin_node_count(melvin_t* melvin);
size_t melvin_edge_count(melvin_t* melvin);
size_t melvin_path_count(melvin_t* melvin);

// Health monitoring
float melvin_get_health_score(melvin_t* melvin);
void melvin_export_metrics(melvin_t* melvin, const char* filename);

// Neural reasoning configuration
void melvin_set_neural_reasoning(melvin_t* melvin, int enabled);
void melvin_set_neural_confidence_threshold(melvin_t* melvin, float threshold);
int melvin_is_neural_reasoning_enabled(melvin_t* melvin);
float melvin_get_neural_confidence_threshold(melvin_t* melvin);

// Graph reasoning configuration
void melvin_set_graph_reasoning(melvin_t* melvin, int enabled);
int melvin_is_graph_reasoning_enabled(melvin_t* melvin);

// Test functions
void melvin_setup_test_data(melvin_t* melvin);

// Debug functions
void melvin_debug_node(melvin_t* melvin, const char* node_text);
void melvin_debug_node_with_context(melvin_t* melvin, const char* node_text, const char* context, const char* query, const char* driver_mode);

// Enhanced reasoning functions
void melvin_set_driver_mode(melvin_t* melvin, const char* mode);
const char* melvin_suggest_next_word(melvin_t* melvin, const char* current_word, const char* context, const char* query);

// Phrase generation functions
const char* melvin_generate_phrase_c(melvin_t* melvin, const char* start_word, int max_tokens, const char* driver_mode, const char* selection_mode, const char* context, const char* query, int debug_enabled);
const char* melvin_generate_answer(melvin_t* melvin, const char* query, const char* driver_mode);

// Enhanced beam search functions
const char* melvin_generate_phrase_beam_c(melvin_t* melvin, const char* start_word, int max_tokens, int beam_k, int expand_M, const char* driver_mode, int probabilistic, float stop_threshold, const char* context, const char* query, int debug_enabled);
void melvin_trace_generation(melvin_t* melvin, const char* seed_word, const char* query, const char* driver_mode);

// ==================== EMERGENT DIMENSIONAL REASONING API ====================
// 
// These functions implement the emergent dimensional reasoning system with:
//   • Context field diffusion over entire memory graph (unlimited context)
//   • Emergent dimensions discovered through experience (no hardcoded drivers)
//   • Variance impact tracking and attribution to active nodes
//   • Dynamic dimensional promotion, demotion, and compression
//   • ShadowWorld rollout simulation for predictive futures
//   • Experience-driven action selection with soft penalties
//   • Meta-learning that adapts all hyperparameters
//   • Continuous evolution where concepts earn influence through success

// Main reasoning loop with emergent dimensional system
// Parameters:
//   query: Input query string
//   driver_mode: "balanced" (legacy parameter, system now self-adapts)
//   debug_enabled: 1 to enable detailed debug output, 0 for silent operation
// Returns: Generated response string
const char* melvin_reason_with_unlimited_context(melvin_t* melvin, const char* query, const char* driver_mode, int debug_enabled);

// Initialize the unlimited context reasoning system
// Must be called before using reasoning functions (automatically called if needed)
void melvin_init_reasoning_system(melvin_t* melvin);

// Set evolution meta-parameters
// Parameters:
//   param_name: One of "alpha", "beta", "lambda", "tau", "risk_floor", 
//               "rollout_horizon", "rollout_branches"
//   value: New parameter value
void melvin_set_evolution_param(melvin_t* melvin, const char* param_name, float value);

// Get context field statistics
// Parameters:
//   mean_potential: Output for mean activation potential (can be NULL)
//   max_potential: Output for maximum activation potential (can be NULL)
//   num_nodes: Output for number of nodes in context field (can be NULL)
void melvin_get_context_field_stats(melvin_t* melvin, float* mean_potential, float* max_potential, int* num_nodes);

#ifdef __cplusplus
}
#endif

// C++ API for Evolutionary Brain Simulation
#ifdef __cplusplus
#include <string>
#include <memory>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <variant>

namespace melvin {

// Forward declarations
class Genome;
class EvolutionaryBrain;
class DataStreamGenerator;
class ProbeSystem;
class EvolutionOrchestrator;
class BrainSimulationRunner;
class Melvin;
class NeuralGraphReasoningSystem;

// Meta-evolution system types
enum class ParamType : uint8_t { Float, Int, Bool, Derived };

struct ParamValue {
    std::variant<float, int32_t, bool> v{};
};

struct ParamSpec {
    std::string name;
    ParamType type{ParamType::Float};
    std::string category;
    double min_val{0.0}, max_val{1.0};
    double default_val{0.0};
    uint64_t created_gen{0};
    uint64_t protect_until_gen{0};
    double contrib_ema{0.0};
    bool active{true};
    bool derived{false};
    
    // Anchor system fields
    bool anchor{false};
    double importance{0.0};        // from contrib_ema or manual
    double max_drift_per_gen{0.05};
    std::string expr;
    std::vector<std::string> deps;
    uint64_t innovation_id{0};
};

struct DynamicGenome {
    std::string id;
    uint64_t seed{0};
    std::unordered_map<std::string, ParamValue> values;
    std::unordered_map<std::string, ParamSpec> specs;
    
    // Anchor system fields
    std::unordered_map<std::string, ParamValue> overlay; // session deltas
    std::unique_ptr<DynamicGenome> anchor_snapshot;      // baseline
    
    struct Meta {
        double creation_prob{0.01};
        double deletion_prob{0.004};
        double creation_mutation_rate{0.10};
        double combination_rate{0.30};
        uint32_t protect_gens{6};
        uint32_t ablation_budget{24};
        double complexity_alpha{0.001};
    } meta;
    
    // Biochemical extensions
    struct ParamBio {
        double half_life_s{0.0};     // 0 => structural (no decay)
        double decay_target{0.0};    // value we decay toward (often default_val)
        bool   ephemeral{false};     // true => auto-prune when near decay_target
    };
    std::unordered_map<std::string, ParamBio> bio;  // per-param biology tags

    struct ModulatorState {
        double level{0.0};           // current concentration (arbitrary units)
        double half_life_s{1.0};
    };
    std::unordered_map<std::string, ModulatorState> modulators; // e.g. "dopamine","ach"
    
    bool has(const std::string& k) const;
    template<class T> T get(const std::string& k, T fallback) const {
        auto it = values.find(k);
        if (it == values.end()) return fallback;
        if constexpr (std::is_same_v<T, float>) {
            if (auto p = std::get_if<float>(&it->second.v)) return *p;
        } else if constexpr (std::is_same_v<T, int32_t>) {
            if (auto p = std::get_if<int32_t>(&it->second.v)) return *p;
        } else if constexpr (std::is_same_v<T, bool>) {
            if (auto p = std::get_if<bool>(&it->second.v)) return *p;
        }
        return fallback;
    }
    
    struct LegacyGenome {
        std::string id;
        uint64_t seed;
        struct Ingest { float temporal_weight_inc = 1.0f; float leap_init = 1.0f; float leap_create_prob = 0.06f; } ingest;
        struct Reason { uint32_t beam_width = 8; uint32_t max_hops = 6; float leap_bias = 0.15f; float abstraction_thresh = 0.78f; } reason;
        struct Storage { bool mmap = true; uint32_t flush_interval = 2048; uint32_t index_stride = 256; } storage;
        struct Sensors { float audio_cluster_thresh = 0.62f; float image_sim_thresh = 0.78f; } sensors;
    };
    
    LegacyGenome to_legacy() const;
    std::string to_json() const;
    void from_json(const std::string& json_str);
    DynamicGenome mutate(float mutation_rate = 0.1f) const;
    DynamicGenome crossover(const DynamicGenome& other) const;
    float distance(const DynamicGenome& other) const;
};

struct InnovationLedger {
    uint64_t next_id{1};
    std::unordered_map<std::string, uint64_t> sig2id;
    uint64_t assign(const std::string& family_sig);
};

struct VarTemplate {
    std::string family;
    ParamType type{ParamType::Float};
    std::string category;
    double lo{0.0}, hi{1.0};
    double def{0.0};
};

class MetaEvolutionSystem {
public:
    MetaEvolutionSystem(InnovationLedger& ledger, uint64_t seed = 0);
    std::string create_from_template(DynamicGenome& g, const VarTemplate& T, uint64_t gen, uint32_t protect);
    std::string create_by_mutation(DynamicGenome& g, const std::string& parent, uint64_t gen, uint32_t protect);
    std::string create_by_combination(DynamicGenome& g, const std::string& a, const std::string& b, uint64_t gen, uint32_t protect);
    VarTemplate pick_template() const;
    std::string pick_param(const DynamicGenome& g) const;
    std::pair<std::string, std::string> pick_param_pair(const DynamicGenome& g) const;
};

struct AttributionSample { 
    std::string name; 
    double delta_fitness; 
};

class FitnessAttribution {
public:
    FitnessAttribution(uint64_t seed = 0);
    std::vector<AttributionSample> estimate_contributions(EvolutionaryBrain& brain, DynamicGenome& g, uint64_t gen);
};

struct RuntimeConfig {
    float leap_bias;
    int beam_width;
    float abstraction_thresh;
    float temporal_weight_inc;
    float leap_init;
    float leap_create_prob;
    bool mmap;
    int flush_interval;
    int index_stride;
    float audio_cluster_thresh;
    float image_sim_thresh;
    
    // Optional: record current modulator levels for instrumentation
    double mod_dopamine{0.0}, mod_ne{0.0}, mod_ach{0.0}, mod_serotonin{0.0};
};

class ParameterAggregators {
public:
    static void fold_leap_bias(RuntimeConfig& rc, const DynamicGenome& g);
    static void fold_beam_width(RuntimeConfig& rc, const DynamicGenome& g);
    static void apply(RuntimeConfig& rc, const DynamicGenome& g);
};

inline double adjusted_fitness(const DynamicGenome& g, double base_fitness) {
    size_t active = 0; 
    for (auto& [_, sp] : g.specs) if (sp.active) ++active;
    return base_fitness - g.meta.complexity_alpha * double(active);
}
DynamicGenome crossover(const DynamicGenome& A, const DynamicGenome& B);
void prune_params(DynamicGenome& g, uint64_t gen);
void meta_evo_tick(EvolutionaryBrain& brain, DynamicGenome& g, InnovationLedger& ledger, MetaEvolutionSystem& meta_sys, FitnessAttribution& attribution, uint64_t gen);
void mutate_meta(DynamicGenome& g);

// Forward declarations for external API - definitions moved to API layer

// Main simulation runner class
class __attribute__((visibility("default"))) BrainSimulationRunner {
public:
    BrainSimulationRunner(const std::string& output_dir, uint64_t master_seed = 1234567890);
    ~BrainSimulationRunner();
    
    void start_simulation();
    void stop_simulation();
    void run_single_generation();
    void export_final_results();
    
private:
    std::string output_dir_;
    std::unique_ptr<EvolutionOrchestrator> orchestrator_;
    bool running_ = false;
};

} // namespace melvin

// Main Melvin class declaration (outside namespace for C compatibility)
// Forward declaration - full definition in melvin.cpp
class Melvin;

#endif
