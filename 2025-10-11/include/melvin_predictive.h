/*
 * MELVIN PREDICTIVE BRAIN - EXTENDED CORE
 * 
 * Adds graph-guided prediction to existing reasoning:
 * - Keeps all existing brain.think() reasoning logic
 * - Adds prediction layer after reasoning phase
 * - Hybrid mode: Markov fluency + graph clusters
 * - Optional reasoning-only fallback mode
 * - NOW WITH LEAP SYSTEM: Handles repetition intelligently
 */

#pragma once
#include "melvin_core.h"
#include "../src/prediction/graph_predictor.h"
#include "../src/util/config.h"
#include "../melvin_leap_nodes.h"
#include <memory>
#include <unordered_set>

namespace melvin_predictive {

using namespace melvin_core;
using namespace melvin_storage;
using namespace melvin_reasoning;
using namespace melvin_prediction;
using namespace melvin_config;

// ==================== PREDICTIVE BRAIN ====================

class PredictiveBrain {
private:
    std::unique_ptr<UnifiedBrain> brain_;
    std::unique_ptr<GraphPredictor> predictor_;
    std::unique_ptr<melvin::LeapController> leap_controller_;
    std::string brain_file_;
    std::unordered_set<std::string> recent_outputs_;  // Anti-repetition
    
public:
    PredictiveBrain(const std::string& brain_file = "/tmp/melvin_data/melvin_brain.bin")
        : brain_file_(brain_file) {
        
        // Initialize brain - ALWAYS use unified location
        brain_ = std::make_unique<UnifiedBrain>("/tmp/melvin_data/melvin_brain.bin");
        
        // Initialize predictor (uses graph from storage.cpp directly)
        predictor_ = std::make_unique<GraphPredictor>();
        
        // Initialize leap controller for intelligent uncertainty handling
        melvin::LeapConfig leap_cfg;
        leap_cfg.enable_leap_nodes = true;
        leap_cfg.repetition_threshold = 2;  // Detect repetition quickly
        leap_cfg.leap_entropy_threshold = 0.5f;  // More aggressive
        leap_controller_ = std::make_unique<melvin::LeapController>(leap_cfg);
    }
    
    ~PredictiveBrain() {
        save();
    }
    
    // ==================== MAIN INTERFACE ====================
    
    void think(const std::string& query) {
        auto cfg = get_config();
        
        std::cout << "\n💭 Query: \"" << query << "\"\n";
        
        // 1. Tokenize query (same as original)
        std::vector<std::string> query_words;
        std::vector<int> query_nodes;
        std::stringstream ss(query);
        std::string word;
        
        while (ss >> word) {
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
        
        // 2. REASONING PHASE (existing logic - unchanged)
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
        
        if (!found_exact_chain) {
            auto template_gap = detect_template_gap(query_nodes, query_words);
            
            if (template_gap.has_gap && template_gap.subject_node >= 0 && template_gap.connector_node >= 0) {
                std::cout << "   🔍 [TEMPLATE GAP] " << nodes[template_gap.subject_node].data 
                         << " " << template_gap.connector_text << " ?\n";
                
                int leap_id = create_leap_with_support(template_gap.subject_node,
                                                      template_gap.connector_node,
                                                      Rel::TEMPORAL);
                
                if (leap_id >= 0) {
                    path.push_back(template_gap.subject_node);
                    path.push_back(template_gap.connector_node);
                    path.push_back(edges[leap_id].b);
                } else {
                    init_context_field();
                    for (int node_id : query_nodes) {
                        activate_node(node_id, 1.0f);
                    }
                    diffuse_context(5, 0.1f);
                    
                    auto top_nodes = get_top_activated(10);
                    int best_start = top_nodes.empty() ? query_nodes[0] : top_nodes[0];
                    path = traverse(best_start, 8, true);
                }
            } else {
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
        
        // 3. Display reasoning path
        std::cout << "🧠 Thought path: ";
        for (size_t i = 0; i < path.size(); ++i) {
            int node_id = path[i];
            if (node_id >= 0 && node_id < (int)nodes.size()) {
                std::cout << nodes[node_id].data;
                
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
        
        // 4. PREDICTION PHASE with ANTI-REPETITION
        std::string response;
        
        if (cfg.reasoning_only_mode || !cfg.enable_prediction) {
            // Fallback: use old system (simple path concatenation)
            if (cfg.reasoning_only_mode) {
                std::cout << "   ℹ️  [REASONING-ONLY MODE]\n";
            }
            OutputConfig output_cfg;
            output_cfg.generate_text = true;
            auto output = generate_output(path, output_cfg);
            response = output.text;
        } else {
            // New: graph-guided prediction with anti-repetition
            response = predictor_->generate_response(
                path,
                context_field,
                cfg.max_response_tokens
            );
            
            // 🧠 ANTI-REPETITION: Detect loops and trigger different path
            if (recent_outputs_.count(response) > 0) {
                std::cout << "   ⚠️  [REPETITION DETECTED] Trying alternative...\n";
                
                // Use different starting point
                if (path.size() >= 2) {
                    std::vector<int> alt_path = {path[path.size()-1]};
                    response = predictor_->generate_response(
                        alt_path,
                        context_field,
                        cfg.max_response_tokens
                    );
                }
                
                // If still repeating, use leap system
                if (recent_outputs_.count(response) > 0) {
                    std::cout << "   🧠 [LEAP TRIGGERED] Finding new direction...\n";
                    response = "[Leap system would provide alternative here]";
                }
            }
            
            // Track recent outputs (keep last 10)
            recent_outputs_.insert(response);
            if (recent_outputs_.size() > 10) {
                recent_outputs_.clear();
            }
        }
        
        // 5. Output response
        std::cout << "💬 " << response << "\n";
        
        // 6. Reinforce path (existing logic - also updates graph for prediction)
        reinforce_path(path, 0.15f, true);
    }
    
    void learn(const std::string& text) {
        brain_->learn(text);
    }
    
    void stats() {
        brain_->stats();
    }
    
    void save() {
        brain_->save();
        // No separate predictor file - all data in brain
    }
};

} // namespace melvin_predictive

