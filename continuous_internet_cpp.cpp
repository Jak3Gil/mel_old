/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN CONTINUOUS INTERNET LEARNING (Pure C++ - Fast!)                   ║
 * ║  Fetches from free APIs and learns in real-time                           ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "melvin/core/storage.h"
#include "melvin/core/learning.h"
#include "melvin/core/leap_inference.h"
#include "melvin/core/episodic_memory.h"
#include "melvin/core/gnn_predictor.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <iomanip>

using namespace melvin;

// Simple HTTP GET using curl (available on macOS)
std::string http_get(const std::string& url) {
    std::string command = "curl -s -A 'MelvinAI/1.0 Educational' '" + url + "'";
    
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "";
    
    std::string result;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    
    pclose(pipe);
    return result;
}

// Extract text from JSON (simple parser)
std::string extract_wikipedia_text(const std::string& json_response) {
    // Find "extract":"..." in JSON
    size_t pos = json_response.find("\"extract\":\"");
    if (pos == std::string::npos) return "";
    
    pos += 11;  // Skip "extract":"
    size_t end = json_response.find("\"", pos);
    if (end == std::string::npos) return "";
    
    std::string text = json_response.substr(pos, end - pos);
    
    // Replace \\n with actual newlines
    size_t newline_pos = 0;
    while ((newline_pos = text.find("\\n", newline_pos)) != std::string::npos) {
        text.replace(newline_pos, 2, "\n");
        newline_pos += 1;
    }
    
    return text;
}

// Parse text into facts (sentences)
std::vector<std::string> parse_into_facts(const std::string& text) {
    std::vector<std::string> facts;
    std::string current;
    
    for (char c : text) {
        if (c == '.' || c == '!' || c == '?') {
            if (!current.empty() && current.length() > 10 && current.length() < 150) {
                // Clean up
                while (!current.empty() && (current[0] == ' ' || current[0] == '\n')) {
                    current.erase(0, 1);
                }
                if (!current.empty()) {
                    facts.push_back(current);
                }
            }
            current.clear();
        } else {
            current += c;
        }
    }
    
    return facts;
}

// Parse fact into concept chain and add to graph
void add_fact_to_graph(Storage* storage, const std::string& fact, bool verbose = true) {
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
    
    // Create chain
    NodeID prev_id = 0;
    
    for (const auto& w : words) {
        auto nodes = storage->find_nodes(w);
        NodeID node_id;
        
        if (nodes.empty()) {
            node_id = storage->create_node(w, NodeType::CONCEPT);
            if (verbose && words.size() <= 8) {  // Only show for short facts
                std::cout << "      [+] " << w;
            }
        } else {
            node_id = nodes[0].id;
        }
        
        if (prev_id != 0) {
            storage->create_edge(prev_id, node_id, RelationType::EXACT, 1.0f);
        }
        
        prev_id = node_id;
    }
    
    if (verbose && words.size() <= 8) {
        std::cout << "\n";
    }
}

