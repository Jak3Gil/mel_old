#include "activation_field_unified.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <set>

namespace melvin {
namespace fields {

// Helper function for atomic float addition (C++17 compatible)
static void atomic_float_add(std::atomic<float>& target, float delta) {
    float old_val = target.load();
    while (!target.compare_exchange_weak(old_val, old_val + delta)) {
        // Retry until successful
    }
}

// ============================================================================
// WorkingContext Implementation
// ============================================================================

void WorkingContext::update_concept(int node_id, float activation, float salience,
                                    const std::vector<float>& embedding) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    
    // Find if concept already exists
    auto it = std::find_if(concepts.begin(), concepts.end(),
                          [node_id](const ActiveConcept& c) { return c.node_id == node_id; });
    
    if (it != concepts.end()) {
        // Update existing
        it->activation = activation;
        it->salience = salience;
        it->last_refresh = std::chrono::high_resolution_clock::now();
        it->embedding = embedding;
    } else {
        // Add new concept
        if (concepts.size() >= 7) {
            // Remove weakest concept
            auto weakest = std::min_element(concepts.begin(), concepts.end(),
                [](const ActiveConcept& a, const ActiveConcept& b) {
                    return a.activation * a.salience < b.activation * b.salience;
                });
            concepts.erase(weakest);
        }
        concepts.emplace_back(node_id, activation, salience, embedding);
    }
}

void WorkingContext::decay_by_time(float decay_rate) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    auto now = std::chrono::high_resolution_clock::now();
    
    for (auto& concept : concepts) {
        auto elapsed = std::chrono::duration<float>(now - concept.last_refresh).count();
        concept.activation *= std::exp(-decay_rate * elapsed);
    }
    
    // Remove concepts below threshold
    concepts.erase(
        std::remove_if(concepts.begin(), concepts.end(),
                      [](const ActiveConcept& c) { return c.activation < 0.01f; }),
        concepts.end()
    );
}

std::vector<int> WorkingContext::get_active_nodes(size_t max_count) {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    
    // Sort by activation * salience
    std::vector<ActiveConcept> sorted = concepts;
    std::sort(sorted.begin(), sorted.end(),
             [](const ActiveConcept& a, const ActiveConcept& b) {
                 return a.activation * a.salience > b.activation * b.salience;
             });
    
    std::vector<int> result;
    for (size_t i = 0; i < std::min(max_count, sorted.size()); ++i) {
        result.push_back(sorted[i].node_id);
    }
    return result;
}

std::vector<float> WorkingContext::get_context_vector() {
    std::lock_guard<std::mutex> lock(buffer_mutex);
    
    if (concepts.empty()) {
        return std::vector<float>(128, 0.0f);  // Default dimension
    }
    
    size_t dim = concepts[0].embedding.size();
    std::vector<float> context(dim, 0.0f);
    float total_weight = 0.0f;
    
    for (const auto& concept : concepts) {
        float weight = concept.activation * concept.salience;
        for (size_t i = 0; i < dim; ++i) {
            context[i] += weight * concept.embedding[i];
        }
        total_weight += weight;
    }
    
    if (total_weight > 0.0f) {
        for (auto& val : context) {
            val /= total_weight;
        }
    }
    
    return context;
}

// ============================================================================
// ContextHorizon Implementation
// ============================================================================

