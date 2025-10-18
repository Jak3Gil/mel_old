/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN CONTINUOUS LEARNING - INFINITE LOOP                               ║
 * ║  Run indefinitely, learning and improving continuously                    ║
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
#include <fstream>
#include <ctime>

using namespace melvin;

// Knowledge streams (can be expanded infinitely!)
std::vector<std::string> get_next_learning_batch(int batch_num) {
    // Rotate through different knowledge domains
    std::vector<std::vector<std::string>> knowledge_domains = {
        // Domain 1: Physics
        {
            "electricity flows through conductors",
            "conductors allow current",
            "current creates magnetism",
            "magnetism generates force",
            "force produces motion"
        },
        
        // Domain 2: Chemistry
        {
            "acids donate protons",
            "bases accept protons",
            "reactions change molecules",
            "catalysts speed reactions",
            "equilibrium balances forces"
        },
        
        // Domain 3: Psychology
        {
            "attention focuses mind",
            "focus improves performance",
            "motivation drives action",
            "habits form behavior",
            "learning changes brain"
        },
        
        // Domain 4: Economics
        {
            "supply meets demand",
            "price reflects value",
            "scarcity increases price",
            "trade creates wealth",
            "investment builds capital"
        },
        
        // Domain 5: Mathematics
        {
            "addition combines numbers",
            "multiplication scales values",
            "equations express relationships",
            "patterns reveal structure",
            "proofs verify truth"
        },
        
        // Domain 6: Philosophy
        {
            "questions seek answers",
            "reasoning builds arguments",
            "logic ensures validity",
            "ethics guides behavior",
            "meaning defines purpose"
        },
        
        // Domain 7: Engineering
        {
            "design solves problems",
            "testing validates solutions",
            "iteration improves quality",
            "optimization maximizes efficiency",
            "systems integrate components"
        }
    };
    
    // Cycle through domains
    int domain_idx = batch_num % knowledge_domains.size();
    return knowledge_domains[domain_idx];
}

std::string get_domain_name(int batch_num) {
    std::vector<std::string> names = {
        "Physics", "Chemistry", "Psychology", "Economics",
        "Mathematics", "Philosophy", "Engineering"
    };
    return names[batch_num % names.size()];
}

void print_progress_bar(int current, int total, const std::string& label) {
    int bar_width = 40;
    float progress = (float)current / total;
    int pos = bar_width * progress;
    
    std::cout << label << " [";
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) std::cout << "█";
        else std::cout << "░";
    }
    std::cout << "] " << std::setw(3) << (int)(progress * 100) << "%\r";
    std::cout.flush();
}

