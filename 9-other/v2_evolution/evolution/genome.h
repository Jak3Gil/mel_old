#pragma once

#include "../core/types_v2.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <random>

namespace melvin::v2::evolution {

// ============================================================================
// GENE FIELD - Single evolvable parameter
// ============================================================================

struct GeneField {
    std::string key;           // Parameter name (e.g., "alpha_saliency")
    float value;               // Current value
    float min_val;             // Minimum allowed value
    float max_val;             // Maximum allowed value
    float mutation_sigma;      // Std dev for Gaussian mutation
    bool log_scale;            // If true, mutate in log-space
    
    GeneField() 
        : value(0.5f), min_val(0.0f), max_val(1.0f), 
          mutation_sigma(0.05f), log_scale(false) {}
    
    GeneField(const std::string& k, float v, float min_v, float max_v, 
              float sigma, bool log = false)
        : key(k), value(v), min_val(min_v), max_val(max_v), 
          mutation_sigma(sigma), log_scale(log) {}
    
    // Clamp value to valid range
    void clamp() {
        if (value < min_val) value = min_val;
        if (value > max_val) value = max_val;
    }
};

// ============================================================================
// GENE MODULE - Group of related genes
// ============================================================================

struct GeneModule {
    std::string name;                  // Module name ("attention", "wm", "neuromod")
    std::vector<GeneField> fields;     // All gene fields in this module
    float mutation_prob;               // Probability of mutating this module
    
    GeneModule() : mutation_prob(0.1f) {}
    
    GeneModule(const std::string& n, float mut_prob = 0.1f)
        : name(n), mutation_prob(mut_prob) {}
    
    // Add a gene field to this module
    void add_gene(const GeneField& field) {
        fields.push_back(field);
    }
    
    // Get gene by key
    GeneField* get_gene(const std::string& key) {
        for (auto& field : fields) {
            if (field.key == key) return &field;
        }
        return nullptr;
    }
    
    const GeneField* get_gene(const std::string& key) const {
        for (const auto& field : fields) {
            if (field.key == key) return &field;
        }
        return nullptr;
    }
};

// ============================================================================
// GENOME - Complete genetic encoding of a brain
// ============================================================================

class Genome {
public:
    GenomeID id;
    GenomeID parent_id;
    uint64_t seed;                      // RNG seed for deterministic expression
    std::vector<GeneModule> modules;    // All gene modules
    
    // Generation tracking
    size_t generation;
    std::string description;
    
    Genome();
    Genome(GenomeID id, uint64_t seed);
    
    // ========================================================================
    // MODULE INITIALIZATION
    // ========================================================================
    
    // Add standard gene modules with default values
    void add_attention_genes();
    void add_predictive_genes();
    void add_wm_genes();
    void add_neuromod_genes();
    void add_consolidation_genes();
    void add_hopfield_genes();
    void add_learning_genes();
    void add_vision_genes();  // NEW! Camera evolution genes
    
    // Initialize with all standard modules
    void initialize_base_genome();
    
    // ========================================================================
    // GENE ACCESS
    // ========================================================================
    
    // Get module by name
    GeneModule* get_module(const std::string& name);
    const GeneModule* get_module(const std::string& name) const;
    
    // Get gene value by module.key notation
    float get_gene_value(const std::string& module, const std::string& key) const;
    bool set_gene_value(const std::string& module, const std::string& key, float value);
    
    // Quick accessors for common genes
    float get_alpha_saliency() const;
    float get_beta_goal() const;
    float get_gamma_curiosity() const;
    float get_wm_slot_count() const;
    float get_da_gain() const;
    
    // ========================================================================
    // SERIALIZATION
    // ========================================================================
    
    bool save(const std::string& path) const;
    bool load(const std::string& path);
    std::string to_json() const;
    bool from_json(const std::string& json_str);
    
    // ========================================================================
    // VALIDATION
    // ========================================================================
    
    bool validate() const;  // Check all genes in valid ranges
    void clamp_all();       // Force all genes to valid ranges
    
    // ========================================================================
    // COMPARISON
    // ========================================================================
    
    // Compute distance between genomes (for novelty search)
    float distance_to(const Genome& other) const;
    
    // Hash for deduplication
    size_t hash() const;
    
private:
    // Helper: create a gene field
    GeneField make_gene(const std::string& key, float value, 
                       float min_val, float max_val, float sigma, 
                       bool log_scale = false) const;
};

// ============================================================================
// GENOME FACTORY
// ============================================================================

class GenomeFactory {
public:
    // Create base genome with default parameters
    static Genome create_base();
    
    // Create random genome within bounds
    static Genome create_random(uint64_t seed);
    
    // Create genome from template with noise
    static Genome create_from_template(const Genome& template_genome, 
                                      float noise_scale, uint64_t seed);
private:
    static uint64_t next_genome_id_;
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Generate unique genome ID
GenomeID generate_genome_id();

// Pretty-print genome
std::string genome_to_string(const Genome& genome);

} // namespace melvin::v2::evolution