std::vector<ContextHorizon::HopNode> ContextHorizon::propagate(
    int origin_node,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    int max_hops,
    float threshold) {
    
    std::vector<HopNode> result;
    std::unordered_map<int, float> best_activation;
    std::unordered_map<int, int> best_distance;
    
    // BFS with priority queue
    struct QueueItem {
        int node_id;
        float activation;
        int distance;
        float path_strength;
        
        bool operator<(const QueueItem& other) const {
            return activation < other.activation;  // Max heap
        }
    };
    
    std::priority_queue<QueueItem> queue;
    queue.push({origin_node, 1.0f, 0, 1.0f});
    best_activation[origin_node] = 1.0f;
    best_distance[origin_node] = 0;
    
    while (!queue.empty()) {
        auto current = queue.top();
        queue.pop();
        
        if (current.distance >= max_hops) continue;
        if (current.activation < threshold) continue;
        
        // Skip if we found a better path already
        if (best_activation.count(current.node_id) && 
            best_activation[current.node_id] > current.activation) {
            continue;
        }
        
        // Add to result
        result.push_back({current.node_id, current.activation, 
                         current.distance, current.path_strength});
        
        // Propagate to neighbors
        auto it = graph.find(current.node_id);
        if (it != graph.end()) {
            for (const auto& [neighbor_id, edge_weight] : it->second) {
                float new_activation = current.activation * edge_weight * 0.8f;  // Decay
                float new_path_strength = current.path_strength * edge_weight;
                
                if (new_activation >= threshold) {
                    if (!best_activation.count(neighbor_id) || 
                        best_activation[neighbor_id] < new_activation) {
                        
                        best_activation[neighbor_id] = new_activation;
                        best_distance[neighbor_id] = current.distance + 1;
                        
                        queue.push({neighbor_id, new_activation, 
                                   current.distance + 1, new_path_strength});
                    }
                }
            }
        }
    }
    
    return result;
}

std::vector<float> ContextHorizon::compute_context_vector(
    const std::vector<HopNode>& neighborhood,
    const std::unordered_map<int, std::vector<float>>& embeddings) {
    
    if (neighborhood.empty()) {
        return std::vector<float>(128, 0.0f);
    }
    
    // Get embedding dimension from first available embedding
    size_t dim = 128;
    for (const auto& node : neighborhood) {
        auto it = embeddings.find(node.node_id);
        if (it != embeddings.end() && !it->second.empty()) {
            dim = it->second.size();
            break;
        }
    }
    
    std::vector<float> context(dim, 0.0f);
    float total_weight = 0.0f;
    
    for (const auto& node : neighborhood) {
        auto it = embeddings.find(node.node_id);
        if (it != embeddings.end()) {
            float weight = node.activation * node.path_strength;
            for (size_t i = 0; i < dim && i < it->second.size(); ++i) {
                context[i] += weight * it->second[i];
            }
            total_weight += weight;
        }
    }
    
    if (total_weight > 0.0f) {
        for (auto& val : context) {
            val /= total_weight;
        }
    }
    
    return context;
}

// ============================================================================
// TemporalHierarchy Implementation
// ============================================================================

int TemporalHierarchy::compress_sequence(const std::vector<int>& node_ids, Level from_level) {
    if (node_ids.empty()) return -1;
    
    // Generate new ID for compressed node
    static int next_temporal_id = 1000000;  // Start from high number
    int new_id = next_temporal_id++;
    
    // Determine target level
    Level to_level;
    switch (from_level) {
        case Level::FRAMES:    to_level = Level::SCENES; break;
        case Level::SCENES:    to_level = Level::EPISODES; break;
        case Level::EPISODES:  to_level = Level::NARRATIVES; break;
        default:              return -1;  // Already at highest level
    }
    
    // Create temporal node
    TemporalNode tnode;
    tnode.level = to_level;
    tnode.constituent_nodes = node_ids;
    tnode.start_time = std::chrono::high_resolution_clock::now();
    tnode.end_time = tnode.start_time;  // Will be updated
    tnode.coherence = 1.0f;  // TODO: Compute based on constituent similarity
    tnode.summary_embedding = std::vector<float>(128, 0.0f);  // TODO: Compute
    
    temporal_nodes[new_id] = tnode;
    return new_id;
}

