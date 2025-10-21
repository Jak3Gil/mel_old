/**
 * @file melvin_intelligent_conversation.cpp
 * @brief REAL INTELLIGENCE - Uses Melvin's actual cognitive pipeline
 * 
 * NO PATTERN MATCHING. NO IF-THEN STATEMENTS.
 * 
 * Instead:
 * 1. Text → Inject activation into context field
 * 2. Activation spreads through graph edges (Hopfield dynamics)
 * 3. Attention selects most active nodes
 * 4. Reasoning traverses from active context
 * 5. Response generated from activated semantic field
 * 6. Learning reinforces connections
 * 
 * This is how Melvin's brain ACTUALLY works.
 */

#include "melvin/include/melvin_graph.h"
#include "melvin/include/melvin_types.h"
#include "melvin/include/context_field.h"
#include "melvin/include/attention_dynamics.h"
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <map>

using namespace melvin;

// ============================================================================
// TEXT INPUT PROCESSOR - Convert text to concept activations
// ============================================================================

class TextInputProcessor {
public:
    TextInputProcessor(AtomicGraph& graph) : graph_(graph) {}
    
    /**
     * Process text input → inject into context field
     * Returns activated concept IDs
     */
    std::vector<uint64_t> process_text(const std::string& text, ContextField& context) {
        std::cout << "\n[Text Input: \"" << text << "\"]\n";
        
        // Tokenize into words
        auto words = tokenize(text);
        std::cout << "  Tokenized into " << words.size() << " words\n";
        
        std::vector<uint64_t> activated_nodes;
        
        // For each word, get/create concept node and inject activation
        for (const auto& word : words) {
            if (word.length() < 3) continue;  // Skip short words
            
            uint64_t node_id = graph_.get_or_create_concept(word);
            
            // Inject activation into context field
            // Strength based on word importance (simple heuristic for now)
            float activation_strength = 0.7f;
            context.inject(node_id, activation_strength, ActivationSource::SENSORY_INPUT);
            
            activated_nodes.push_back(node_id);
            std::cout << "    Injected: " << word << " (node " << node_id << ") → " 
                      << activation_strength << "\n";
        }
        
        return activated_nodes;
    }
    
private:
    AtomicGraph& graph_;
    
    std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> words;
        std::string word;
        
        for (char c : text) {
            if (std::isalnum(c)) {
                word += std::tolower(c);
            } else if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        if (!word.empty()) words.push_back(word);
        
        return words;
    }
};

// ============================================================================
// COGNITIVE CONVERSATION ENGINE - Uses Real Brain Pipeline
// ============================================================================

class CognitiveConversationEngine {
public:
    CognitiveConversationEngine(AtomicGraph& graph, ContextField& context)
        : graph_(graph)
        , context_(context)
        , attention_()
        , text_processor_(graph)
        , cycle_count_(0)
    {
        std::cout << "\n[CognitiveConversationEngine] Using REAL cognitive pipeline\n";
        std::cout << "  - Context field for activation spreading\n";
        std::cout << "  - Attention dynamics for focus selection\n";
        std::cout << "  - Graph traversal for reasoning\n";
        std::cout << "  - Hebbian learning for edge formation\n";
    }
    