int main(int argc, char** argv) {
    
    // Parse command line arguments
    int max_epochs = 0;  // 0 = infinite
    bool save_checkpoints = true;
    int save_interval = 10;
    
    if (argc > 1) {
        max_epochs = std::atoi(argv[1]);
    }
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                               ║\n";
    std::cout << "║  🧠 MELVIN CONTINUOUS LEARNING - INFINITE MODE                ║\n";
    std::cout << "║                                                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    if (max_epochs == 0) {
        std::cout << "⚡ Running in INFINITE mode - will run until interrupted\n";
        std::cout << "   Press Ctrl+C to stop gracefully\n\n";
    } else {
        std::cout << "⚡ Running for " << max_epochs << " epochs\n\n";
    }
    
    std::cout << "Features:\n";
    std::cout << "  • Real-time fact learning\n";
    std::cout << "  • Automatic LEAP discovery\n";
    std::cout << "  • Episodic memory tracking\n";
    std::cout << "  • GNN embedding updates\n";
    std::cout << "  • Periodic checkpoints\n";
    std::cout << "  • Live statistics\n\n";
    
    std::cout << "Press Enter to start continuous learning...";
    std::cin.get();
    
    // Initialize components
    std::cout << "\n🔧 Initializing Melvin...\n";
    
    auto storage = std::make_unique<Storage>();
    storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    std::cout << "  ✓ Storage loaded\n";
    
    auto learning = std::make_unique<LearningSystem>(storage.get());
    std::cout << "  ✓ Learning system ready\n";
    
    episodic::EpisodicMemory::Config ep_config;
    ep_config.verbose = false;
    auto episodes = std::make_unique<episodic::EpisodicMemory>(storage.get(), ep_config);
    std::cout << "  ✓ Episodic memory initialized\n";
    
    gnn::GNNPredictor::Config gnn_config;
    gnn_config.embed_dim = 16;
    gnn_config.verbose = false;
    auto gnn = std::make_unique<gnn::GNNPredictor>(gnn_config);
    std::cout << "  ✓ GNN predictor ready\n";
    
    auto reasoning = std::make_unique<ReasoningEngine>();
    std::cout << "  ✓ Reasoning engine loaded\n\n";
    
    // Initialize GNN
    auto all_nodes = storage->get_all_nodes();
    gnn->initialize_embeddings(all_nodes);
    
    // Open log file
    std::ofstream log_file("continuous_learning.log", std::ios::app);
    auto start_time = std::chrono::system_clock::now();
    
    // Main continuous learning loop
    int epoch = 0;
    int total_leaps_created = 0;
    int total_facts_learned = 0;
    
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  CONTINUOUS LEARNING STARTED                                  ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    while (max_epochs == 0 || epoch < max_epochs) {
        epoch++;
        
        // Get next batch of facts
        auto facts = get_next_learning_batch(epoch - 1);
        std::string domain = get_domain_name(epoch - 1);
        
        // Create episode
        std::string context = domain + " (Epoch " + std::to_string(epoch) + ")";
        uint32_t episode_id = episodes->create_episode(context);
        
        // Learn facts
        std::cout << "📚 Epoch " << std::setw(4) << epoch << " | " << std::setw(12) << domain 
                  << " | Learning " << facts.size() << " facts...\n";
        
        int learned = 0;
        for (const auto& fact : facts) {
            if (learning->teach_fact(fact, domain)) {
                learned++;
                total_facts_learned++;
            }
        }
        
        episodes->end_episode(episode_id);
        
        // Update GNN every epoch
        all_nodes = storage->get_all_nodes();
        auto all_edges = storage->get_all_edges();
        gnn->message_pass(all_nodes, all_edges, storage.get());
        
        // Create LEAPs every 5 epochs (or if lots of new facts)
        if (epoch % 5 == 0) {
            std::cout << "  🧠 Discovering patterns...\n";
            
            leap::LEAPInference::Config leap_config;
            leap_config.max_transitive_hops = 5;
            leap_config.min_shared_neighbors = 2;
            leap_config.leap_confidence = 0.7f;
            leap_config.verbose = false;
            
            leap::LEAPInference leap_system(leap_config);
            int leaps = leap_system.create_leap_connections(storage.get());
            total_leaps_created += leaps;
            
            if (leaps > 0) {
                std::cout << "  ✨ Created " << leaps << " LEAP shortcuts!\n";
            }
        }
        
        // Display compact stats every epoch
        size_t nodes = storage->node_count();
        size_t edges = storage->edge_count();
        size_t exact = storage->edge_count_by_type(RelationType::EXACT);
        size_t leap = storage->edge_count_by_type(RelationType::LEAP);
        
        std::cout << "  📊 Nodes: " << std::setw(4) << nodes 
                  << " | Edges: " << std::setw(4) << edges
                  << " (EXACT: " << std::setw(3) << exact 
                  << ", LEAP: " << std::setw(3) << leap << ")\n";
        
        // Save checkpoint periodically
        if (epoch % save_interval == 0 && save_checkpoints) {
            storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
            episodes->save("melvin/data/episodes.melvin");
            std::cout << "  💾 Checkpoint saved\n";
        }
        
        // Log to file
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        log_file << epoch << "," << nodes << "," << edges << "," 
                 << exact << "," << leap << "," << domain << ","
                 << std::ctime(&now_time);
        log_file.flush();
        
        // Summary every 10 epochs
        if (epoch % 10 == 0) {
            std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
            std::cout << "  MILESTONE: Epoch " << epoch << "\n";
            std::cout << "  Total facts learned: " << total_facts_learned << "\n";
            std::cout << "  Total LEAPs created: " << total_leaps_created << "\n";
            std::cout << "  Knowledge base: " << nodes << " nodes, " << edges << " edges\n";
            std::cout << "  LEAP rate: " << std::fixed << std::setprecision(2) 
                      << (exact > 0 ? (float)leap / exact : 0.0f) << " LEAPs per EXACT\n";
            
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                now - start_time
            ).count();
            std::cout << "  Time running: " << duration << " seconds\n";
            std::cout << "  Learning rate: " << (duration > 0 ? total_facts_learned / duration : 0) 
                      << " facts/second\n";
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
        }
        
        // Small delay to make it visible (remove for max speed)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Final save
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  CONTINUOUS LEARNING SESSION COMPLETE                         ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    episodes->save("melvin/data/episodes.melvin");
    
    std::cout << "📊 FINAL STATISTICS\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    storage->print_stats();
    episodes->print_stats();
    
    std::cout << "✨ SESSION SUMMARY\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  Total epochs: " << epoch << "\n";
    std::cout << "  Facts learned: " << total_facts_learned << "\n";
    std::cout << "  LEAPs created: " << total_leaps_created << "\n";
    
    auto end_time = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        end_time - start_time
    ).count();
    std::cout << "  Time: " << duration << " seconds\n";
    std::cout << "  Rate: " << (duration > 0 ? total_facts_learned / duration : 0) 
              << " facts/second\n\n";
    
    std::cout << "💾 All knowledge saved to melvin/data/\n";
    std::cout << "📝 Log saved to continuous_learning.log\n\n";
    
    std::cout << "🚀 Melvin is smarter than when he started!\n\n";
    
    return 0;
}

