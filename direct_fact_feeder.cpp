/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  DIRECT FACT FEEDER - Feed facts directly to Melvin's brain              ║
 * ║  Reads facts from stdin or file and adds to global storage               ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "melvin/core/storage.h"
#include "melvin/core/learning.h"
#include "melvin/core/leap_inference.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>

using namespace melvin;

struct FactStats {
    int facts_read = 0;
    int facts_processed = 0;
    int nodes_created = 0;
    int edges_created = 0;
    int leaps_created = 0;
    double elapsed_seconds = 0.0;
};

// Parse a fact into concepts and relationships
std::vector<std::tuple<std::string, std::string, std::string>> parse_fact(const std::string& fact) {
    std::vector<std::tuple<std::string, std::string, std::string>> triples;
    
    // Simple pattern matching for relationships
    std::string lower_fact = fact;
    std::transform(lower_fact.begin(), lower_fact.end(), lower_fact.begin(), ::tolower);
    
    // Find relationship verbs
    std::vector<std::string> verbs = {
        " is ", " are ", " was ", " were ",
        " has ", " have ", " had ",
        " does ", " do ", " did ",
        " contains ", " contain ", " contained ",
        " produces ", " produce ", " produced ",
        " creates ", " create ", " created ",
        " enables ", " enable ", " enabled ",
        " allows ", " allow ", " allowed ",
        " causes ", " cause ", " caused ",
        " affects ", " affect ", " affected ",
        " controls ", " control ", " controlled ",
        " forms ", " form ", " formed ",
        " converts ", " convert ", " converted ",
        " generates ", " generate ", " generated ",
        " transmits ", " transmit ", " transmitted ",
        " connects ", " connect ", " connected ",
        " involves ", " involve ", " involved ",
        " requires ", " require ", " required "
    };
    
    for (const auto& verb : verbs) {
        size_t pos = lower_fact.find(verb);
        if (pos != std::string::npos) {
            std::string subject = fact.substr(0, pos);
            std::string object = fact.substr(pos + verb.length());
            
            // Clean up
            subject.erase(0, subject.find_first_not_of(" \t\r\n"));
            subject.erase(subject.find_last_not_of(" \t\r\n.") + 1);
            object.erase(0, object.find_first_not_of(" \t\r\n"));
            object.erase(object.find_last_not_of(" \t\r\n.") + 1);
            
            if (!subject.empty() && !object.empty() && subject != object) {
                std::string relation = verb;
                relation.erase(0, relation.find_first_not_of(" "));
                relation.erase(relation.find_last_not_of(" ") + 1);
                
                triples.push_back(std::make_tuple(subject, relation, object));
                break;
            }
        }
    }
    
    return triples;
}

// Feed facts to Melvin
FactStats feed_facts(Storage* storage, LearningSystem* learning, 
                    const std::vector<std::string>& facts, bool create_leaps, bool word_fallback) {
    FactStats stats;
    auto start = std::chrono::steady_clock::now();
    
    stats.facts_read = facts.size();
    
    // Process each fact
    for (const auto& fact : facts) {
        if (fact.empty() || fact.length() < 10) continue;
        
        // Parse into triples
        auto triples = parse_fact(fact);
        
        if (triples.empty()) {
            // Fallback: ONLY if word_fallback enabled
            if (word_fallback) {
                std::istringstream iss(fact);
                std::vector<std::string> words;
                std::string word;
                while (iss >> word) {
                    if (word.length() > 3) {  // Skip short words
                        words.push_back(word);
                    }
                }
                
                // Create chain
                for (size_t i = 0; i + 1 < words.size(); ++i) {
                    NodeID from = storage->create_node(words[i], NodeType::CONCEPT);
                    NodeID to = storage->create_node(words[i+1], NodeType::CONCEPT);
                    EdgeID edge = storage->create_edge(from, to, RelationType::EXACT, 1.0f);
                    if (edge != 0) stats.edges_created++;
                }
                stats.nodes_created += words.size();
            }
            // If no fallback, just skip unparseable facts
        } else {
            // Use parsed triples
            for (const auto& triple : triples) {
                std::string subject = std::get<0>(triple);
                std::string relation = std::get<1>(triple);
                std::string object = std::get<2>(triple);
                
                NodeID from = storage->create_node(subject, NodeType::CONCEPT);
                NodeID rel = storage->create_node(relation, NodeType::CONCEPT);
                NodeID to = storage->create_node(object, NodeType::CONCEPT);
                
                EdgeID e1 = storage->create_edge(from, rel, RelationType::EXACT, 1.0f);
                EdgeID e2 = storage->create_edge(rel, to, RelationType::EXACT, 1.0f);
                
                if (e1 != 0) stats.edges_created++;
                if (e2 != 0) stats.edges_created++;
                stats.nodes_created += 3;
            }
        }
        
        stats.facts_processed++;
    }
    
    // Create LEAPs if requested
    if (create_leaps) {
        leap::LEAPInference::Config leap_config;
        leap_config.max_transitive_hops = 3;
        leap_config.min_shared_neighbors = 2;
        leap_config.leap_confidence = 0.7f;
        leap_config.verbose = false;
        
        leap::LEAPInference leap_system(leap_config);
        stats.leaps_created = leap_system.create_leap_connections(storage);
    }
    
    auto end = std::chrono::steady_clock::now();
    stats.elapsed_seconds = std::chrono::duration<double>(end - start).count();
    
    return stats;
}

