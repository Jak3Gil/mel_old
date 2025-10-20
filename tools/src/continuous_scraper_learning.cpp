/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN CONTINUOUS LEARNING WITH INTELLIGENT WEB SCRAPING                 ║
 * ║  Diverse sources → Quality facts → Real-time learning                     ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "melvin/core/storage.h"
#include "melvin/core/learning.h"
#include "melvin/core/leap_inference.h"
#include "melvin/core/episodic_memory.h"
#include "melvin/core/gnn_predictor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>
#include <cstdlib>

using namespace melvin;

void parse_and_add_fact(Storage* storage, const std::string& fact, bool verbose = false) {
    std::istringstream iss(fact);
    std::vector<std::string> words;
    std::string word;
    
    while (iss >> word) {
        // Clean word
        std::string clean;
        for (char c : word) {
            if (std::isalnum(c) || c == '_') {
                clean += std::tolower(c);
            }
        }
        if (clean.length() >= 2) {
            words.push_back(clean);
        }
    }
    
    if (words.size() < 2) return;
    
    // Show fact if verbose
    if (verbose) {
        std::cout << "    📝 ";
        if (fact.length() > 70) {
            std::cout << fact.substr(0, 70) << "...";
        } else {
            std::cout << fact;
        }
        std::cout << std::endl;
    }
    
    // Create chain
    NodeID prev_id = 0;
    for (const auto& w : words) {
        auto nodes = storage->find_nodes(w);
        NodeID node_id = nodes.empty() ? 
            storage->create_node(w, NodeType::CONCEPT) : nodes[0].id;
        
        if (prev_id != 0) {
            storage->create_edge(prev_id, node_id, RelationType::EXACT, 1.0f);
        }
        prev_id = node_id;
    }
}