std::vector<int> TemporalHierarchy::retrieve_temporal_context(
    std::chrono::high_resolution_clock::time_point query_time,
    Level level,
    int max_results) {
    
    std::vector<std::pair<int, float>> candidates;  // (node_id, relevance)
    
    for (const auto& [id, tnode] : temporal_nodes) {
        if (tnode.level != level) continue;
        
        // Compute temporal relevance
        auto time_diff = std::abs(std::chrono::duration<float>(
            query_time - tnode.start_time).count());
        
        float relevance = tnode.coherence * std::exp(-time_diff / 60.0f);  // 1-min decay
        candidates.push_back({id, relevance});
    }
    
    // Sort by relevance
    std::sort(candidates.begin(), candidates.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Return top results
    std::vector<int> result;
    for (size_t i = 0; i < std::min((size_t)max_results, candidates.size()); ++i) {
        result.push_back(candidates[i].first);
    }
    return result;
}

// ============================================================================
// UnifiedActivationField Implementation
// ============================================================================

UnifiedActivationField::UnifiedActivationField() 
    : total_energy_(0.0f),
      last_tick_(std::chrono::high_resolution_clock::now()) {
}

UnifiedActivationField::~UnifiedActivationField() {
}

void UnifiedActivationField::inject_energy(int node_id, float energy, 
                                           const std::vector<float>& embedding) {
    std::lock_guard<std::mutex> lock(field_mutex_);
    
    activations_[node_id] += energy;
    embeddings_[node_id] = embedding;
    atomic_float_add(total_energy_, energy);
    
    // Update working context
    float salience = energy / max_total_energy_;  // Normalize
    working_context_.update_concept(node_id, activations_[node_id], salience, embedding);
}

float UnifiedActivationField::get_activation(int node_id) const {
    std::lock_guard<std::mutex> lock(field_mutex_);
    auto it = activations_.find(node_id);
    return (it != activations_.end()) ? it->second : 0.0f;
}

void UnifiedActivationField::set_activation(int node_id, float activation) {
    std::lock_guard<std::mutex> lock(field_mutex_);
    float old_activation = activations_[node_id];
    activations_[node_id] = activation;
    atomic_float_add(total_energy_, activation - old_activation);
}

void UnifiedActivationField::tick(float dt) {
    auto now = std::chrono::high_resolution_clock::now();
    float actual_dt = std::chrono::duration<float>(now - last_tick_).count();
    last_tick_ = now;
    
    // Run all field dynamics in parallel (conceptually)
    decay_activations(actual_dt);
    spread_activation(actual_dt);
    update_working_context(actual_dt);
    hopfield_update(actual_dt);
    process_messages();
}

void UnifiedActivationField::decay_activations(float dt) {
    std::lock_guard<std::mutex> lock(field_mutex_);
    
    float decay_rate = 0.1f;  // TODO: Get from genome
    float decay_factor = std::exp(-decay_rate * dt);
    
    for (auto& [node_id, activation] : activations_) {
        float old_val = activation;
        activation *= decay_factor;
        atomic_float_add(total_energy_, activation - old_val);
        
        if (activation < 0.001f) {
            activation = 0.0f;
        }
    }
}

void UnifiedActivationField::spread_activation(float dt) {
    std::lock_guard<std::mutex> lock(field_mutex_);
    
    std::unordered_map<int, float> spread_delta;
    float spread_rate = 0.3f;  // TODO: Get from genome
    
    for (const auto& [from_node, activation] : activations_) {
        if (activation < 0.01f) continue;
        
        auto it = edges_out_.find(from_node);
        if (it != edges_out_.end()) {
            for (const auto& edge : it->second) {
                float transfer = activation * edge.get_effective_weight() * spread_rate * dt;
                spread_delta[edge.to_node] += transfer;
                spread_delta[from_node] -= transfer;
            }
        }
    }
    
    // Apply spread
    for (const auto& [node_id, delta] : spread_delta) {
        activations_[node_id] += delta;
        atomic_float_add(total_energy_, delta);
    }
}

void UnifiedActivationField::update_working_context(float dt) {
    working_context_.decay_by_time(dt);
}

void UnifiedActivationField::hopfield_update(float dt) {
    // Simple attractor dynamics - nodes with strong mutual connections stabilize
    std::lock_guard<std::mutex> lock(field_mutex_);
    
    std::unordered_map<int, float> hopfield_delta;
    
    for (const auto& [node_id, activation] : activations_) {
        if (activation < 0.01f) continue;
        
        // Sum incoming activation
        float incoming = 0.0f;
        auto it_in = edges_in_.find(node_id);
        if (it_in != edges_in_.end()) {
            for (const auto& edge : it_in->second) {
                auto it_act = activations_.find(edge.from_node);
                if (it_act != activations_.end()) {
                    incoming += it_act->second * edge.get_effective_weight();
                }
            }
        }
        
        // Hopfield update rule: Δa = tanh(incoming) - a
        float target = std::tanh(incoming * 0.1f);
        hopfield_delta[node_id] = (target - activation) * 0.1f * dt;
    }
    
    // Apply updates
    for (const auto& [node_id, delta] : hopfield_delta) {
        activations_[node_id] += delta;
        atomic_float_add(total_energy_, delta);
    }
}

void UnifiedActivationField::process_messages() {
    std::vector<FieldMessage> messages;
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!message_queue_.empty()) {
            messages.push_back(message_queue_.front());
            message_queue_.pop();
        }
    }
    
    // Process each message type
    for (const auto& msg : messages) {
        switch (msg.type) {
            case MessageType::SENSORY_INPUT:
                inject_energy(msg.target_node_id, msg.energy, msg.data);
                break;
                
            case MessageType::PREDICTION_ERROR:
                // Adjust edge weights based on error
                // TODO: Implement prediction error learning
                break;
                
            case MessageType::GOAL_UPDATE:
                // Bias activations toward goal
                // TODO: Implement goal-directed spreading
                break;
                
            case MessageType::CONSOLIDATION:
                // Trigger memory consolidation
                // TODO: Implement
                break;
                
            case MessageType::EVOLUTION:
                // Trigger genome mutation
                // TODO: Implement
                break;
                
            case MessageType::REFLECTION:
                // Meta-cognitive query
                // TODO: Implement
                break;
                
            default:
                break;
        }
    }
}

