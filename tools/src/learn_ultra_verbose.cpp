/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN ULTRA-VERBOSE MODE                                                ║
 * ║  Shows EVERYTHING happening in the background!                            ║
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
#include <chrono>
#include <thread>
#include <iomanip>
#include <random>

using namespace melvin;

std::string generate_fact() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    static std::vector<std::string> subjects = {
        "quantum mechanics", "thermodynamics", "neural networks",
        "DNA replication", "protein synthesis", "cellular respiration",
        "photosynthesis", "evolution", "gravity", "magnetism"
    };
    
    static std::vector<std::string> verbs = {
        "governs", "explains", "creates", "produces", "transforms",
        "regulates", "influences", "enables", "supports", "describes"
    };
    
    static std::vector<std::string> objects = {
        "physical phenomena", "chemical reactions", "biological processes",
        "natural systems", "energy transfer", "information flow",
        "pattern formation", "complex behaviors", "emergent properties"
    };
    
    std::uniform_int_distribution<> s(0, subjects.size() - 1);
    std::uniform_int_distribution<> v(0, verbs.size() - 1);
    std::uniform_int_distribution<> o(0, objects.size() - 1);
    
    return subjects[s(gen)] + " " + verbs[v(gen)] + " " + objects[o(gen)];
}

void learn_fact_ultra_verbose(Storage* storage, episodic::EpisodicMemory* episodes, 
                              uint32_t episode_id, const std::string& fact, int fact_num) {
    
    std::cout << "\n┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓\n";
    std::cout << "┃ FACT #" << fact_num << ": " << fact << "\n";
    std::cout << "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛\n";
    
    // Parse into words
    std::istringstream iss(fact);
    std::vector<std::string> words;
    std::string word;
    
    std::cout << "  [PARSER] Tokenizing...\n";
    while (iss >> word) {
        std::string clean;
        for (char c : word) {
            if (std::isalnum(c) || c == '_') {
                clean += std::tolower(c);
            }
        }
        if (clean.length() >= 2) {
            words.push_back(clean);
            std::cout << "    → Token: \"" << clean << "\"\n";
        }
    }
    
    std::cout << "  [PARSER] Extracted " << words.size() << " tokens\n\n";
    
    if (words.size() < 2) {
        std::cout << "  [SKIP] Too few tokens\n";
        return;
    }
    
    // Create node chain
    std::cout << "  [STORAGE] Creating node chain...\n";
    NodeID prev_id = 0;
    
    for (size_t i = 0; i < words.size(); i++) {
        const auto& w = words[i];
        
        std::cout << "    [" << (i+1) << "/" << words.size() << "] Processing: \"" << w << "\"\n";
        
        // Find existing node
        auto nodes = storage->find_nodes(w);
        NodeID node_id;
        
        if (nodes.empty()) {
            // Create new node
            node_id = storage->create_node(w, NodeType::CONCEPT);
            std::cout << "      ✨ CREATED NEW NODE: ID=" << node_id << " content=\"" << w << "\"\n";
            
            // Set timestamp
            Node node;
            if (storage->get_node(node_id, node)) {
                node.creation_timestamp = episodic::get_timestamp_ms();
                node.episode_id = episode_id;
                storage->update_node(node);
                std::cout << "      ⏰ Timestamp: " << node.creation_timestamp << "\n";
                std::cout << "      📚 Episode: " << episode_id << "\n";
            }
            
            // Add to episode
            if (episodes && episode_id > 0) {
                episodes->add_node(episode_id, node_id);
                std::cout << "      📖 Added to episode memory\n";
            }
        } else {
            // Reuse existing node
            node_id = nodes[0].id;
            std::cout << "      ♻️  REUSED EXISTING NODE: ID=" << node_id << " content=\"" << w << "\"\n";
        }
        
        // Create edge from previous
        if (prev_id != 0) {
            EdgeID edge_id = storage->create_edge(prev_id, node_id, RelationType::EXACT, 1.0f);
            std::cout << "      🔗 CREATED EDGE: " << storage->get_node_content(prev_id) 
                      << " → " << w << " (ID=" << edge_id << ", weight=1.0)\n";
            
            // Set edge timestamp
            Edge edge;
            if (storage->get_edge(edge_id, edge)) {
                edge.creation_timestamp = episodic::get_timestamp_ms();
                edge.episode_id = episode_id;
                storage->update_edge(edge);
                std::cout << "      ⏰ Edge timestamp set\n";
            }
            
            // Add to episode
            if (episodes && episode_id > 0 && edge_id > 0) {
                episodes->add_edge(episode_id, edge_id);
                std::cout << "      📖 Edge added to episode\n";
            }
        }
        
        prev_id = node_id;
    }
    
    std::cout << "  [STORAGE] ✅ Fact fully integrated into knowledge graph\n";
}

