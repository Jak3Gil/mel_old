/*
 * Teach Knowledge - Load real data into Melvin
 */

#include "../core/storage.h"
#include "../core/types.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>

using namespace melvin;

struct Fact {
    std::string subject;
    std::string relation;
    std::string object;
};

Fact parse_fact(const std::string& line) {
    Fact fact;
    std::istringstream iss(line);
    
    // Simple parsing: "subject relation object"
    std::vector<std::string> words;
    std::string word;
    while (iss >> word) {
        words.push_back(word);
    }
    
    if (words.size() >= 3) {
        fact.subject = words[0];
        fact.relation = words[1];
        // Object might be multiple words
        fact.object = words[2];
        for (size_t i = 3; i < words.size(); ++i) {
            fact.object += " " + words[i];
        }
    }
    
    return fact;
}

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  TEACH MELVIN - Load Knowledge Base                   ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    // Create storage
    Storage storage;
    
    // Load existing data
    std::cout << "Loading existing memory...\n";
    storage.load("data/nodes.melvin", "data/edges.melvin");
    std::cout << "  Before: " << storage.node_count() << " nodes, " 
              << storage.edge_count() << " edges\n\n";
    
    // Read teaching file
    std::cout << "Reading knowledge base...\n";
    std::ifstream file("data/knowledge_base.tch");
    if (!file.is_open()) {
        std::cerr << "✗ Failed to open data/knowledge_base.tch\n";
        return 1;
    }
    
    std::vector<Fact> facts;
    std::string line;
    int line_num = 0;
    
    while (std::getline(file, line)) {
        line_num++;
        
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;
        
        Fact fact = parse_fact(line);
        if (!fact.subject.empty() && !fact.object.empty()) {
            facts.push_back(fact);
        }
    }
    
    std::cout << "  Found " << facts.size() << " facts to teach\n\n";
    
    // Teach each fact
    std::cout << "Teaching facts...\n";
    int count = 0;
    auto start = std::chrono::steady_clock::now();
    
    for (const auto& fact : facts) {
        // Create nodes for subject and object
        auto subject_id = storage.create_node(fact.subject, NodeType::CONCEPT);
        auto object_id = storage.create_node(fact.object, NodeType::CONCEPT);
        
        // All taught facts are EXACT connections (directly from input)
        RelationType rel = RelationType::EXACT;
        
        // Create EXACT edge with full weight for known facts
        storage.create_edge(subject_id, object_id, rel, 1.0f);
        
        count++;
        if (count % 10 == 0) {
            std::cout << "  Taught " << count << " facts...\n";
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "  ✓ Taught " << count << " facts in " << duration.count() << "ms\n\n";
    
    // Show final stats
    std::cout << "Final memory state:\n";
    std::cout << "  After:  " << storage.node_count() << " nodes, " 
              << storage.edge_count() << " edges\n\n";
    
    // Save to disk
    std::cout << "Saving to disk...\n";
    if (storage.save("data/nodes.melvin", "data/edges.melvin")) {
        std::cout << "  ✓ Saved successfully\n";
    } else {
        std::cout << "  ✗ Save failed\n";
        return 1;
    }
    
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ KNOWLEDGE BASE LOADED                             ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Melvin now knows " << count << " facts!\n";
    std::cout << "Next: Run demos/test_reasoning to see multi-hop reasoning\n\n";
    
    return 0;
}

