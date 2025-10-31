/**
 * @file melvin_brain.h
 * @brief MELVIN's Complete Brain System - All in One File
 * 
 * This is THE file for Melvin's intelligence.
 * Everything else is implementation details.
 * 
 * PRINCIPLE: Change = Need - Cost
 * Connections emerge from usage, not pre-design.
 */

#ifndef MELVIN_BRAIN_H
#define MELVIN_BRAIN_H

#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cctype>

namespace melvin {
namespace brain {

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

struct Edge;
struct Node;
class Graph;
class Brain;

// ============================================================================
// EDGE: A connection between nodes (emerges from co-activation)
// ============================================================================

struct Edge {
    int to_node;
    float strength;           // Connection strength (0-1)
    float need_signal;        // Accumulated need (information value)
    float cost;               // Maintenance cost
    int coactivations;        // Times nodes fired together
    float last_use_time;      // Last activation time
    int successful_uses;      // Times this led to correct answer
    int total_uses;           // Total times used
    
    Edge(int to, float initial = 0.01f) :
        to_node(to),
        strength(initial),
        need_signal(0.0f),
        cost(0.001f),
        coactivations(0),
        last_use_time(0.0f),
        successful_uses(0),
        total_uses(0)
    {}
};

// ============================================================================
// NODE: A concept/word in the knowledge graph
// ============================================================================

struct Node {
    int id;
    std::string token;
    std::vector<float> embedding;
    float activation;          // Current energy level
    int usage_count;          // How often used
    std::unordered_map<int, Edge> edges;  // Outgoing connections
    
    Node(int node_id, const std::string& t = "", const std::vector<float>& emb = {}) :
        id(node_id),
        token(t),
        embedding(emb),
        activation(0.0f),
        usage_count(0)
    {}
};

// ============================================================================
// GRAPH: The knowledge graph (nodes + edges)
// ============================================================================

class Graph {
public:
    Graph() : 
        next_node_id_(0),
        current_time_(0.0f),
        total_energy_(100.0f),
        current_energy_(0.0f),
        decay_rate_(0.9f),
        spread_factor_(0.3f)
    {}
    
    // Activate nodes (input)
    void activate(const std::vector<int>& node_ids, float strength = 1.0f) {
        std::lock_guard<std::mutex> lock(mutex_);
        current_time_ += 1.0f;
        
        for (int node_id : node_ids) {
            auto it = nodes_.find(node_id);
            if (it != nodes_.end()) {
                it->second.activation += strength;
                it->second.usage_count++;
                recent_activations_[node_id] = current_time_;
                consume_energy(0.1f * strength);
            }
        }
        
        // Record co-activations (for connection formation)
        for (size_t i = 0; i < node_ids.size(); i++) {
            for (size_t j = i + 1; j < node_ids.size(); j++) {
                int a = node_ids[i];
                int b = node_ids[j];
                if (nodes_.count(a) && nodes_.count(b)) {
                    auto& node_a = nodes_[a];
                    if (node_a.edges.count(b) == 0) {
                        node_a.edges[b] = Edge(b, 0.01f);
                    }
                    auto& edge = node_a.edges[b];
                    edge.coactivations++;
                    edge.last_use_time = current_time_;
                }
            }
        }
    }
    
    // Spread activation through connections
    void spread(float dt = 1.0f) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Decay activations
        for (auto& [id, node] : nodes_) {
            node.activation *= decay_rate_;
            
            // Weaken unused edges
            for (auto& [to_id, edge] : node.edges) {
                float time_since = current_time_ - edge.last_use_time;
                if (time_since > 100.0f) {
                    edge.strength *= 0.99f;
                }
            }
        }
        
        // Spread through edges where need > cost
        std::unordered_map<int, float> new_activations;
        for (const auto& [from_id, node] : nodes_) {
            if (node.activation < 0.01f) continue;
            
            for (const auto& [to_id, edge] : node.edges) {
                float need = compute_need(from_id, to_id);
                float cost = compute_cost(from_id, to_id);
                
                if (need > cost) {
                    float transfer = node.activation * edge.strength * spread_factor_;
                    transfer *= (need - cost);
                    new_activations[to_id] += transfer;
                    consume_energy(edge.cost * 0.1f);
                }
            }
        }
        
        // Apply new activations
        for (const auto& [node_id, energy] : new_activations) {
            auto it = nodes_.find(node_id);
            if (it != nodes_.end()) {
                it->second.activation += energy;
            }
        }
        
        // Recover energy
        current_energy_ = std::max(0.0f, current_energy_ - 1.0f * dt);
    }
    
