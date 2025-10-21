#include "genome.h"
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

namespace melvin::v2::evolution {

// ============================================================================
// STATIC MEMBERS
// ============================================================================

uint64_t GenomeFactory::next_genome_id_ = 1000;

// ============================================================================
// GENOME IMPLEMENTATION
// ============================================================================

Genome::Genome() 
    : id(generate_genome_id()), parent_id(0), seed(12345), generation(0) {}

Genome::Genome(GenomeID id_, uint64_t seed_)
    : id(id_), parent_id(0), seed(seed_), generation(0) {}

// ========================================================================
// ATTENTION GENES
// ========================================================================

void Genome::add_attention_genes() {
    GeneModule attention("attention", 0.15f);
    
    // Attention formula weights: F = α·S + β·R + γ·C + δ·N + ε·P
    attention.add_gene(make_gene("alpha_saliency", 0.40f, 0.0f, 1.0f, 0.05f));
    attention.add_gene(make_gene("beta_goal", 0.30f, 0.0f, 1.0f, 0.05f));
    attention.add_gene(make_gene("gamma_curiosity", 0.20f, 0.0f, 1.0f, 0.05f));
    attention.add_gene(make_gene("delta_need", 0.05f, 0.0f, 1.0f, 0.02f));
    attention.add_gene(make_gene("epsilon_persistence", 0.05f, 0.0f, 1.0f, 0.02f));
    
    // Attention dynamics
    attention.add_gene(make_gene("inertia_threshold", 1.15f, 1.0f, 2.0f, 0.05f));
    attention.add_gene(make_gene("inhibition_decay", 0.95f, 0.8f, 1.0f, 0.02f));
    attention.add_gene(make_gene("softmax_temperature", 0.2f, 0.05f, 1.0f, 0.05f));
    attention.add_gene(make_gene("min_focus_duration", 5.0f, 1.0f, 30.0f, 1.0f));
    attention.add_gene(make_gene("max_focus_duration", 100.0f, 10.0f, 300.0f, 10.0f));
    
    modules.push_back(attention);
}

// ========================================================================
// PREDICTIVE CODING GENES
// ========================================================================

void Genome::add_predictive_genes() {
    GeneModule predictive("predictive", 0.12f);
    
    // Error weighting
    predictive.add_gene(make_gene("lambda_error_weight", 0.5f, 0.0f, 1.0f, 0.05f));
    predictive.add_gene(make_gene("eta_update_rate", 0.01f, 0.001f, 0.1f, 0.005f, true));
    predictive.add_gene(make_gene("feedback_depth", 3.0f, 1.0f, 10.0f, 0.5f));
    
    // Prediction horizon
    predictive.add_gene(make_gene("prediction_steps", 5.0f, 1.0f, 20.0f, 1.0f));
    predictive.add_gene(make_gene("surprise_threshold", 0.3f, 0.1f, 1.0f, 0.05f));
    
    modules.push_back(predictive);
}

// ========================================================================
// WORKING MEMORY GENES
// ========================================================================

void Genome::add_wm_genes() {
    GeneModule wm("working_memory", 0.10f);
    
    // Capacity and dynamics
    wm.add_gene(make_gene("slot_count", 7.0f, 3.0f, 12.0f, 0.5f));  // Classic 7±2
    wm.add_gene(make_gene("decay_tau", 5.0f, 1.0f, 20.0f, 1.0f));
    wm.add_gene(make_gene("gating_threshold", 0.5f, 0.1f, 0.9f, 0.05f));
    wm.add_gene(make_gene("precision_decay", 0.95f, 0.8f, 1.0f, 0.02f));
    
    // Refresh and maintenance
    wm.add_gene(make_gene("refresh_rate", 0.2f, 0.05f, 0.5f, 0.05f));
    wm.add_gene(make_gene("binding_strength", 0.7f, 0.3f, 1.0f, 0.05f));
    
    modules.push_back(wm);
}

// ========================================================================
// NEUROMODULATOR GENES
// ========================================================================

void Genome::add_neuromod_genes() {
    GeneModule neuromod("neuromodulators", 0.20f);
    
    // Baseline levels
    neuromod.add_gene(make_gene("da_baseline", 0.5f, 0.0f, 1.0f, 0.05f));
    neuromod.add_gene(make_gene("ne_baseline", 0.5f, 0.0f, 1.0f, 0.05f));
    neuromod.add_gene(make_gene("ach_baseline", 0.5f, 0.0f, 1.0f, 0.05f));
    neuromod.add_gene(make_gene("serotonin_baseline", 0.5f, 0.0f, 1.0f, 0.05f));
    
    // Gain parameters (sensitivity to events)
    neuromod.add_gene(make_gene("da_gain", 0.5f, 0.1f, 2.0f, 0.1f));
    neuromod.add_gene(make_gene("ne_gain", 0.5f, 0.1f, 2.0f, 0.1f));
    neuromod.add_gene(make_gene("ach_gain", 0.5f, 0.1f, 2.0f, 0.1f));
    neuromod.add_gene(make_gene("serotonin_gain", 0.5f, 0.1f, 2.0f, 0.1f));
    
    // Decay rates (homeostasis)
    neuromod.add_gene(make_gene("da_decay", 0.95f, 0.8f, 1.0f, 0.02f));
    neuromod.add_gene(make_gene("ne_decay", 0.90f, 0.8f, 1.0f, 0.02f));
    neuromod.add_gene(make_gene("ach_decay", 0.92f, 0.8f, 1.0f, 0.02f));
    neuromod.add_gene(make_gene("serotonin_decay", 0.98f, 0.9f, 1.0f, 0.01f));
    
    modules.push_back(neuromod);
}

// ========================================================================
// CONSOLIDATION GENES
// ========================================================================

void Genome::add_consolidation_genes() {
    GeneModule consolidation("consolidation", 0.08f);
    
    // Replay parameters
    consolidation.add_gene(make_gene("replay_rate", 0.1f, 0.01f, 1.0f, 0.05f));
    consolidation.add_gene(make_gene("sleep_interval", 1000.0f, 100.0f, 5000.0f, 100.0f));
    consolidation.add_gene(make_gene("synaptic_scaling", 0.99f, 0.95f, 1.0f, 0.01f));
    
    // Episodic → Semantic transfer
    consolidation.add_gene(make_gene("semantic_threshold", 3.0f, 1.0f, 10.0f, 0.5f));
    consolidation.add_gene(make_gene("consolidation_strength", 0.5f, 0.1f, 1.0f, 0.05f));
    
    modules.push_back(consolidation);
}

// ========================================================================
// HOPFIELD GENES
// ========================================================================

void Genome::add_hopfield_genes() {
    GeneModule hopfield("hopfield", 0.10f);
    
    // Attractor dynamics
    hopfield.add_gene(make_gene("capacity_scale", 0.15f, 0.05f, 0.3f, 0.02f));
    hopfield.add_gene(make_gene("temperature", 5.0f, 1.0f, 20.0f, 1.0f));
    hopfield.add_gene(make_gene("max_steps", 20.0f, 5.0f, 50.0f, 2.0f));
    hopfield.add_gene(make_gene("convergence_threshold", 0.01f, 0.001f, 0.1f, 0.01f));
    hopfield.add_gene(make_gene("normalize_embeddings", 1.0f, 0.0f, 1.0f, 0.0f));  // Boolean
    
    modules.push_back(hopfield);
}

// ========================================================================
// LEARNING GENES
// ========================================================================

void Genome::add_learning_genes() {
    GeneModule learning("learning", 0.15f);
    
    // Hebbian learning
    learning.add_gene(make_gene("hebbian_rate", 0.001f, 0.0001f, 0.01f, 0.001f, true));
    learning.add_gene(make_gene("anti_hebbian_rate", 0.0005f, 0.0f, 0.005f, 0.0005f, true));
    
    // LEAP formation
    learning.add_gene(make_gene("leap_threshold", 0.7f, 0.3f, 0.9f, 0.05f));
    learning.add_gene(make_gene("leap_formation_prob", 0.1f, 0.01f, 0.5f, 0.05f));
    
    // Edge decay
    learning.add_gene(make_gene("edge_decay_rate", 0.999f, 0.99f, 1.0f, 0.001f));
    learning.add_gene(make_gene("noise_floor", 0.001f, 0.0001f, 0.01f, 0.001f));
    
    modules.push_back(learning);
}

// ========================================================================
// INITIALIZATION
// ========================================================================

// ========================================================================
// VISION GENES (Camera Evolution!)
// ========================================================================

void Genome::add_vision_genes() {
    GeneModule vision("vision", 0.18f);
    
    // Edge detection (V1 cortex analog)
    vision.add_gene(make_gene("edge_threshold", 0.30f, 0.1f, 0.9f, 0.05f));
    vision.add_gene(make_gene("edge_weight", 0.30f, 0.0f, 1.0f, 0.05f));
    
    // Motion detection (MT cortex analog)
    vision.add_gene(make_gene("motion_sensitivity", 0.50f, 0.1f, 2.0f, 0.1f));
    vision.add_gene(make_gene("motion_weight", 0.40f, 0.0f, 1.0f, 0.05f));
    
    // Color processing (V4 cortex analog)
    vision.add_gene(make_gene("color_variance_threshold", 0.20f, 0.05f, 0.8f, 0.05f));
    vision.add_gene(make_gene("color_weight", 0.30f, 0.0f, 1.0f, 0.05f));
    
    // Spatial processing
    vision.add_gene(make_gene("patch_size", 32.0f, 16.0f, 64.0f, 4.0f));
    vision.add_gene(make_gene("min_object_size", 100.0f, 25.0f, 500.0f, 25.0f));
    vision.add_gene(make_gene("grouping_threshold", 0.50f, 0.1f, 0.9f, 0.05f));
    
    // Novelty detection
    vision.add_gene(make_gene("novelty_threshold", 0.30f, 0.1f, 0.8f, 0.05f));
    
    modules.push_back(vision);
}

void Genome::initialize_base_genome() {
    modules.clear();
    
    add_attention_genes();
    add_predictive_genes();
    add_wm_genes();
    add_neuromod_genes();
    add_consolidation_genes();
    add_hopfield_genes();
    add_learning_genes();
    add_vision_genes();  // ← NEW! Camera evolution genes
    
    description = "Base genome with default parameters + vision";
}

// ========================================================================
// GENE ACCESS
// ========================================================================

GeneModule* Genome::get_module(const std::string& name) {
    for (auto& module : modules) {
        if (module.name == name) return &module;
    }
    return nullptr;
}

const GeneModule* Genome::get_module(const std::string& name) const {
    for (const auto& module : modules) {
        if (module.name == name) return &module;
    }
    return nullptr;
}

float Genome::get_gene_value(const std::string& module, const std::string& key) const {
    auto mod = get_module(module);
    if (!mod) return 0.0f;
    
    auto gene = mod->get_gene(key);
    if (!gene) return 0.0f;
    
    return gene->value;
}

bool Genome::set_gene_value(const std::string& module, const std::string& key, float value) {
    auto mod = get_module(module);
    if (!mod) return false;
    
    auto gene = mod->get_gene(key);
    if (!gene) return false;
    
    gene->value = value;
    gene->clamp();
    return true;
}

// Quick accessors
float Genome::get_alpha_saliency() const {
    return get_gene_value("attention", "alpha_saliency");
}

float Genome::get_beta_goal() const {
    return get_gene_value("attention", "beta_goal");
}

float Genome::get_gamma_curiosity() const {
    return get_gene_value("attention", "gamma_curiosity");
}

float Genome::get_wm_slot_count() const {
    return get_gene_value("working_memory", "slot_count");
}

float Genome::get_da_gain() const {
    return get_gene_value("neuromodulators", "da_gain");
}

// ========================================================================
// SERIALIZATION
// ========================================================================

std::string Genome::to_json() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"id\": " << id << ",\n";
    oss << "  \"parent_id\": " << parent_id << ",\n";
    oss << "  \"seed\": " << seed << ",\n";
    oss << "  \"generation\": " << generation << ",\n";
    oss << "  \"description\": \"" << description << "\",\n";
    oss << "  \"modules\": [\n";
    
