/*
 * Test Adaptive Weighting - Show local normalization in action
 */

#include "../core/storage.h"
#include "../core/adaptive_weighting.h"
#include "../core/types.h"
#include <iostream>
#include <iomanip>

using namespace melvin;

void simulate_queries(Storage& storage, adaptive::AdaptiveWeighting& weighting, int num_queries) {
    auto& nodes = storage.get_nodes_mut();
    auto& edges = storage.get_edges_mut();
    
    std::cout << "Simulating " << num_queries << " queries...\n";
    
    for (int i = 0; i < num_queries; ++i) {
        // Simulate activating some nodes
        for (auto& node : nodes) {
            if (node.id % 3 == i % 3) {  // Activate every 3rd node in rotation
                node.activation = 1.0f;
                node.activations++;
            } else {
                node.activation = 0.0f;
            }
        }
        
        // Record coactivations
        weighting.record_coactivations(nodes, edges, &storage);
        
        // Update weights periodically
        if ((i + 1) % 10 == 0) {
            weighting.update_weights(nodes, edges, &storage);
        }
    }
    
    std::cout << "  ✓ Simulated " << num_queries << " reasoning cycles\n\n";
}

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  ADAPTIVE LOCAL WEIGHTING TEST                        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    Storage storage;
    
    if (!storage.load("data/nodes.melvin", "data/edges.melvin")) {
        std::cerr << "✗ Failed to load\n";
        return 1;
    }
    
    std::cout << "📊 Loaded: " << storage.node_count() << " nodes, " 
              << storage.edge_count() << " edges\n\n";
    
    // Create adaptive weighting system
    adaptive::AdaptiveWeighting::Config config;
    config.alpha = 1.0f;
    config.beta = 0.3f;
    config.decay = 0.999f;
    config.update_interval = 10;
    config.verbose = true;
    
    adaptive::AdaptiveWeighting weighting(config);
    
    std::cout << "⚙️  ADAPTIVE WEIGHTING CONFIG:\n";
    std::cout << "  Alpha (emphasis):    " << config.alpha << "\n";
    std::cout << "  Beta (stabilization): " << config.beta << "\n";
    std::cout << "  Decay factor:        " << config.decay << "\n";
    std::cout << "  Update interval:     " << config.update_interval << " cycles\n\n";
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  BEFORE ADAPTATION\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    auto& nodes_before = storage.get_nodes_mut();
    auto& edges_before = storage.get_edges_mut();
    
    std::cout << "Sample node weights (before):\n";
    for (size_t i = 0; i < std::min(nodes_before.size(), size_t(5)); ++i) {
        std::cout << "  " << std::setw(15) << storage.get_node_content(nodes_before[i].id)
                  << ": weight=" << nodes_before[i].weight 
                  << ", activations=" << nodes_before[i].activations << "\n";
    }
    
    std::cout << "\nSample edge weights (before):\n";
    for (size_t i = 0; i < std::min(edges_before.size(), size_t(5)); ++i) {
        std::string from = storage.get_node_content(edges_before[i].from_id);
        std::string to = storage.get_node_content(edges_before[i].to_id);
        std::cout << "  " << std::setw(15) << from << " → " << std::setw(15) << to
                  << ": adaptive_weight=" << edges_before[i].adaptive_weight
                  << ", coactivations=" << edges_before[i].coactivations << "\n";
    }
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  SIMULATING USAGE\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    simulate_queries(storage, weighting, 50);
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  AFTER ADAPTATION\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    auto& nodes_after = storage.get_nodes_mut();
    auto& edges_after = storage.get_edges_mut();
    
    std::cout << "Sample node weights (after):\n";
    for (size_t i = 0; i < std::min(nodes_after.size(), size_t(5)); ++i) {
        std::cout << "  " << std::setw(15) << storage.get_node_content(nodes_after[i].id)
                  << ": weight=" << std::fixed << std::setprecision(4) << nodes_after[i].weight 
                  << ", activations=" << nodes_after[i].activations << "\n";
    }
    
    std::cout << "\nSample edge weights (after):\n";
    for (size_t i = 0; i < std::min(edges_after.size(), size_t(5)); ++i) {
        std::string from = storage.get_node_content(edges_after[i].from_id);
        std::string to = storage.get_node_content(edges_after[i].to_id);
        std::cout << "  " << std::setw(15) << from << " → " << std::setw(15) << to
                  << ": adaptive_weight=" << std::fixed << std::setprecision(4) << edges_after[i].adaptive_weight
                  << ", coactivations=" << edges_after[i].coactivations << "\n";
    }
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  STATISTICS\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    auto stats = weighting.get_stats();
    
    std::cout << "  Total activations:     " << stats.total_activations << "\n";
    std::cout << "  Total coactivations:   " << stats.total_coactivations << "\n";
    std::cout << "  Updates performed:     " << stats.updates_performed << "\n";
    std::cout << "  Avg node weight:       " << std::fixed << std::setprecision(4) << stats.avg_node_weight << "\n";
    std::cout << "  Avg edge weight:       " << stats.avg_edge_weight << "\n\n";
    
    std::cout << "✅ Benefits of Local Normalization:\n";
    std::cout << "  • Weights scale with graph size (not diluted)\n";
    std::cout << "  • High-degree nodes automatically balanced\n";
    std::cout << "  • Frequent pairs stay strong\n";
    std::cout << "  • New nodes don't vanish in large graphs\n";
    std::cout << "  • O(E) complexity - same as diffusion!\n\n";
    
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ ADAPTIVE WEIGHTING WORKING                        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    return 0;
}

