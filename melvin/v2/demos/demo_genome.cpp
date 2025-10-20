/*
 * MELVIN v2 - Genome System Demo
 * 
 * Demonstrates:
 * - Creating a base genome
 * - Accessing gene values
 * - Genome serialization
 * - Genome mutations (basic)
 */

#include "../evolution/genome.h"
#include "../core/global_workspace.h"
#include "../core/neuromodulators.h"
#include <iostream>
#include <iomanip>

using namespace melvin::v2;
using namespace melvin::v2::evolution;

void print_section(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(60, '=') << "\n";
}

void demo_base_genome() {
    print_section("1. Creating Base Genome");
    
    // Create base genome with default parameters
    Genome genome = GenomeFactory::create_base();
    
    std::cout << genome_to_string(genome) << std::endl;
    
    // Show some key genes
    std::cout << "\nKey Genes:\n";
    std::cout << "  Attention:\n";
    std::cout << "    alpha_saliency:   " << genome.get_alpha_saliency() << "\n";
    std::cout << "    beta_goal:        " << genome.get_beta_goal() << "\n";
    std::cout << "    gamma_curiosity:  " << genome.get_gamma_curiosity() << "\n";
    
    std::cout << "  Working Memory:\n";
    std::cout << "    slot_count:       " << genome.get_wm_slot_count() << "\n";
    std::cout << "    decay_tau:        " << genome.get_gene_value("working_memory", "decay_tau") << "\n";
    
    std::cout << "  Neuromodulators:\n";
    std::cout << "    DA gain:          " << genome.get_da_gain() << "\n";
    std::cout << "    NE baseline:      " << genome.get_gene_value("neuromodulators", "ne_baseline") << "\n";
}

void demo_gene_access() {
    print_section("2. Gene Access & Modification");
    
    Genome genome = GenomeFactory::create_base();
    
    // Show original value
    float orig_alpha = genome.get_alpha_saliency();
    std::cout << "Original alpha_saliency: " << orig_alpha << "\n";
    
    // Modify
    genome.set_gene_value("attention", "alpha_saliency", 0.6f);
    float new_alpha = genome.get_alpha_saliency();
    std::cout << "Modified alpha_saliency: " << new_alpha << "\n";
    
    // Validate genome
    bool valid = genome.validate();
    std::cout << "Genome valid: " << (valid ? "YES" : "NO") << "\n";
    
    // Try invalid value
    genome.set_gene_value("attention", "alpha_saliency", 2.0f);  // Out of bounds
    genome.clamp_all();
    std::cout << "After clamping invalid value: " << genome.get_alpha_saliency() << "\n";
}

void demo_serialization() {
    print_section("3. Genome Serialization");
    
    Genome genome = GenomeFactory::create_base();
    genome.description = "Demo genome for serialization";
    
    // Serialize to JSON
    std::string json = genome.to_json();
    std::cout << "JSON (first 500 chars):\n";
    std::cout << json.substr(0, 500) << "...\n";
    
    // Save to file
    bool saved = genome.save("/tmp/demo_genome.json");
    std::cout << "\nSaved to /tmp/demo_genome.json: " << (saved ? "SUCCESS" : "FAILED") << "\n";
}

void demo_genome_variants() {
    print_section("4. Genome Variants");
    
    // Create base
    Genome base = GenomeFactory::create_base();
    
    // Create random variant
    Genome random = GenomeFactory::create_random(54321);
    
    // Create template variant (base + noise)
    Genome variant = GenomeFactory::create_from_template(base, 0.1f, 99999);
    
    std::cout << "Base genome:\n";
    std::cout << "  alpha_saliency: " << base.get_alpha_saliency() << "\n";
    std::cout << "  DA gain:        " << base.get_da_gain() << "\n";
    
    std::cout << "\nRandom genome:\n";
    std::cout << "  alpha_saliency: " << random.get_alpha_saliency() << "\n";
    std::cout << "  DA gain:        " << random.get_da_gain() << "\n";
    
    std::cout << "\nVariant genome (base + 10% noise):\n";
    std::cout << "  alpha_saliency: " << variant.get_alpha_saliency() << "\n";
    std::cout << "  DA gain:        " << variant.get_da_gain() << "\n";
    
    // Compute distance
    float dist_base_random = base.distance_to(random);
    float dist_base_variant = base.distance_to(variant);
    
    std::cout << "\nDistances:\n";
    std::cout << "  base → random:  " << dist_base_random << "\n";
    std::cout << "  base → variant: " << dist_base_variant << "\n";
}

void demo_genome_to_neuromod() {
    print_section("5. Genome → Neuromodulator Expression");
    
    Genome genome = GenomeFactory::create_base();
    
    // Express neuromodulators from genome
    Neuromodulators neuromod(genome);
    
    std::cout << "Neuromodulator state from genome:\n";
    std::cout << neuromod_state_to_string(neuromod.get_state()) << "\n";
    
    std::cout << "\nComputed effects:\n";
    std::cout << "  Plasticity rate:    " << neuromod.get_plasticity_rate() << "\n";
    std::cout << "  Exploration bias:   " << neuromod.get_exploration_bias() << "\n";
    std::cout << "  Attention gain:     " << neuromod.get_attention_gain() << "\n";
    std::cout << "  Stability bias:     " << neuromod.get_stability_bias() << "\n";
    
    // Simulate prediction error
    std::cout << "\nSimulating positive prediction error (+0.5)...\n";
    neuromod.on_prediction_error(0.5f);
    std::cout << neuromod_state_to_string(neuromod.get_state()) << "\n";
    std::cout << "  New plasticity rate: " << neuromod.get_plasticity_rate() << "\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN v2 - Genome System Demo                      ║\n";
    std::cout << "║  Evolution-First Cognitive Architecture              ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    
    try {
        demo_base_genome();
        demo_gene_access();
        demo_serialization();
        demo_genome_variants();
        demo_genome_to_neuromod();
        
        print_section("✓ All Demos Complete");
        std::cout << "\nGenome system is working correctly!\n";
        std::cout << "Next: Run evolution to see genomes adapt.\n\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

