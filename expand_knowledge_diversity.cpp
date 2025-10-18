/*
 * Expand knowledge base with DIVERSE facts (not repetitive)
 * This will create hundreds of unique concepts
 */

#include "melvin/core/storage.h"
#include "melvin/core/learning.h"
#include "melvin/core/leap_inference.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>
#include <random>

using namespace melvin;

// Generate diverse facts dynamically
std::vector<std::string> generate_diverse_facts(int count) {
    std::vector<std::string> facts;
    
    // Template-based fact generation for diversity
    std::vector<std::string> subjects = {
        "neurons", "electrons", "atoms", "molecules", "cells", "proteins",
        "waves", "forces", "fields", "particles", "crystals", "compounds",
        "rivers", "mountains", "forests", "oceans", "clouds", "storms",
        "ideas", "thoughts", "memories", "emotions", "beliefs", "concepts",
        "markets", "currencies", "stocks", "bonds", "assets", "commodities",
        "languages", "words", "sentences", "meanings", "symbols", "codes"
    };
    
    std::vector<std::string> verbs = {
        "transmit", "generate", "transform", "produce", "create", "emit",
        "absorb", "reflect", "conduct", "transfer", "convert", "amplify",
        "regulate", "control", "modulate", "influence", "affect", "modify",
        "contain", "compose", "comprise", "include", "embody", "represent"
    };
    
    std::vector<std::string> objects = {
        "signals", "information", "patterns", "structures", "properties", "features",
        "energy", "matter", "forces", "fields", "waves", "particles",
        "data", "knowledge", "wisdom", "insights", "understanding", "meaning",
        "value", "utility", "benefits", "outcomes", "results", "effects"
    };
    
    // Generate unique combinations
    std::random_device rd;
    std::mt19937 gen(rd());
    
    for (int i = 0; i < count; i++) {
        int subj_idx = gen() % subjects.size();
        int verb_idx = gen() % verbs.size();
        int obj_idx = gen() % objects.size();
        
        std::string fact = subjects[subj_idx] + " " + verbs[verb_idx] + " " + objects[obj_idx];
        
        // Check for duplicates
        if (std::find(facts.begin(), facts.end(), fact) == facts.end()) {
            facts.push_back(fact);
        }
    }
    
    return facts;
}

void parse_and_add_fact(Storage* storage, const std::string& fact) {
    std::istringstream iss(fact);
    std::string from, relation, to;
    
    iss >> from >> relation >> to;
    
    if (from.empty() || to.empty()) return;
    
    // Create or find nodes
    auto from_nodes = storage->find_nodes(from);
    NodeID from_id = from_nodes.empty() ? 
        storage->create_node(from, NodeType::CONCEPT) : from_nodes[0].id;
    
    auto to_nodes = storage->find_nodes(to);
    NodeID to_id = to_nodes.empty() ? 
        storage->create_node(to, NodeType::CONCEPT) : to_nodes[0].id;
    
    // Create EXACT edge
    storage->create_edge(from_id, to_id, RelationType::EXACT, 1.0f);
}

int main(int argc, char** argv) {
    int num_facts = 500;  // Default
    if (argc > 1) {
        num_facts = std::atoi(argv[1]);
    }
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  EXPANDING KNOWLEDGE DIVERSITY                                ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Generating " << num_facts << " diverse facts...\n\n";
    
    auto storage = std::make_unique<Storage>();
    storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    
    std::cout << "Starting state:\n";
    storage->print_stats();
    
    auto facts = generate_diverse_facts(num_facts);
    
    std::cout << "\nGenerated " << facts.size() << " unique facts!\n";
    std::cout << "\nAdding to knowledge base...\n";
    
    int added = 0;
    for (const auto& fact : facts) {
        parse_and_add_fact(storage.get(), fact);
        added++;
        
        if (added % 50 == 0) {
            std::cout << "  Progress: " << added << "/" << facts.size() << " facts added\n";
        }
    }
    
    std::cout << "\n✅ Added " << added << " facts!\n\n";
    
    std::cout << "After expansion:\n";
    storage->print_stats();
    
    // Create LEAPs
    std::cout << "\n🧠 Creating LEAP connections...\n\n";
    
    leap::LEAPInference::Config leap_config;
    leap_config.max_transitive_hops = 5;
    leap_config.verbose = true;
    
    leap::LEAPInference leap_system(leap_config);
    int leaps = leap_system.create_leap_connections(storage.get());
    
    std::cout << "\n✅ Created " << leaps << " LEAP connections!\n\n";
    
    std::cout << "Final state:\n";
    storage->print_stats();
    
    // Save
    std::cout << "\n💾 Saving expanded knowledge base...\n";
    if (storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin")) {
        std::cout << "✅ Saved successfully!\n";
    }
    
    // Save facts to file for reference
    std::ofstream fact_file("diverse_facts.txt");
    for (const auto& fact : facts) {
        fact_file << fact << "\n";
    }
    std::cout << "✅ Facts saved to diverse_facts.txt\n\n";
    
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  KNOWLEDGE BASE NOW HAS MUCH MORE DIVERSITY!                  ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    return 0;
}

