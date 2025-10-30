/**
 * @file melvin_simple_chat.cpp
 * @brief Simple ChatGPT-style demo (no full cognitive OS)
 */

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>

// Simple response generation based on keywords
class SimpleResponder {
public:
    SimpleResponder() {
        // Build response templates
        responses_["hello"] = {"Hi there! I'm Melvin, a cognitive AI system.", 
                               "Hello! How can I help you today?",
                               "Hey! Great to chat with you."};
        
        responses_["intelligence"] = {"Intelligence is the ability to learn, reason, and adapt.",
                                      "I think of intelligence as pattern recognition plus flexible goal pursuit.",
                                      "Intelligence involves processing information and making useful decisions."};
        
        responses_["learn"] = {"Yes! I learn through Hebbian connections - neurons that fire together wire together.",
                               "Absolutely. Every interaction updates my knowledge graph and tunes my genome.",
                               "Learning is my core function. I adapt my parameters with every conversation."};
        
        responses_["think"] = {"I think by spreading activation across my knowledge graph.",
                               "Thinking, for me, is semantic traversal through connected concepts.",
                               "My thoughts are patterns of energy flowing through neural networks."};
        
        responses_["you"] = {"I'm Melvin - a cognitive OS with 70+ evolving genes that control how I reason.",
                             "I'm an AI system built on spreading activation and continuous evolution.",
                             "I'm a unified intelligence pipeline - always learning, always adapting."};
        
        responses_["what"] = {"That's a good question. Let me process that...",
                              "I'm thinking about that through my semantic network.",
                              "Interesting query. My activation field is lighting up."};
        
        responses_["why"] = {"The reason involves multiple factors in my reasoning process.",
                             "That traces back to causal connections in my knowledge graph.",
                             "Good question - let me trace the logic chain."};
        
        responses_["how"] = {"The mechanism involves spreading activation and dynamic scoring.",
                             "It works through semantic traversal and adaptive weighting.",
                             "The process uses parallel field dynamics and Hebbian learning."};
        
        responses_["can"] = {"I'll check my current capabilities... yes, likely!",
                             "Based on my genome configuration, I should be able to handle that.",
                             "Let me see if my current parameters support that..."};
        
        responses_["genome"] = {"My genome has 70+ genes controlling activation, learning, memory, and more.",
                                "Every parameter - from decay rates to curiosity weights - is a gene that evolves.",
                                "My genome self-tunes continuously. No static hyperparameters!"};
        
        responses_["evolve"] = {"I evolve at 5Hz through reflection ticks, adjusting all genes towards intelligence.",
                                "Evolution happens continuously - faster learning, better memory, deeper reasoning.",
                                "Every tick, my parameters adapt. I'm never static."};
        
        fallback_responses_ = {
            "I'm processing that through my knowledge graph.",
            "Interesting input - let me think about it.",
            "My activation field is exploring related concepts.",
            "I'm still learning about that domain.",
            "That's activating several concept clusters in my network."
        };
    }
    
    std::string respond(const std::string& input) {
        turn_count_++;
        
        // Convert to lowercase for matching
        std::string lower_input = input;
        std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);
        
        // Find matching keywords
        for (const auto& [keyword, response_list] : responses_) {
            if (lower_input.find(keyword) != std::string::npos) {
                // Cycle through responses for variety
                int idx = turn_count_ % response_list.size();
                return response_list[idx];
            }
        }
        
        // Fallback
        int idx = turn_count_ % fallback_responses_.size();
        return fallback_responses_[idx];
    }
    
    int get_turn_count() const { return turn_count_; }
    
private:
    std::unordered_map<std::string, std::vector<std::string>> responses_;
    std::vector<std::string> fallback_responses_;
    int turn_count_ = 0;
};

void print_banner() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                      ║\n";
    std::cout << "║              MELVIN CHAT DEMO v1.0                   ║\n";
    std::cout << "║         ChatGPT-style Output Preview                 ║\n";
    std::cout << "║                                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "💡 This is a simple demo of ChatGPT-style output.\n";
    std::cout << "   The full system uses:\n";
    std::cout << "   • Unified intelligence pipeline\n";
    std::cout << "   • 70+ evolving genes\n";
    std::cout << "   • Spreading activation\n";
    std::cout << "   • Hebbian learning\n";
    std::cout << "   • Continuous self-improvement\n\n";
}

int main() {
    print_banner();
    
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                      ║\n";
    std::cout << "║  💬 Chat with Melvin                                 ║\n";
    std::cout << "║                                                      ║\n";
    std::cout << "║  Type your message and press Enter                  ║\n";
    std::cout << "║  Type 'quit', 'exit', or 'q' to stop                ║\n";
    std::cout << "║                                                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    SimpleResponder melvin;
    std::string user_input;
    
    while (true) {
        // Prompt
        std::cout << "\n\033[1;36mYou:\033[0m ";
        std::getline(std::cin, user_input);
        
        // Check for exit
        if (user_input == "quit" || user_input == "exit" || user_input == "q") {
            break;
        }
        
        if (user_input.empty()) {
            continue;
        }
        
        // Generate response
        std::cout << "\n\033[1;32mMelvin:\033[0m ";
        std::string response = melvin.respond(user_input);
        std::cout << response << "\n";
    }
    
    std::cout << "\n\n📊 Session Summary:\n";
    std::cout << "   • Turns: " << melvin.get_turn_count() << "\n";
    std::cout << "   • Genes evolved: " << melvin.get_turn_count() * 70 << "+ (in full system)\n";
    std::cout << "   • Learning: Continuous adaptation (in full system)\n";
    
    std::cout << "\n👋 Thanks for chatting!\n\n";
    
    std::cout << "🚀 To run the FULL cognitive system with:\n";
    std::cout << "   • Real knowledge graph\n";
    std::cout << "   • Spreading activation\n";
    std::cout << "   • Hebbian learning\n";
    std::cout << "   • Genome evolution\n";
    std::cout << "   • All 70+ genes\n";
    std::cout << "\n   Run: ./bin/melvin_jetson\n\n";
    
    return 0;
}

