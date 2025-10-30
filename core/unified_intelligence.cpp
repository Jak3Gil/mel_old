/**
 * @file unified_intelligence.cpp
 * @brief Implementation of unified intelligence pipeline
 */

#include "unified_intelligence.h"
#include <queue>
#include <set>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cmath>

namespace melvin {
namespace intelligence {

UnifiedIntelligence::UnifiedIntelligence() :
    current_mode_(metacognition::ReasoningMode::EXPLORATORY)
{
    // All components share the same genome
    reflection_controller_.set_genome(&genome_);
}

void UnifiedIntelligence::initialize(
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    const std::unordered_map<int, std::vector<float>>& embeddings,
    const std::unordered_map<std::string, int>& word_to_id,
    const std::unordered_map<int, std::string>& id_to_word
) {
    std::lock_guard<std::mutex> lock(graph_mutex_);
    graph_ = graph;
    embeddings_ = embeddings;
    word_to_id_ = word_to_id;
    id_to_word_ = id_to_word;
    
    // Find max node ID to initialize next_node_id_
    int max_id = 0;
    for (const auto& [id, _] : graph_) {
        if (id > max_id) max_id = id;
    }
    for (const auto& [id, _] : id_to_word_) {
        if (id > max_id) max_id = id;
    }
    next_node_id_.store(max_id + 1, std::memory_order_relaxed);
}

UnifiedResult UnifiedIntelligence::reason(const std::string& query) {
    UnifiedResult result;
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STAGE 1: UNDERSTAND QUERY (Genome-driven)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    // Tokenize and filter stop words
    std::vector<std::string> tokens = tokenize_and_filter(query);
    if (tokens.empty()) {
        result.answer = "I didn't understand the question.";
        return result;
    }
    
    // Compute query embedding
    std::vector<float> query_embedding = compute_embedding(tokens);
    
    // Classify intent
    result.intent = classify_intent(tokens, query_embedding);
    result.strategy = get_strategy(result.intent);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STAGE 2: ACTIVATE & TRAVERSE (Genome-driven temperature, thresholds)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    // Activate seed nodes
    std::vector<int> seeds = activate_nodes(tokens);
    if (seeds.empty()) {
        result.answer = "I don't recognize those concepts.";
        return result;
    }
    
    // Spread activation using genome parameters
    std::unordered_map<int, float> activations;
    std::unordered_map<int, std::vector<int>> paths;
    
    spread_activation(seeds, result.strategy, query_embedding, activations, paths);
    
    if (activations.empty()) {
        result.answer = "I couldn't find related information.";
        return result;
    }
    
    result.active_nodes = activations.size();
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STAGE 3: SCORE & RANK (Genome-driven α, β, γ)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::vector<std::pair<int, float>> ranked = score_and_rank(
        activations,
        paths,
        query_embedding
    );
    
    // Extract top concepts for result
    for (size_t i = 0; i < std::min(size_t(5), ranked.size()); i++) {
        auto it = id_to_word_.find(ranked[i].first);
        if (it != id_to_word_.end()) {
            result.top_concepts.push_back({it->second, ranked[i].second});
        }
    }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STAGE 4: SYNTHESIZE ANSWER (Intent-driven templates)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    result.answer = synthesize_answer(ranked, result.intent, tokens);
    
    // Generate explanation from path
    if (!ranked.empty() && paths.count(ranked[0].first)) {
        const auto& path = paths[ranked[0].first];
        if (path.size() > 1) {
            std::stringstream ss;
            for (size_t i = 0; i < std::min(size_t(3), path.size()); i++) {
                auto it = id_to_word_.find(path[i]);
                if (it != id_to_word_.end()) {
                    result.reasoning_path.push_back(it->second);
                }
            }
        }
    }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STAGE 5: UPDATE METRICS (Continuous monitoring)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    update_metrics(activations, ranked);
    
    // Copy metrics to result
    result.confidence = current_metrics_.confidence;
    result.coherence = current_metrics_.coherence;
    result.novelty = current_metrics_.novelty;
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STAGE 6: HEBBIAN LEARNING (Neurons that fire together wire together)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    // Strengthen connections between co-activated nodes
    apply_hebbian_learning(activations, 0.01f);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STAGE 7: REFLECT & ADAPT (Autonomous mode switching)
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    reflect_and_adapt();
    
    result.mode = current_mode_;
    
    // Save for learning
    last_result_ = result;
    
    return result;
}

void UnifiedIntelligence::learn(bool correct) {
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // UNIFIED LEARNING: Update entire system simultaneously
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    // 1. Tune genome parameters (α, β, γ)
    genome_.tune_from_feedback(
        last_result_.confidence,
        last_result_.coherence,
        correct
    );
    
    // 2. Adapt temperature based on confidence
    genome_.adapt_temperature(last_result_.confidence);
    
    // 3. Log to meta-learning profile
    std::string intent_str;
    switch (last_result_.intent) {
        case language::ReasoningIntent::DEFINE: intent_str = "DEFINE"; break;
        case language::ReasoningIntent::LOCATE: intent_str = "LOCATE"; break;
        case language::ReasoningIntent::CAUSE: intent_str = "CAUSE"; break;
        case language::ReasoningIntent::COMPARE: intent_str = "COMPARE"; break;
        default: intent_str = "UNKNOWN"; break;
    }
    
    genome_.log_reasoning_episode(
        intent_str,
        last_result_.confidence,
        last_result_.coherence,
        correct
    );
}

void UnifiedIntelligence::save(const std::string& filepath) {
    genome_.save(filepath);
}

void UnifiedIntelligence::load(const std::string& filepath) {
    genome_.load(filepath);
}

void UnifiedIntelligence::reset() {
    genome_ = evolution::DynamicGenome();
    metrics_tracker_.reset();
    reflection_controller_.reset();
    current_mode_ = metacognition::ReasoningMode::EXPLORATORY;
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// PRIVATE PIPELINE STAGES
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

std::vector<std::string> UnifiedIntelligence::tokenize_and_filter(const std::string& query) {
    // Tokenize
    std::vector<std::string> all_tokens;
    std::stringstream ss(query);
    std::string word;
    
    while (ss >> word) {
        word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        if (!word.empty()) {
            all_tokens.push_back(word);
        }
    }
    
    // Filter stop words
    return intent_classifier_.get_content_words(all_tokens);
}

language::ReasoningIntent UnifiedIntelligence::classify_intent(
    const std::vector<std::string>& tokens,
    const std::vector<float>& query_embedding
) {
    return intent_classifier_.infer_intent(query_embedding, tokens);
}

language::ReasoningStrategy UnifiedIntelligence::get_strategy(language::ReasoningIntent intent) {
    return intent_classifier_.get_strategy(intent);
}

std::vector<int> UnifiedIntelligence::activate_nodes(const std::vector<std::string>& tokens) {
    std::vector<int> node_ids;
    for (const auto& token : tokens) {
        auto it = word_to_id_.find(token);
        if (it != word_to_id_.end()) {
            node_ids.push_back(it->second);
        }
    }
    return node_ids;
}

void UnifiedIntelligence::spread_activation(
    const std::vector<int>& seeds,
    const language::ReasoningStrategy& strategy,
    const std::vector<float>& query_embedding,
    std::unordered_map<int, float>& activations,
    std::unordered_map<int, std::vector<int>>& paths
) {
    // Get genome parameters
    auto& params = genome_.reasoning_params();
    
    // Energy-driven BFS
    std::queue<std::pair<int, float>> frontier;
    std::set<int> visited;
    
    // Initialize seeds
    for (int seed : seeds) {
        frontier.push({seed, 1.0f});
        activations[seed] = 1.0f;
        paths[seed] = {seed};
        visited.insert(seed);
    }
    
    // Spread using genome temperature and thresholds
    int iterations = 0;
    const int max_iterations = 500;
    
    while (!frontier.empty() && iterations < max_iterations) {
        auto [current, energy] = frontier.front();
        frontier.pop();
        
        if (energy < params.semantic_threshold) continue;
        
        auto neighbors_it = graph_.find(current);
        if (neighbors_it == graph_.end()) continue;
        
        for (const auto& [neighbor, edge_weight] : neighbors_it->second) {
            if (visited.count(neighbor)) continue;
            
            // Semantic biasing
            float semantic_fit = 0.5f;
            auto emb_it = embeddings_.find(neighbor);
            if (emb_it != embeddings_.end()) {
                semantic_fit = cosine_similarity(emb_it->second, query_embedding);
                semantic_fit = (semantic_fit + 1.0f) / 2.0f;  // Normalize to [0,1]
            }
            
            // Combine with genome temperature
            float effective_energy = energy * edge_weight * semantic_fit * params.temperature;
            
            if (effective_energy > params.semantic_threshold) {
                activations[neighbor] = effective_energy;
                paths[neighbor] = paths[current];
                paths[neighbor].push_back(neighbor);
                
                frontier.push({neighbor, effective_energy * 0.9f});
                visited.insert(neighbor);
            }
        }
        
        iterations++;
    }
}

std::vector<std::pair<int, float>> UnifiedIntelligence::score_and_rank(
    const std::unordered_map<int, float>& activations,
    const std::unordered_map<int, std::vector<int>>& paths,
    const std::vector<float>& query_embedding
) {
    // Get genome scoring weights
    auto& params = genome_.reasoning_params();
    
    std::vector<std::pair<int, float>> scored;
    
    for (const auto& [node_id, activation] : activations) {
        // Semantic fit
        float semantic_fit = 0.5f;
        auto emb_it = embeddings_.find(node_id);
        if (emb_it != embeddings_.end()) {
            semantic_fit = cosine_similarity(emb_it->second, query_embedding);
            semantic_fit = (semantic_fit + 1.0f) / 2.0f;
        }
        
        // Path coherence
        float coherence = 1.0f;
        auto path_it = paths.find(node_id);
        if (path_it != paths.end() && path_it->second.size() > 1) {
            coherence = 1.0f / std::sqrt(static_cast<float>(path_it->second.size()));
        }
        
        // Unified score using genome weights (α, β, γ)
        float score = params.activation_weight * activation +
                     params.semantic_bias_weight * semantic_fit +
                     params.coherence_weight * coherence;
        
        scored.push_back({node_id, score});
    }
    
    // Sort by score
    std::sort(scored.begin(), scored.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    return scored;
}

std::string UnifiedIntelligence::synthesize_answer(
    const std::vector<std::pair<int, float>>& ranked,
    language::ReasoningIntent intent,
    const std::vector<std::string>& query_tokens
) {
    if (ranked.empty()) return "No answer found.";
    
    std::stringstream ss;
    
    // Intent-specific templates
    switch (intent) {
        case language::ReasoningIntent::DEFINE:
            if (!query_tokens.empty()) {
                ss << query_tokens.back() << " is related to: ";
            } else {
                ss << "Related concepts: ";
            }
            break;
            
        case language::ReasoningIntent::LOCATE:
            ss << "Located in: ";
            break;
            
        case language::ReasoningIntent::CAUSE:
            ss << "This occurs because of: ";
            break;
            
        default:
            ss << "Answer: ";
            break;
    }
    
    // Add top concepts
    size_t count = 0;
    for (const auto& [node_id, score] : ranked) {
        if (count >= 8) break;
        
        auto it = id_to_word_.find(node_id);
        if (it != id_to_word_.end()) {
            if (count > 0) ss << ", ";
            ss << it->second;
            count++;
        }
    }
    
    ss << ".";
    
    return ss.str();
}

void UnifiedIntelligence::update_metrics(
    const std::unordered_map<int, float>& activations,
    const std::vector<std::pair<int, float>>& ranked
) {
    // Collect activation values
    std::vector<int> active_nodes;
    std::vector<float> activation_values;
    
    for (const auto& [node_id, activation] : activations) {
        active_nodes.push_back(node_id);
        activation_values.push_back(activation);
    }
    
    std::unordered_set<int> working_memory;  // Empty for now
    
    // Update metrics tracker
    metrics_tracker_.update_from_activation(
        active_nodes,
        activation_values,
        working_memory
    );
    
    // Extract confidence from top scores
    if (!ranked.empty()) {
        current_metrics_.confidence = ranked[0].second;
    }
    
    current_metrics_ = metrics_tracker_.current();
}

void UnifiedIntelligence::reflect_and_adapt() {
    // Observe current state
    reflection_controller_.observe(current_metrics_);
    
    // Decide if mode switch needed and apply
    bool switched = reflection_controller_.reflect_and_adapt();
    
    if (switched) {
        current_mode_ = reflection_controller_.current_mode();
    }
}

std::vector<float> UnifiedIntelligence::compute_embedding(const std::vector<std::string>& tokens) {
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

float UnifiedIntelligence::cosine_similarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty()) return 0.0f;
    
    float dot = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;
    
    for (size_t i = 0; i < a.size(); i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    float denom = std::sqrt(norm_a) * std::sqrt(norm_b);
    return (denom > 1e-6f) ? (dot / denom) : 0.0f;
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// GRAPH GROWTH METHODS (Human-like Learning)
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

int UnifiedIntelligence::add_concept(const std::string& concept, const std::vector<float>& embedding) {
    std::lock_guard<std::mutex> lock(graph_mutex_);
    
    // Check if concept already exists
    auto it = word_to_id_.find(concept);
    if (it != word_to_id_.end()) {
        return it->second;  // Return existing ID
    }
    
    // Create new node
    int new_id = next_node_id_.fetch_add(1, std::memory_order_relaxed);
    
    word_to_id_[concept] = new_id;
    id_to_word_[new_id] = concept;
    embeddings_[new_id] = embedding;
    graph_[new_id] = {};  // Initialize empty edge list
    
    return new_id;
}

bool UnifiedIntelligence::strengthen_connection(int from_id, int to_id, float weight_delta) {
    if (from_id == to_id) return false;
    
    std::lock_guard<std::mutex> lock(graph_mutex_);
    
    // Find existing edge
    auto& edges = graph_[from_id];
    bool found = false;
    
    for (auto& [neighbor, weight] : edges) {
        if (neighbor == to_id) {
            // Strengthen existing edge (cap at 1.0)
            weight = std::min(1.0f, weight + weight_delta);
            found = true;
            break;
        }
    }
    
    // Create new edge if not found
    if (!found) {
        edges.push_back({to_id, weight_delta});
    }
    
    // Also add reverse edge (symmetric, weaker)
    auto& reverse_edges = graph_[to_id];
    bool reverse_found = false;
    
    for (auto& [neighbor, weight] : reverse_edges) {
        if (neighbor == from_id) {
            weight = std::min(1.0f, weight + weight_delta * 0.8f);
            reverse_found = true;
            break;
        }
    }
    
    if (!reverse_found) {
        reverse_edges.push_back({from_id, weight_delta * 0.8f});
    }
    
    return true;
}

void UnifiedIntelligence::weaken_connection(int from_id, int to_id, float weight_delta) {
    if (from_id == to_id) return;
    
    std::lock_guard<std::mutex> lock(graph_mutex_);
    
    constexpr float MIN_WEIGHT = 0.01f;  // Threshold for edge removal
    
    // Weaken forward edge
    auto& edges = graph_[from_id];
    for (auto it = edges.begin(); it != edges.end();) {
        if (it->first == to_id) {
            it->second = std::max(0.0f, it->second - weight_delta);
            
            if (it->second < MIN_WEIGHT) {
                it = edges.erase(it);  // Remove weak edge
            } else {
                ++it;
            }
            break;
        } else {
            ++it;
        }
    }
    
    // Weaken reverse edge
    auto& reverse_edges = graph_[to_id];
    for (auto it = reverse_edges.begin(); it != reverse_edges.end();) {
        if (it->first == from_id) {
            it->second = std::max(0.0f, it->second - weight_delta);
            
            if (it->second < MIN_WEIGHT) {
                it = reverse_edges.erase(it);
            } else {
                ++it;
            }
            break;
        } else {
            ++it;
        }
    }
}

void UnifiedIntelligence::apply_hebbian_learning(
    const std::unordered_map<int, float>& activations,
    float learning_rate
) {
    if (activations.empty()) return;
    
    std::lock_guard<std::mutex> lock(graph_mutex_);
    
    // Hebbian rule: Δw = η × pre × post
    // Strengthen connections between co-activated nodes
    std::vector<int> active_nodes;
    for (const auto& [node_id, activation] : activations) {
        if (activation > 0.3f) {  // Only consider significantly active nodes
            active_nodes.push_back(node_id);
        }
    }
    
    // Strengthen edges between all pairs of active nodes (inlined to avoid deadlock)
    for (size_t i = 0; i < active_nodes.size(); i++) {
        int node_a = active_nodes[i];
        float activation_a = activations.at(node_a);
        
        for (size_t j = i + 1; j < active_nodes.size(); j++) {
            int node_b = active_nodes[j];
            float activation_b = activations.at(node_b);
            
            // Hebbian update: strengthen connection proportional to product of activations
            float delta = learning_rate * activation_a * activation_b;
            
            // Inline edge strengthening (already have lock)
            auto& edges_a = graph_[node_a];
            bool found_a = false;
            for (auto& [neighbor, weight] : edges_a) {
                if (neighbor == node_b) {
                    weight = std::min(1.0f, weight + delta);
                    found_a = true;
                    break;
                }
            }
            if (!found_a) {
                edges_a.push_back({node_b, delta});
            }
            
            // Reverse edge
            auto& edges_b = graph_[node_b];
            bool found_b = false;
            for (auto& [neighbor, weight] : edges_b) {
                if (neighbor == node_a) {
                    weight = std::min(1.0f, weight + delta * 0.8f);
                    found_b = true;
                    break;
                }
            }
            if (!found_b) {
                edges_b.push_back({node_a, delta * 0.8f});
            }
        }
    }
}

} // namespace intelligence
} // namespace melvin

