/**
 * @file simple_conversation_demo.cpp
 * @brief Simplified conversation demo using existing v2 components
 * 
 * This demo shows the v2 architecture working WITHOUT v1 dependencies.
 * It demonstrates:
 * - Global Workspace
 * - Neuromodulators
 * - Genome System
 * - Template-based language generation (simplified)
 */

#include "../core/global_workspace.h"
#include "../core/neuromodulators.h"
#include "../evolution/genome.h"
#include <iostream>
#include <string>
#include <map>
#include <vector>

using namespace melvin::v2;

// ============================================================================
// SIMPLE LANGUAGE GENERATOR (No v1 dependencies)
// ============================================================================

class SimpleLanguageGenerator {
public:
    SimpleLanguageGenerator() {
        // Populate knowledge base with simple facts
        knowledge_["cup"] = {
            "A cup is a container.",
            "Cups are used for drinking beverages."
        };
        knowledge_["fire"] = {
            "Fire is a combustion reaction.",
            "Fire produces heat and light."
        };
        knowledge_["water"] = {
            "Water is a liquid.",
            "Water is essential for life."
        };
        knowledge_["robot"] = {
            "A robot is a mechanical agent.",
            "Robots can perform automated tasks."
        };
    }
    
    std::string describe(const std::string& concept_name) const {
        auto it = knowledge_.find(concept_name);
        if (it == knowledge_.end()) {
            return "I don't know about " + concept_name + " yet.";
        }
        
        std::string result;
        for (const auto& fact : it->second) {
            result += fact + " ";
        }
        return result;
    }
    
    bool knows_about(const std::string& concept_name) const {
        return knowledge_.find(concept_name) != knowledge_.end();
    }
    
private:
    std::map<std::string, std::vector<std::string>> knowledge_;
};

// ============================================================================
// SIMPLE CONVERSATION ENGINE
// ============================================================================

class SimpleConversationEngine {
public:
    SimpleConversationEngine(GlobalWorkspace& workspace, Neuromodulators& neuromod)
        : workspace_(workspace)
        , neuromod_(neuromod)
        , turn_count_(0)
    {
    }
    
    std::string respond(const std::string& user_message) {
        turn_count_++;
        
        // Parse query type
        std::string lower = to_lower(user_message);
        
        // Handle greetings
        if (lower.find("hello") != std::string::npos || 
            lower.find("hi") != std::string::npos) {
            return handle_greeting();
        }
        
        // Handle goodbyes
        if (lower.find("bye") != std::string::npos || 
            lower.find("goodbye") != std::string::npos) {
            return "Goodbye! It was nice talking with you.";
        }
        
        // Handle "what is" queries
        if (lower.find("what is") != std::string::npos) {
            std::string concept_name = extract_concept(lower, "what is");
            return lang_gen_.describe(concept_name);
        }
        
        // Handle "who are you"
        if (lower.find("who are you") != std::string::npos) {
            return "I'm Melvin, a cognitive robot with a brain-inspired architecture. I'm learning to understand the world!";
        }
        
        // Fallback
        return "I'm still learning to converse. Try asking 'What is X?' about something!";
    }
    
    size_t get_turn_count() const { return turn_count_; }
    
private:
    GlobalWorkspace& workspace_;
    Neuromodulators& neuromod_;
    SimpleLanguageGenerator lang_gen_;
    size_t turn_count_;
    
    std::string handle_greeting() const {
        float dopamine = neuromod_.get_dopamine();
        
        if (dopamine > 0.7f) {
            return "Hello! I'm excited to chat with you!";
        } else if (dopamine > 0.5f) {
            return "Hello! How can I help you?";
        } else {
            return "Hello.";
        }
    }
    
    std::string to_lower(const std::string& str) const {
        std::string result = str;
        for (char& c : result) {
            c = std::tolower(c);
        }
        return result;
    }
    
    std::string extract_concept(const std::string& query, const std::string& pattern) const {
        size_t pos = query.find(pattern);
        if (pos == std::string::npos) return "";
        
        std::string rest = query.substr(pos + pattern.length());
        
        // Remove question mark and "a"
        rest.erase(std::remove(rest.begin(), rest.end(), '?'), rest.end());
        rest.erase(std::remove(rest.begin(), rest.end(), '.'), rest.end());
        
        // Trim whitespace
        rest.erase(0, rest.find_first_not_of(" \t"));
        rest.erase(rest.find_last_not_of(" \t") + 1);
        
        // Remove "a " or "an "
        if (rest.substr(0, 2) == "a ") rest = rest.substr(2);
        if (rest.substr(0, 3) == "an ") rest = rest.substr(3);
        
        return rest;
    }
};

