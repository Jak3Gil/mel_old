/*
 * Reasoning Engine Implementation - LEAP + Multi-hop + Adaptive Weighting + GNN + Energy
 */

#include "reasoning.h"
#include "adaptive_weighting.h"
#include "gnn_predictor.h"
#include "energy_field.h"
#include <algorithm>
#include <queue>
#include <set>

namespace melvin {

// ============================================================================
// CONFIG
// ============================================================================

ReasoningEngine::Config::Config() {
    // Default relation priors
    relation_priors["EXACT"] = 1.0f;   // Exact connections have full confidence
    relation_priors["LEAP"] = 0.7f;    // LEAP connections are guesses (lower confidence)
}

// ============================================================================
// IMPLEMENTATION CLASS
// ============================================================================

class ReasoningEngine::Impl {
public:
    Config config;
    Stats stats;
    adaptive::AdaptiveWeighting adaptive_weighting;
    gnn::GNNPredictor gnn_predictor;
    energy::EnergyField energy_field;
    
    // Paths for auto-save
    std::string nodes_path_;
    std::string edges_path_;
    
    // Find nodes matching query
    std::vector<NodeID> find_start_nodes(const std::string& query, Storage* storage) {
        std::vector<NodeID> start_nodes;
        auto nodes = storage->find_nodes(query);
        
        for (const auto& node : nodes) {
            start_nodes.push_back(node.id);
        }
        
        return start_nodes;
    }
    
    // Energy-based adaptive beam search
    std::vector<ReasoningPath> beam_search(
        const std::vector<NodeID>& start_nodes,
        Storage* storage,
        float& final_energy,
        int& actual_hops
    ) {
        std::vector<ReasoningPath> beam;
        
        // Initialize beam with start nodes
        for (const auto& node_id : start_nodes) {
            ReasoningPath path;
            path.nodes.push_back(node_id);
            path.confidence = 1.0f;  // Initial energy
            path.total_weight = 0.0f;
            beam.push_back(path);
        }
        
        float total_energy = 1.0f;
        int hop = 0;
        auto start_time = std::chrono::steady_clock::now();
        
        // Energy-based adaptive loop (replaces fixed MAX_HOPS)
        while (true) {
            std::vector<ReasoningPath> new_beam;
            float new_energy = 0.0f;
            
            for (const auto& path : beam) {
                auto last_node = path.nodes.back();
                auto edges = storage->get_edges_from(last_node);
                
                // Expand each edge with energy propagation
                for (size_t e = 0; e < edges.size(); ++e) {
                    const auto& edge = edges[e];
                    
                    // Calculate signal strength using adaptive weight
                    // Uses effective_weight which combines base + adaptive
                    float signal = path.confidence * edge.get_effective_weight();
                    
                    // Only propagate if signal is meaningful
                    if (signal > config.energy_epsilon) {
                        ReasoningPath new_path = path;
                        new_path.nodes.push_back(edge.to_id);
                        new_path.edges.push_back(e);
                        new_path.total_weight += edge.get_weight();
                        
                        // Energy decays as we go deeper
                        new_path.confidence = signal * config.energy_decay;
                        
                        new_energy += signal;
                        new_beam.push_back(new_path);
                    }
                }
            }
            
            // Termination conditions
            if (new_energy < config.energy_floor) {
                // Energy exhausted - natural stopping point
                break;
            }
            
            if (hop >= config.max_hops_safety) {
                // Safety limit reached
                stats.safety_terminations++;
                break;
            }
            
            // Check time budget
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time
            ).count();
            
            if (elapsed > config.time_budget_ms) {
                // Time budget exceeded
                break;
            }
            
            // Check for runaway loops
            if (new_energy > config.energy_cap) {
                // Energy increasing - something's wrong
                break;
            }
            
            // Keep top beam_width paths
            std::sort(new_beam.begin(), new_beam.end(),
                [](const ReasoningPath& a, const ReasoningPath& b) {
                    return a.confidence > b.confidence;
                });
            
            if (new_beam.size() > static_cast<size_t>(config.beam_width)) {
                new_beam.resize(config.beam_width);
            }
            
            beam = new_beam;
            
            if (beam.empty()) break;
            
            total_energy = new_energy;
            hop++;
        }
        
        // Record final state
        final_energy = total_energy;
        actual_hops = hop;
        
        if (hop < config.max_hops_safety) {
            stats.early_terminations++;
        }
        
        return beam;
    }
};

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

