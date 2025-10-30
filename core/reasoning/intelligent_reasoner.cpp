/**
 * @file intelligent_reasoner.cpp
 * @brief Implementation of intelligent reasoner
 */

#include "intelligent_reasoner.h"
#include <queue>
#include <set>
#include <algorithm>

namespace melvin {
namespace reasoning {

IntelligentReasoner::IntelligentReasoner() {
    scorer_.set_genome(&genome_);
    reflection_controller_.set_genome(&genome_);
}

void IntelligentReasoner::initialize(
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    const std::unordered_map<int, std::vector<float>>& embeddings,
    const std::unordered_map<std::string, int>& word_to_id,
    const std::unordered_map<int, std::string>& id_to_word
) {
    graph_ = graph;
    embeddings_ = embeddings;
    word_to_id_ = word_to_id;
    id_to_word_ = id_to_word;
}

ReasoningResult IntelligentReasoner::answer(const std::string& query) {
    ReasoningResult result;
    
    // Step 1: Tokenize
    std::vector<std::string> tokens = melvin::language::tokenize(query);
    if (tokens.empty()) {
        result.answer = "I didn't understand the question.";
        return result;
    }
    
    // Step 2: Compute query embedding
    std::vector<float> query_embedding = melvin::language::compute_simple_embedding(tokens);
    
    // Step 3: Classify intent
    result.intent = intent_classifier_.infer_intent(query_embedding, tokens);
    
    // Step 4: Get reasoning strategy for this intent
    melvin::language::ReasoningStrategy strategy = 
        intent_classifier_.get_strategy(result.intent);
    
    // Step 5: Extract content words (filter stop words)
    std::vector<std::string> content_words = 
        intent_classifier_.get_content_words(tokens);
    
    // Step 6: Activate query nodes
    std::vector<int> query_node_ids = activate_query_nodes(content_words);
    
    if (query_node_ids.empty()) {
        result.answer = "I don't recognize those concepts in my knowledge.";
        return result;
    }
    
    // Step 7: Spread activation (energy-driven, semantic-biased)
    std::unordered_map<int, float> activations;
    std::unordered_map<int, std::vector<int>> paths;
    
    spread_activation(query_node_ids, strategy, activations, paths);
    
    if (activations.empty()) {
        result.answer = "I couldn't find related information.";
        return result;
    }
    
    // Step 8: Score all activated nodes
    std::vector<int> active_node_list;
    for (const auto& [node_id, _] : activations) {
        active_node_list.push_back(node_id);
    }
    
    std::vector<ScoredNode> scored = scorer_.score_all(
        active_node_list,
        activations,
        embeddings_,
        query_embedding,
        paths
    );
    
    // Step 9: Update metrics
    update_metrics(active_node_list, activations, scored);
    
    // Step 9b: Reflect and adapt (meta-cognition)
    reflection_controller_.observe(metrics_tracker_.current());
    bool mode_switched = reflection_controller_.reflect_and_adapt();
    
    if (mode_switched) {
        // Mode changed - could optionally retry reasoning with new mode
        // For now, just log it for next query
    }
    
    // Step 10: Get top nodes
    result.top_nodes = scorer_.get_top_n(scored, 10);
    
    // Step 11: Synthesize natural language answer
    result.answer = synthesizer_.generate(
        result.top_nodes,
        id_to_word_,
        result.intent,
        tokens,
        query_node_ids
    );
    
    // Step 12: Generate explanation
    if (!result.top_nodes.empty()) {
        result.explanation = synthesizer_.generate_explanation(
            result.top_nodes[0],
            id_to_word_
        );
    }
    
    // Step 13: Copy metrics
    auto& current_metrics = metrics_tracker_.current();
    result.confidence = current_metrics.confidence;
    result.coherence = current_metrics.coherence;
    result.novelty = current_metrics.novelty;
    
    // Save for potential feedback
    last_result_ = result;
    
    return result;
}

void IntelligentReasoner::feedback(bool correct) {
    // Learn from feedback
    scorer_.learn_from_feedback(
        last_result_.confidence,
        last_result_.coherence,
        correct
    );
    
    // Log to genome's meta-learning
    std::string intent_str;
    switch (last_result_.intent) {
        case melvin::language::ReasoningIntent::DEFINE: intent_str = "DEFINE"; break;
        case melvin::language::ReasoningIntent::LOCATE: intent_str = "LOCATE"; break;
        case melvin::language::ReasoningIntent::CAUSE: intent_str = "CAUSE"; break;
        case melvin::language::ReasoningIntent::COMPARE: intent_str = "COMPARE"; break;
        default: intent_str = "UNKNOWN"; break;
    }
    
    genome_.log_reasoning_episode(
        intent_str,
        last_result_.confidence,
        last_result_.coherence,
        correct
    );
}

void IntelligentReasoner::save_genome(const std::string& filepath) {
    genome_.save(filepath);
}

void IntelligentReasoner::load_genome(const std::string& filepath) {
    genome_.load(filepath);
}

std::vector<int> IntelligentReasoner::activate_query_nodes(
    const std::vector<std::string>& tokens
) {
    std::vector<int> node_ids;
    
    for (const auto& token : tokens) {
        auto it = word_to_id_.find(token);
        if (it != word_to_id_.end()) {
            node_ids.push_back(it->second);
        }
    }
    
    return node_ids;
}

void IntelligentReasoner::spread_activation(
    const std::vector<int>& seed_nodes,
    melvin::language::ReasoningStrategy strategy,
    std::unordered_map<int, float>& activations,
    std::unordered_map<int, std::vector<int>>& paths
) {
    // Energy-driven BFS with semantic biasing
    std::queue<std::pair<int, float>> frontier;
    std::set<int> visited;
    
    // Initialize with seed nodes
    for (int seed : seed_nodes) {
        frontier.push({seed, 1.0f});
        activations[seed] = 1.0f;
        paths[seed] = {seed};
        visited.insert(seed);
    }
    
    // Get genome parameters
    auto& params = genome_.reasoning_params();
    float temperature = params.temperature;
    float semantic_threshold = params.semantic_threshold;
    
    // Spread until energy depleted or max iterations
    int max_iterations = 1000;
    int iteration = 0;
    
    while (!frontier.empty() && iteration < max_iterations) {
        auto [current_node, energy] = frontier.front();
        frontier.pop();
        
        // Stop if energy too low
        if (energy < 0.01f) continue;
        
        // Get neighbors
        auto neighbors_it = graph_.find(current_node);
        if (neighbors_it == graph_.end()) continue;
        
        // Spread to neighbors
        for (const auto& [neighbor, edge_weight] : neighbors_it->second) {
            if (visited.count(neighbor)) continue;
            
            // Semantic biasing: check if neighbor is relevant
            float semantic_fit = 1.0f;  // Default
            auto emb_it = embeddings_.find(neighbor);
            if (emb_it != embeddings_.end() && !emb_it->second.empty()) {
                // Could compute similarity to query embedding here
                // For now, just use edge weight as proxy
                semantic_fit = edge_weight;
            }
            
            // Apply temperature scaling
            float effective_energy = energy * edge_weight * semantic_fit * temperature;
            
            if (effective_energy > semantic_threshold) {
                // Activate neighbor
                activations[neighbor] = effective_energy;
                
                // Record path
                paths[neighbor] = paths[current_node];
                paths[neighbor].push_back(neighbor);
                
                // Add to frontier
                frontier.push({neighbor, effective_energy * 0.9f});  // Decay
                visited.insert(neighbor);
            }
        }
        
        iteration++;
    }
}

std::vector<ScoredNode> IntelligentReasoner::score_activated_nodes(
    const std::unordered_map<int, float>& activations,
    const std::unordered_map<int, std::vector<int>>& paths,
    const std::vector<float>& query_embedding
) {
    std::vector<int> active_nodes;
    for (const auto& [node_id, _] : activations) {
        active_nodes.push_back(node_id);
    }
    
    return scorer_.score_all(
        active_nodes,
        activations,
        embeddings_,
        query_embedding,
        paths
    );
}

void IntelligentReasoner::update_metrics(
    const std::vector<int>& active_nodes,
    const std::unordered_map<int, float>& activations,
    const std::vector<ScoredNode>& scored_nodes
) {
    // Collect activation values
    std::vector<float> activation_values;
    for (int node_id : active_nodes) {
        auto it = activations.find(node_id);
        if (it != activations.end()) {
            activation_values.push_back(it->second);
        }
    }
    
    // Empty working memory (for now)
    std::unordered_set<int> working_memory;
    
    // Update metrics
    metrics_tracker_.update_from_activation(
        active_nodes,
        activation_values,
        working_memory
    );
    
    // Extract paths and strengths from scored nodes
    std::vector<std::vector<int>> paths;
    std::vector<float> path_strengths;
    
    for (const auto& snode : scored_nodes) {
        if (!snode.best_path.empty()) {
            paths.push_back(snode.best_path);
            path_strengths.push_back(snode.final_score);
        }
    }
    
    metrics_tracker_.update_from_paths(paths, path_strengths);
}

} // namespace reasoning
} // namespace melvin