    for (size_t m = 0; m < modules.size(); ++m) {
        const auto& module = modules[m];
        oss << "    {\n";
        oss << "      \"name\": \"" << module.name << "\",\n";
        oss << "      \"mutation_prob\": " << module.mutation_prob << ",\n";
        oss << "      \"genes\": [\n";
        
        for (size_t g = 0; g < module.fields.size(); ++g) {
            const auto& gene = module.fields[g];
            oss << "        {\n";
            oss << "          \"key\": \"" << gene.key << "\",\n";
            oss << "          \"value\": " << gene.value << ",\n";
            oss << "          \"min_val\": " << gene.min_val << ",\n";
            oss << "          \"max_val\": " << gene.max_val << ",\n";
            oss << "          \"mutation_sigma\": " << gene.mutation_sigma << ",\n";
            oss << "          \"log_scale\": " << (gene.log_scale ? "true" : "false") << "\n";
            oss << "        }";
            if (g < module.fields.size() - 1) oss << ",";
            oss << "\n";
        }
        
        oss << "      ]\n";
        oss << "    }";
        if (m < modules.size() - 1) oss << ",";
        oss << "\n";
    }
    
    oss << "  ]\n";
    oss << "}\n";
    
    return oss.str();
}

bool Genome::save(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    
    file << to_json();
    return true;
}

