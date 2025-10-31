/**
 * @file emergent_intelligence.cpp
 * @brief Simplified unified intelligence implementation
 */

#include "emergent_intelligence.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace melvin {
namespace emergent {

EmergentIntelligence::EmergentIntelligence() {
}

void EmergentIntelligence::initialize(
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    const std::unordered_map<int, std::vector<float>>& embeddings,
    const std::unordered_map<std::string, int>& word_to_id,
    const std::unordered_map<int, std::string>& id_to_word
) {
    word_to_id_ = word_to_id;
    id_to_word_ = id_to_word;
    
    // Migrate existing graph to emergent graph
    // Convert pre-existing edges to emergent edges
    for (const auto& [node_id, neighbors] : graph) {
        // Get or create node
        std::string token;
        auto token_it = id_to_word.find(node_id);
        if (token_it != id_to_word.end()) {
            token = token_it->second;
        }
        
        std::vector<float> embedding;
        auto emb_it = embeddings.find(node_id);
        if (emb_it != embeddings.end()) {
            embedding = emb_it->second;
        }
        
        // Add node to emergent graph
        graph_.add_node(token, embedding);
        
        // Existing edges start with their weights as initial strength
        // They will adapt based on need-cost in future updates
        // (We could migrate them here, but let them emerge naturally)
    }
}

SimpleResult EmergentIntelligence::reason(const std::string& query) {
    SimpleResult result;
    
    // 1. Tokenize query
    std::vector<std::string> tokens = tokenize(query);
    if (tokens.empty()) {
        result.answer = "I didn't understand that.";
        return result;
    }
    
    // 2. Find corresponding nodes (or create if needed)
    std::vector<int> node_ids;
    for (const auto& token : tokens) {
        auto it = word_to_id_.find(token);
        if (it != word_to_id_.end()) {
            node_ids.push_back(it->second);
        } else {
            // New concept - create node (if energy available)
            std::vector<float> embedding = compute_embedding({token});
            int new_id = graph_.add_node(token, embedding);
            if (new_id >= 0) {
                node_ids.push_back(new_id);
                word_to_id_[token] = new_id;
                id_to_word_[new_id] = token;
            }
        }
    }
    
    if (node_ids.empty()) {
        result.answer = "I don't recognize those concepts.";
        return result;
    }
    
    // 3. Activate nodes (this triggers connection formation if co-activated)
    graph_.activate(node_ids, 1.0f);
    
    // 4. Let activation spread through emergent connections
    for (int i = 0; i < 5; i++) {
        graph_.spread_activation(1.0f);
    }
    
    // 5. Update connections based on need - cost
    graph_.update_connections();
    
    // 6. Get active nodes (answer emerges from activation pattern)
    std::vector<int> active = graph_.get_active_nodes(0.1f);
    
    // 7. Generate answer from active nodes
    result.answer = generate_answer(active);
    
    // 8. Track nodes used for learning
    last_nodes_used_ = active;
    
    // 9. Confidence = how much activation is concentrated in top nodes
    if (!active.empty()) {
        float top_activation = graph_.get_activation(active[0]);
        float total_activation = 0.0f;
        for (int node_id : active) {
            total_activation += graph_.get_activation(node_id);
        }
        result.confidence = (total_activation > 0.0f) 
            ? std::min(1.0f, top_activation / total_activation * 3.0f)
            : 0.1f;
    } else {
        result.confidence = 0.1f;
    }
    
    // 10. Extract active concepts for result
    for (size_t i = 0; i < std::min(size_t(10), active.size()); i++) {
        auto it = id_to_word_.find(active[i]);
        if (it != id_to_word_.end()) {
            result.active_concepts.push_back(it->second);
        }
    }
    
    return result;
}

void EmergentIntelligence::learn(bool correct, const std::vector<int>& nodes_used) {
    // BIOLOGICAL LEARNING: Adjust need/cost signals based on outcome
    
    // Use nodes_used if provided, otherwise use last_nodes_used_
    const std::vector<int>& to_update = nodes_used.empty() ? last_nodes_used_ : nodes_used;
    
    if (to_update.empty()) return;
    
    // Get the graph (non-const for modification)
    auto& nodes = graph_.nodes();
    
    // If correct: increase need for connections used
    // If wrong: increase cost or decrease need
    
    float learning_rate = 0.1f;
    
    for (size_t i = 0; i < to_update.size(); i++) {
        int node_id = to_update[i];
        
        auto node_it = nodes.find(node_id);
        if (node_it == nodes.end()) continue;
        
        auto& node = node_it->second;
        
        // Update edges from this node
        for (auto& [to_id, edge] : node.edges) {
            if (correct) {
                // Success → increase need (this connection is useful)
                edge.need_signal += learning_rate;
                edge.information_value += learning_rate * 0.1f;
                
                // Decrease cost slightly (becomes more efficient)
                edge.cost *= 0.99f;
            } else {
                // Failure → decrease need or increase cost
                edge.need_signal *= 0.95f;
                edge.cost += learning_rate * 0.1f;
            }
        }
    }
    
    // Prune connections that are now too costly
    graph_.prune_weak_connections(0.01f);
}

std::vector<std::string> EmergentIntelligence::tokenize(const std::string& query) {
    std::vector<std::string> tokens;
    std::stringstream ss(query);
    std::string word;
    
    while (ss >> word) {
        // Remove punctuation
        word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
        // To lowercase
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        
        if (!word.empty() && word.length() > 1) {  // Skip single chars
            tokens.push_back(word);
        }
    }
    
    return tokens;
}

std::vector<float> EmergentIntelligence::compute_embedding(const std::vector<std::string>& tokens) {
    // Simple hash-based embedding (can be replaced with real model)
    std::vector<float> embedding(128, 0.0f);
    
    for (const auto& token : tokens) {
        size_t hash = std::hash<std::string>{}(token);
        for (size_t i = 0; i < 128; i++) {
            embedding[i] += std::sin(static_cast<float>(hash + i) * 0.01f);
        }
    }
    
    // Normalize
    float norm = 0.0f;
    for (float val : embedding) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    
    if (norm > 1e-6f) {
        for (float& val : embedding) {
            val /= norm;
        }
    }
    
    return embedding;
}

std::string EmergentIntelligence::generate_answer(const std::vector<int>& active_nodes) {
    if (active_nodes.empty()) {
        return "I'm not sure how to answer that.";
    }
    
    // Simple answer: list top active concepts
    // In a real system, this could be more sophisticated,
    // but answers emerge from activation patterns, not templates
    
    std::stringstream ss;
    
    // Get top 5 active nodes
    int count = 0;
    for (int node_id : active_nodes) {
        if (count >= 5) break;
        
        auto it = id_to_word_.find(node_id);
        if (it != id_to_word_.end()) {
            if (count > 0) ss << ", ";
            ss << it->second;
            count++;
        }
    }
    
    std::string answer = ss.str();
    if (!answer.empty()) {
        // Capitalize first letter
        answer[0] = std::toupper(answer[0]);
        answer += ".";
    } else {
        answer = "I'm processing that.";
    }
    
    return answer;
}

} // namespace emergent
} // namespace melvin