    // Update connections: Change = Need - Cost
    void update_connections() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto& [from_id, node] : nodes_) {
            for (auto it = node.edges.begin(); it != node.edges.end();) {
                auto& [to_id, edge] = *it;
                
                float need = compute_need(from_id, to_id);
                float cost = compute_cost(from_id, to_id);
                float change = need - cost;
                
                if (change > 0.0f) {
                    // Strengthen (need > cost)
                    edge.strength = std::min(1.0f, edge.strength + 0.01f * change);
                    edge.need_signal += 0.01f * change;
                    if (need > cost * 2.0f) {
                        edge.cost *= 0.999f;  // More efficient
                    }
                } else {
                    // Weaken (cost > need)
                    edge.strength = std::max(0.0f, edge.strength - 0.005f * std::abs(change));
                    edge.cost *= 1.001f;
                }
                
                // Prune weak edges
                if (edge.strength < 0.001f || edge.strength * need < 0.05f) {
                    it = node.edges.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
    
    // Add node (or get existing)
    int add_node(const std::string& token, const std::vector<float>& embedding) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = token_to_id_.find(token);
        if (it != token_to_id_.end()) {
            return it->second;
        }
        
        if (current_energy_ >= total_energy_ * 0.9f) {
            return -1;  // Energy constrained
        }
        
        int new_id = next_node_id_++;
        nodes_[new_id] = Node(new_id, token, embedding);
        token_to_id_[token] = new_id;
        id_to_token_[new_id] = token;
        consume_energy(1.0f);
        
        return new_id;
    }
    
    // Get active nodes
    std::vector<int> get_active(float threshold = 0.1f) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<int> active;
        for (const auto& [id, node] : nodes_) {
            if (node.activation > threshold) {
                active.push_back(id);
            }
        }
        std::sort(active.begin(), active.end(), [this](int a, int b) {
            return nodes_.at(a).activation > nodes_.at(b).activation;
        });
        return active;
    }
    
    float get_activation(int node_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodes_.find(node_id);
        return (it != nodes_.end()) ? it->second.activation : 0.0f;
    }
    
    // Accessors
    std::unordered_map<int, Node>& nodes() { return nodes_; }
    const std::unordered_map<int, Node>& nodes() const { return nodes_; }
    const std::unordered_map<std::string, int>& token_to_id() const { return token_to_id_; }
    const std::unordered_map<int, std::string>& id_to_token() const { return id_to_token_; }
    
private:
    std::unordered_map<int, Node> nodes_;
    std::unordered_map<std::string, int> token_to_id_;
    std::unordered_map<int, std::string> id_to_token_;
    mutable std::mutex mutex_;
    int next_node_id_;
    float current_time_;
    float total_energy_;
    float current_energy_;
    float decay_rate_;
    float spread_factor_;
    std::unordered_map<int, float> recent_activations_;
    
    void consume_energy(float amount) {
        current_energy_ += amount;
        current_energy_ = std::min(current_energy_, total_energy_);
    }
    
    float compute_need(int from_id, int to_id) const {
        auto from_it = nodes_.find(from_id);
        auto to_it = nodes_.find(to_id);
        if (from_it == nodes_.end() || to_it == nodes_.end()) return 0.0f;
        
        const auto& from_node = from_it->second;
        auto edge_it = from_node.edges.find(to_id);
        if (edge_it == from_node.edges.end()) return 0.1f;
        
        const auto& edge = edge_it->second;
        
        // Need = predictive value + utility + success rate
        float prediction = std::min(1.0f, edge.coactivations / 50.0f);
        float utility = std::min(1.0f, edge.coactivations / 100.0f);
        float success = edge.total_uses > 0 ? 
            (float)edge.successful_uses / edge.total_uses : 0.5f;
        float novelty = std::exp(-(current_time_ - edge.last_use_time) / 50.0f);
        
        return 0.3f * prediction + 0.2f * utility + 0.3f * success + 0.2f * novelty;
    }
    
    float compute_cost(int from_id, int to_id) const {
        auto from_it = nodes_.find(from_id);
        if (from_it == nodes_.end()) return 1.0f;
        
        auto edge_it = from_it->second.edges.find(to_id);
        if (edge_it == from_it->second.edges.end()) return 0.05f;
        
        const auto& edge = edge_it->second;
        float energy_cost = edge.cost * edge.strength;
        float memory_cost = 0.001f;
        float compute_cost = 0.001f * edge.strength;
        
        return energy_cost + memory_cost + compute_cost;
    }
};

// ============================================================================
// BRAIN: The complete intelligence system
// ============================================================================

class Brain {
public:
    Brain() {}
    
