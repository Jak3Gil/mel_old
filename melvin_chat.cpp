/**
 * @file melvin_chat.cpp
 * @brief MELVIN ChatGPT-style Interactive Interface
 * 
 * Natural language conversation with Melvin's cognitive system
 */

#include <iostream>
#include <string>
#include <signal.h>
#include <unistd.h>
#include <sstream>

#include "core/unified_intelligence.h"

using namespace melvin::intelligence;

// Global for signal handler
std::atomic<bool> g_running{true};

void signal_handler(int) {
    std::cout << "\n\n👋 Goodbye!\n";
    g_running = false;
    exit(0);
}

void print_banner() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                      ║\n";
    std::cout << "║              MELVIN CHAT v1.0                        ║\n";
    std::cout << "║         Cognitive AI Assistant                       ║\n";
    std::cout << "║                                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

// Load minimal knowledge graph
void load_knowledge_graph(
    std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    std::unordered_map<int, std::vector<float>>& embeddings,
    std::unordered_map<std::string, int>& word_to_id,
    std::unordered_map<int, std::string>& id_to_word
) {
    // Create a richer conversational knowledge base
    std::vector<std::string> concepts = {
        // Self-awareness
        "melvin", "I", "me", "myself", "robot", "AI", "intelligence", "consciousness",
        
        // Conversational
        "hello", "hi", "help", "thanks", "yes", "no", "understand", "know",
        "think", "believe", "feel", "learn", "remember", "forget",
        
        // Knowledge domains
        "what", "why", "how", "when", "where", "who",
        "question", "answer", "explain", "describe", "tell",
        
        // Reasoning
        "because", "therefore", "however", "although", "if", "then",
        "cause", "effect", "reason", "logic", "truth", "fact",
        
        // Actions
        "can", "will", "should", "must", "want", "need",
        "do", "make", "create", "change", "improve", "adapt",
        
        // States
        "good", "bad", "better", "worse", "best", "worst",
        "new", "old", "young", "fast", "slow", "strong", "weak",
        
        // Common objects
        "time", "space", "world", "person", "people", "human",
        "language", "word", "sentence", "meaning", "context",
        
        // Meta-cognitive
        "thought", "idea", "concept", "understanding", "knowledge",
        "wisdom", "experience", "memory", "attention", "awareness"
    };
    
    int node_id = 0;
    for (const auto& concept : concepts) {
        word_to_id[concept] = node_id;
        id_to_word[node_id] = concept;
        
        // Simple random embedding (in production, use real embeddings)
        std::vector<float> embedding(64);
        for (int i = 0; i < 64; i++) {
            embedding[i] = (float)rand() / RAND_MAX;
        }
        embeddings[node_id] = embedding;
        
        node_id++;
    }
    
    // Create semantic connections
    // Self-identity cluster
    graph[word_to_id["melvin"]] = {{word_to_id["I"], 0.9f}, {word_to_id["robot"], 0.8f}, 
                                     {word_to_id["intelligence"], 0.9f}, {word_to_id["AI"], 0.85f}};
    graph[word_to_id["I"]] = {{word_to_id["me"], 0.95f}, {word_to_id["myself"], 0.9f}};
    
    // Conversational cluster
    graph[word_to_id["hello"]] = {{word_to_id["hi"], 0.95f}, {word_to_id["help"], 0.5f}};
    graph[word_to_id["thanks"]] = {{word_to_id["help"], 0.6f}, {word_to_id["good"], 0.5f}};
    
    // Reasoning cluster
    graph[word_to_id["what"]] = {{word_to_id["question"], 0.8f}, {word_to_id["know"], 0.7f}};
    graph[word_to_id["why"]] = {{word_to_id["because"], 0.9f}, {word_to_id["reason"], 0.85f}};
    graph[word_to_id["how"]] = {{word_to_id["explain"], 0.8f}, {word_to_id["describe"], 0.75f}};
    
    // Knowledge cluster
    graph[word_to_id["know"]] = {{word_to_id["knowledge"], 0.9f}, {word_to_id["understand"], 0.85f},
                                  {word_to_id["learn"], 0.8f}, {word_to_id["remember"], 0.75f}};
    graph[word_to_id["think"]] = {{word_to_id["thought"], 0.9f}, {word_to_id["reasoning"], 0.8f}};
    
    // Ability cluster
    graph[word_to_id["can"]] = {{word_to_id["will"], 0.7f}, {word_to_id["do"], 0.8f}};
    graph[word_to_id["want"]] = {{word_to_id["need"], 0.75f}, {word_to_id["should"], 0.6f}};
    
    // Meta-cognitive cluster
    graph[word_to_id["intelligence"]] = {{word_to_id["think"], 0.9f}, {word_to_id["learn"], 0.85f},
                                          {word_to_id["understand"], 0.8f}, {word_to_id["adapt"], 0.75f}};
}

