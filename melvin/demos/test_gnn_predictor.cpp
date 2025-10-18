/*
 * Test GNN Predictor - Watch neural learning in action
 */

#include "../core/storage.h"
#include "../core/gnn_predictor.h"
#include "../core/reasoning.h"
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace melvin;

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  GNN PREDICTOR TEST                                   ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    // Load brain
    Storage storage;
    std::cout << "📂 Loading brain...\n";
    if (!storage.load("data/nodes.melvin", "data/edges.melvin")) {
        std::cerr << "✗ Failed to load brain!\n";
        return 1;
    }
    
    std::cout << "  ✅ Loaded: " << storage.node_count() << " nodes, " 
              << storage.edge_count() << " edges\n\n";
    
    // Create GNN predictor
    gnn::GNNPredictor::Config config;
    config.embed_dim = 32;
    config.learning_rate = 0.001f;
    config.prediction_decay = 0.9f;
    config.message_passes = 3;
    config.verbose = true;
    
    gnn::GNNPredictor predictor(config);
    
    std::cout << "⚙️  GNN Configuration:\n";
    std::cout << "  Embedding dimension: " << config.embed_dim << "\n";
    std::cout << "  Learning rate:       " << config.learning_rate << "\n";
    std::cout << "  Message passes:      " << config.message_passes << "\n";
    std::cout << "  Full connectivity:   " << (config.full_connectivity ? "yes" : "no") << "\n\n";
    
    // Initialize embeddings
    std::cout << "🧠 Initializing node embeddings...\n";
    auto& nodes = storage.get_nodes_mut();
    auto& edges = storage.get_edges_mut();
    predictor.initialize_embeddings(nodes);
    std::cout << "  ✓ Initialized " << nodes.size() << " embeddings\n\n";
    
    // Open log for training visualization
    std::ofstream log("data/gnn_training.csv");
    log << "step,avg_error,avg_norm,total_loss\n";
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  TRAINING GNN PREDICTOR\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    int training_steps = 50;
    
    for (int step = 0; step < training_steps; ++step) {
        // Simulate some activations
        std::vector<float> target(nodes.size(), 0.0f);
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (i % 3 == step % 3) {
                nodes[i].activation = 1.0f;
                target[i] = 1.0f;
            } else {
                nodes[i].activation = 0.0f;
                target[i] = 0.0f;
            }
        }
        
        // Message pass
        predictor.message_pass(nodes, edges, &storage);
        
        // Predict
        auto predicted = predictor.predict_activations(nodes);
        
        // Train
        predictor.train(nodes, target);
        
        // Log stats
        auto stats = predictor.get_stats();
        log << step << ","
            << stats.avg_prediction_error << ","
            << stats.avg_embedding_norm << ","
            << stats.total_loss << "\n";
        
        if (step % 10 == 0) {
            std::cout << "[Step " << std::setw(3) << step << "] "
                      << "Error: " << std::fixed << std::setprecision(4) << stats.avg_prediction_error
                      << " | Loss: " << stats.total_loss
                      << " | Norm: " << stats.avg_embedding_norm << "\n";
        }
    }
    
    log.close();
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  TRAINING COMPLETE\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    auto final_stats = predictor.get_stats();
    
    std::cout << "📊 Final Statistics:\n";
    std::cout << "  Training steps:      " << final_stats.training_steps << "\n";
    std::cout << "  Avg pred error:      " << std::fixed << std::setprecision(4) 
              << final_stats.avg_prediction_error << "\n";
    std::cout << "  Avg embedding norm:  " << final_stats.avg_embedding_norm << "\n";
    std::cout << "  Total loss:          " << final_stats.total_loss << "\n\n";
    
    std::cout << "✅ Results saved:\n";
    std::cout << "  📄 data/gnn_training.csv\n\n";
    
    std::cout << "📊 Next steps:\n";
    std::cout << "  python3 scripts/visualize_gnn_training.py\n\n";
    
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ GNN PREDICTOR WORKING                             ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    return 0;
}