    std::string respond(const std::string& user_input) {
        cycle_count_++;
        std::cout << "\n═══════════════════════════════════════════════════════\n";
        std::cout << "  COGNITIVE CYCLE #" << cycle_count_ << "\n";
        std::cout << "═══════════════════════════════════════════════════════\n";
        
        // ====================================================================
        // PHASE 1: PERCEPTION - Inject text into context field
        // ====================================================================
        std::cout << "\n[PHASE 1: PERCEPTION]\n";
        auto input_nodes = text_processor_.process_text(user_input, context_);
        
        // ====================================================================
        // PHASE 2: ACTIVATION SPREADING - Let ideas flow through graph
        // ====================================================================
        std::cout << "\n[PHASE 2: ACTIVATION SPREADING]\n";
        std::cout << "  Running diffusion cycles...\n";
        
        // Diffuse activation through graph edges (like neural spreading)
        for (int i = 0; i < 3; i++) {
            context_.diffuse(0.08f);  // Spread activation
            context_.decay();          // Forgetting
            std::cout << "    Cycle " << (i+1) << ": Activation spread\n";
        }
        
        context_.normalize();  // Prevent runaway
        
        // ====================================================================
        // PHASE 3: ATTENTION - Select most active concepts
        // ====================================================================
        std::cout << "\n[PHASE 3: ATTENTION SELECTION]\n";
        
        // Get top activated nodes (what Melvin is "thinking about")
        // Use the actual context_field API
        auto active_nodes = context_.get_top_active(10);  // Top 10 most active
        
        std::cout << "  Top active concepts after spreading:\n";
        for (size_t i = 0; i < std::min((size_t)5, active_nodes.size()); i++) {
            auto [node_id, activation] = active_nodes[i];
            std::cout << "    #" << (i+1) << ": Node " << node_id 
                      << " (activation: " << activation << ")\n";
        }
        
        // ====================================================================
        // PHASE 4: REASONING - Traverse graph from active concepts
        // ====================================================================
        std::cout << "\n[PHASE 4: REASONING]\n";
        
        // Build response from activated semantic network
        std::vector<std::string> response_elements;
        std::map<uint64_t, int> visited_count;
        
        // For each highly active node, explore its neighborhood
        for (size_t i = 0; i < std::min((size_t)3, active_nodes.size()); i++) {
            auto [node_id, activation] = active_nodes[i];
            
            if (activation < 0.3f) break;  // Only use strongly active
            
            // Get neighbors of this active concept
            auto neighbors = graph_.all_neighbors(node_id, 16);
            
            if (!neighbors.empty()) {
                std::cout << "    Node " << node_id << " has " 
                          << neighbors.size() << " connections\n";
                
                // Explore neighborhood to build understanding
                for (uint64_t neighbor_id : neighbors) {
                    visited_count[neighbor_id]++;
                    
                    // Check what relation connects them
                    for (int r = 0; r < 12; r++) {
                        Rel rel = static_cast<Rel>(r);
                        float w = graph_.get_edge_weight(node_id, neighbor_id, rel);
                        
                        if (w > 0.2f) {
                            response_elements.push_back(
                                "Node " + std::to_string(node_id) + " " +
                                relation_name(rel) + " node " +  
                                std::to_string(neighbor_id) + " (weight: " +
                                std::to_string(w) + ")"
                            );
                            
                            if (response_elements.size() >= 5) break;
                        }
                    }
                    if (response_elements.size() >= 5) break;
                }
            } else {
                response_elements.push_back(
                    "Node " + std::to_string(node_id) + " is isolated (new concept)"
                );
            }
        }
        
        // ====================================================================
        // PHASE 5: RESPONSE GENERATION - From activated field
        // ====================================================================
        std::cout << "\n[PHASE 5: RESPONSE GENERATION]\n";
        
        std::string response;
        
        if (response_elements.empty()) {
            response = "My context field is building activation patterns. " +
                      std::to_string(active_nodes.size()) + " concepts are active. " +
                      "This is fresh territory - teach me more!";
        } else {
            response = "Based on my activated semantic field, I found " +
                      std::to_string(response_elements.size()) + " strong connections:\n";
            
            for (size_t i = 0; i < std::min((size_t)3, response_elements.size()); i++) {
                response += "  • " + response_elements[i] + "\n";
            }
            
            // Add context field state
            response += "\nCurrent activation state: " +
                       std::to_string(active_nodes.size()) + " concepts above threshold.";
        }
        
        // ====================================================================
        // PHASE 6: LEARNING - Reinforce connections from this interaction
        // ====================================================================
        std::cout << "\n[PHASE 6: LEARNING]\n";
        
        // Connect concepts that co-occurred in input
        if (input_nodes.size() >= 2) {
            for (size_t i = 0; i < input_nodes.size() - 1; i++) {
                graph_.add_or_bump_edge(
                    input_nodes[i], 
                    input_nodes[i+1],
                    Rel::CO_OCCURS_WITH,
                    0.1f  // Hebbian reinforcement
                );
                std::cout << "  Reinforced: node " << input_nodes[i] 
                          << " ↔ " << input_nodes[i+1] << "\n";
            }
        }
        
        std::cout << "  Brain now has " << graph_.edge_count() << " total edges\n";
        
        return response;
    }
    
private:
    AtomicGraph& graph_;
    ContextField& context_;
    AttentionDynamics attention_;
    TextInputProcessor text_processor_;
    uint64_t cycle_count_;
    