void UnifiedActivationField::post_message(const FieldMessage& msg) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    message_queue_.push(msg);
}

std::vector<FieldMessage> UnifiedActivationField::drain_messages(MessageType type) {
    std::vector<FieldMessage> result;
    std::vector<FieldMessage> remaining;
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!message_queue_.empty()) {
        auto msg = message_queue_.front();
        message_queue_.pop();
        
        if (msg.type == type) {
            result.push_back(msg);
        } else {
            remaining.push_back(msg);
        }
    }
    
    // Put remaining messages back
    for (const auto& msg : remaining) {
        message_queue_.push(msg);
    }
    
    return result;
}

std::vector<float> UnifiedActivationField::compute_global_context(int origin_node, int max_hops) {
    std::lock_guard<std::mutex> lock(field_mutex_);
    
    // Convert edges to graph format for propagation
    std::unordered_map<int, std::vector<std::pair<int, float>>> graph;
    for (const auto& [from_node, edges] : edges_out_) {
        for (const auto& edge : edges) {
            graph[from_node].push_back({edge.to_node, edge.get_effective_weight()});
        }
    }
    
    auto neighborhood = context_horizon_.propagate(origin_node, graph, max_hops);
    return context_horizon_.compute_context_vector(neighborhood, embeddings_);
}