    // Initialize with existing graph data
    void load_graph(
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& edges,
        const std::unordered_map<int, std::vector<float>>& embeddings,
        const std::unordered_map<std::string, int>& word_to_id,
        const std::unordered_map<int, std::string>& id_to_word
    ) {
        // Migrate existing graph
        for (const auto& [node_id, neighbors] : edges) {
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
            
            graph_.add_node(token, embedding);
        }
        
        word_to_id_ = word_to_id;
        id_to_word_ = id_to_word;
    }
    
    // Process a query
    struct Result {
        std::string answer;
        float confidence;
        std::vector<std::string> concepts;
        
        Result() : confidence(0.0f) {}
    };
    
    Result think(const std::string& query) {
        Result result;
        
        // 1. Tokenize
        std::vector<std::string> tokens = tokenize(query);
        if (tokens.empty()) {
            result.answer = "I didn't understand that.";
            return result;
        }
        
        // 2. Find/create nodes
        std::vector<int> node_ids;
        for (const auto& token : tokens) {
            auto it = word_to_id_.find(token);
            if (it != word_to_id_.end()) {
                node_ids.push_back(it->second);
            } else {
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
        
        // 3. Activate and spread
        graph_.activate(node_ids, 1.0f);
        for (int i = 0; i < 5; i++) {
            graph_.spread(1.0f);
        }
        graph_.update_connections();
        
        // 4. Get active nodes
        std::vector<int> active = graph_.get_active(0.1f);
        
        // 5. Generate answer
        result.answer = generate_answer(active);
        result.confidence = compute_confidence(active);
        
        // 6. Extract concepts
        for (size_t i = 0; i < std::min(size_t(10), active.size()); i++) {
            auto it = id_to_word_.find(active[i]);
            if (it != id_to_word_.end()) {
                result.concepts.push_back(it->second);
            }
        }
        
        last_nodes_used_ = active;
        return result;
    }
    
    // Learn from feedback
    void learn(bool correct) {
        auto& nodes = graph_.nodes();
        
        for (int node_id : last_nodes_used_) {
            auto node_it = nodes.find(node_id);
            if (node_it == nodes.end()) continue;
            
            for (auto& [to_id, edge] : node_it->second.edges) {
                edge.total_uses++;
                if (correct) {
                    edge.successful_uses++;
                    edge.need_signal += 0.1f;
                    edge.cost *= 0.99f;
                } else {
                    edge.cost += 0.1f;
                }
            }
        }
        
        graph_.update_connections();
    }
    
    // Get the graph (for external access)
    Graph& graph() { return graph_; }
    const Graph& graph() const { return graph_; }
    
private:
    Graph graph_;
    std::unordered_map<std::string, int> word_to_id_;
    std::unordered_map<int, std::string> id_to_word_;
    std::vector<int> last_nodes_used_;
    
    std::vector<std::string> tokenize(const std::string& query) {
        std::vector<std::string> tokens;
        std::stringstream ss(query);
        std::string word;
        
        while (ss >> word) {
            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            if (!word.empty() && word.length() > 1) {
                tokens.push_back(word);
            }
        }
        return tokens;
    }
    
    std::vector<float> compute_embedding(const std::vector<std::string>& tokens) {
        std::vector<float> embedding(128, 0.0f);
        for (const auto& token : tokens) {
            size_t hash = std::hash<std::string>{}(token);
            for (size_t i = 0; i < 128; i++) {
                embedding[i] += std::sin(static_cast<float>(hash + i) * 0.01f);
            }
        }
        
        float norm = 0.0f;
        for (float val : embedding) norm += val * val;
        norm = std::sqrt(norm);
        if (norm > 1e-6f) {
            for (float& val : embedding) val /= norm;
        }
        return embedding;
    }
    
    std::string generate_answer(const std::vector<int>& active_nodes) {
        if (active_nodes.empty()) {
            return "I'm not sure how to answer that.";
        }
        
        std::stringstream ss;
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
            answer[0] = std::toupper(answer[0]);
            answer += ".";
        } else {
            answer = "I'm processing that.";
        }
        return answer;
    }
    
    float compute_confidence(const std::vector<int>& active_nodes) {
        if (active_nodes.empty()) return 0.1f;
        
        float top = graph_.get_activation(active_nodes[0]);
        float total = 0.0f;
        for (int node_id : active_nodes) {
            total += graph_.get_activation(node_id);
        }
        return (total > 0.0f) ? std::min(1.0f, top / total * 3.0f) : 0.1f;
    }
};

} // namespace brain
} // namespace melvin

#endif // MELVIN_BRAIN_H