    std::string relation_name(Rel r) const {
        switch (r) {
            case Rel::TEMPORAL_NEXT: return "→(next)";
            case Rel::SPATIALLY_NEAR: return "→(near)";
            case Rel::SIMILAR_FEATURES: return "→(similar)";
            case Rel::PART_OF: return "⊂(part of)";
            case Rel::CO_OCCURS_WITH: return "↔(with)";
            case Rel::OBSERVED_AS: return "→(seen as)";
            case Rel::NAMED_AS: return "=(named)";
            case Rel::USED_FOR: return "→(used for)";
            case Rel::EXPECTS: return "⇒(expects)";
            case Rel::CAUSES: return "⇒(causes)";
            case Rel::ACTIVE_FOCUS: return "👁(focus)";
            case Rel::INHIBITS: return "⊣(inhibits)";
            default: return "→";
        }
    }
};

// ============================================================================
// MAIN - Real Cognitive Loop
// ============================================================================

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║    MELVIN - REAL COGNITIVE CONVERSATION SYSTEM            ║\n";
    std::cout << "║                                                            ║\n";
    std::cout << "║  Using ACTUAL brain pipeline:                             ║\n";
    std::cout << "║  • Context Field (activation spreading)                   ║\n";
    std::cout << "║  • Attention Dynamics (focus selection)                   ║\n";
    std::cout << "║  • Graph Traversal (reasoning)                            ║\n";
    std::cout << "║  • Hebbian Learning (edge reinforcement)                  ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    
    try {
        std::cout << "\n🧠 Loading brain...\n";
        
        // Initialize graph
        AtomicGraph graph;
        if (graph.load("melvin_nodes.bin", "melvin_edges.bin")) {
            std::cout << "  ✓ Loaded: " << graph.node_count() << " nodes, " 
                      << graph.edge_count() << " edges\n";
        } else {
            std::cout << "  ✓ Fresh brain ready to learn\n";
        }
        
        // Initialize context field (working memory / activation)
        ContextField context(graph);
        std::cout << "  ✓ Context field initialized\n";
        
        // Initialize cognitive conversation engine (attention is internal)
        CognitiveConversationEngine cognition(graph, context);
        std::cout << "  ✓ Cognitive engine ready\n";
        
        std::cout << "\n✅ Melvin's brain is ACTIVE!\n";
        std::cout << "════════════════════════════════════════════════════════════\n";
        std::cout << "\nThis uses REAL cognitive processing:\n";
        std::cout << "  1. Your text activates concept nodes\n";
        std::cout << "  2. Activation spreads through edges (like neurons!)\n";
        std::cout << "  3. Attention selects most active concepts\n";
        std::cout << "  4. Response emerges from activated field\n";
        std::cout << "  5. Connections strengthen (Hebbian learning)\n";
        std::cout << "\nType 'quit' to exit\n\n";
        
        // Main conversation loop
        while (true) {
            std::cout << "\n------------------------------------------------------------\n";
            std::cout << "You: ";
            
            std::string input;
            std::getline(std::cin, input);
            
            if (input.empty()) continue;
            if (input == "quit" || input == "exit" || input == "q") {
                std::cout << "\nMelvin: Goodbye! Saving brain state...\n";
                break;
            }
            
            // Run full cognitive cycle
            std::string response = cognition.respond(input);
            
            std::cout << "\n" << std::string(60, '-') << "\n";
            std::cout << "Melvin: " << response << "\n";
            std::cout << std::string(60, '-') << "\n";
        }
        
        // Save learned knowledge
        graph.save("melvin_nodes.bin", "melvin_edges.bin");
        std::cout << "\n💾 Brain saved: " << graph.node_count() << " nodes, " 
                  << graph.edge_count() << " edges\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

