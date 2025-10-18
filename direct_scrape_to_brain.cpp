/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  DIRECT WEB SCRAPING TO MELVIN'S BRAIN                                    ║
 * ║  No files - Facts go DIRECTLY into node/edge storage!                     ║
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
#include <random>

using namespace melvin;

// Fetch Wikipedia summary directly (using curl)
std::string fetch_wikipedia_random() {
    // Get random article title
    std::string cmd = "curl -s 'https://en.wikipedia.org/w/api.php?action=query&list=random&rnnamespace=0&rnlimit=1&format=json' 2>/dev/null";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    
    std::string result;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);
    
    // Extract title from JSON (simple parse)
    size_t title_pos = result.find("\"title\":\"");
    if (title_pos == std::string::npos) return "";
    
    title_pos += 9;
    size_t title_end = result.find("\"", title_pos);
    if (title_end == std::string::npos) return "";
    
    std::string title = result.substr(title_pos, title_end - title_pos);
    
    // Now fetch article content
    std::string article_url = "https://en.wikipedia.org/w/api.php?action=query&prop=extracts&exintro&explaintext&format=json&titles=" + title;
    std::string fetch_cmd = "curl -s '" + article_url + "' 2>/dev/null";
    
    pipe = popen(fetch_cmd.c_str(), "r");
    if (!pipe) return "";
    
    result.clear();
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);
    
    // Extract text
    size_t extract_pos = result.find("\"extract\":\"");
    if (extract_pos == std::string::npos) return "";
    
    extract_pos += 11;
    size_t extract_end = result.find("\"", extract_pos);
    if (extract_end == std::string::npos) return "";
    
    std::string text = result.substr(extract_pos, extract_end - extract_pos);
    
    // Replace \\n with spaces
    for (size_t i = 0; i < text.length() - 1; i++) {
        if (text[i] == '\\' && text[i+1] == 'n') {
            text[i] = ' ';
            text[i+1] = ' ';
        }
    }
    
    return text;
}

// Generate high-quality synthetic fact
std::string generate_synthetic_fact() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    static std::vector<std::string> subjects = {
        "quantum mechanics", "thermodynamics", "electromagnetism",
        "cellular respiration", "protein synthesis", "DNA replication",
        "neural networks", "machine learning", "consciousness",
        "plate tectonics", "water cycle", "carbon cycle",
        "market dynamics", "supply chains", "economic systems"
    };
    
    static std::vector<std::string> verbs = {
        "governs", "describes", "explains", "influences", "regulates",
        "creates", "produces", "transforms", "enables", "supports"
    };
    
    static std::vector<std::string> objects = {
        "physical phenomena", "chemical reactions", "biological processes",
        "natural systems", "energy transfer", "information flow",
        "pattern formation", "complex behaviors", "emergent properties"
    };
    
    std::uniform_int_distribution<> subj_dist(0, subjects.size() - 1);
    std::uniform_int_distribution<> verb_dist(0, verbs.size() - 1);
    std::uniform_int_distribution<> obj_dist(0, objects.size() - 1);
    
    return subjects[subj_dist(gen)] + " " + verbs[verb_dist(gen)] + " " + objects[obj_dist(gen)];
}

// Parse fact and add DIRECTLY to storage
void learn_fact_directly(Storage* storage, episodic::EpisodicMemory* episodes, 
                        uint32_t episode_id, const std::string& fact, bool show = true) {
    if (show) {
        if (fact.length() > 70) {
            std::cout << "  📖 " << fact.substr(0, 70) << "...\n";
        } else {
            std::cout << "  📖 " << fact << "\n";
        }
    }
    
    // Parse into words
    std::istringstream iss(fact);
    std::vector<std::string> words;
    std::string word;
    
    while (iss >> word) {
        std::string clean;
        for (char c : word) {
            if (std::isalnum(c) || c == '_') {
                clean += std::tolower(c);
            }
        }
        if (clean.length() >= 2 && clean.length() <= 30) {
            words.push_back(clean);
        }
    }
    
    if (words.size() < 2) return;
    
    // Create chain directly in storage
    NodeID prev_id = 0;
    
    for (const auto& w : words) {
        // Find or create node
        auto nodes = storage->find_nodes(w);
        NodeID node_id;
        
        if (nodes.empty()) {
            node_id = storage->create_node(w, NodeType::CONCEPT);
            
            // Set timestamp and episode
            Node node;
            if (storage->get_node(node_id, node)) {
                node.creation_timestamp = episodic::get_timestamp_ms();
                node.episode_id = episode_id;
                storage->update_node(node);
            }
            
            // Add to episode
            if (episodes && episode_id > 0) {
                episodes->add_node(episode_id, node_id);
            }
        } else {
            node_id = nodes[0].id;
        }
        
        // Create edge
        if (prev_id != 0) {
            EdgeID edge_id = storage->create_edge(prev_id, node_id, RelationType::EXACT, 1.0f);
            
            // Set timestamp and episode
            Edge edge;
            if (storage->get_edge(edge_id, edge)) {
                edge.creation_timestamp = episodic::get_timestamp_ms();
                edge.episode_id = episode_id;
                storage->update_edge(edge);
            }
            
            // Add to episode
            if (episodes && episode_id > 0 && edge_id > 0) {
                episodes->add_edge(episode_id, edge_id);
            }
        }
        
        prev_id = node_id;
    }
}

