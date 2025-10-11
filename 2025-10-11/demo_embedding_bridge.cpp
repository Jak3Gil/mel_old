/*
 * DEMO: Embedding Bridge - Connecting Graph Semantics to Token Predictions
 * 
 * This demonstrates how the embedding bridge translates graph-level
 * semantic activations into token-level prediction biases.
 */

#include "src/embeddings/embedding_bridge.h"
#include "src/embeddings/embedding_learning.h"
#include "melvin_leap_nodes.h"
#include <iostream>
#include <iomanip>

using namespace melvin;
using namespace melvin::embeddings;

void print_header(const std::string& title) {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║ " << title;
    for (size_t i = title.length(); i < 56; i++) std::cout << " ";
    std::cout << " ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n\n";
}

int main() {
    print_header("EMBEDDING BRIDGE DEMONSTRATION");
    
    std::cout << "This system connects:\n";
    std::cout << "  Graph Nodes (concepts) → Node Embeddings → Activation Vector\n";
    std::cout << "  Token Predictions ← Token Embeddings ← Similarity Bias\n\n";
    
    std::cout << "Press Enter to start...\n";
    std::cin.get();
    
    // Demo 1: Create and initialize bridge
    print_header("DEMO 1: Initialize Embedding Bridge");
    
    EmbeddingBridgeConfig config;
    config.embedding_dim = 64;
    config.lambda_graph_bias = 0.5f;
    config.enable_embedding_bridge = true;
    
    std::cout << "Configuration:\n";
    std::cout << "  • Embedding dimension: " << config.embedding_dim << "\n";
    std::cout << "  • Bias strength (λ): " << config.lambda_graph_bias << "\n";
    std::cout << "  • Similarity threshold: " << config.similarity_threshold << "\n";
    std::cout << "  • Learning rate: " << config.learning_rate_embeddings << "\n\n";
    
    EmbeddingBridge bridge(config);
    
    std::cout << "✓ Embedding bridge created\n";
    std::cout << "  • Node embeddings ready: " << config.embedding_dim << "D vectors\n";
    std::cout << "  • Token embeddings ready: " << config.embedding_dim << "D vectors\n";
    
    std::cout << "\nPress Enter for next demo...\n";
    std::cin.get();
    
    // Demo 2: Compute activation vector
    print_header("DEMO 2: Activation Vector from Graph Nodes");
    
    std::cout << "Simulating active graph nodes:\n";
    std::vector<uint64_t> active_nodes = {1001, 1002, 1003, 1004};
    std::vector<float> activations = {0.8f, 0.6f, 0.9f, 0.5f};
    
    for (size_t i = 0; i < active_nodes.size(); ++i) {
        std::cout << "  • Node " << active_nodes[i] 
                  << " (activation=" << std::fixed << std::setprecision(2) 
                  << activations[i] << ")\n";
    }
    
    std::cout << "\nComputing weighted activation vector...\n";
    auto activation_vec = bridge.compute_activation_vector(active_nodes, activations);
    
    std::cout << "✓ Activation vector created:\n";
    std::cout << "  • Total activation: " << activation_vec.total_activation << "\n";
    std::cout << "  • Contributing nodes: " << activation_vec.contributing_nodes.size() << "\n";
    std::cout << "  • Vector norm: " << (activation_vec.is_zero() ? "zero" : "normalized") << "\n";
    
    std::cout << "\nPress Enter for next demo...\n";
    std::cin.get();
    
    // Demo 3: Token bias computation
    print_header("DEMO 3: Token Bias Field from Activation");
    
    std::cout << "Candidate tokens for prediction:\n";
    std::vector<std::string> candidate_tokens = {
        "fire", "water", "smoke", "cloud", "rain", 
        "sky", "ground", "wind", "storm", "sun"
    };
    
    for (const auto& token : candidate_tokens) {
        std::cout << "  • " << token << "\n";
    }
    
    std::cout << "\nComputing similarity bias...\n";
    auto bias_field = bridge.compute_token_bias(activation_vec, candidate_tokens);
    
    std::cout << "\n✓ Bias field computed:\n";
    std::vector<std::pair<std::string, float>> ranked_biases;
    for (const auto& token : candidate_tokens) {
        float bias = bias_field.get_bias(token);
        if (bias > 0.0f) {
            ranked_biases.emplace_back(token, bias);
        }
    }
    
    std::sort(ranked_biases.begin(), ranked_biases.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::cout << "  Top biased tokens:\n";
    for (size_t i = 0; i < std::min(size_t(5), ranked_biases.size()); ++i) {
        std::cout << "    " << (i+1) << ". " << ranked_biases[i].first 
                  << " (bias=" << std::fixed << std::setprecision(4) 
                  << ranked_biases[i].second << ")\n";
    }
    
    std::cout << "\nPress Enter for next demo...\n";
    std::cin.get();
    
    // Demo 4: Learning feedback
    print_header("DEMO 4: Learning from Predictions");
    
    std::cout << "Simulating successful prediction: 'fire'\n\n";
    
    EmbeddingLearner learner(&bridge);
    
    std::vector<uint64_t> generated_path = {1001, 1002};
    float coherence = 0.75f;
    
    std::cout << "Before learning:\n";
    bridge.print_statistics();
    
    std::cout << "\nApplying learning feedback...\n";
    learner.learn_from_success(active_nodes, activations, generated_path, coherence);
    
    std::cout << "\nAfter learning:\n";
    learner.print_statistics();
    
    std::cout << "\n💡 What happened:\n";
    std::cout << "  • Token embeddings moved toward activation vector\n";
    std::cout << "  • Node embeddings aligned with predicted tokens\n";
    std::cout << "  • Future predictions will be more coherent\n";
    
    std::cout << "\nPress Enter for next demo...\n";
    std::cin.get();
    
    // Demo 5: Integration with leap system
    print_header("DEMO 5: Integration with Leap System");
    
    std::cout << "The embedding bridge enhances leap effectiveness:\n\n";
    
    std::cout << "Without embedding bridge:\n";
    std::cout << "  LeapNode → Bias field (graph connectivity only)\n";
    std::cout << "  Limited to direct neighbors\n\n";
    
    std::cout << "With embedding bridge:\n";
    std::cout << "  LeapNode → Concept vector (centroid embedding)\n";
    std::cout << "  Activation vector includes leap concepts\n";
    std::cout << "  Token bias from SEMANTIC similarity\n";
    std::cout << "  Can influence tokens not directly connected!\n\n";
    
    // Simulate leap integration
    LeapNode mock_leap;
    mock_leap.activation = 0.7f;
    mock_leap.cohesion = 0.65f;
    mock_leap.concept_vector = std::vector<float>(64, 0.1f);  // Mock embedding
    
    std::vector<LeapNode> active_leaps = {mock_leap};
    
    std::cout << "Computing activation with leap:\n";
    auto leap_activation = bridge.compute_activation_with_leaps(
        active_nodes, activations, active_leaps);
    
    std::cout << "✓ Leap contribution added:\n";
    std::cout << "  • Total activation (with leap): " << leap_activation.total_activation << "\n";
    std::cout << "  • Leap boost: " << config.leap_embedding_boost << "x\n";
    
    std::cout << "\nPress Enter for final summary...\n";
    std::cin.get();
    
    // Final summary
    print_header("COMPLETE SYSTEM FLOW");
    
    std::cout << "The full prediction pipeline now includes:\n\n";
    
    std::cout << "1. Graph Reasoning\n";
    std::cout << "   ↓ Active nodes from beam search\n\n";
    
    std::cout << "2. Leap System (if uncertain)\n";
    std::cout << "   ↓ Generalized cluster concepts\n\n";
    
    std::cout << "3. 🌉 EMBEDDING BRIDGE 🌉\n";
    std::cout << "   ↓ Graph → Activation Vector\n";
    std::cout << "   ↓ Similarity → Token Bias\n\n";
    
    std::cout << "4. Predictive Model\n";
    std::cout << "   ↓ Markov probabilities + Graph bias + Leap bias\n\n";
    
    std::cout << "5. Token Sampling\n";
    std::cout << "   ↓ Coherent, semantically grounded output\n\n";
    
    std::cout << "6. Learning Feedback\n";
    std::cout << "   ↓ Align embeddings for future improvement\n\n";
    
    std::cout << "✨ RESULT:\n";
    std::cout << "  • Fluent like a language model\n";
    std::cout << "  • Grounded in graph semantics\n";
    std::cout << "  • Intelligent uncertainty handling\n";
    std::cout << "  • Self-improving through feedback\n\n";
    
    print_header("DEMONSTRATION COMPLETE");
    
    std::cout << "The embedding bridge is now fully integrated!\n\n";
    
    std::cout << "✅ Key capabilities:\n";
    std::cout << "  • Node-to-token semantic translation\n";
    std::cout << "  • Graph activation → prediction bias\n";
    std::cout << "  • Leap system enhancement\n";
    std::cout << "  • Continuous learning alignment\n";
    std::cout << "  • Toggleable via configuration\n\n";
    
    std::cout << "Next: Use this in actual generation!\n";
    std::cout << "  auto path = generate_path(context, config, \n";
    std::cout << "                           &leap_controller, &embedding_bridge);\n\n";
    
    return 0;
}

