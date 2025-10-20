/*
 * Feed dictionary to Melvin with VERBOSE real-time output
 * Shows every fact being learned + periodic stats
 */

#include "melvin/core/storage.h"
#include "melvin/core/learning.h"
#include "melvin/core/leap_inference.h"
#include "melvin/core/episodic_memory.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <iomanip>

using namespace melvin;

void parse_and_add_fact(Storage* storage, const std::string& fact, int fact_num) {
    std::istringstream iss(fact);
    std::vector<std::string> words;
    std::string word;
    
    while (iss >> word) {
        words.push_back(word);
    }
    
    if (words.size() < 2) return;
    
    // Show the fact being learned
    if (fact_num % 100 == 0 || fact_num < 20) {
        std::cout << "  [" << std::setw(5) << fact_num << "] ";
        if (fact.length() > 60) {
            std::cout << fact.substr(0, 60) << "...";
        } else {
            std::cout << fact;
        }
        std::cout << std::endl;
    } else if (fact_num % 10 == 0) {
        std::cout << "  [" << std::setw(5) << fact_num << "] Learning..." << std::endl;
    }
    
    // Create chain of concepts
    NodeID prev_id = 0;
    
    for (size_t i = 0; i < words.size(); i++) {
        std::string current_word = words[i];
        
        // Clean up word
        current_word.erase(std::remove_if(current_word.begin(), current_word.end(), 
                     [](char c) { return !std::isalnum(c) && c != '_'; }), 
                     current_word.end());
        
        if (current_word.empty() || current_word.length() < 2) continue;
        
        // Find or create node
        auto nodes = storage->find_nodes(current_word);
        NodeID node_id;
        
        if (nodes.empty()) {
            node_id = storage->create_node(current_word, NodeType::CONCEPT);
        } else {
            node_id = nodes[0].id;
        }
        
        // Create edge from previous word
        if (prev_id != 0) {
            storage->create_edge(prev_id, node_id, RelationType::EXACT, 1.0f);
        }
        
        prev_id = node_id;
    }
}

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  FEEDING DICTIONARY TO MELVIN (VERBOSE MODE)                  ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    // Load existing knowledge base
    auto storage = std::make_unique<Storage>();
    storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    
    std::cout << "📊 Starting knowledge base:\n";
    std::cout << "  Nodes: " << storage->node_count() << "\n";
    std::cout << "  Edges: " << storage->edge_count() << "\n\n";
    
    // Read facts from file
    std::cout << "📖 Reading facts from internet_facts.txt...\n\n";
    
    std::ifstream fact_file("internet_facts.txt");
    if (!fact_file) {
        std::cerr << "❌ Could not open internet_facts.txt\n";
        return 1;
    }
    
    std::vector<std::string> facts;
    std::string line;
    
    while (std::getline(fact_file, line)) {
        if (!line.empty() && line.length() > 10 && line[0] != '#') {
            facts.push_back(line);
        }
    }
    
    std::cout << "  ✓ Loaded " << facts.size() << " facts\n\n";
    
    // Setup episodic memory
    episodic::EpisodicMemory::Config ep_config;
    ep_config.verbose = false;
    auto episodes = std::make_unique<episodic::EpisodicMemory>(storage.get(), ep_config);
    uint32_t episode_id = episodes->create_episode("Dictionary ingestion");
    
    // Learn facts with progress display
    std::cout << "🧠 Learning facts (showing every 10th):\n\n";
    
    auto start_time = std::chrono::steady_clock::now();
    size_t nodes_before = storage->node_count();
    size_t edges_before = storage->edge_count();
    
    for (size_t i = 0; i < facts.size(); i++) {
        parse_and_add_fact(storage.get(), facts[i], i + 1);
        
        // Show progress every 100 facts
        if ((i + 1) % 100 == 0) {
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
            float rate = elapsed > 0 ? (i + 1) / (float)elapsed : 0;
            
            std::cout << "\n  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
            std::cout << "  📊 Progress: " << (i + 1) << "/" << facts.size() 
                      << " (" << (100 * (i + 1) / facts.size()) << "%)\n";
            std::cout << "  Nodes: " << storage->node_count() 
                      << " (+" << (storage->node_count() - nodes_before) << ")\n";
            std::cout << "  Edges: " << storage->edge_count() 
                      << " (+" << (storage->edge_count() - edges_before) << ")\n";
            std::cout << "  Rate: " << std::fixed << std::setprecision(1) << rate << " facts/sec\n";
            std::cout << "  Time: " << elapsed << "s\n";
            std::cout << "  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
        }
    }
    
    episodes->end_episode(episode_id);
    
    auto learning_time = std::chrono::steady_clock::now();
    auto learning_elapsed = std::chrono::duration_cast<std::chrono::seconds>(learning_time - start_time).count();
    
    std::cout << "\n✅ Learned " << facts.size() << " facts in " << learning_elapsed << " seconds!\n\n";
    
    std::cout << "📊 After learning (before LEAPs):\n";
    std::cout << "  Nodes: " << storage->node_count() 
              << " (+" << (storage->node_count() - nodes_before) << ")\n";
    std::cout << "  Edges: " << storage->edge_count() 
              << " (+" << (storage->edge_count() - edges_before) << ")\n\n";
    
    // Create LEAP connections (this is the slow part!)
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  DISCOVERING PATTERNS (This may take a few minutes...)        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "🧠 Running LEAP inference...\n";
    std::cout << "   (Finding transitive shortcuts and similarity patterns)\n\n";
    
    leap::LEAPInference::Config leap_config;
    leap_config.max_transitive_hops = 5;
    leap_config.min_shared_neighbors = 2;
    leap_config.leap_confidence = 0.7f;
    leap_config.verbose = false;  // Don't show each LEAP (too many!)
    
    auto leap_start = std::chrono::steady_clock::now();
    
    leap::LEAPInference leap_system(leap_config);
    
    std::cout << "  Phase 1: Transitive shortcuts (A→B→C creates A→C)...\n";
    int transitive = leap_system.create_transitive_leaps(storage.get());
    std::cout << "    ✓ Created " << transitive << " transitive LEAPs\n\n";
    
    std::cout << "  Phase 2: Similarity patterns (shared targets)...\n";
    int similarity = leap_system.create_similarity_leaps(storage.get());
    std::cout << "    ✓ Created " << similarity << " similarity LEAPs\n\n";
    
    int total_leaps = transitive + similarity;
    
    auto leap_end = std::chrono::steady_clock::now();
    auto leap_elapsed = std::chrono::duration_cast<std::chrono::seconds>(leap_end - leap_start).count();
    
    std::cout << "✅ LEAP creation complete in " << leap_elapsed << " seconds!\n";
    std::cout << "  Total LEAPs created: " << total_leaps << "\n\n";
    
    // Final stats
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  FINAL KNOWLEDGE BASE                                         ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    storage->print_stats();
    
    auto total_time = std::chrono::steady_clock::now();
    auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(total_time - start_time).count();
    
    std::cout << "\n⏱  TIMING:\n";
    std::cout << "  Learning: " << learning_elapsed << "s\n";
    std::cout << "  LEAP creation: " << leap_elapsed << "s\n";
    std::cout << "  Total: " << total_elapsed << "s\n\n";
    
    // Save
    std::cout << "💾 Saving knowledge base...\n";
    storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    episodes->save("melvin/data/episodes.melvin");
    std::cout << "✅ Saved!\n\n";
    
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN NOW KNOWS " << storage->node_count() << " CONCEPTS!                              ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    return 0;
}

