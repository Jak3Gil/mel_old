/**
 * @file answer_synthesizer.cpp
 * @brief Implementation of answer synthesizer
 */

#include "answer_synthesizer.h"
#include <sstream>
#include <algorithm>

namespace melvin {
namespace reasoning {

AnswerSynthesizer::AnswerSynthesizer() {}

std::string AnswerSynthesizer::generate(
    const std::vector<ScoredNode>& scored_nodes,
    const std::unordered_map<int, std::string>& id_to_word,
    melvin::language::ReasoningIntent intent,
    const std::vector<std::string>& query_tokens,
    const std::vector<int>& query_node_ids
) {
    if (scored_nodes.empty()) {
        return "I don't have enough information to answer that question.";
    }
    
    // Route to intent-specific generator
    switch (intent) {
        case melvin::language::ReasoningIntent::DEFINE:
            return generate_definition(scored_nodes, id_to_word, query_tokens);
        
        case melvin::language::ReasoningIntent::LOCATE:
            return generate_location(scored_nodes, id_to_word, query_tokens);
        
        case melvin::language::ReasoningIntent::CAUSE:
            return generate_cause(scored_nodes, id_to_word, query_tokens);
        
        case melvin::language::ReasoningIntent::COMPARE:
            return generate_comparison(scored_nodes, id_to_word, query_tokens);
        
        default:
            return generate_generic(scored_nodes, id_to_word, query_tokens);
    }
}

std::string AnswerSynthesizer::generate_explanation(
    const ScoredNode& answer_node,
    const std::unordered_map<int, std::string>& id_to_word
) {
    if (answer_node.best_path.size() < 2) {
        return "Direct association.";
    }
    
    std::stringstream ss;
    ss << "Reasoning: ";
    
    for (size_t i = 0; i < std::min(size_t(5), answer_node.best_path.size()); i++) {
        auto it = id_to_word.find(answer_node.best_path[i]);
        if (it != id_to_word.end()) {
            ss << it->second;
            if (i < std::min(size_t(4), answer_node.best_path.size() - 1)) {
                ss << " → ";
            }
        }
    }
    
    if (answer_node.best_path.size() > 5) {
        ss << " ... (" << (answer_node.best_path.size() - 5) << " more steps)";
    }
    
    return ss.str();
}

std::string AnswerSynthesizer::generate_definition(
    const std::vector<ScoredNode>& nodes,
    const std::unordered_map<int, std::string>& id_to_word,
    const std::vector<std::string>& query_tokens
) {
    std::string subject = extract_subject(query_tokens, 
                                         melvin::language::ReasoningIntent::DEFINE);
    
    std::stringstream ss;
    
    if (!subject.empty()) {
        ss << capitalize_first(subject);
    } else {
        ss << "The answer";
    }
    
    ss << " is related to: ";
    
    // Take top 8 non-query nodes
    int count = 0;
    for (const auto& node : nodes) {
        if (count >= 8) break;
        
        auto it = id_to_word.find(node.node_id);
        if (it != id_to_word.end()) {
            if (count > 0) ss << ", ";
            ss << it->second;
            count++;
        }
    }
    
    ss << ".";
    
    return ss.str();
}

std::string AnswerSynthesizer::generate_location(
    const std::vector<ScoredNode>& nodes,
    const std::unordered_map<int, std::string>& id_to_word,
    const std::vector<std::string>& query_tokens
) {
    std::string subject = extract_subject(query_tokens,
                                         melvin::language::ReasoningIntent::LOCATE);
    
    std::stringstream ss;
    
    if (!subject.empty()) {
        ss << capitalize_first(subject) << " is located in ";
    } else {
        ss << "Located in ";
    }
    
    // Take top 3 location nodes
    int count = 0;
    for (const auto& node : nodes) {
        if (count >= 3) break;
        
        auto it = id_to_word.find(node.node_id);
        if (it != id_to_word.end()) {
            if (count > 0) ss << ", ";
            ss << it->second;
            count++;
        }
    }
    
    ss << ".";
    
    return ss.str();
}

std::string AnswerSynthesizer::generate_cause(
    const std::vector<ScoredNode>& nodes,
    const std::unordered_map<int, std::string>& id_to_word,
    const std::vector<std::string>& query_tokens
) {
    std::string subject = extract_subject(query_tokens,
                                         melvin::language::ReasoningIntent::CAUSE);
    
    std::stringstream ss;
    ss << "This occurs because of: ";
    
    // Take top 5 cause nodes
    int count = 0;
    for (const auto& node : nodes) {
        if (count >= 5) break;
        
        auto it = id_to_word.find(node.node_id);
        if (it != id_to_word.end()) {
            if (count > 0) ss << ", ";
            ss << it->second;
            count++;
        }
    }
    
    ss << ".";
    
    return ss.str();
}

std::string AnswerSynthesizer::generate_comparison(
    const std::vector<ScoredNode>& nodes,
    const std::unordered_map<int, std::string>& id_to_word,
    const std::vector<std::string>& query_tokens
) {
    std::stringstream ss;
    ss << "Key differences include: ";
    
    // Take top 6 distinguishing nodes
    int count = 0;
    for (const auto& node : nodes) {
        if (count >= 6) break;
        
        auto it = id_to_word.find(node.node_id);
        if (it != id_to_word.end()) {
            if (count > 0) ss << ", ";
            ss << it->second;
            count++;
        }
    }
    
    ss << ".";
    
    return ss.str();
}

std::string AnswerSynthesizer::generate_generic(
    const std::vector<ScoredNode>& nodes,
    const std::unordered_map<int, std::string>& id_to_word,
    const std::vector<std::string>& query_tokens
) {
    std::stringstream ss;
    ss << "Related concepts: ";
    
    // Take top 10 nodes
    int count = 0;
    for (const auto& node : nodes) {
        if (count >= 10) break;
        
        auto it = id_to_word.find(node.node_id);
        if (it != id_to_word.end()) {
            if (count > 0) ss << ", ";
            ss << it->second;
            count++;
        }
    }
    
    ss << ".";
    
    return ss.str();
}

std::string AnswerSynthesizer::extract_subject(
    const std::vector<std::string>& query_tokens,
    melvin::language::ReasoningIntent intent
) {
    // Simple heuristic: last non-stop-word is often the subject
    std::unordered_set<std::string> stop_words = {
        "what", "where", "why", "how", "when", "is", "are", "the", "a", "an",
        "of", "in", "to", "for", "on", "at", "by", "with", "from"
    };
    
    for (auto it = query_tokens.rbegin(); it != query_tokens.rend(); ++it) {
        if (stop_words.find(*it) == stop_words.end()) {
            return *it;
        }
    }
    
    return "";
}

std::string AnswerSynthesizer::capitalize_first(const std::string& str) {
    if (str.empty()) return str;
    
    std::string result = str;
    result[0] = std::toupper(result[0]);
    return result;
}

bool AnswerSynthesizer::is_query_node(int node_id, const std::vector<int>& query_node_ids) {
    return std::find(query_node_ids.begin(), query_node_ids.end(), node_id) 
           != query_node_ids.end();
}

} // namespace reasoning
} // namespace melvin

