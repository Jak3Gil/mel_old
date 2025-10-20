/*
 * Feed internet knowledge to Melvin's brain
 * Reads facts from internet_facts.txt and adds them to knowledge base
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

using namespace melvin;

void parse_and_add_fact(Storage* storage, const std::string& fact) {
    // Simple parser: first word → last word
    std::istringstream iss(fact);
    std::vector<std::string> words;
    std::string word;
    
    while (iss >> word) {
        words.push_back(word);
    }
    
    if (words.size() < 2) return;
    
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
    std::cout << "║  FEEDING INTERNET KNOWLEDGE TO MELVIN                         ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    // Load existing knowledge base
    auto storage = std::make_unique<Storage>();
    storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    
    std::cout << "Starting knowledge base:\n";
    storage->print_stats();
    
    // Setup episodic memory
    episodic::EpisodicMemory::Config ep_config;
    ep_config.verbose = false;
    auto episodes = std::make_unique<episodic::EpisodicMemory>(storage.get(), ep_config);
    
    // Create episode for internet learning
    uint32_t episode_id = episodes->create_episode("Internet knowledge ingestion");
    
    // Read facts from file
    std::cout << "\n📖 Reading facts from internet_facts.txt...\n";
    
    std::ifstream fact_file("internet_facts.txt");
    if (!fact_file) {
        std::cerr << "❌ Could not open internet_facts.txt\n";
        std::cerr << "   Run: python3 internet_knowledge_fetcher.py\n";
        return 1;
    }
    
    std::vector<std::string> facts;
    std::string line;
    
    while (std::getline(fact_file, line)) {
        if (!line.empty() && line.length() > 10) {
            facts.push_back(line);
        }
    }
    
    std::cout << "  ✓ Loaded " << facts.size() << " facts from internet\n\n";
    
    // Learn facts
    std::cout << "🧠 Learning facts...\n";
    
    int learned = 0;
    int skipped = 0;
    
    for (const auto& fact : facts) {
        parse_and_add_fact(storage.get(), fact);
        learned++;
        
        if (learned % 50 == 0) {
            std::cout << "  Progress: " << learned << "/" << facts.size() << " facts\n";
        }
    }
    
    episodes->end_episode(episode_id);
    
    std::cout << "\n✅ Learned " << learned << " facts!\n\n";
    
    std::cout << "After learning:\n";
    storage->print_stats();
    
    // Create LEAP connections
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  DISCOVERING PATTERNS                                         ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    leap::LEAPInference::Config leap_config;
    leap_config.max_transitive_hops = 5;
    leap_config.min_shared_neighbors = 2;
    leap_config.leap_confidence = 0.7f;
    leap_config.verbose = true;
    
    leap::LEAPInference leap_system(leap_config);
    int leaps = leap_system.create_leap_connections(storage.get());
    
    std::cout << "\n✅ Created " << leaps << " LEAP connections!\n\n";
    
    std::cout << "Final knowledge base:\n";
    storage->print_stats();
    
    // Save
    std::cout << "\n💾 Saving expanded knowledge base...\n";
    if (storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin")) {
        std::cout << "✅ Saved to melvin/data/\n";
    }
    
    if (episodes->save("melvin/data/episodes.melvin")) {
        std::cout << "✅ Episodes saved\n";
    }
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN HAS LEARNED FROM THE INTERNET!                        ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Next steps:\n";
    std::cout << "  • Run: ./inspect_kb (see what Melvin learned)\n";
    std::cout << "  • Run: ./test_reasoning (test knowledge)\n";
    std::cout << "  • Run: cd melvin && ./melvin (ask questions)\n\n";
    
    return 0;
}

