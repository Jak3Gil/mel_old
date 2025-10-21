/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN CONTINUOUS LEARNING - LIVE DEMONSTRATION                          ║
 * ║  Watch Melvin learn and grow in real-time                                 ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "melvin/core/storage.h"
#include "melvin/core/learning.h"
#include "melvin/core/leap_inference.h"
#include "melvin/core/episodic_memory.h"
#include "melvin/core/gnn_predictor.h"
#include "melvin/core/reasoning.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>

using namespace melvin;

void clear_screen() {
    std::cout << "\033[2J\033[1;1H";
}

void print_header(int epoch, int total_epochs) {
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN CONTINUOUS LEARNING - LIVE                            ║\n";
    std::cout << "║  Epoch " << std::setw(3) << epoch << "/" << total_epochs << "                                                   ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
}

void print_stats(Storage* storage, episodic::EpisodicMemory* episodes, int leaps_created) {
    size_t nodes = storage->node_count();
    size_t edges = storage->edge_count();
    size_t exact = storage->edge_count_by_type(RelationType::EXACT);
    size_t leap = storage->edge_count_by_type(RelationType::LEAP);
    
    auto ep_stats = episodes->get_stats();
    
    std::cout << "📊 KNOWLEDGE BASE\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  Concepts (Nodes):    " << std::setw(6) << nodes << "\n";
    std::cout << "  Total Connections:   " << std::setw(6) << edges << "\n";
    std::cout << "    → EXACT (taught):  " << std::setw(6) << exact << " [confidence: 1.0]\n";
    std::cout << "    → LEAP (inferred): " << std::setw(6) << leap << " [confidence: 0.7]\n";
    std::cout << "  Knowledge Density:   " << std::setw(6) << std::fixed << std::setprecision(2) 
              << (nodes > 0 ? (float)edges / nodes : 0.0f) << " edges/node\n";
    std::cout << "  Storage Size:        " << std::setw(6) 
              << (nodes * 4 + edges * 8) / 1024.0f << " KB\n";
    std::cout << "\n";
    
    std::cout << "🧠 EPISODIC MEMORY\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  Episodes Created:    " << std::setw(6) << ep_stats.total_episodes << "\n";
    std::cout << "  Active Episodes:     " << std::setw(6) << ep_stats.active_episodes << "\n";
    std::cout << "  Avg Episode Size:    " << std::setw(6) << std::fixed << std::setprecision(1)
              << ep_stats.avg_episode_size << " nodes\n";
    std::cout << "  Total Nodes Tracked: " << std::setw(6) << ep_stats.total_nodes_in_episodes << "\n";
    std::cout << "\n";
    
    std::cout << "⚡ LEARNING ACTIVITY\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  LEAPs Created This Session: " << std::setw(6) << leaps_created << "\n";
    std::cout << "  LEAP Formation Rate:        " << std::setw(6) << std::fixed << std::setprecision(2)
              << (exact > 0 ? (float)leap / exact : 0.0f) << " LEAPs/EXACT\n";
    std::cout << "\n";
}

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                               ║\n";
    std::cout << "║  🧠 MELVIN CONTINUOUS LEARNING DEMONSTRATION                  ║\n";
    std::cout << "║                                                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "This demo will:\n";
    std::cout << "  1. Load existing knowledge base\n";
    std::cout << "  2. Feed Melvin new facts continuously\n";
    std::cout << "  3. Create LEAP connections automatically\n";
    std::cout << "  4. Track episodic memories\n";
    std::cout << "  5. Show real-time statistics\n\n";
    
    std::cout << "Press Enter to start...";
    std::cin.get();
    
    // Initialize components
    auto storage = std::make_unique<Storage>();
    storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    
    auto learning = std::make_unique<LearningSystem>(storage.get());
    
    episodic::EpisodicMemory::Config ep_config;
    ep_config.verbose = false;
    auto episodes = std::make_unique<episodic::EpisodicMemory>(storage.get(), ep_config);
    
    gnn::GNNPredictor::Config gnn_config;
    gnn_config.embed_dim = 16;
    gnn_config.verbose = false;
    auto gnn = std::make_unique<gnn::GNNPredictor>(gnn_config);
    
    auto reasoning = std::make_unique<ReasoningEngine>();
    
    // Training data streams (simulating continuous input)
    std::vector<std::vector<std::string>> learning_streams = {
        // Stream 1: Advanced physics
        {
            "light travels fast",
            "fast means quick",
            "quick saves time",
            "electricity powers devices",
            "devices use energy",
            "energy comes from power"
        },
        
        // Stream 2: Advanced biology
        {
            "cells form tissue",
            "tissue forms organs",
            "organs form systems",
            "DNA contains genes",
            "genes determine traits",
            "traits define characteristics"
        },
        
        // Stream 3: Social concepts
        {
            "communication uses language",
            "language conveys meaning",
            "meaning creates understanding",
            "cooperation enables teamwork",
            "teamwork achieves goals",
            "goals drive progress"
        },
        
        // Stream 4: Technology
        {
            "computers process data",
            "data represents information",
            "information contains knowledge",
            "algorithms solve problems",
            "problems require solutions",
            "solutions create value"
        },
        
        // Stream 5: More connections
        {
            "temperature affects state",
            "state changes matter",
            "pressure influences behavior",
            "time measures change",
            "change indicates evolution",
            "evolution drives adaptation"
        }
    };
    
    int total_epochs = learning_streams.size();
    int total_leaps_created = 0;
    
    // Initialize GNN embeddings
    auto all_nodes = storage->get_all_nodes();
    gnn->initialize_embeddings(all_nodes);
    
    // Main learning loop
    for (int epoch = 0; epoch < total_epochs; epoch++) {
        clear_screen();
        print_header(epoch + 1, total_epochs);
        
        // Create episode for this learning session
        std::string context = "Learning stream " + std::to_string(epoch + 1);
        uint32_t episode_id = episodes->create_episode(context);
        
        std::cout << "📖 Learning new facts...\n\n";
        
        // Teach facts from current stream
        for (const auto& fact : learning_streams[epoch]) {
            if (learning->teach_fact(fact, context)) {
                std::cout << "  ✓ " << fact << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
        
        episodes->end_episode(episode_id);
        
        std::cout << "\n🔬 Running GNN message passing...\n";
        all_nodes = storage->get_all_nodes();
        auto all_edges = storage->get_all_edges();
        gnn->message_pass(all_nodes, all_edges, storage.get());
        
        std::cout << "✓ GNN updated with " << all_nodes.size() << " node embeddings\n\n";
        
        // Create LEAP connections every epoch
        if (epoch % 1 == 0) {  // Every epoch
            std::cout << "🧠 Discovering patterns and creating LEAPs...\n\n";
            
            leap::LEAPInference::Config leap_config;
            leap_config.max_transitive_hops = 5;
            leap_config.min_shared_neighbors = 2;
            leap_config.leap_confidence = 0.7f;
            leap_config.verbose = false;
            
            leap::LEAPInference leap_system(leap_config);
            int leaps = leap_system.create_leap_connections(storage.get());
            total_leaps_created += leaps;
            
            if (leaps > 0) {
                std::cout << "  ✨ Created " << leaps << " new LEAP connections!\n\n";
            }
        }
        
        // Display statistics
        std::cout << "\n";
        print_stats(storage.get(), episodes.get(), total_leaps_created);
        
        // Show sample reasoning
        if (epoch > 0) {
            std::cout << "🎯 SAMPLE REASONING\n";
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
            
            Query q;
            q.text = "What is fire?";
            auto answer = reasoning->infer(q, storage.get());
            
            std::cout << "  Query: \"" << q.text << "\"\n";
            std::cout << "  Paths found: " << answer.paths.size() << "\n";
            
            if (!answer.paths.empty()) {
                std::cout << "  Best path: " << answer.paths[0].hop_count() << " hops\n";
            }
            std::cout << "\n";
        }
        
        // Progress bar
        std::cout << "PROGRESS: [";
        int bar_width = 50;
        int pos = bar_width * (epoch + 1) / total_epochs;
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) std::cout << "█";
            else std::cout << "░";
        }
        std::cout << "] " << ((epoch + 1) * 100 / total_epochs) << "%\n\n";
        
        if (epoch < total_epochs - 1) {
            std::cout << "⏳ Next epoch in 2 seconds...\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    
    // Final summary
    clear_screen();
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  CONTINUOUS LEARNING COMPLETE!                                ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "📊 FINAL RESULTS\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    print_stats(storage.get(), episodes.get(), total_leaps_created);
    
    std::cout << "🎓 WHAT MELVIN LEARNED\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  • Advanced physics concepts\n";
    std::cout << "  • Biological structures and processes\n";
    std::cout << "  • Social and communication patterns\n";
    std::cout << "  • Technology and computation\n";
    std::cout << "  • Environmental dynamics\n\n";
    
    std::cout << "🌟 EMERGENT CONNECTIONS\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  Melvin autonomously discovered " << total_leaps_created << " LEAP shortcuts!\n";
    std::cout << "  These are inferred connections Melvin created on his own.\n\n";
    
    // Show some interesting paths
    std::cout << "🔍 MULTI-HOP REASONING EXAMPLES\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    struct PathExample {
        std::string from, to;
    };
    
    std::vector<PathExample> examples = {
        {"fire", "comfort"},
        {"light", "understanding"},
        {"energy", "value"},
        {"water", "life"}
    };
    
    for (const auto& ex : examples) {
        auto from_nodes = storage->find_nodes(ex.from);
        auto to_nodes = storage->find_nodes(ex.to);
        
        if (!from_nodes.empty() && !to_nodes.empty()) {
            auto paths = storage->find_paths(from_nodes[0].id, to_nodes[0].id, 10);
            
            if (!paths.empty()) {
                std::cout << "  " << ex.from << " → " << ex.to << ": ";
                std::cout << paths[0].hop_count() << " hops\n";
                
                std::cout << "    ";
                for (size_t i = 0; i < std::min(size_t(6), paths[0].nodes.size()); i++) {
                    std::cout << storage->get_node_content(paths[0].nodes[i]);
                    if (i < std::min(size_t(6), paths[0].nodes.size()) - 1) std::cout << " → ";
                    if (i == 5 && paths[0].nodes.size() > 6) std::cout << "...";
                }
                std::cout << "\n\n";
            }
        }
    }
    
    // Episode summary
    std::cout << "📚 EPISODES REMEMBERED\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    auto recent_episodes = episodes->recall_recent(5);
    for (const auto& ep : recent_episodes) {
        std::cout << "  Episode " << ep.id << ": " << ep.context 
                  << " (" << ep.size() << " nodes)\n";
    }
    std::cout << "\n";
    
    // Save final state
    std::cout << "💾 Saving knowledge base...\n";
    if (storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin")) {
        std::cout << "✅ Saved to melvin/data/\n";
    }
    
    if (episodes->save("melvin/data/episodes.melvin")) {
        std::cout << "✅ Episodes saved\n";
    }
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN HAS LEARNED AND GROWN!                                ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    auto final_ep_stats = episodes->get_stats();
    
    std::cout << "✨ Key Achievements:\n";
    std::cout << "  • Knowledge base expanded autonomously\n";
    std::cout << "  • " << total_leaps_created << " pattern shortcuts discovered\n";
    std::cout << "  • " << final_ep_stats.total_episodes << " temporal episodes created\n";
    std::cout << "  • Multi-hop reasoning chains established\n";
    std::cout << "  • All learning preserved in graph\n\n";
    
    std::cout << "🚀 Melvin is smarter than before - and it's all traceable!\n\n";
    
    return 0;
}