int main(int argc, char* argv[]) {
    bool create_leaps = false;
    bool quiet = false;
    bool word_level_fallback = false;  // NEW: make fallback optional
    std::string input_file;
    
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--leaps") {
            create_leaps = true;
        } else if (arg == "--quiet" || arg == "-q") {
            quiet = true;
        } else if (arg == "--word-level") {
            word_level_fallback = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options] [input_file]\n";
            std::cout << "\nOptions:\n";
            std::cout << "  --leaps         Create LEAP connections after ingestion\n";
            std::cout << "  --word-level    Enable word-level fallback (creates more nodes)\n";
            std::cout << "  --quiet         Minimal output\n";
            std::cout << "  --help          Show this help\n";
            std::cout << "\nIf no input file, reads from stdin\n";
            return 0;
        } else {
            input_file = arg;
        }
    }
    
    if (!quiet) {
        std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  DIRECT FACT FEEDER - Adding to Melvin's Brain               ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    }
    
    // Load storage from correct location
    auto storage = std::make_unique<Storage>();
    bool loaded = storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    
    size_t initial_nodes = storage->node_count();
    size_t initial_edges = storage->edge_count();
    
    if (!quiet) {
        if (loaded) {
            std::cout << "✓ Loaded existing brain: " << initial_nodes 
                      << " nodes, " << initial_edges << " edges\n\n";
        } else {
            std::cout << "ℹ️  Starting fresh brain\n\n";
        }
    }
    
    // Setup learning
    LearningSystem::Config learning_config;
    learning_config.learning_rate = 0.01f;
    auto learning = std::make_unique<LearningSystem>(storage.get(), learning_config);
    
    // Read facts
    std::vector<std::string> facts;
    
    if (input_file.empty()) {
        // Read from stdin
        if (!quiet) std::cout << "Reading facts from stdin...\n";
        std::string line;
        while (std::getline(std::cin, line)) {
            if (!line.empty()) {
                facts.push_back(line);
            }
        }
    } else {
        // Read from file
        if (!quiet) std::cout << "Reading facts from " << input_file << "...\n";
        std::ifstream file(input_file);
        if (!file) {
            std::cerr << "Error: Cannot open file: " << input_file << "\n";
            return 1;
        }
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                facts.push_back(line);
            }
        }
    }
    
    if (facts.empty()) {
        std::cerr << "Error: No facts to process\n";
        return 1;
    }
    
    if (!quiet) std::cout << "✓ Read " << facts.size() << " facts\n\n";
    
    // Process facts
    if (!quiet) std::cout << "Processing facts...\n";
    FactStats stats = feed_facts(storage.get(), learning.get(), facts, create_leaps, word_level_fallback);
    
    // Save to correct location
    if (!quiet) std::cout << "\nSaving to global storage...\n";
    bool saved = storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    
    if (!saved) {
        std::cerr << "Error: Failed to save storage\n";
        return 1;
    }
    
    size_t final_nodes = storage->node_count();
    size_t final_edges = storage->edge_count();
    
    // Print results
    if (quiet) {
        // Just print compact stats
        std::cout << facts.size() << " " 
                  << stats.facts_processed << " "
                  << final_nodes - initial_nodes << " "
                  << final_edges - initial_edges << " "
                  << stats.elapsed_seconds << "\n";
    } else {
        std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  RESULTS                                                      ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
        
        std::cout << "Facts:\n";
        std::cout << "  Read:       " << stats.facts_read << "\n";
        std::cout << "  Processed:  " << stats.facts_processed << "\n\n";
        
        std::cout << "Brain Growth:\n";
        std::cout << "  Nodes:      " << initial_nodes << " → " << final_nodes 
                  << " (+" << (final_nodes - initial_nodes) << ")\n";
        std::cout << "  Edges:      " << initial_edges << " → " << final_edges
                  << " (+" << (final_edges - initial_edges) << ")\n";
        if (create_leaps) {
            std::cout << "  LEAPs:      " << stats.leaps_created << " created\n";
        }
        std::cout << "\n";
        
        std::cout << "Performance:\n";
        std::cout << "  Time:       " << stats.elapsed_seconds << " seconds\n";
        std::cout << "  Rate:       " << (stats.facts_processed / stats.elapsed_seconds) 
                  << " facts/sec\n\n";
        
        std::cout << "✅ Saved to global storage (melvin/data/nodes.melvin, melvin/data/edges.melvin)\n\n";
    }
    
    return 0;
}