bool Genome::load(const std::string& path) {
    // TODO: Implement JSON parsing
    // For now, just placeholder
    return false;
}

bool Genome::from_json(const std::string& json_str) {
    // TODO: Implement JSON parsing
    return false;
}

// ========================================================================
// VALIDATION
// ========================================================================

bool Genome::validate() const {
    for (const auto& module : modules) {
        for (const auto& gene : module.fields) {
            if (gene.value < gene.min_val || gene.value > gene.max_val) {
                return false;
            }
        }
    }
    return true;
}

void Genome::clamp_all() {
    for (auto& module : modules) {
        for (auto& gene : module.fields) {
            gene.clamp();
        }
    }
}

// ========================================================================
// COMPARISON
// ========================================================================

float Genome::distance_to(const Genome& other) const {
    float total_distance = 0.0f;
    int count = 0;
    
    for (const auto& module : modules) {
        auto other_module = other.get_module(module.name);
        if (!other_module) continue;
        
        for (const auto& gene : module.fields) {
            auto other_gene = other_module->get_gene(gene.key);
            if (!other_gene) continue;
            
            // Normalized distance
            float range = gene.max_val - gene.min_val;
            if (range > 0) {
                float diff = std::abs(gene.value - other_gene->value) / range;
                total_distance += diff * diff;
                count++;
            }
        }
    }
    
    return count > 0 ? std::sqrt(total_distance / count) : 0.0f;
}