// ============================================================================
// DEMO SCENARIOS
// ============================================================================

void run_demo(SimpleConversationEngine& conv) {
    std::cout << "\n═══════════════════════════════════════════════════\n";
    std::cout << "  MELVIN v2 - Simple Conversation Demo\n";
    std::cout << "═══════════════════════════════════════════════════\n\n";
    
    std::vector<std::string> queries = {
        "Hello!",
        "Who are you?",
        "What is a cup?",
        "What is fire?",
        "What is water?",
        "What is a robot?",
        "What is quantum_physics?",
        "Goodbye"
    };
    
    for (const auto& query : queries) {
        std::string response = conv.respond(query);
        std::cout << "You:    " << query << "\n";
        std::cout << "Melvin: " << response << "\n\n";
    }
    
    std::cout << "Total turns: " << conv.get_turn_count() << "\n";
}

void interactive_mode(SimpleConversationEngine& conv) {
    std::cout << "\n═══════════════════════════════════════════════════\n";
    std::cout << "  INTERACTIVE MODE\n";
    std::cout << "═══════════════════════════════════════════════════\n\n";
    
    std::cout << "Chat with Melvin! (type 'quit' to exit)\n";
    std::cout << "Try: What is X? | Who are you? | Hello\n\n";
    
    while (true) {
        std::cout << "You: ";
        std::string input;
        std::getline(std::cin, input);
        
        if (input.empty()) continue;
        if (input == "quit" || input == "exit") break;
        
        std::string response = conv.respond(input);
        std::cout << "Melvin: " << response << "\n\n";
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║     MELVIN v2 - CONVERSATION DEMO                 ║\n";
    std::cout << "║                                                    ║\n";
    std::cout << "║  Simple demonstration of v2 architecture          ║\n";
    std::cout << "║  with conversation capabilities                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";
    
    try {
        // Initialize v2 components
        std::cout << "\n[Initializing v2 components...]\n";
        
        // Create genome
        evolution::Genome genome = evolution::GenomeFactory::create_base();
        std::cout << "  ✓ Genome system initialized\n";
        
        // Create global workspace
        GlobalWorkspace workspace;
        std::cout << "  ✓ Global Workspace initialized\n";
        
        // Create neuromodulators
        Neuromodulators neuromod(genome);
        std::cout << "  ✓ Neuromodulators initialized\n";
        std::cout << "    DA=" << neuromod.get_dopamine()
                  << " NE=" << neuromod.get_norepinephrine()
                  << " ACh=" << neuromod.get_acetylcholine()
                  << " 5-HT=" << neuromod.get_serotonin() << "\n";
        
        // Create conversation engine
        SimpleConversationEngine conversation(workspace, neuromod);
        std::cout << "  ✓ Conversation Engine initialized\n";
        
        std::cout << "\n✓ All systems ready!\n";
        
        // Check command line args
        bool interactive = false;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--interactive" || arg == "-i") {
                interactive = true;
            }
        }
        
        if (interactive) {
            interactive_mode(conversation);
        } else {
            run_demo(conversation);
            
            // Offer interactive
            std::cout << "\nWant to try interactive mode? (y/n): ";
            std::string response;
            std::getline(std::cin, response);
            if (response == "y" || response == "Y") {
                interactive_mode(conversation);
            }
        }
        
        std::cout << "\n═══════════════════════════════════════════════════\n";
        std::cout << "  ✓ Demo Complete!\n";
        std::cout << "═══════════════════════════════════════════════════\n\n";
        
        std::cout << "This demonstrates:\n";
        std::cout << "  ✓ v2 Global Workspace (conscious focus)\n";
        std::cout << "  ✓ v2 Neuromodulators (emotional state)\n";
        std::cout << "  ✓ v2 Genome System (evolvable params)\n";
        std::cout << "  ✓ Simple conversation capability\n";
        std::cout << "  ✓ Emotional tone modulation\n\n";
        
        std::cout << "Next step: Integrate with v1's 4.29M edge graph!\n\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

