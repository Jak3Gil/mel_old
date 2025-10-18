/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  DEMO: LEAP Pattern Synthesis                                            ║
 * ║  Shows how Melvin creates new concept nodes from overlapping attractors  ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "melvin/core/storage.h"
#include "melvin/core/hopfield_diffusion.h"
#include "melvin/core/leap_synthesis.h"
#include <iostream>
#include <iomanip>

using namespace melvin;
using namespace melvin::hopfield;

void print_header(const std::string& title) {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  " << std::left << std::setw(60) << title << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🌌 MELVIN LEAP PATTERN SYNTHESIS DEMO                         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    
    // ========================================================================
    // SETUP
    // ========================================================================
    
    print_header("SETUP: Creating Knowledge Base");
    
    auto storage = std::make_unique<Storage>();
    
    // Create some test concepts that will form overlapping patterns
    std::cout << "Creating test concepts...\n";
    
    NodeID fire = storage->create_node("fire", NodeType::CONCEPT);
    NodeID heat = storage->create_node("heat", NodeType::CONCEPT);
    NodeID light = storage->create_node("light", NodeType::CONCEPT);
    NodeID photosynthesis = storage->create_node("photosynthesis", NodeType::CONCEPT);
    NodeID energy = storage->create_node("energy", NodeType::CONCEPT);
    NodeID plant = storage->create_node("plant", NodeType::CONCEPT);
    NodeID sun = storage->create_node("sun", NodeType::CONCEPT);
    NodeID combustion = storage->create_node("combustion", NodeType::CONCEPT);
    
    // Create connections
    storage->create_edge(fire, heat, RelationType::EXACT, 0.9f);
    storage->create_edge(fire, light, RelationType::EXACT, 0.9f);
    storage->create_edge(fire, combustion, RelationType::EXACT, 0.8f);
    
    storage->create_edge(photosynthesis, light, RelationType::EXACT, 0.9f);
    storage->create_edge(photosynthesis, energy, RelationType::EXACT, 0.9f);
    storage->create_edge(photosynthesis, plant, RelationType::EXACT, 0.8f);
    
    storage->create_edge(sun, light, RelationType::EXACT, 1.0f);
    storage->create_edge(sun, heat, RelationType::EXACT, 0.9f);
    storage->create_edge(sun, energy, RelationType::EXACT, 0.8f);
    
    std::cout << "✓ Created " << storage->node_count() << " nodes\n";
    std::cout << "✓ Created " << storage->edge_count() << " edges\n";
    
    // ========================================================================
    // INITIALIZE HOPFIELD-DIFFUSION
    // ========================================================================
    
    print_header("INITIALIZING HOPFIELD-DIFFUSION");
    
    HopfieldDiffusion::Config hd_config;
    hd_config.embedding_dim = 32;
    hd_config.beta = 5.0f;
    hd_config.max_diffusion_steps = 15;
    hd_config.verbose = true;
    
    auto hopfield = std::make_unique<HopfieldDiffusion>(hd_config);
    
    // Initialize embeddings
    auto& nodes = storage->get_nodes_mut();
    for (auto& node : nodes) {
        hopfield->initialize_embedding(node);
    }
    
    std::cout << "✓ Initialized " << nodes.size() << " node embeddings\n";
    
    // ========================================================================
    // SETUP LEAP SYNTHESIS
    // ========================================================================
    
    print_header("INITIALIZING LEAP SYNTHESIS");
    
    LeapSynthesisConfig leap_config;
    leap_config.activation_threshold = 0.4f;
    leap_config.overlap_threshold = 0.5f;
    leap_config.noise_scale = 0.05f;
    leap_config.curiosity_threshold = 0.6f;
    leap_config.stability_threshold = 0.01f;
    leap_config.verbose = true;
    
    auto leap_synthesis = std::make_unique<LeapSynthesis>(leap_config);
    
    std::cout << "Configuration:\n";
    std::cout << "  Activation threshold:  " << leap_config.activation_threshold << "\n";
    std::cout << "  Overlap threshold:     " << leap_config.overlap_threshold << "\n";
    std::cout << "  Curiosity threshold:   " << leap_config.curiosity_threshold << "\n";
    std::cout << "  Stability threshold:   " << leap_config.stability_threshold << "\n";
    
    // ========================================================================
    // RUN REASONING WITH LEAP SYNTHESIS
    // ========================================================================
    
    print_header("RUNNING HOPFIELD-DIFFUSION REASONING");
    
    // Activate query nodes
    std::cout << "Activating query: 'fire' and 'photosynthesis'\n\n";
    for (auto& node : nodes) {
        if (node.id == fire || node.id == photosynthesis) {
            node.activation = 1.0f;
        } else {
            node.activation = 0.1f;
        }
    }
    
    // Run diffusion
    auto result = hopfield->run_diffusion(nodes, storage.get());
    
    std::cout << "\nDiffusion complete:\n";
    std::cout << "  Steps taken:     " << result.steps_taken << "\n";
    std::cout << "  Final energy:    " << std::fixed << std::setprecision(3) << result.final_energy << "\n";
    std::cout << "  Converged:       " << (result.converged ? "Yes" : "No") << "\n";
    
    // ========================================================================
    // ATTEMPT LEAP SYNTHESIS
    // ========================================================================
    
    print_header("ATTEMPTING LEAP SYNTHESIS");
    
    // Simulate high curiosity and stable system
    float curiosity = 0.8f;  // High curiosity
    float energy_change_per_step = 0.005f;  // Stable
    
    std::cout << "Conditions:\n";
    std::cout << "  Curiosity:             " << curiosity << " (threshold: " << leap_config.curiosity_threshold << ")\n";
    std::cout << "  Energy change/step:    " << energy_change_per_step << " (threshold: " << leap_config.stability_threshold << ")\n";
    std::cout << "\n";
    
    int leaps_created = leap_synthesis->attempt_leap_synthesis(
        nodes,
        storage.get(),
        hopfield.get(),
        curiosity,
        energy_change_per_step
    );
    
    std::cout << "\n✨ Created " << leaps_created << " LEAP node(s)\n";
    
    // ========================================================================
    // SHOW RESULTS
    // ========================================================================
    
    print_header("LEAP SYNTHESIS RESULTS");
    
    const auto& leap_links = leap_synthesis->get_leap_links();
    
    if (leap_links.empty()) {
        std::cout << "No LEAPs created (conditions not met or no overlaps detected)\n";
        std::cout << "\nTry adjusting:\n";
        std::cout << "  • Lower activation_threshold (currently " << leap_config.activation_threshold << ")\n";
        std::cout << "  • Lower overlap_threshold (currently " << leap_config.overlap_threshold << ")\n";
        std::cout << "  • Increase diffusion steps for more activation spread\n";
    } else {
        std::cout << "Created " << leap_links.size() << " LEAP link(s):\n\n";
        
        for (size_t i = 0; i < leap_links.size(); ++i) {
            const auto& link = leap_links[i];
            
            std::string src_a = storage->get_node_content(link.source_a);
            std::string src_b = storage->get_node_content(link.source_b);
            std::string leap = storage->get_node_content(link.leap_node);
            
            std::cout << "  " << (i + 1) << ". \"" << src_a << "\" ⇄ \"" << src_b << "\"\n";
            std::cout << "     → Created: \"" << leap << "\"\n";
            std::cout << "     → Strength:     " << std::fixed << std::setprecision(3) << link.strength << "\n";
            std::cout << "     → Novelty:      " << std::fixed << std::setprecision(3) << link.novelty << "\n";
            std::cout << "     → ΔE:           " << std::fixed << std::setprecision(3) << link.energy_improvement << "\n";
            std::cout << "\n";
        }
    }
    
    // Show statistics
    leap_synthesis->print_summary();
    
    // ========================================================================
    // SHOW FINAL KNOWLEDGE GRAPH
    // ========================================================================
    
    print_header("FINAL KNOWLEDGE GRAPH");
    
    std::cout << "Total nodes: " << storage->node_count() << " (+" << leaps_created << " LEAP)\n";
    std::cout << "Total edges: " << storage->edge_count() << "\n\n";
    
    // Show active nodes
    std::cout << "Most active nodes:\n";
    std::vector<std::pair<std::string, float>> active_nodes;
    for (const auto& node : nodes) {
        if (node.activation > 0.3f) {
            active_nodes.push_back({storage->get_node_content(node.id), node.activation});
        }
    }
    
    std::sort(active_nodes.begin(), active_nodes.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (size_t i = 0; i < std::min(size_t(10), active_nodes.size()); ++i) {
        std::cout << "  " << (i + 1) << ". \"" << active_nodes[i].first << "\" - "
                  << std::fixed << std::setprecision(3) << active_nodes[i].second << "\n";
    }
    
    // ========================================================================
    // PERSISTENCE
    // ========================================================================
    
    print_header("SAVING RESULTS");
    
    if (leap_synthesis->save_leap_links("melvin/data/leap_links.bin")) {
        std::cout << "✓ Saved LEAP links to melvin/data/leap_links.bin\n";
    }
    
    storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    std::cout << "✓ Saved knowledge graph\n";
    
    // ========================================================================
    // SUMMARY
    // ========================================================================
    
    print_header("SUMMARY");
    
    std::cout << "This demo showed:\n";
    std::cout << "  ✓ Hopfield-Diffusion reasoning with overlapping concepts\n";
    std::cout << "  ✓ Detection of attractor clusters\n";
    std::cout << "  ✓ Synthesis of LEAP nodes from overlaps\n";
    std::cout << "  ✓ Energy-based validation\n";
    std::cout << "  ✓ Graph linking and persistence\n\n";
    
    std::cout << "Integration points:\n";
    std::cout << "  1. After each diffusion cycle in reasoning loop\n";
    std::cout << "  2. Check curiosity > threshold\n";
    std::cout << "  3. Check system stability (low ΔE)\n";
    std::cout << "  4. Call leap_synthesis->attempt_leap_synthesis(...)\n";
    std::cout << "  5. New concepts emerge dynamically!\n\n";
    
    std::cout << "Next steps:\n";
    std::cout << "  • Integrate into melvin/core/reasoning.cpp\n";
    std::cout << "  • Add curiosity variable to ReasoningEngine\n";
    std::cout << "  • Call LEAP synthesis after convergence\n";
    std::cout << "  • Track and use emergent patterns\n\n";
    
    return 0;
}