size_t Genome::hash() const {
    size_t h = std::hash<uint64_t>{}(id);
    h ^= std::hash<uint64_t>{}(seed) << 1;
    return h;
}

// ========================================================================
// HELPER
// ========================================================================

GeneField Genome::make_gene(const std::string& key, float value, 
                           float min_val, float max_val, float sigma,
                           bool log_scale) const {
    return GeneField(key, value, min_val, max_val, sigma, log_scale);
}

// ============================================================================
// GENOME FACTORY
// ============================================================================

Genome GenomeFactory::create_base() {
    Genome genome(next_genome_id_++, 12345);
    genome.initialize_base_genome();
    genome.generation = 0;
    genome.description = "Factory base genome";
    return genome;
}

Genome GenomeFactory::create_random(uint64_t seed) {
    Genome genome(next_genome_id_++, seed);
    genome.initialize_base_genome();
    
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // Randomize all genes within their bounds
    for (auto& module : genome.modules) {
        for (auto& gene : module.fields) {
            float t = dist(rng);
            gene.value = gene.min_val + t * (gene.max_val - gene.min_val);
        }
    }
    
    genome.description = "Random genome";
    return genome;
}

Genome GenomeFactory::create_from_template(const Genome& template_genome, 
                                          float noise_scale, uint64_t seed) {
    Genome genome = template_genome;
    genome.id = next_genome_id_++;
    genome.parent_id = template_genome.id;
    genome.seed = seed;
    genome.generation = template_genome.generation + 1;
    
    std::mt19937 rng(seed);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    // Add Gaussian noise to all genes
    for (auto& module : genome.modules) {
        for (auto& gene : module.fields) {
            float noise = dist(rng) * gene.mutation_sigma * noise_scale;
            gene.value += noise;
            gene.clamp();
        }
    }
    
    genome.description = "Template variant";
    return genome;
}

// ============================================================================
// HELPERS
// ============================================================================

GenomeID generate_genome_id() {
    static GenomeID next_id = 1;
    return next_id++;
}

std::string genome_to_string(const Genome& genome) {
    std::ostringstream oss;
    oss << "Genome #" << genome.id << " (gen " << genome.generation << ")\n";
    oss << "  Seed: " << genome.seed << "\n";
    oss << "  Parent: " << genome.parent_id << "\n";
    oss << "  Modules: " << genome.modules.size() << "\n";
    
    for (const auto& module : genome.modules) {
        oss << "  • " << module.name << " (" << module.fields.size() << " genes)\n";
    }
    
    return oss.str();
}

} // namespace melvin::v2::evolution