int main(int argc, char** argv) {
    int max_cycles = 3;  // Short demo by default
    int facts_per_cycle = 2;
    
    if (argc > 1) max_cycles = std::atoi(argv[1]);
    if (argc > 2) facts_per_cycle = std::atoi(argv[2]);
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                               ║\n";
    std::cout << "║  🔬 MELVIN ULTRA-VERBOSE MODE                                 ║\n";
    std::cout << "║     See EVERYTHING happening inside the brain!                ║\n";
    std::cout << "║                                                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "This will show you:\n";
    std::cout << "  • Every token being parsed\n";
    std::cout << "  • Every node being created/reused\n";
    std::cout << "  • Every edge being formed\n";
    std::cout << "  • Episode memory operations\n";
    std::cout << "  • GNN message passing\n";
    std::cout << "  • LEAP discovery process\n";
    std::cout << "  • Storage operations\n\n";
    
    std::cout << "⚠️  Warning: This produces A LOT of output!\n\n";
    std::cout << "Running: " << max_cycles << " cycles, " << facts_per_cycle << " facts each\n\n";
    std::cout << "Press Enter to start...";
    std::cin.get();
    
    // Initialize
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  INITIALIZATION PHASE                                         ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "[STORAGE] Loading knowledge base...\n";
    auto storage = std::make_unique<Storage>();
    bool loaded = storage->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    std::cout << "  [STORAGE] Load result: " << (loaded ? "SUCCESS" : "FAILED/EMPTY") << "\n";
    std::cout << "  [STORAGE] Nodes in memory: " << storage->node_count() << "\n";
    std::cout << "  [STORAGE] Edges in memory: " << storage->edge_count() << "\n\n";
    
    std::cout << "[EPISODIC] Initializing episodic memory...\n";
    episodic::EpisodicMemory::Config ep_config;
    ep_config.verbose = false;
    auto episodes = std::make_unique<episodic::EpisodicMemory>(storage.get(), ep_config);
    std::cout << "  [EPISODIC] Ready\n\n";
    
    std::cout << "[GNN] Initializing graph neural network...\n";
    gnn::GNNPredictor::Config gnn_config;
    gnn_config.embed_dim = 16;
    gnn_config.verbose = false;
    auto gnn = std::make_unique<gnn::GNNPredictor>(gnn_config);
    std::cout << "  [GNN] Embedding dimension: " << gnn_config.embed_dim << "\n";
    
    auto nodes = storage->get_all_nodes();
    std::cout << "  [GNN] Initializing embeddings for " << nodes.size() << " nodes...\n";
    gnn->initialize_embeddings(nodes);
    std::cout << "  [GNN] ✅ Embeddings initialized\n\n";
    
    size_t start_nodes = storage->node_count();
    size_t start_edges = storage->edge_count();
    
    std::cout << "✅ All systems initialized\n\n";
    
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  LEARNING PHASE                                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
    
    int total_facts = 0;
    
    for (int cycle = 1; cycle <= max_cycles; cycle++) {
        std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  CYCLE " << cycle << " of " << max_cycles << std::string(52, ' ') << "║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
        
        std::cout << "\n[EPISODIC] Creating episode for cycle " << cycle << "...\n";
        uint32_t episode_id = episodes->create_episode("Cycle " + std::to_string(cycle));
        std::cout << "  [EPISODIC] Episode ID: " << episode_id << "\n";
        std::cout << "  [EPISODIC] Start time: " << episodic::get_timestamp_ms() << "\n\n";
        
        // Generate facts
        std::vector<std::string> facts;
        for (int i = 0; i < facts_per_cycle; i++) {
            facts.push_back(generate_fact());
        }
        
        std::cout << "[FACTS] Generated " << facts.size() << " facts for this cycle\n\n";
        
        // Learn each fact
        for (size_t i = 0; i < facts.size(); i++) {
            learn_fact_ultra_verbose(storage.get(), episodes.get(), episode_id, facts[i], total_facts + i + 1);
            std::cout << "\n";
        }
        
        total_facts += facts.size();
        
        std::cout << "[EPISODIC] Closing episode " << episode_id << "...\n";
        episodes->end_episode(episode_id);
        std::cout << "  [EPISODIC] End time: " << episodic::get_timestamp_ms() << "\n";
        std::cout << "  [EPISODIC] Episode complete\n\n";
        
        // GNN update
        std::cout << "[GNN] Running message passing...\n";
        auto all_nodes = storage->get_all_nodes();
        auto all_edges = storage->get_all_edges();
        
        std::cout << "  [GNN] Processing " << all_nodes.size() << " nodes, " 
                  << all_edges.size() << " edges\n";
        std::cout << "  [GNN] Message passing iteration 1...\n";
        gnn->message_pass(all_nodes, all_edges, storage.get());
        std::cout << "  [GNN] ✅ Node embeddings updated\n\n";
        
        // LEAP inference every cycle (for demo)
        if (cycle % 1 == 0 && cycle >= 2) {  // Skip first cycle
            std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
            std::cout << "║  LEAP INFERENCE PHASE (Pattern Discovery)                     ║\n";
            std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
            
            leap::LEAPInference::Config leap_config;
            leap_config.max_transitive_hops = 3;  // Reduced for visibility
            leap_config.min_shared_neighbors = 2;
            leap_config.leap_confidence = 0.7f;
            leap_config.verbose = true;  // SHOW EACH LEAP!
            
            leap::LEAPInference leap_system(leap_config);
            
            std::cout << "[LEAP] Starting transitive shortcut discovery...\n";
            int transitive = leap_system.create_transitive_leaps(storage.get());
            std::cout << "[LEAP] ✅ Created " << transitive << " transitive LEAPs\n\n";
            
            std::cout << "[LEAP] Starting similarity pattern discovery...\n";
            int similarity = leap_system.create_similarity_leaps(storage.get());
            std::cout << "[LEAP] ✅ Created " << similarity << " similarity LEAPs\n\n";
        }
        
        // Stats
        std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  CYCLE " << cycle << " COMPLETE                                          ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
        
        std::cout << "[STATS] Current brain state:\n";
        std::cout << "  Nodes: " << storage->node_count() << " (+" << (storage->node_count() - start_nodes) << ")\n";
        std::cout << "  Edges: " << storage->edge_count() << " (+" << (storage->edge_count() - start_edges) << ")\n";
        std::cout << "  EXACT edges: " << storage->edge_count_by_type(RelationType::EXACT) << "\n";
        std::cout << "  LEAP edges: " << storage->edge_count_by_type(RelationType::LEAP) << "\n";
        std::cout << "  Episodes: " << episodes->get_stats().total_episodes << "\n\n";
        
        if (cycle < max_cycles) {
            std::cout << "⏳ Next cycle in 2 seconds...\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    
    // Final save
    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  SAVING TO DISK                                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "[STORAGE] Writing nodes to melvin/data/nodes.melvin...\n";
    std::cout << "[STORAGE] Writing edges to melvin/data/edges.melvin...\n";
    storage->save("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    std::cout << "  [STORAGE] ✅ Knowledge base saved\n\n";
    
    std::cout << "[EPISODIC] Writing episodes to melvin/data/episodes.melvin...\n";
    episodes->save("melvin/data/episodes.melvin");
    std::cout << "  [EPISODIC] ✅ Episodes saved\n\n";
    
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ULTRA-VERBOSE SESSION COMPLETE                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "📊 WHAT YOU SAW:\n";
    std::cout << "  ✓ " << total_facts << " facts parsed into tokens\n";
    std::cout << "  ✓ Nodes created/reused for each token\n";
    std::cout << "  ✓ Edges formed between sequential tokens\n";
    std::cout << "  ✓ Timestamps and episodes tracked\n";
    std::cout << "  ✓ GNN embeddings updated\n";
    std::cout << "  ✓ LEAP shortcuts discovered\n";
    std::cout << "  ✓ Everything saved to disk\n\n";
    
    std::cout << "🧠 Final brain state:\n";
    storage->print_stats();
    std::cout << "\n";
    
    return 0;
}

