#include "ReasoningEngine.hpp"
#include "../storage.h"
#include <algorithm>
#include <sstream>
#include <cmath>

namespace melvin {

ReasoningEngine::ReasoningEngine(std::shared_ptr<Storage> storage) 
    : storage_(storage), confidence_threshold_(0.45f), max_depth_(6), beam_width_(8),
      reasoning_count_(0), total_confidence_(0.0f), total_path_length_(0) {
    
    // Initialize graph reasoning system
    graph_system_ = std::make_unique<GraphReasoningSystem>("data/", config_);
    
    // Set default configuration
    config_.confidence_threshold_low = 0.45;
    config_.confidence_threshold_high = 0.7;
    config_.enable_learning = true;
    config_.enable_mining = true;
    config_.enable_generalization = true;
    config_.max_response_length = 200;
}

Thought ReasoningEngine::reason(const InputConcept& input) {
    // Try different reasoning strategies in order of preference
    Thought result;
    
    // First try anchor-based reasoning (most stable)
    result = anchor_based_reasoning(input);
    if (result.confidence > confidence_threshold_) {
        update_statistics(result);
        return result;
    }
    
    // Then try beam search (good balance)
    result = beam_search_reasoning(input);
    if (result.confidence > confidence_threshold_) {
        update_statistics(result);
        return result;
    }
    
    // Finally try iterative deepening (most thorough)
    result = iterative_deepening_reasoning(input);
    update_statistics(result);
    
    return result;
}

std::vector<Thought> ReasoningEngine::reason_batch(const std::vector<InputConcept>& inputs) {
    std::vector<Thought> results;
    results.reserve(inputs.size());
    
    for (const auto& input : inputs) {
        results.push_back(reason(input));
    }
    
    return results;
}

Thought ReasoningEngine::process_query(const std::string& query) {
    // Convert string query to concept
    InputConcept input_concept(query);
    input_concept.tokens = tokenize(query);
    
    // Create nodes for tokens
    for (const std::string& token : input_concept.tokens) {
        NodeID node_id = get_or_create_node(token);
        input_concept.node_ids.push_back(node_id);
    }
    
    return reason(input_concept);
}

Thought ReasoningEngine::process_concept_query(const InputConcept& input_concept) {
    return reason(input_concept);
}

Thought ReasoningEngine::beam_search_reasoning(const InputConcept& input) {
    Thought thought;
    thought.query = input.text;
    
    // Find anchor nodes
    std::vector<NodeID> anchors = find_anchor_nodes(input);
    if (anchors.empty()) {
        thought.text = "I don't know yet.";
        thought.confidence = 0.0f;
        return thought;
    }
    
    // Perform beam search
    std::vector<NodeID> path;
    std::vector<Rel> relations;
    
    // Use the existing beam search system
    // Note: This would integrate with the actual beam search implementation
    // For now, we'll simulate the process
    
    // Expand from anchors
    path = expand_from_anchors(anchors, max_depth_);
    
    if (path.size() > 1) {
        thought.path = path;
        thought.confidence = compute_path_confidence(path, relations);
        thought.output_type = infer_output_type(input, path);
        thought.text = generate_response_text(path, thought.output_type);
    } else {
        thought.text = "I don't know yet.";
        thought.confidence = 0.0f;
    }
    
    return thought;
}

Thought ReasoningEngine::iterative_deepening_reasoning(const InputConcept& input) {
    Thought thought;
    thought.query = input.text;
    
    // Find anchor nodes
    std::vector<NodeID> anchors = find_anchor_nodes(input);
    if (anchors.empty()) {
        thought.text = "I don't know yet.";
        thought.confidence = 0.0f;
        return thought;
    }
    
    // Iterative deepening search
    std::vector<NodeID> best_path;
    float best_confidence = 0.0f;
    
    for (int depth = 1; depth <= max_depth_; ++depth) {
        std::vector<NodeID> path = expand_from_anchors(anchors, depth);
        float confidence = compute_path_confidence(path, {});
        
        if (confidence > best_confidence) {
            best_confidence = confidence;
            best_path = path;
        }
        
        // Early termination if we have high confidence
        if (confidence > config_.confidence_threshold_high) {
            break;
        }
    }
    
    if (!best_path.empty()) {
        thought.path = best_path;
        thought.confidence = best_confidence;
        thought.output_type = infer_output_type(input, best_path);
        thought.text = generate_response_text(best_path, thought.output_type);
    } else {
        thought.text = "I don't know yet.";
        thought.confidence = 0.0f;
    }
    
    return thought;
}

Thought ReasoningEngine::anchor_based_reasoning(const InputConcept& input) {
    Thought thought;
    thought.query = input.text;
    
    // Find anchor nodes
    std::vector<NodeID> anchors = find_anchor_nodes(input);
    if (anchors.empty()) {
        thought.text = "I don't know yet.";
        thought.confidence = 0.0f;
        return thought;
    }
    
    // Start with the strongest anchor
    NodeID primary_anchor = anchors[0];
    
    // Look for direct connections from the anchor
    std::vector<NodeID> path = {primary_anchor};
    
    // Try to find a meaningful path from the anchor
    // This would integrate with the actual graph traversal
    // For now, we'll simulate finding a path
    
    if (path.size() > 1) {
        thought.path = path;
        thought.confidence = compute_path_confidence(path, {});
        thought.output_type = infer_output_type(input, path);
        thought.text = generate_response_text(path, thought.output_type);
    } else {
        thought.text = "I don't know yet.";
        thought.confidence = 0.0f;
    }
    
    return thought;
}

OutputType ReasoningEngine::infer_output_type(const InputConcept& input, const std::vector<NodeID>& path) {
    // Analyze the input and path to determine output type
    
    if (is_definition_query(input)) {
        return OutputType::TEXT;
    } else if (is_factual_query(input)) {
        return OutputType::TEXT;
    } else if (is_arithmetic_query(input)) {
        return OutputType::TEXT;
    } else if (is_creative_query(input)) {
        return OutputType::ABSTRACTION;
    } else if (path.size() > 3) {
        return OutputType::MULTI_MODAL;
    } else {
        return OutputType::TEXT;
    }
}

std::string ReasoningEngine::generate_response_text(const std::vector<NodeID>& path, OutputType type) {
    // Convert path to text based on output type
    std::ostringstream response;
    
    switch (type) {
        case OutputType::TEXT:
            // Generate straightforward text response
            response << "Based on my reasoning, ";
            // Add path-based content
            break;
            
        case OutputType::ABSTRACTION:
            // Generate abstract response
            response << "From a broader perspective, ";
            break;
            
        case OutputType::MULTI_MODAL:
            // Generate multi-modal response
            response << "Considering multiple aspects, ";
            break;
            
        default:
            response << "I think ";
            break;
    }
    
    // Add path-based content
    // This would integrate with the actual path-to-text generation
    response << "this is what I understand.";
    
    return response.str();
}

void ReasoningEngine::set_graph_reasoning_config(const GraphReasoningConfig& config) {
    config_ = config;
    // Note: GraphReasoningSystem doesn't have a configure method
    // This would be implemented based on the actual GraphReasoningSystem API
}

void ReasoningEngine::set_confidence_threshold(float threshold) {
    confidence_threshold_ = threshold;
}

void ReasoningEngine::set_max_depth(int max_depth) {
    max_depth_ = max_depth;
}

void ReasoningEngine::set_beam_width(int beam_width) {
    beam_width_ = beam_width;
}

size_t ReasoningEngine::get_reasoning_count() const {
    return reasoning_count_.load();
}

float ReasoningEngine::get_average_confidence() const {
    size_t count = reasoning_count_.load();
    if (count == 0) return 0.0f;
    return total_confidence_.load() / count;
}

size_t ReasoningEngine::get_average_path_length() const {
    size_t count = reasoning_count_.load();
    if (count == 0) return 0;
    return total_path_length_.load() / count;
}

void ReasoningEngine::reset_statistics() {
    reasoning_count_.store(0);
    total_confidence_.store(0.0f);
    total_path_length_.store(0);
}

std::vector<NodeID> ReasoningEngine::find_anchor_nodes(const InputConcept& input) {
    std::vector<NodeID> anchors;
    
    // Find the most relevant nodes from the input concept
    for (const NodeID& node_id : input.node_ids) {
        if (!is_zero_id(node_id)) {
            anchors.push_back(node_id);
        }
    }
    
    // Sort by relevance (this would use actual node weights)
    std::sort(anchors.begin(), anchors.end());
    
    return anchors;
}

std::vector<NodeID> ReasoningEngine::expand_from_anchors(const std::vector<NodeID>& anchors, int depth) {
    std::vector<NodeID> path;
    
    if (anchors.empty()) return path;
    
    // Start with the first anchor
    path.push_back(anchors[0]);
    
    // Expand from the anchor (this would use actual graph traversal)
    // For now, we'll simulate expansion
    for (int i = 1; i < depth && i < anchors.size(); ++i) {
        path.push_back(anchors[i]);
    }
    
    return path;
}

float ReasoningEngine::compute_path_confidence(const std::vector<NodeID>& path, const std::vector<Rel>& relations) {
    if (path.empty()) return 0.0f;
    
    // Base confidence from path length
    float confidence = 1.0f - (path.size() - 1) * 0.1f;
    
    // Boost confidence for shorter, more direct paths
    if (path.size() <= 2) {
        confidence *= 1.2f;
    }
    
    // Reduce confidence for very long paths
    if (path.size() > 5) {
        confidence *= 0.8f;
    }
    
    return std::max(0.0f, std::min(1.0f, confidence));
}

std::string ReasoningEngine::path_to_text(const std::vector<NodeID>& path, const std::vector<Rel>& relations) {
    // Convert path to human-readable text
    // This would integrate with the actual path-to-text generation
    std::ostringstream text;
    
    for (size_t i = 0; i < path.size(); ++i) {
        if (i > 0) {
            text << " ";
        }
        // Add node text (this would look up actual node content)
        text << "node" << i;
    }
    
    return text.str();
}

bool ReasoningEngine::should_abstain(float confidence) {
    return confidence < confidence_threshold_;
}

bool ReasoningEngine::is_definition_query(const InputConcept& input) {
    // Check if the input is asking for a definition
    std::string text = input.text;
    std::transform(text.begin(), text.end(), text.begin(), ::tolower);
    
    return text.find("what is") != std::string::npos ||
           text.find("define") != std::string::npos ||
           text.find("meaning of") != std::string::npos;
}

bool ReasoningEngine::is_factual_query(const InputConcept& input) {
    // Check if the input is asking for factual information
    std::string text = input.text;
    std::transform(text.begin(), text.end(), text.begin(), ::tolower);
    
    return text.find("what") != std::string::npos ||
           text.find("when") != std::string::npos ||
           text.find("where") != std::string::npos ||
           text.find("how") != std::string::npos;
}

bool ReasoningEngine::is_arithmetic_query(const InputConcept& input) {
    // Check if the input involves arithmetic
    std::string text = input.text;
    
    return text.find('+') != std::string::npos ||
           text.find('-') != std::string::npos ||
           text.find('*') != std::string::npos ||
           text.find('/') != std::string::npos ||
           text.find('=') != std::string::npos;
}

bool ReasoningEngine::is_creative_query(const InputConcept& input) {
    // Check if the input is asking for creative output
    std::string text = input.text;
    std::transform(text.begin(), text.end(), text.begin(), ::tolower);
    
    return text.find("imagine") != std::string::npos ||
           text.find("create") != std::string::npos ||
           text.find("generate") != std::string::npos ||
           text.find("design") != std::string::npos;
}

void ReasoningEngine::update_statistics(const Thought& thought) {
    reasoning_count_++;
    total_confidence_ = total_confidence_.load() + thought.confidence;
    total_path_length_ = total_path_length_.load() + thought.path.size();
}

std::vector<std::string> ReasoningEngine::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string word;
    
    while (iss >> word) {
        // Basic preprocessing
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        
        // Remove punctuation (simple approach)
        word.erase(std::remove_if(word.begin(), word.end(), 
            [](char c) { return std::ispunct(c); }), word.end());
        
        if (!word.empty()) {
            tokens.push_back(word);
        }
    }
    
    return tokens;
}

NodeID ReasoningEngine::get_or_create_node(const std::string& text) {
    // Create new node via storage
    NodeID node_id = {};
    // Note: In a real implementation, this would call storage_->create_node()
    // For now, we'll generate a simple hash-based ID
    std::hash<std::string> hasher;
    size_t hash = hasher(text);
    std::memcpy(node_id.data(), &hash, std::min(sizeof(hash), node_id.size()));
    
    return node_id;
}

} // namespace melvin