ReasoningEngine::ReasoningEngine(const Config& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
}

ReasoningEngine::~ReasoningEngine() = default;

void ReasoningEngine::set_auto_save_paths(const std::string& nodes_path, const std::string& edges_path) {
    impl_->nodes_path_ = nodes_path;
    impl_->edges_path_ = edges_path;
}

Answer ReasoningEngine::infer(const Query& query, Storage* storage) {
    Answer answer;
    
    if (!storage) {
        answer.text = "Error: No storage provided";
        answer.confidence = 0.0f;
        return answer;
    }
    
    // Find start nodes
    auto start_nodes = impl_->find_start_nodes(query.text, storage);
    
    if (start_nodes.empty()) {
        answer.text = "I don't have information about that.";
        answer.confidence = 0.0f;
        return answer;
    }
    
    // Perform energy-based adaptive beam search
    float final_energy = 0.0f;
    int actual_hops = 0;
    auto paths = impl_->beam_search(start_nodes, storage, final_energy, actual_hops);
    
    if (paths.empty()) {
        answer.text = "I couldn't find a reasoning path.";
        answer.confidence = 0.0f;
        return answer;
    }
    
    // Get best path
    const auto& best_path = paths[0];
    answer.paths.push_back(best_path);
    answer.confidence = best_path.confidence;
    
    // Generate text from final node
    Node final_node;
    if (storage->get_node(best_path.nodes.back(), final_node)) {
        answer.text = storage->get_node_content(final_node.id);
    } else {
        answer.text = "Answer found but content unavailable.";
    }
    
    // Record energy-based metrics
    answer.metrics["final_energy"] = final_energy;
    answer.metrics["actual_hops"] = static_cast<float>(actual_hops);
    answer.metrics["path_length"] = static_cast<float>(best_path.nodes.size());
    
    // Update stats
    impl_->stats.queries_processed++;
    impl_->stats.paths_explored += paths.size();
    impl_->stats.nodes_activated += best_path.nodes.size();
    
    // Update running averages for energy-based stats
    float n = static_cast<float>(impl_->stats.queries_processed);
    impl_->stats.avg_hops_actual = 
        (impl_->stats.avg_hops_actual * (n - 1) + actual_hops) / n;
    impl_->stats.avg_energy_final = 
        (impl_->stats.avg_energy_final * (n - 1) + final_energy) / n;
    
    // Record activations for adaptive weighting
    impl_->adaptive_weighting.record_activation(
        storage->get_nodes_mut(),
        best_path.nodes
    );
    
    // Update adaptive weights periodically
    impl_->adaptive_weighting.update_weights(
        storage->get_nodes_mut(),
        storage->get_edges_mut(),
        storage
    );
    
    // GNN PREDICTOR: Message passing and learning
    auto& nodes = storage->get_nodes_mut();
    auto& edges = storage->get_edges_mut();
    
    // Perform message passing to update embeddings
    impl_->gnn_predictor.message_pass(nodes, edges, storage);
    
    // Predict next activation states
    std::vector<float> predicted = impl_->gnn_predictor.predict_activations(nodes);
    
    // Get actual activations (current state)
    std::vector<float> actual(nodes.size(), 0.0f);
    for (size_t i = 0; i < nodes.size(); ++i) {
        actual[i] = nodes[i].activation;
    }
    
    // Train on prediction error
    impl_->gnn_predictor.train(nodes, actual);
    
    // Create LEAP connections from failed predictions (optional)
    // impl_->gnn_predictor.create_leaps_from_errors(nodes, predicted, actual, storage);
    
    // AUTO-SAVE: Persist all changes after reasoning
    if (!impl_->nodes_path_.empty() && !impl_->edges_path_.empty()) {
        storage->save(impl_->nodes_path_, impl_->edges_path_);
    }
    
    return answer;
}

std::vector<ReasoningPath> ReasoningEngine::find_paths(
    const NodeID& start,
    const NodeID& end,
    Storage* storage,
    int max_hops
) {
    // TODO: Implement targeted path finding
    return {};
}

void ReasoningEngine::set_config(const Config& config) {
    impl_->config = config;
}

const ReasoningEngine::Config& ReasoningEngine::get_config() const {
    return impl_->config;
}

ReasoningEngine::Stats ReasoningEngine::get_stats() const {
    return impl_->stats;
}

void ReasoningEngine::reset_stats() {
    impl_->stats = Stats();
}

} // namespace melvin