void UnifiedActivationField::add_edge(int from, int to, HybridEdge::Type type, float weight,
                                      const std::vector<float>& from_emb,
                                      const std::vector<float>& to_emb) {
    std::lock_guard<std::mutex> lock(field_mutex_);
    
    // Compute embedding similarity (cosine)
    float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
    size_t dim = std::min(from_emb.size(), to_emb.size());
    for (size_t i = 0; i < dim; ++i) {
        dot += from_emb[i] * to_emb[i];
        norm_a += from_emb[i] * from_emb[i];
        norm_b += to_emb[i] * to_emb[i];
    }
    float embedding_sim = (norm_a > 0 && norm_b > 0) ? 
        dot / (std::sqrt(norm_a) * std::sqrt(norm_b)) : 0.0f;
    
    HybridEdge edge(from, to, type, weight, embedding_sim);
    edges_out_[from].push_back(edge);
    edges_in_[to].push_back(edge);
}

const std::vector<HybridEdge>& UnifiedActivationField::get_edges_from(int node_id) const {
    static std::vector<HybridEdge> empty;
    std::lock_guard<std::mutex> lock(field_mutex_);
    auto it = edges_out_.find(node_id);
    return (it != edges_out_.end()) ? it->second : empty;
}

void UnifiedActivationField::bind_cross_modal(int text_node, int vision_node, int motor_node,
                                               float temporal_overlap) {
    std::lock_guard<std::mutex> lock(field_mutex_);
    
    float text_act = activations_[text_node];
    float vision_act = activations_[vision_node];
    float motor_act = activations_[motor_node];
    
    // Δw = η × activation_text × activation_vision × temporal_overlap
    float binding_strength = 0.1f * text_act * vision_act * temporal_overlap;
    
    // Create bidirectional cross-modal edges
    if (embeddings_.count(text_node) && embeddings_.count(vision_node)) {
        add_edge(text_node, vision_node, HybridEdge::Type::EXACT, binding_strength,
                embeddings_[text_node], embeddings_[vision_node]);
        add_edge(vision_node, text_node, HybridEdge::Type::EXACT, binding_strength,
                embeddings_[vision_node], embeddings_[text_node]);
    }
    
    if (motor_node >= 0 && embeddings_.count(motor_node)) {
        float motor_binding = 0.1f * vision_act * motor_act * temporal_overlap;
        if (embeddings_.count(vision_node)) {
            add_edge(vision_node, motor_node, HybridEdge::Type::VISUOMOTOR, motor_binding,
                    embeddings_[vision_node], embeddings_[motor_node]);
        }
    }
}

void UnifiedActivationField::trigger_reflection(const std::string& query) {
    std::lock_guard<std::mutex> lock(reflection_mutex_);
    reflections_.push_back(query);
    // TODO: Actually process the reflection
}

std::vector<std::string> UnifiedActivationField::get_reflections() {
    std::lock_guard<std::mutex> lock(reflection_mutex_);
    return reflections_;
}

size_t UnifiedActivationField::get_active_node_count() const {
    std::lock_guard<std::mutex> lock(field_mutex_);
    return std::count_if(activations_.begin(), activations_.end(),
                        [](const auto& p) { return p.second >= 0.01f; });
}

float UnifiedActivationField::get_total_energy() const {
    return total_energy_.load();
}

float UnifiedActivationField::get_coherence() const {
    return compute_field_coherence();
}

float UnifiedActivationField::compute_field_coherence() const {
    std::lock_guard<std::mutex> lock(field_mutex_);
    
    // Coherence = variance of activation magnitudes (low = stable)
    std::vector<float> activations;
    for (const auto& [node_id, act] : activations_) {
        if (act >= 0.01f) {
            activations.push_back(act);
        }
    }
    
    if (activations.empty()) return 0.0f;
    
    float mean = std::accumulate(activations.begin(), activations.end(), 0.0f) / activations.size();
    float variance = 0.0f;
    for (float act : activations) {
        float diff = act - mean;
        variance += diff * diff;
    }
    variance /= activations.size();
    
    // Return inverse of variance (high coherence = low variance)
    return 1.0f / (1.0f + variance);
}

} // namespace fields
} // namespace melvin