int main(int argc, char** argv) {
    int max_cycles = 10;
    int facts_per_cycle = 5;
    int leap_interval = 5;
    
    if (argc > 1) max_cycles = std::atoi(argv[1]);
    if (argc > 2) facts_per_cycle = std::atoi(argv[2]);
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                               ║\n";
    std::cout << "║  🌐 MELVIN CONTINUOUS INTERNET LEARNING (C++ - FAST!)         ║\n";
    std::cout << "║                                                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    if (max_cycles == 0) {
        std::cout << "⚡ Running in INFINITE mode - Press Ctrl+C to stop\n";
    } else {
        std::cout << "⚡ Running for " << max_cycles << " cycles\n";
    }
    
    std::cout << "\nConfiguration:\n";
    std::cout << "  • Facts per cycle: " << facts_per_cycle << "\n";
    std::cout << "  • LEAP inference: Every " << leap_interval << " cycles\n";
    std::cout << "  • Show facts: YES (verbose mode)\n\n";
    
    std::cout << "Sources:\n";
    std::cout << "  ✓ Wikipedia API (free, using curl)\n";
    std::cout << "  ✓ Local generation (fallback)\n\n";
    
    std::cout << "Press Enter to start continuous learning...";
    std::cin.get();
    
    // Initialize Melvin
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
    
    std::cout << "  ✓ All systems ready\n\n";
    
    // Wikipedia topics to rotate through
    std::vector<std::string> topics = {
        "Physics", "Chemistry", "Biology", "Astronomy", "Geology",
        "Computer_Science", "Mathematics", "Neuroscience", "Psychology",
        "Philosophy", "Economics", "History", "Geography", "Medicine",
        "Engineering", "Agriculture", "Ecology", "Genetics", "Robotics"
    };
    
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  CONTINUOUS LEARNING STARTED                                  ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    auto start_time = std::chrono::steady_clock::now();
    int total_facts_learned = 0;
    int total_leaps_created = 0;
    
    for (int cycle = 1; cycle <= max_cycles || max_cycles == 0; cycle++) {
        // Select topic
        std::string topic = topics[(cycle - 1) % topics.size()];
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", localtime(&time));
        
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "[" << time_str << "] 📚 Cycle " << std::setw(4) << cycle 
                  << " | Topic: " << std::setw(18) << std::left << topic << "\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
        
        // Create episode
        uint32_t episode_id = episodes->create_episode("Internet: " + topic);
        
        // Fetch from Wikipedia
        std::cout << "🌐 Fetching from Wikipedia API...\n";
        
        std::string url = "https://en.wikipedia.org/w/api.php?action=query&prop=extracts&exintro&explaintext&format=json&titles=" + topic;
        std::string response = http_get(url);
        
        std::vector<std::string> facts;
        
        if (!response.empty()) {
            std::string wiki_text = extract_wikipedia_text(response);
            
            if (!wiki_text.empty()) {
                facts = parse_into_facts(wiki_text);
                
                if (facts.size() > facts_per_cycle) {
                    facts.resize(facts_per_cycle);
                }
                
                std::cout << "  ✓ Fetched " << facts.size() << " facts from Wikipedia\n\n";
            }
        }
        
        // Fallback to local generation if fetch failed
        if (facts.empty()) {
            std::cout << "  ⚠ Wikipedia unavailable, generating locally...\n\n";
            
            std::vector<std::string> subjects = {"energy", "force", "mass", "velocity", "acceleration"};
            std::vector<std::string> verbs = {"creates", "produces", "generates", "transfers", "affects"};
            std::vector<std::string> objects = {"motion", "heat", "light", "work", "power"};
            
            for (int i = 0; i < facts_per_cycle; i++) {
                std::string fact = subjects[rand() % subjects.size()] + " " + 
                                  verbs[rand() % verbs.size()] + " " + 
                                  objects[rand() % objects.size()];
                facts.push_back(fact);
            }
        }
        
        // Learn facts (showing each one!)
        std::cout << "🧠 Learning facts:\n\n";
        
        int learned_this_cycle = 0;
        for (const auto& fact : facts) {
            std::cout << "  📖 \"" << fact << "\"\n";
            add_fact_to_graph(storage.get(), fact, true);
            learned_this_cycle++;
            total_facts_learned++;
        }
        
        episodes->end_episode(episode_id);
        
        std::cout << "\n  ✅ Learned " << learned_this_cycle << " facts\n";
        
        // Update GNN
        auto all_nodes = storage->get_all_nodes();
        auto all_edges = storage->get_all_edges();
        gnn->message_pass(all_nodes, all_edges, storage.get());
        
        // Create LEAPs periodically
        if (cycle % leap_interval == 0) {
            std::cout << "\n  🧠 Discovering patterns and creating LEAPs...\n";
            
            leap::LEAPInference::Config leap_config;
            leap_config.max_transitive_hops = 5;
            leap_config.min_shared_neighbors = 2;
            leap_config.leap_confidence = 0.7f;
            leap_config.verbose = false;
            
            leap::LEAPInference leap_system(leap_config);
            int leaps = leap_system.create_leap_connections(storage.get());
            total_leaps_created += leaps;
            
            if (leaps > 0) {
                std::cout << "  ✨ Created " << leaps << " new LEAP shortcuts!\n";
            }
        }
        
        // Show stats
        std::cout << "\n  📊 Knowledge Base:\n";
        std::cout << "      Nodes: " << storage->node_count() 
                  << " | Edges: " << storage->edge_count()
                  << " (EXACT: " << storage->edge_count_by_type(RelationType::EXACT)
                  << ", LEAP: " << storage->edge_count_by_type(RelationType::LEAP) << ")\n\n";
        
        // Save checkpoint every 10 cycles
        if (cycle % 10 == 0) {
            std::cout << "  💾 Saving checkpoint...\n";
            storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
            episodes->save("melvin/data/episodes.melvin");
            
            // Milestone summary
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
            
            std::cout << "\n  ╔═══════════════════════════════════════════════════════════╗\n";
            std::cout << "  ║  MILESTONE: Cycle " << std::setw(4) << cycle << std::setfill(' ') << "                                       ║\n";
            std::cout << "  ╚═══════════════════════════════════════════════════════════╝\n";
            std::cout << "    Total facts learned: " << total_facts_learned << "\n";
            std::cout << "    Total LEAPs created: " << total_leaps_created << "\n";
            std::cout << "    Time running: " << elapsed << " seconds\n";
            std::cout << "    Learning rate: " << (elapsed > 0 ? total_facts_learned / elapsed : 0) 
                      << " facts/second\n";
            std::cout << "  ═══════════════════════════════════════════════════════════\n\n";
        }
        
        std::cout << "\n";
        
        // Small delay (1 second) - much faster than Python!
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // Final save and summary
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  CONTINUOUS LEARNING SESSION COMPLETE                         ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    episodes->save("melvin/data/episodes.melvin");
    
    auto end_time = std::chrono::steady_clock::now();
    auto total_elapsed = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
    
    std::cout << "📊 FINAL STATISTICS\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    storage->print_stats();
    episodes->print_stats();
    
    std::cout << "✨ SESSION SUMMARY\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  Cycles completed: " << (max_cycles > 0 ? max_cycles : total_facts_learned / facts_per_cycle) << "\n";
    std::cout << "  Facts learned: " << total_facts_learned << "\n";
    std::cout << "  LEAPs created: " << total_leaps_created << "\n";
    std::cout << "  Time: " << total_elapsed << " seconds\n";
    std::cout << "  Rate: " << (total_elapsed > 0 ? (double)total_facts_learned / total_elapsed : 0) 
              << " facts/second\n\n";
    
    std::cout << "💾 All knowledge saved to melvin/data/\n\n";
    
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN IS SMARTER!                                           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Next steps:\n";
    std::cout << "  • ./inspect_kb - See what Melvin learned\n";
    std::cout << "  • ./test_reasoning - Test knowledge\n";
    std::cout << "  • cd melvin && ./melvin - Ask questions\n\n";
    
    return 0;
}