int main(int argc, char** argv) {
    int max_cycles = 0;  // 0 = infinite
    int facts_per_cycle = 15;
    int leap_interval = 10;
    int save_interval = 20;
    bool show_facts = true;
    
    if (argc > 1) max_cycles = std::atoi(argv[1]);
    if (argc > 2) facts_per_cycle = std::atoi(argv[2]);
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                               ║\n";
    std::cout << "║  🕷️  INTELLIGENT WEB SCRAPING + CONTINUOUS LEARNING           ║\n";
    std::cout << "║                                                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    if (max_cycles == 0) {
        std::cout << "⚡ INFINITE MODE - Runs until Ctrl+C\n";
    } else {
        std::cout << "⚡ Running for " << max_cycles << " cycles\n";
    }
    
    std::cout << "\nConfiguration:\n";
    std::cout << "  • Facts per cycle: " << facts_per_cycle << "\n";
    std::cout << "  • LEAP inference: Every " << leap_interval << " cycles\n";
    std::cout << "  • Auto-save: Every " << save_interval << " cycles\n";
    std::cout << "  • Show facts: " << (show_facts ? "YES" : "NO") << "\n\n";
    
    std::cout << "Sources:\n";
    std::cout << "  ✓ Random Wikipedia articles (diverse topics)\n";
    std::cout << "  ✓ Simple Wikipedia (simpler language)\n";
    std::cout << "  ✓ Wiktionary (definitions)\n";
    std::cout << "  ✓ High-quality synthetic (fallback)\n\n";
    
    std::cout << "Press Enter to start continuous learning...";
    std::cin.get();
    
    // Initialize
    std::cout << "\n🔧 Initializing Melvin...\n";
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
    
    auto nodes = storage->get_all_nodes();
    gnn->initialize_embeddings(nodes);
    
    std::cout << "  ✓ All systems ready\n";
    std::cout << "  Starting nodes: " << storage->node_count() << "\n";
    std::cout << "  Starting edges: " << storage->edge_count() << "\n\n";
    
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  INTELLIGENT SCRAPING STARTED                                 ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    auto start_time = std::chrono::steady_clock::now();
    int total_facts = 0;
    int total_leaps = 0;
    size_t start_nodes = storage->node_count();
    size_t start_edges = storage->edge_count();
    
    for (int cycle = 1; max_cycles == 0 || cycle <= max_cycles; cycle++) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", localtime(&time));
        
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "[" << time_str << "] 🕷️  Cycle " << std::setw(4) << cycle << "\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
        
        // Create episode
        uint32_t episode_id = episodes->create_episode("Scraped Knowledge Cycle " + std::to_string(cycle));
        
        // Run intelligent scraper (Python)
        std::cout << "🌐 Scraping diverse sources...\n";
        std::string scraper_cmd = "python3 intelligent_scraper.py " + std::to_string(facts_per_cycle);
        int scraper_result = system(scraper_cmd.c_str());
        
        if (scraper_result != 0) {
            std::cout << "  ⚠ Scraper issue, will use synthetic facts\n";
        }
        
        // Read fetched facts
        std::ifstream fact_file("internet_facts.txt");
        std::vector<std::string> facts;
        std::string line;
        
        while (std::getline(fact_file, line)) {
            if (!line.empty() && line.length() > 10) {
                facts.push_back(line);
            }
        }
        
        if (facts.empty()) {
            std::cout << "  ⚠ No facts fetched, skipping cycle\n\n";
            continue;
        }
        
        std::cout << "  ✓ Fetched " << facts.size() << " facts\n\n";
        
        // Learn facts
        if (show_facts) {
            std::cout << "  🧠 Learning:\n";
        }
        
        for (const auto& fact : facts) {
            if (show_facts) {
                parse_and_add_fact(storage.get(), fact, true);
            } else {
                parse_and_add_fact(storage.get(), fact, false);
            }
            total_facts++;
        }
        
        episodes->end_episode(episode_id);
        
        // Update GNN
        auto all_nodes = storage->get_all_nodes();
        auto all_edges = storage->get_all_edges();
        gnn->message_pass(all_nodes, all_edges, storage.get());
        
        // LEAP inference periodically
        if (cycle % leap_interval == 0) {
            std::cout << "\n  🧠 Discovering patterns (LEAP inference)...\n";
            
            leap::LEAPInference::Config leap_config;
            leap_config.max_transitive_hops = 4;  // Reduced for speed
            leap_config.min_shared_neighbors = 2;
            leap_config.leap_confidence = 0.7f;
            leap_config.verbose = false;
            
            leap::LEAPInference leap_system(leap_config);
            int leaps = leap_system.create_leap_connections(storage.get());
            total_leaps += leaps;
            
            if (leaps > 0) {
                std::cout << "    ✨ Created " << leaps << " new LEAP shortcuts\n";
            }
        }
        
        // Display stats
        size_t current_nodes = storage->node_count();
        size_t current_edges = storage->edge_count();
        size_t exact_edges = storage->edge_count_by_type(RelationType::EXACT);
        size_t leap_edges = storage->edge_count_by_type(RelationType::LEAP);
        
        std::cout << "\n  📊 Knowledge: " << current_nodes << " nodes, " 
                  << current_edges << " edges (EXACT: " << exact_edges 
                  << ", LEAP: " << leap_edges << ")\n";
        std::cout << "      Growth: +" << (current_nodes - start_nodes) << " nodes, +"
                  << (current_edges - start_edges) << " edges\n";
        
        // Save checkpoint
        if (cycle % save_interval == 0) {
            std::cout << "\n  💾 Saving checkpoint...";
            storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
            episodes->save("melvin/data/episodes.melvin");
            std::cout << " ✓\n";
        }
        
        // Milestone report
        if (cycle % 50 == 0) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - start_time
            ).count();
            
            std::cout << "\n  ╔═══════════════════════════════════════════════════════════╗\n";
            std::cout << "  ║  MILESTONE: Cycle " << cycle << std::setfill(' ') << std::setw(4) << "                                       ║\n";
            std::cout << "  ╚═══════════════════════════════════════════════════════════╝\n";
            std::cout << "    Total facts learned: " << total_facts << "\n";
            std::cout << "    Total LEAPs created: " << total_leaps << "\n";
            std::cout << "    Node growth: " << (current_nodes - start_nodes) 
                      << " (" << (100 * (current_nodes - start_nodes) / start_nodes) << "%)\n";
            std::cout << "    Edge growth: " << (current_edges - start_edges)
                      << " (" << (100 * (current_edges - start_edges) / start_edges) << "%)\n";
            std::cout << "    Time running: " << elapsed << "s (" << (elapsed / 60) << "m)\n";
            std::cout << "    Learning rate: " << (elapsed > 0 ? total_facts / elapsed : 0) 
                      << " facts/sec\n";
            std::cout << "  ═══════════════════════════════════════════════════════════\n";
        }
        
        std::cout << "\n";
        
        // Delay between cycles (2 seconds for scraping)
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    // Final save and summary
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  INTELLIGENT SCRAPING SESSION COMPLETE                        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    episodes->save("melvin/data/episodes.melvin");
    
    auto end_time = std::chrono::steady_clock::now();
    auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        end_time - start_time
    ).count();
    
    size_t final_nodes = storage->node_count();
    size_t final_edges = storage->edge_count();
    
    std::cout << "📊 FINAL STATISTICS\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    storage->print_stats();
    
    std::cout << "\n✨ SESSION SUMMARY\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  Cycles: " << (max_cycles > 0 ? max_cycles : total_facts / facts_per_cycle) << "\n";
    std::cout << "  Facts learned: " << total_facts << "\n";
    std::cout << "  LEAPs created: " << total_leaps << "\n";
    std::cout << "  Node growth: " << (final_nodes - start_nodes) 
              << " (" << (100 * (final_nodes - start_nodes) / start_nodes) << "%)\n";
    std::cout << "  Edge growth: " << (final_edges - start_edges)
              << " (" << (100 * (final_edges - start_edges) / start_edges) << "%)\n";
    std::cout << "  Time: " << total_elapsed << "s (" << (total_elapsed / 60) << " minutes)\n";
    std::cout << "  Rate: " << (total_elapsed > 0 ? (double)total_facts / total_elapsed : 0) 
              << " facts/sec\n\n";
    
    std::cout << "💾 All knowledge saved to melvin/data/\n\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN IS SMARTER!                                           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    return 0;
}