int main(int argc, char** argv) {
    int max_cycles = 10;
    int facts_per_cycle = 5;
    int leap_interval = 10;
    int save_interval = 20;
    
    if (argc > 1) max_cycles = std::atoi(argv[1]);
    if (argc > 2) facts_per_cycle = std::atoi(argv[2]);
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                               ║\n";
    std::cout << "║  🧠 DIRECT WEB SCRAPING TO BRAIN                              ║\n";
    std::cout << "║     (No files - Facts → Nodes/Edges directly!)                ║\n";
    std::cout << "║                                                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    if (max_cycles == 0) {
        std::cout << "⚡ INFINITE MODE\n";
    } else {
        std::cout << "⚡ Running for " << max_cycles << " cycles\n";
    }
    
    std::cout << "\nConfiguration:\n";
    std::cout << "  • Facts per cycle: " << facts_per_cycle << "\n";
    std::cout << "  • LEAP inference: Every " << leap_interval << " cycles\n";
    std::cout << "  • Direct to storage: YES (no intermediate files)\n\n";
    
    std::cout << "Sources:\n";
    std::cout << "  ✓ Random Wikipedia (different each time)\n";
    std::cout << "  ✓ High-quality synthetic (ensures diversity)\n\n";
    
    std::cout << "Press Enter to start...";
    std::cin.get();
    
    // Initialize Melvin
    std::cout << "\n🔧 Initializing...\n";
    auto storage = std::make_unique<Storage>();
    storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    
    episodic::EpisodicMemory::Config ep_config;
    ep_config.verbose = false;
    auto episodes = std::make_unique<episodic::EpisodicMemory>(storage.get(), ep_config);
    
    gnn::GNNPredictor::Config gnn_config;
    gnn_config.embed_dim = 16;
    gnn_config.verbose = false;
    auto gnn = std::make_unique<gnn::GNNPredictor>(gnn_config);
    
    auto nodes = storage->get_all_nodes();
    gnn->initialize_embeddings(nodes);
    
    size_t start_nodes = storage->node_count();
    size_t start_edges = storage->edge_count();
    
    std::cout << "  ✓ Starting: " << start_nodes << " nodes, " << start_edges << " edges\n\n";
    
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  DIRECT LEARNING STARTED                                      ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    auto start_time = std::chrono::steady_clock::now();
    int total_facts = 0;
    int total_leaps = 0;
    
    for (int cycle = 1; max_cycles == 0 || cycle <= max_cycles; cycle++) {
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "🕷️  Cycle " << std::setw(4) << cycle << "\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
        
        // Create episode for this cycle
        uint32_t episode_id = episodes->create_episode("Scrape Cycle " + std::to_string(cycle));
        
        std::cout << "🌐 Fetching from Wikipedia...\n\n";
        
        // Fetch Wikipedia fact
        std::string wiki_text = fetch_wikipedia_random();
        
        std::vector<std::string> facts;
        
        if (!wiki_text.empty()) {
            // Parse into sentences
            std::string current;
            for (char c : wiki_text) {
                if (c == '.' || c == '!' || c == '?') {
                    if (current.length() > 20 && current.length() < 150) {
                        // Clean up
                        while (!current.empty() && (current[0] == ' ' || current[0] == '\n')) {
                            current.erase(0, 1);
                        }
                        if (!current.empty()) {
                            facts.push_back(current);
                            if (facts.size() >= facts_per_cycle) break;
                        }
                    }
                    current.clear();
                } else {
                    current += c;
                }
            }
        }
        
        // Add synthetic facts for diversity
        while (facts.size() < facts_per_cycle) {
            facts.push_back(generate_synthetic_fact());
        }
        
        std::cout << "🧠 Learning " << facts.size() << " facts DIRECTLY to brain:\n\n";
        
        // Learn each fact DIRECTLY into storage
        for (const auto& fact : facts) {
            learn_fact_directly(storage.get(), episodes.get(), episode_id, fact, true);
            total_facts++;
        }
        
        episodes->end_episode(episode_id);
        
        // Update GNN embeddings
        auto all_nodes = storage->get_all_nodes();
        auto all_edges = storage->get_all_edges();
        gnn->message_pass(all_nodes, all_edges, storage.get());
        
        // LEAP inference periodically
        if (cycle % leap_interval == 0) {
            std::cout << "\n🧠 LEAP Discovery Phase...\n";
            
            leap::LEAPInference::Config leap_config;
            leap_config.max_transitive_hops = 4;
            leap_config.min_shared_neighbors = 2;
            leap_config.leap_confidence = 0.7f;
            leap_config.verbose = false;
            
            auto leap_start = std::chrono::steady_clock::now();
            
            leap::LEAPInference leap_system(leap_config);
            int leaps = leap_system.create_leap_connections(storage.get());
            total_leaps += leaps;
            
            auto leap_end = std::chrono::steady_clock::now();
            auto leap_time = std::chrono::duration_cast<std::chrono::seconds>(leap_end - leap_start).count();
            
            std::cout << "  ✨ Created " << leaps << " LEAPs in " << leap_time << "s\n";
        }
        
        // Display stats
        size_t current_nodes = storage->node_count();
        size_t current_edges = storage->edge_count();
        
        std::cout << "\n📊 Brain State:\n";
        std::cout << "  Nodes: " << current_nodes << " (+" << (current_nodes - start_nodes) << ")\n";
        std::cout << "  Edges: " << current_edges << " (+" << (current_edges - start_edges) << ")\n";
        std::cout << "  EXACT: " << storage->edge_count_by_type(RelationType::EXACT) << "\n";
        std::cout << "  LEAP: " << storage->edge_count_by_type(RelationType::LEAP) << "\n";
        
        // Save checkpoint
        if (cycle % save_interval == 0) {
            std::cout << "\n💾 Saving checkpoint...";
            storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
            episodes->save("melvin/data/episodes.melvin");
            std::cout << " ✓\n";
        }
        
        // Milestone
        if (cycle % 50 == 0) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
            
            std::cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
            std::cout << "║  MILESTONE: " << cycle << " CYCLES                                   ║\n";
            std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
            std::cout << "  Facts: " << total_facts << " | LEAPs: " << total_leaps << "\n";
            std::cout << "  Growth: +" << (current_nodes - start_nodes) << " nodes, +"
                      << (current_edges - start_edges) << " edges\n";
            std::cout << "  Time: " << elapsed << "s | Rate: " 
                      << (elapsed > 0 ? total_facts/elapsed : 0) << " facts/s\n";
            std::cout << "═══════════════════════════════════════════════════════════\n";
        }
        
        std::cout << "\n";
        
        // Delay (1 second for API respect)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // Final save
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  SESSION COMPLETE                                             ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    episodes->save("melvin/data/episodes.melvin");
    
    std::cout << "📊 FINAL STATE:\n";
    storage->print_stats();
    
    size_t final_nodes = storage->node_count();
    size_t final_edges = storage->edge_count();
    
    auto end_time = std::chrono::steady_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
    
    std::cout << "\n✨ SUMMARY:\n";
    std::cout << "  Facts learned: " << total_facts << "\n";
    std::cout << "  LEAPs created: " << total_leaps << "\n";
    std::cout << "  Node growth: +" << (final_nodes - start_nodes) 
              << " (" << (100*(final_nodes - start_nodes)/start_nodes) << "%)\n";
    std::cout << "  Edge growth: +" << (final_edges - start_edges)
              << " (" << (100*(final_edges - start_edges)/start_edges) << "%)\n";
    std::cout << "  Time: " << total_time << "s\n\n";
    
    std::cout << "💾 Knowledge saved to melvin/data/ (nodes.melvin, edges.melvin)\n\n";
    
    return 0;
}

