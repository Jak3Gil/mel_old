/*
 * 🧠 Hopfield-Diffusion Reasoning Demo
 * 
 * Demonstrates the upgraded reasoning system with:
 * - Vector embeddings for memory
 * - Attention-based diffusion
 * - Energy minimization (Hopfield-style)
 * - Blended Hebbian + gradient learning
 * 
 * [Hopfield-Diffusion Upgrade Demo]
 */

#include "melvin/core/storage.h"
#include "melvin/core/hopfield_diffusion.h"
#include <iostream>
#include <iomanip>

using namespace melvin;

void print_header(const std::string& title) {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  " << std::left << std::setw(60) << title << " ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
}

int main() {
    print_header("🧠 HOPFIELD-DIFFUSION REASONING DEMO");
    
    // ========================================================================
    // STEP 1: Load existing knowledge base
    // ========================================================================
    
    std::cout << "Step 1: Loading knowledge base...\n\n";
    
    auto storage = std::make_unique<Storage>();
    bool loaded = storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    
    if (!loaded) {
        std::cerr << "❌ Failed to load knowledge base\n";
        std::cerr << "   Run ./optimized_melvin_demo first to create knowledge base\n";
        return 1;
    }
    
    std::cout << "  ✓ Loaded knowledge base\n";
    std::cout << "    Nodes: " << storage->node_count() << "\n";
    std::cout << "    Edges: " << storage->edge_count() << "\n";
    
    // ========================================================================
    // STEP 2: Initialize Hopfield-Diffusion System
    // ========================================================================
    
    print_header("INITIALIZING HOPFIELD-DIFFUSION SYSTEM");
    
    hopfield::HopfieldDiffusion::Config hd_config;
    hd_config.embedding_dim = 32;
    hd_config.beta = 5.0f;
    hd_config.max_diffusion_steps = 15;
    hd_config.verbose = true;  // Show energy evolution
    hd_config.normalize_embeddings = true;
    
    auto hd = std::make_unique<hopfield::HopfieldDiffusion>(hd_config);
    
    std::cout << "  ✓ Hopfield-Diffusion configured:\n";
    std::cout << "    Embedding dimension: " << hd_config.embedding_dim << "\n";
    std::cout << "    Beta (attention sharpness): " << hd_config.beta << "\n";
    std::cout << "    Max diffusion steps: " << hd_config.max_diffusion_steps << "\n";
    
    // ========================================================================
    // STEP 3: Ensure embeddings for active nodes
    // ========================================================================
    
    print_header("INITIALIZING VECTOR EMBEDDINGS");
    
    std::cout << "  Checking embeddings for nodes...\n";
    
    // Get some nodes to work with
    auto& all_nodes = storage->get_nodes_mut();
    std::vector<Node> active_nodes;
    
    // Select first 50 nodes as active set
    int num_active = std::min(50, static_cast<int>(all_nodes.size()));
    for (int i = 0; i < num_active; ++i) {
        active_nodes.push_back(all_nodes[i]);
    }
    
    // Initialize embeddings
    int initialized = 0;
    for (auto& node : active_nodes) {
        if (node.embedding.empty()) {
            hd->initialize_embedding(node);
            initialized++;
        }
    }
    
    std::cout << "  ✓ Initialized " << initialized << " embeddings\n";
    std::cout << "  ✓ Active set: " << active_nodes.size() << " nodes\n";
    
    // Set some initial activations (simulate query)
    std::cout << "\n  Setting initial activations...\n";
    for (size_t i = 0; i < std::min(size_t(5), active_nodes.size()); ++i) {
        active_nodes[i].activation = 1.0f - (i * 0.15f);
        std::string content = storage->get_node_content(active_nodes[i].id);
        std::cout << "    Node " << i << ": " << content.substr(0, 30) 
                  << " (a=" << active_nodes[i].activation << ")\n";
    }
    
    // ========================================================================
    // STEP 4: Compute initial energy
    // ========================================================================
    
    print_header("COMPUTING HOPFIELD ENERGY");
    
    float initial_energy = hd->compute_energy(active_nodes);
    
    std::cout << "  Initial system energy: " << std::fixed << std::setprecision(4) 
              << initial_energy << "\n";
    std::cout << "  (Lower energy = more stable/coherent state)\n";
    
    // ========================================================================
    // STEP 5: Run attention-based diffusion
    // ========================================================================
    
    print_header("RUNNING ATTENTION-BASED DIFFUSION");
    
    std::cout << "  Starting diffusion process...\n\n";
    
    auto result = hd->run_diffusion(active_nodes, storage.get());
    
    std::cout << "\n  ✓ Diffusion complete!\n";
    std::cout << "    Steps taken: " << result.steps_taken << "\n";
    std::cout << "    Converged: " << (result.converged ? "Yes" : "No (max steps)") << "\n";
    std::cout << "    Final energy: " << std::fixed << std::setprecision(4) 
              << result.final_energy << "\n";
    std::cout << "    Energy change: " << (result.final_energy - initial_energy) << "\n";
    
    // ========================================================================
    // STEP 6: Show energy trajectory
    // ========================================================================
    
    print_header("ENERGY TRAJECTORY (Convergence Path)");
    
    std::cout << "  Energy evolution over time:\n\n";
    std::cout << "  Step | Energy      | ΔE\n";
    std::cout << "  ─────┼─────────────┼────────────\n";
    
    for (size_t i = 0; i < result.energy_trajectory.size(); ++i) {
        float energy = result.energy_trajectory[i];
        float delta = (i > 0) ? (energy - result.energy_trajectory[i-1]) : 0.0f;
        
        std::cout << "  " << std::setw(4) << i << " | " 
                  << std::setw(11) << std::fixed << std::setprecision(4) << energy << " | "
                  << std::setw(11) << delta << "\n";
        
        if (i >= 10 && result.energy_trajectory.size() > 15) {
            std::cout << "  ...  | ...         | ...\n";
            i = result.energy_trajectory.size() - 3;
        }
    }
    
    // ========================================================================
    // STEP 7: Show final activation state
    // ========================================================================
    
    print_header("FINAL ACTIVATION STATE (After Diffusion)");
    
    // Sort by activation
    std::vector<std::pair<float, std::string>> final_state;
    for (const auto& node : active_nodes) {
        if (node.activation > 0.01f) {
            std::string content = storage->get_node_content(node.id);
            final_state.push_back({node.activation, content});
        }
    }
    
    std::sort(final_state.begin(), final_state.end(), std::greater<>());
    
    std::cout << "  Top activated nodes (stable attractor state):\n\n";
    for (size_t i = 0; i < std::min(size_t(15), final_state.size()); ++i) {
        std::cout << "    " << std::setw(2) << (i+1) << ". " 
                  << std::setw(40) << std::left << final_state[i].second.substr(0, 40)
                  << " a=" << std::fixed << std::setprecision(3) << final_state[i].first << "\n";
    }
    
    // ========================================================================
    // STEP 8: Update weights using blended learning
    // ========================================================================
    
    print_header("APPLYING BLENDED LEARNING (Hebbian + Gradient)");
    
    std::cout << "  Updating edge weights...\n";
    
    auto& edges = storage->get_edges_mut();
    hd->update_weights(active_nodes, edges, storage.get());
    
    std::cout << "  ✓ Weights updated using:\n";
    std::cout << "    - Local Hebbian term (η_local * a[i] * a[j])\n";
    std::cout << "    - Global gradient term (η_global * β * cos_sim)\n";
    
    // ========================================================================
    // STEP 9: Apply normalization pass
    // ========================================================================
    
    print_header("NORMALIZATION PASS (Stability)");
    
    std::cout << "  Applying activation leak and embedding normalization...\n";
    hd->normalize_pass(active_nodes);
    std::cout << "  ✓ Normalization complete\n";
    
    // ========================================================================
    // STEP 10: Display statistics
    // ========================================================================
    
    print_header("HOPFIELD-DIFFUSION STATISTICS");
    
    auto stats = hd->get_stats();
    
    std::cout << "  Diffusion runs: " << stats.diffusion_runs << "\n";
    std::cout << "  Total steps: " << stats.total_steps << "\n";
    std::cout << "  Convergences: " << stats.convergences << "\n";
    std::cout << "  Avg steps per run: " << std::fixed << std::setprecision(2) 
              << stats.avg_steps_per_run << "\n";
    std::cout << "  Avg final energy: " << std::fixed << std::setprecision(4) 
              << stats.avg_final_energy << "\n";
    std::cout << "  Min energy seen: " << stats.min_energy_seen << "\n";
    std::cout << "  Max energy seen: " << stats.max_energy_seen << "\n";
    
    // ========================================================================
    // SUMMARY
    // ========================================================================
    
    print_header("🎉 HOPFIELD-DIFFUSION UPGRADE COMPLETE!");
    
    std::cout << "Key Features Demonstrated:\n\n";
    std::cout << "  ✅ Vector embeddings for stable memory\n";
    std::cout << "  ✅ Attention-based diffusion (exp(β * cos_sim))\n";
    std::cout << "  ✅ Hopfield energy minimization\n";
    std::cout << "  ✅ Energy convergence tracking\n";
    std::cout << "  ✅ Blended Hebbian + gradient learning\n";
    std::cout << "  ✅ Activation leak + embedding normalization\n\n";
    
    std::cout << "Performance:\n";
    std::cout << "  Energy descent: " << initial_energy << " → " << result.final_energy << "\n";
    std::cout << "  Convergence: " << result.steps_taken << " steps\n";
    std::cout << "  Stable state: " << final_state.size() << " active nodes\n\n";
    
    std::cout << "🧠 Melvin's reasoning is now Hopfield-Diffusion powered!\n\n";
    
    return 0;
}