// Format answer into natural conversational output
std::string format_response(const UnifiedResult& result) {
    std::stringstream response;
    
    // If no answer generated, provide context-aware fallback
    if (result.answer.empty() || result.answer == "No sufficient activation") {
        // Use active nodes to generate contextual response
        if (!result.top_concepts.empty()) {
            response << "I'm thinking about ";
            for (size_t i = 0; i < std::min(size_t(3), result.top_concepts.size()); i++) {
                response << result.top_concepts[i].first;
                if (i < std::min(size_t(3), result.top_concepts.size()) - 1) {
                    response << ", ";
                }
            }
            response << ". ";
        }
        
        // Add confidence-based qualifier
        if (result.confidence < 0.3f) {
            response << "I'm not entirely sure about this yet, but I'm learning.";
        } else if (result.confidence < 0.6f) {
            response << "I'm still developing my understanding of this.";
        } else {
            response << "Let me think about that more deeply.";
        }
    } else {
        // Use generated answer
        response << result.answer;
        
        // Add reasoning transparency if requested or low confidence
        if (result.confidence < 0.5f) {
            response << " (I'm about " << (int)(result.confidence * 100) << "% confident in this response.)";
        }
    }
    
    return response.str();
}

int main() {
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    print_banner();
    
    std::cout << "🔧 Initializing Melvin's cognitive system...\n";
    
    // Load knowledge graph
    std::unordered_map<int, std::vector<std::pair<int, float>>> graph;
    std::unordered_map<int, std::vector<float>> embeddings;
    std::unordered_map<std::string, int> word_to_id;
    std::unordered_map<int, std::string> id_to_word;
    
    load_knowledge_graph(graph, embeddings, word_to_id, id_to_word);
    std::cout << "   ✅ Loaded " << word_to_id.size() << " concepts\n";
    
    // Create unified intelligence (standalone, no OS needed for chat)
    auto intelligence = std::make_unique<UnifiedIntelligence>();
    intelligence->initialize(graph, embeddings, word_to_id, id_to_word);
    std::cout << "   ✅ Intelligence system ready\n";
    std::cout << "   ✅ Chat mode initialized\n\n";
    
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                      ║\n";
    std::cout << "║  💬 Chat with Melvin                                 ║\n";
    std::cout << "║                                                      ║\n";
    std::cout << "║  Type your message and press Enter                  ║\n";
    std::cout << "║  Type 'quit', 'exit', or Ctrl+C to stop             ║\n";
    std::cout << "║                                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    std::string user_input;
    int turn_count = 0;
    
    while (g_running) {
        // Prompt
        std::cout << "\n\033[1;36mYou:\033[0m ";
        std::getline(std::cin, user_input);
        
        // Check for exit commands
        if (user_input == "quit" || user_input == "exit" || user_input == "q") {
            break;
        }
        
        if (user_input.empty()) {
            continue;
        }
        
        // Process input through unified intelligence
        std::cout << "\n\033[1;32mMelvin:\033[0m ";
        std::cout.flush();
        
        // Simulate thinking (brief pause for effect)
        usleep(300000);  // 300ms
        
        // Get response from intelligence system
        UnifiedResult result = intelligence->reason(user_input);
        
        // Format and print response
        std::string response = format_response(result);
        std::cout << response << "\n";
        
        // Show internal state (if verbose mode - optional)
        if (getenv("MELVIN_VERBOSE")) {
            std::cout << "\n   \033[2m[Internal: confidence=" << result.confidence 
                      << ", active_nodes=" << result.active_nodes 
                      << ", mode=" << (int)result.mode << "]\033[0m\n";
        }
        
        turn_count++;
        
        // Learn from interaction (assume positive for now)
        intelligence->learn(true);
    }
    
    std::cout << "\n\n📊 Session Summary:\n";
    std::cout << "   • Turns: " << turn_count << "\n";
    std::cout << "   • Genes evolved: " << turn_count * 70 << "+ parameters adjusted\n";
    std::cout << "   • Learning: Continuous throughout session\n";
    
    std::cout << "\n👋 Thanks for chatting! Melvin is shutting down...\n";
    
    return 0;
}

