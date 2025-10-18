/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN AUTONOMOUS LEARNING DEMO                                          ║
 * ║  Demonstrates the full LLM-level learning system                          ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "melvin/core/storage.h"
#include "melvin/core/episodic_memory.h"
#include "melvin/core/tokenizer.h"
#include "melvin/core/gnn_predictor.h"
#include "melvin/core/dataset_loader.h"
#include "melvin/core/leap_inference.h"
#include <iostream>
#include <vector>

using namespace melvin;

void print_banner(const std::string& text) {
    std::cout << "\n╔═══════════════════════════════════════════════╗\n";
    std::cout << "║  " << text;
    for (size_t i = text.length(); i < 45; i++) std::cout << " ";
    std::cout << "║\n";
    std::cout << "╚═══════════════════════════════════════════════╝\n\n";
}

int main() {
    print_banner("MELVIN AUTONOMOUS LEARNING DEMO");
    
    std::cout << "This demo shows Melvin's ability to:\n";
    std::cout << "  1. Load existing knowledge\n";
    std::cout << "  2. Create episodic memories\n";
    std::cout << "  3. Learn new information\n";
    std::cout << "  4. Generate LEAP connections\n";
    std::cout << "  5. Reason across multiple hops\n\n";
    
    // ========================================================================
    // 1. LOAD EXISTING KNOWLEDGE
    // ========================================================================
    
    print_banner("1. LOADING KNOWLEDGE BASE");
    
    auto storage = std::make_unique<Storage>();
    if (!storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin")) {
        std::cerr << "❌ Failed to load knowledge base\n";
        return 1;
    }
    
    std::cout << "✅ Knowledge base loaded successfully!\n";
    storage->print_stats();
    
    // ========================================================================
    // 2. INITIALIZE EPISODIC MEMORY
    // ========================================================================
    
    print_banner("2. EPISODIC MEMORY SYSTEM");
    
    episodic::EpisodicMemory::Config ep_config;
    ep_config.max_episodes = 1000;
    ep_config.verbose = true;
    
    auto episodes = std::make_unique<episodic::EpisodicMemory>(storage.get(), ep_config);
    
    std::cout << "✅ Episodic memory initialized\n\n";
    
    // Create some episodes
    std::cout << "Creating learning episodes...\n";
    
    uint32_t ep1 = episodes->create_episode("Learning about physics");
    auto fire_nodes = storage->find_nodes("fire");
    if (!fire_nodes.empty()) {
        episodes->add_node(ep1, fire_nodes[0].id);
    }
    episodes->end_episode(ep1);
    
    uint32_t ep2 = episodes->create_episode("Learning about biology");
    auto sun_nodes = storage->find_nodes("sun");
    if (!sun_nodes.empty()) {
        episodes->add_node(ep2, sun_nodes[0].id);
    }
    episodes->end_episode(ep2);
    
    episodes->print_stats();
    
    // ========================================================================
    // 3. TOKENIZER SETUP
    // ========================================================================
    
    print_banner("3. TOKENIZER");
    
    tokenizer::Tokenizer::Config tok_config;
    tok_config.vocab_size = 5000;
    tok_config.verbose = false;
    
    auto tokenizer = std::make_unique<tokenizer::Tokenizer>(tok_config);
    
    // Train on some sample text
    std::vector<std::string> training_corpus = {
        "fire produces heat and warmth",
        "the sun is a star that produces light",
        "plants need water and sunlight to grow",
        "animals need oxygen and food for energy"
    };
    
    std::cout << "Training tokenizer on corpus...\n";
    tokenizer->train_from_corpus(training_corpus);
    
    std::cout << "✅ Tokenizer ready with " << tokenizer->vocab_size() << " tokens\n\n";
    
    // Test tokenization
    std::string test_text = "fire produces heat";
    auto tokens = tokenizer->encode(test_text);
    std::cout << "Test: \"" << test_text << "\"\n";
    std::cout << "  Tokens: " << tokens.size() << " [";
    for (size_t i = 0; i < std::min(size_t(5), tokens.size()); i++) {
        std::cout << tokens[i];
        if (i < tokens.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";
    std::cout << "  Decoded: \"" << tokenizer->decode(tokens) << "\"\n\n";
    
    // ========================================================================
    // 4. GNN PREDICTOR
    // ========================================================================
    
    print_banner("4. GRAPH NEURAL NETWORK");
    
    gnn::GNNPredictor::Config gnn_config;
    gnn_config.embed_dim = 16;
    gnn_config.learning_rate = 0.01f;
    gnn_config.verbose = false;
    
    auto gnn = std::make_unique<gnn::GNNPredictor>(gnn_config);
    
    std::cout << "✅ GNN predictor initialized\n";
    std::cout << "  Embedding dimension: " << gnn_config.embed_dim << "\n";
    std::cout << "  Learning rate: " << gnn_config.learning_rate << "\n\n";
    
    // Initialize embeddings
    auto all_nodes = storage->get_all_nodes();
    std::cout << "Initializing embeddings for " << all_nodes.size() << " nodes...\n";
    gnn->initialize_embeddings(all_nodes);
    
    // Run message passing
    std::cout << "Running message passing...\n";
    auto all_edges = storage->get_all_edges();
    gnn->message_pass(all_nodes, all_edges, storage.get());
    
    // Predict activations
    auto predictions = gnn->predict_activations(all_nodes);
    std::cout << "✅ Predicted activations for " << predictions.size() << " nodes\n\n";
    
    // ========================================================================
    // 5. DATASET LOADER
    // ========================================================================
    
    print_banner("5. DATASET INGESTION");
    
    dataset::DatasetLoader::Config loader_config;
    loader_config.batch_size = 10;
    loader_config.create_episodes = true;
    loader_config.verbose = true;
    
    auto loader = std::make_unique<dataset::DatasetLoader>(
        storage.get(), episodes.get(), tokenizer.get(), loader_config
    );
    
    std::cout << "✅ Dataset loader initialized\n\n";
    
    // Ingest some new knowledge
    std::cout << "Ingesting new knowledge...\n";
    std::vector<std::string> new_facts = {
        "Learning is a continuous process.",
        "Knowledge grows through experience.",
        "Understanding requires practice.",
        "Wisdom comes from reflection."
    };
    
    loader->ingest_batch(new_facts);
    loader->print_stats();
    
    // ========================================================================
    // 6. AUTONOMOUS LEAP CREATION
    // ========================================================================
    
    print_banner("6. AUTONOMOUS LEAP GENERATION");
    
    std::cout << "Melvin is now analyzing the knowledge graph...\n";
    std::cout << "Looking for patterns and creating shortcuts...\n\n";
    
    leap::LEAPInference::Config leap_config;
    leap_config.max_transitive_hops = 5;
    leap_config.min_shared_neighbors = 2;
    leap_config.leap_confidence = 0.7f;
    leap_config.verbose = true;
    
    leap::LEAPInference leap_system(leap_config);
    int new_leaps = leap_system.create_leap_connections(storage.get());
    
    std::cout << "\n✅ Created " << new_leaps << " additional LEAP connections!\n\n";
    
    // ========================================================================
    // 7. REASONING DEMONSTRATIONS
    // ========================================================================
    
    print_banner("7. MULTI-HOP REASONING");
    
    struct ReasoningTest {
        std::string from;
        std::string to;
        std::string description;
    };
    
    std::vector<ReasoningTest> tests = {
        {"fire", "comfort", "Physics chain"},
        {"sun", "life", "Biology chain"},
        {"exercise", "wellbeing", "Health chain"},
        {"reading", "wisdom", "Knowledge chain"}
    };
    
    for (const auto& test : tests) {
        auto from_nodes = storage->find_nodes(test.from);
        auto to_nodes = storage->find_nodes(test.to);
        
        if (!from_nodes.empty() && !to_nodes.empty()) {
            auto paths = storage->find_paths(from_nodes[0].id, to_nodes[0].id, 10);
            
            if (!paths.empty()) {
                std::cout << "✓ " << test.from << " → " << test.to 
                          << " (" << test.description << ")\n";
                std::cout << "  " << paths[0].hop_count() << " hops";
                
                // Count EXACT vs LEAP
                int exact = 0, leap = 0;
                for (auto edge_id : paths[0].edges) {
                    Edge e;
                    if (storage->get_edge(edge_id, e)) {
                        if (e.relation == RelationType::EXACT) exact++;
                        else leap++;
                    }
                }
                std::cout << " (" << exact << " EXACT + " << leap << " LEAP)\n";
                
                // Show path
                std::cout << "  ";
                for (size_t i = 0; i < paths[0].nodes.size(); i++) {
                    std::cout << storage->get_node_content(paths[0].nodes[i]);
                    if (i < paths[0].nodes.size() - 1) std::cout << " → ";
                }
                std::cout << "\n\n";
            } else {
                std::cout << "✗ " << test.from << " → " << test.to 
                          << ": No path found\n\n";
            }
        }
    }
    
    // ========================================================================
    // 8. FINAL STATISTICS
    // ========================================================================
    
    print_banner("8. FINAL STATISTICS");
    
    std::cout << "Knowledge Base:\n";
    storage->print_stats();
    
    std::cout << "\nEpisodic Memory:\n";
    episodes->print_stats();
    
    std::cout << "\nGNN Predictor:\n";
    auto gnn_stats = gnn->get_stats();
    std::cout << "  Training steps: " << gnn_stats.training_steps << "\n";
    std::cout << "  LEAPs created from errors: " << gnn_stats.leaps_created << "\n";
    
    std::cout << "\nDataset Loader:\n";
    auto ds_stats = loader->get_stats();
    std::cout << "  Texts ingested: " << ds_stats.total_texts << "\n";
    std::cout << "  Tokens processed: " << ds_stats.total_tokens << "\n";
    std::cout << "  Nodes created: " << ds_stats.nodes_created << "\n";
    std::cout << "  Episodes created: " << ds_stats.episodes_created << "\n";
    
    // ========================================================================
    // 9. WHAT MAKES MELVIN BETTER
    // ========================================================================
    
    print_banner("MELVIN'S ADVANTAGES");
    
    std::cout << "✓ Traceable Reasoning\n";
    std::cout << "  Every prediction has a graph path\n\n";
    
    std::cout << "✓ EXACT vs LEAP Awareness\n";
    std::cout << "  " << storage->edge_count_by_type(RelationType::EXACT) << " EXACT (facts), "
              << storage->edge_count_by_type(RelationType::LEAP) << " LEAP (guesses)\n\n";
    
    std::cout << "✓ Episodic Memory\n";
    std::cout << "  Can recall when things were learned\n\n";
    
    std::cout << "✓ Autonomous Learning\n";
    std::cout << "  Continuously creates new LEAP connections\n\n";
    
    std::cout << "✓ Multi-Hop Reasoning\n";
    std::cout << "  Chains facts across 10+ hops\n\n";
    
    std::cout << "✓ No Hallucinations\n";
    std::cout << "  Graph constrains all outputs\n\n";
    
    print_banner("AUTONOMOUS LEARNING ACTIVE");
    
    std::cout << "Melvin is now ready for:\n";
    std::cout << "  • Continuous learning from datasets\n";
    std::cout << "  • Self-directed knowledge gap exploration\n";
    std::cout << "  • Graph-constrained text generation\n";
    std::cout << "  • Hybrid reasoning (graph + sequence)\n\n";
    
    std::cout << "Next steps:\n";
    std::cout << "  1. Train on larger datasets (Wikipedia)\n";
    std::cout << "  2. Enable full transformer for generation\n";
    std::cout << "  3. Deploy for real-world learning\n\n";
    
    return 0;
}

