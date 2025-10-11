/*
 * MELVIN UNIFIED BRAIN - CORE ORCHESTRATOR
 * Central control for the complete cognitive loop
 */

#pragma once
#include "melvin_storage.h"
#include "melvin_reasoning.h"
#include "melvin_sensory.h"
#include "melvin_output.h"
#include "melvin_learning.h"

namespace melvin_core {

using namespace melvin_storage;
using namespace melvin_reasoning;
using namespace melvin_sensory;
using namespace melvin_output;
using namespace melvin_learning;

// ==================== UNIFIED BRAIN CLASS ====================

class UnifiedBrain {
private:
    int generation_ = 0;
    int cycle_count_ = 0;
    std::string brain_file_ = "melvin_unified_brain.bin";
    
public:
    UnifiedBrain(const std::string& brain_file = "melvin_unified_brain.bin") 
        : brain_file_(brain_file) {
        load_brain(brain_file_);
    }
    
    ~UnifiedBrain() {
        save_brain(brain_file_);
    }
    
    // ==================== CORE LOOP ====================
    
    void think(const std::string& query) {
        std::cout << "\n💭 Query: \"" << query << "\"\n";
        
        // 1. Tokenize and clean query
        std::vector<std::string> query_words;
        std::vector<int> query_nodes;
        std::stringstream ss(query);
        std::string word;
        
        while (ss >> word) {
            // Clean word
            word.erase(std::remove_if(word.begin(), word.end(), 
                      [](char c) { return std::ispunct(c); }), word.end());
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            
            if (word.length() >= 2) {
                query_words.push_back(word);
                
                auto it = node_lookup.find(word);
                if (it != node_lookup.end()) {
                    query_nodes.push_back(it->second);
                }
            }
        }
        
        if (query_nodes.empty()) {
            std::cout << "   ⚠️  No known words in query\n";
            return;
        }
        
        // 2. Try to find EXACT chain first
        std::vector<int> path;
        bool found_exact_chain = false;
        
        if (query_nodes.size() >= 2) {
            for (size_t i = 0; i + 1 < query_nodes.size(); ++i) {
                auto exact_path = find_exact_chain(query_nodes[i], query_nodes[i+1], 5);
                if (!exact_path.empty() && exact_path.size() > 1) {
                    path = exact_path;
                    found_exact_chain = true;
                    std::cout << "   ✅ [EXACT CHAIN] Found direct path\n";
                    break;
                }
            }
        }
        
        // 3. If no EXACT chain, try template-gap LEAP reasoning
        if (!found_exact_chain) {
            // 3a. Detect if query has template structure
            auto template_gap = detect_template_gap(query_nodes, query_words);
            
            if (template_gap.has_gap && template_gap.subject_node >= 0 && template_gap.connector_node >= 0) {
                std::cout << "   🔍 [TEMPLATE GAP] " << nodes[template_gap.subject_node].data 
                         << " " << template_gap.connector_text << " ?\n";
                std::cout << "   📊 Analyzing crowd support...\n";
                
                // 3b. Use crowd-support LEAP creation
                int leap_id = create_leap_with_support(template_gap.subject_node, 
                                                      template_gap.connector_node, 
                                                      Rel::TEMPORAL);
                
                if (leap_id >= 0) {
                    // LEAP created with crowd support!
                    // Now traverse from subject through the LEAP
                    path.push_back(template_gap.subject_node);
                    path.push_back(template_gap.connector_node);
                    path.push_back(edges[leap_id].b);  // The target
                } else {
                    // No sufficient crowd support, fall back to similarity-based
                    std::cout << "   🔄 Falling back to similarity-based LEAP...\n";
                    
                    init_context_field();
                    for (int node_id : query_nodes) {
                        activate_node(node_id, 1.0f);
                    }
                    diffuse_context(5, 0.1f);
                    
                    auto top_nodes = get_top_activated(10);
                    
                    for (int query_node : query_nodes) {
                        for (int activated_node : top_nodes) {
                            if (query_node != activated_node) {
                                float score = compute_leap_score(query_node, activated_node);
                                if (score > LEAP_THRESHOLD) {
                                    int fallback_leap = create_leap_if_needed(query_node, activated_node);
                                    if (fallback_leap >= 0) {
                                        std::cout << "   🔮 [FALLBACK LEAP] " << nodes[query_node].data 
                                                 << " --LEAP--> " << nodes[activated_node].data 
                                                 << " (score: " << std::fixed << std::setprecision(2) << score << ")\n";
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    
                    int best_start = top_nodes.empty() ? query_nodes[0] : top_nodes[0];
                    path = traverse(best_start, 8, true);
                }
            } else {
                // 3c. No template detected, use old approach
                init_context_field();
                for (int node_id : query_nodes) {
                    activate_node(node_id, 1.0f);
                }
                diffuse_context(5, 0.1f);
                
                auto top_nodes = get_top_activated(10);
                int best_start = top_nodes.empty() ? query_nodes[0] : top_nodes[0];
                path = traverse(best_start, 8, true);
            }
        }
        
        // 4. Generate output
        OutputConfig config;
        config.generate_text = true;
        auto output = generate_output(path, config);
        
        // 5. Display path with edge type annotations
        std::cout << "🧠 Thought path: ";
        for (size_t i = 0; i < path.size(); ++i) {
            int node_id = path[i];
            if (node_id >= 0 && node_id < (int)nodes.size()) {
                std::cout << nodes[node_id].data;
                
                // Show edge type for next connection
                if (i + 1 < path.size()) {
                    auto it = adjacency.find(node_id);
                    if (it != adjacency.end()) {
                        for (int edge_id : it->second) {
                            if (edges[edge_id].b == path[i+1]) {
                                std::cout << (edges[edge_id].is_exact() ? " =[EXACT]=> " : " ~[LEAP]~> ");
                                break;
                            }
                        }
                    }
                }
            }
        }
        std::cout << "\n";
        
        speak(output.text);
        
        // 6. Reinforce successful path
        reinforce_path(path, 0.15f, true);
        
        cycle_count_++;
    }
    
    void learn(const std::string& text) {
        std::cout << "📚 Learning: \"" << text << "\"\n";
        teach_text(text);
    }
    
    void learn_multimodal(const std::string& text, 
                         const std::vector<uint8_t>& audio,
                         const std::string& concept_label) {
        std::cout << "🌐 Learning multimodal pair: \"" << concept_label << "\"\n";
        teach_multimodal_pair(text, audio, concept_label);
    }
    
    void evolve() {
        generation_++;
        melvin_learning::evolve(generation_);
    }
    
    void consolidate() {
        consolidate_memory();
    }
    
    void decay() {
        decay_pass(0.02f);
    }
    
    void stats() {
        print_stats();
        std::cout << "   Generation: " << generation_ << "\n";
        std::cout << "   Cycles: " << cycle_count_ << "\n";
        std::cout << "   Coherence: " << std::fixed << std::setprecision(3) 
                  << evaluate_coherence() << "\n";
    }
    
    void save() {
        save_brain(brain_file_);
    }
};

// ==================== DEMO FUNCTIONS ====================

inline void run_basic_demo() {
    std::cout << "🧠 MELVIN UNIFIED BRAIN - EXACT + LEAP DEMO\n";
    std::cout << "===========================================\n\n";
    std::cout << "This demo shows:\n";
    std::cout << "  • EXACT connections from training\n";
    std::cout << "  • LEAP creation when gaps exist\n";
    std::cout << "  • Automatic promotion of successful LEAPs to EXACT\n\n";
    
    UnifiedBrain brain;
    
    // Phase 1: Teach facts (creates EXACT connections)
    std::cout << "📚 PHASE 1: Teaching Facts (EXACT connections)\n";
    std::cout << std::string(50, '=') << "\n";
    brain.learn("dogs are fluffy");
    brain.learn("cats are animals");  // Cats exist but not as fluffy
    brain.learn("birds can fly");
    
    std::cout << "\n";
    brain.stats();
    
    // Phase 2: Query with known EXACT chain
    std::cout << "\n📖 PHASE 2: Query With Known Facts (EXACT chain)\n";
    std::cout << std::string(50, '=') << "\n";
    std::cout << "Query uses direct training: 'dogs are fluffy'\n";
    brain.think("what are dogs");
    
    // Phase 3: Query with unknown combination (should create LEAP)
    std::cout << "\n🔮 PHASE 3: Query With Unknown Combination (LEAP creation)\n";
    std::cout << std::string(50, '=') << "\n";
    std::cout << "Query: 'are cats fluffy?' - Not directly trained!\n";
    std::cout << "  • We know: dogs ARE fluffy\n";
    std::cout << "  • We know: cats ARE animals\n";
    std::cout << "  • Shared word: 'are' - both connect via 'are'!\n";
    std::cout << "  • System should infer: cats ~LEAP~> fluffy\n\n";
    brain.think("are cats fluffy");
    
    std::cout << "\n";
    brain.stats();
    
    // Phase 4: Reinforce the leap
    std::cout << "\n🔁 PHASE 4: Reinforce LEAP (repeated query)\n";
    std::cout << std::string(50, '=') << "\n";
    std::cout << "Asking again to strengthen the LEAP...\n";
    brain.think("what are cats");
    brain.think("what are cats");
    brain.think("what are cats");
    
    // Phase 5: Check for promotion
    std::cout << "\n✨ PHASE 5: Check For Promotion\n";
    std::cout << std::string(50, '=') << "\n";
    brain.stats();
}

inline void run_multimodal_demo() {
    std::cout << "🧠 MELVIN UNIFIED BRAIN - MULTIMODAL DEMO\n";
    std::cout << "=========================================\n\n";
    
    UnifiedBrain brain;
    
    // Teach text
    brain.learn("cats are mammals");
    brain.learn("dogs are mammals");
    
    // Teach multimodal pairs
    std::vector<uint8_t> meow_sound = {0x4D, 0x45, 0x4F, 0x57};  // "MEOW" in bytes
    std::vector<uint8_t> bark_sound = {0x42, 0x41, 0x52, 0x4B};  // "BARK" in bytes
    
    brain.learn_multimodal("cat says meow", meow_sound, "cat");
    brain.learn_multimodal("dog says bark", bark_sound, "dog");
    
    std::cout << "\n";
    brain.stats();
    
    // Test reasoning
    std::cout << "\n" << std::string(50, '=') << "\n";
    brain.think("what is a cat");
    
    std::cout << "\n" << std::string(50, '=') << "\n";
    brain.think("what sound does cat make");
    
    // Show cross-modal edges
    std::cout << "\n📊 Cross-modal connections:\n";
    int cross_modal_count = 0;
    for (const auto& edge : edges) {
        if (edge.is_cross_modal) {
            std::cout << "   " << nodes[edge.a].data << " ←→ " 
                     << nodes[edge.b].data << " (w=" << edge.w << ")\n";
            cross_modal_count++;
        }
    }
    std::cout << "   Total: " << cross_modal_count << " cross-modal edges\n";
}

inline void run_continuous_demo(int cycles = 100) {
    std::cout << "🧠 MELVIN UNIFIED BRAIN - CONTINUOUS LEARNING\n";
    std::cout << "=============================================\n\n";
    
    UnifiedBrain brain;
    
    std::vector<std::string> knowledge = {
        "cats are mammals",
        "dogs are mammals",
        "mammals have fur",
        "birds can fly",
        "birds have feathers",
        "fish live in water",
        "water is liquid",
        "liquids can flow",
        "energy is important",
        "plants need sunlight"
    };
    
    for (int cycle = 0; cycle < cycles; ++cycle) {
        // Learn random fact
        std::string fact = knowledge[rand() % knowledge.size()];
        brain.learn(fact);
        
        // Test occasionally
        if (cycle % 10 == 0) {
            std::cout << "\n🔄 Cycle " << cycle << "\n";
            brain.think("what are cats");
            brain.stats();
        }
        
        // Evolve occasionally
        if (cycle % 20 == 0) {
            brain.evolve();
        }
        
        // Decay occasionally
        if (cycle % 15 == 0) {
            brain.decay();
        }
        
        // Consolidate occasionally
        if (cycle % 30 == 0) {
            brain.consolidate();
        }
    }
    
    std::cout << "\n🎉 FINAL STATE:\n";
    brain.stats();
    brain.save();
}

} // namespace melvin_core

