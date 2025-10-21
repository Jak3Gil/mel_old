/*
 * Hopfield-Diffusion Reasoning Implementation
 * [Hopfield-Diffusion Upgrade]
 * 
 * Combines Hopfield energy minimization with attention-based diffusion
 */

#include "hopfield_diffusion.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <random>
#include <algorithm>

namespace melvin {
namespace hopfield {

// ============================================================================
// IMPLEMENTATION CLASS
// ============================================================================

class HopfieldDiffusion::Impl {
public:
    Config config;
    Stats stats;
    std::mt19937 rng;
    
    Impl() : rng(std::random_device{}()) {}
    
    // Initialize embedding with small random values
    void init_embedding(std::vector<float>& embedding) {
        std::normal_distribution<float> dist(0.0f, 0.1f);
        embedding.resize(config.embedding_dim);
        for (auto& val : embedding) {
            val = dist(rng);
        }
    }
    
    // Normalize vector to unit length
    void normalize_vec(std::vector<float>& vec) {
        if (vec.empty()) return;
        
        float norm = 0.0f;
        for (float v : vec) {
            norm += v * v;
        }
        norm = std::sqrt(norm);
        
        if (norm > 1e-8f) {
            for (float& v : vec) {
                v /= norm;
            }
        }
    }
    
    // Cosine similarity between two vectors
    float cos_sim(const std::vector<float>& a, const std::vector<float>& b) const {
        if (a.empty() || b.empty() || a.size() != b.size()) return 0.0f;
        
        float dot = 0.0f;
        float norm_a = 0.0f;
        float norm_b = 0.0f;
        
        for (size_t i = 0; i < a.size(); ++i) {
            dot += a[i] * b[i];
            norm_a += a[i] * a[i];
            norm_b += b[i] * b[i];
        }
        
        float denom = std::sqrt(norm_a * norm_b);
        return (denom > 1e-8f) ? (dot / denom) : 0.0f;
    }
};

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

HopfieldDiffusion::HopfieldDiffusion(const Config& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
}

HopfieldDiffusion::~HopfieldDiffusion() = default;

void HopfieldDiffusion::initialize_embedding(Node& node) {
    // [Hopfield-Diffusion Upgrade]
    impl_->init_embedding(node.embedding);
    if (impl_->config.normalize_embeddings) {
        impl_->normalize_vec(node.embedding);
    }
}

void HopfieldDiffusion::normalize_embedding(Node& node) {
    // [Hopfield-Diffusion Upgrade]
    impl_->normalize_vec(node.embedding);
}

float HopfieldDiffusion::cosine_similarity(const Node& a, const Node& b) const {
    // [Hopfield-Diffusion Upgrade]
    return impl_->cos_sim(a.embedding, b.embedding);
}

float HopfieldDiffusion::compute_energy(const std::vector<Node>& nodes) {
    // [Hopfield-Diffusion Upgrade] - Hopfield energy function
    // E = -Σ β * cos_sim(i,j) * a[i] * a[j]
    
    float energy = 0.0f;
    float beta = impl_->config.beta;
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        for (size_t j = i + 1; j < nodes.size(); ++j) {
            if (nodes[i].embedding.empty() || nodes[j].embedding.empty()) continue;
            
            float sim = impl_->cos_sim(nodes[i].embedding, nodes[j].embedding);
            float interaction = beta * sim * nodes[i].activation * nodes[j].activation;
            energy -= interaction;  // Negative because we want to minimize
        }
    }
    
    return energy;
}

float HopfieldDiffusion::diffusion_step(
    std::vector<Node>& nodes,
    Storage* storage
) {
    // [Hopfield-Diffusion Upgrade] - Attention-based diffusion
    
    if (nodes.empty() || !storage) return 0.0f;
    
    float energy_before = compute_energy(nodes);
    
    // Temporary storage for new activations
    std::vector<float> new_activations(nodes.size(), 0.0f);
    
    // For each node, compute attention-weighted update
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].embedding.empty()) continue;
        
        float sum_attention = 0.0f;
        float weighted_sum = 0.0f;
        
        // Get edges from this node
        auto edges = storage->get_edges_from(nodes[i].id);
        
        for (const auto& edge : edges) {
            // Find target node in our active set
            auto it = std::find_if(nodes.begin(), nodes.end(),
                [&](const Node& n) { return n.id == edge.to_id; });
            
            if (it != nodes.end() && !it->embedding.empty()) {
                // Compute attention weight
                float sim = impl_->cos_sim(nodes[i].embedding, it->embedding);
                float attn = std::exp(impl_->config.beta * sim);
                
                sum_attention += attn;
                weighted_sum += attn * it->activation * edge.get_effective_weight();
            }
        }
        
        // Normalize and update
        if (sum_attention > 1e-8f) {
            new_activations[i] = weighted_sum / sum_attention;
        } else {
            new_activations[i] = nodes[i].activation * impl_->config.activation_leak;
        }
        
        // GUARD-RAIL 3: Clamp activations during diffusion
        new_activations[i] = std::max(0.0f, std::min(1.0f, new_activations[i]));
        
        // GUARD-RAIL 4: NaN/Inf check
        if (std::isnan(new_activations[i]) || std::isinf(new_activations[i])) {
            new_activations[i] = 0.0f;
        }
    }
    
    // Apply new activations with safety check
    for (size_t i = 0; i < nodes.size(); ++i) {
        nodes[i].activation = new_activations[i];
    }
    
    float energy_after = compute_energy(nodes);
    return energy_after - energy_before;  // Energy change
}

HopfieldDiffusion::DiffusionResult HopfieldDiffusion::run_diffusion(
    std::vector<Node>& nodes,
    Storage* storage
) {
    // [Hopfield-Diffusion Upgrade] - Full diffusion with convergence check
    
    DiffusionResult result;
    
    if (nodes.empty() || !storage) return result;
    
    // Ensure all nodes have embeddings
    for (auto& node : nodes) {
        if (node.embedding.empty()) {
            initialize_embedding(node);
        }
    }
    
    float prev_energy = compute_energy(nodes);
    result.energy_trajectory.push_back(prev_energy);
    
    if (impl_->config.verbose) {
        std::cout << "[Hopfield-Diffusion] Starting energy: " << prev_energy << std::endl;
    }
    
    for (int step = 0; step < impl_->config.max_diffusion_steps; ++step) {
        float delta_energy = diffusion_step(nodes, storage);
        float current_energy = prev_energy + delta_energy;
        
        result.energy_trajectory.push_back(current_energy);
        result.steps_taken++;
        
        if (impl_->config.verbose && step % 5 == 0) {
            // Print top activated nodes
            std::vector<std::pair<float, std::string>> top_nodes;
            for (const auto& n : nodes) {
                if (n.activation > 0.1f) {
                    top_nodes.push_back({n.activation, storage->get_node_content(n.id)});
                }
            }
            std::sort(top_nodes.begin(), top_nodes.end(), std::greater<>());
            
            std::cout << "[Diffusion t=" << step << "] Energy=" << std::fixed 
                      << std::setprecision(3) << current_energy;
            
            if (!top_nodes.empty()) {
                std::cout << "  Top nodes: ";
                for (size_t i = 0; i < std::min(size_t(3), top_nodes.size()); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << top_nodes[i].second.substr(0, 15) 
                              << "(" << top_nodes[i].first << ")";
                }
            }
            std::cout << std::endl;
        }
        
        // Check convergence
        if (std::abs(delta_energy) < impl_->config.energy_epsilon) {
            result.converged = true;
            if (impl_->config.verbose) {
                std::cout << "[Hopfield-Diffusion] Converged at step " << step 
                          << " (ΔE=" << delta_energy << ")" << std::endl;
            }
            break;
        }
        
        prev_energy = current_energy;
    }
    
    result.final_energy = prev_energy;
    
    // Update stats
    impl_->stats.diffusion_runs++;
    impl_->stats.total_steps += result.steps_taken;
    impl_->stats.avg_steps_per_run = static_cast<float>(impl_->stats.total_steps) / impl_->stats.diffusion_runs;
    
    if (result.converged) {
        impl_->stats.convergences++;
    }
    
    float n = static_cast<float>(impl_->stats.diffusion_runs);
    impl_->stats.avg_final_energy = (impl_->stats.avg_final_energy * (n - 1) + result.final_energy) / n;
    
    if (impl_->stats.diffusion_runs == 1 || result.final_energy < impl_->stats.min_energy_seen) {
        impl_->stats.min_energy_seen = result.final_energy;
    }
    if (impl_->stats.diffusion_runs == 1 || result.final_energy > impl_->stats.max_energy_seen) {
        impl_->stats.max_energy_seen = result.final_energy;
    }
    
    return result;
}

void HopfieldDiffusion::hebbian_update(Edge& edge, const Node& from, const Node& to) {
    // [Hopfield-Diffusion Upgrade] - Local Hebbian learning with guard-rails
    // ΔW = η_local * a[i] * a[j]
    
    // GUARD-RAIL 5: Check for valid activations
    if (std::isnan(from.activation) || std::isnan(to.activation)) return;
    if (std::isinf(from.activation) || std::isinf(to.activation)) return;
    
    float delta = impl_->config.eta_local * from.activation * to.activation;
    
    // GUARD-RAIL 6: Bound delta to prevent spikes
    delta = std::max(-0.1f, std::min(0.1f, delta));
    
    float new_weight = edge.get_weight() + delta;
    new_weight = std::max(0.0f, std::min(1.0f, new_weight));  // Clamp to [0,1]
    edge.set_weight(new_weight);
}

void HopfieldDiffusion::gradient_update(Edge& edge, const Node& from, const Node& to) {
    // [Hopfield-Diffusion Upgrade] - Global gradient (energy-based) with guard-rails
    // ΔW = η_global * β * cos_sim(i,j)
    
    if (from.embedding.empty() || to.embedding.empty()) return;
    
    float sim = impl_->cos_sim(from.embedding, to.embedding);
    
    // GUARD-RAIL 7: Check for valid similarity
    if (std::isnan(sim) || std::isinf(sim)) return;
    sim = std::max(-1.0f, std::min(1.0f, sim));  // Clamp cosine to valid range
    
    float delta = impl_->config.eta_global * impl_->config.beta * sim;
    
    // GUARD-RAIL 8: Bound gradient delta
    delta = std::max(-0.05f, std::min(0.05f, delta));
    
    float new_weight = edge.get_weight() + delta;
    new_weight = std::max(0.0f, std::min(1.0f, new_weight));
    edge.set_weight(new_weight);
}

void HopfieldDiffusion::update_weights(
    std::vector<Node>& nodes,
    std::vector<Edge>& edges,
    Storage* storage
) {
    // [Hopfield-Diffusion Upgrade] - Blended learning
    
    for (auto& edge : edges) {
        // Find the from and to nodes
        Node* from_node = nullptr;
        Node* to_node = nullptr;
        
        for (auto& n : nodes) {
            if (n.id == edge.from_id) from_node = &n;
            if (n.id == edge.to_id) to_node = &n;
        }
        
        if (from_node && to_node) {
            // Apply both Hebbian and gradient updates
            hebbian_update(edge, *from_node, *to_node);
            gradient_update(edge, *from_node, *to_node);
            
            // Optional: Decay old weights slightly for stability
            float current = edge.get_weight();
            edge.set_weight(current * 0.999f);  // Very mild decay
        }
    }
}

void HopfieldDiffusion::normalize_pass(std::vector<Node>& nodes) {
    // [Hopfield-Diffusion Upgrade] - Stability normalization with guard-rails
    
    for (auto& node : nodes) {
        // GUARD-RAIL 1: Activation bounds (prevent explosion)
        if (std::isnan(node.activation) || std::isinf(node.activation)) {
            node.activation = 0.0f;  // Reset to safe value
        } else {
            node.activation = std::max(0.0f, std::min(1.0f, node.activation));
        }
        
        // Apply activation leak
        node.activation *= impl_->config.activation_leak;
        
        // GUARD-RAIL 2: Embedding norm clamp (prevents explosion)
        if (!node.embedding.empty()) {
            float norm = 0.0f;
            for (float v : node.embedding) {
                norm += v * v;
            }
            norm = std::sqrt(norm);
            
            // Clamp to unit sphere (or slightly larger)
            if (norm > 1.05f || std::isnan(norm) || std::isinf(norm)) {
                // Renormalize to unit length
                impl_->normalize_vec(node.embedding);
            } else if (impl_->config.normalize_embeddings) {
                // Regular normalization
                impl_->normalize_vec(node.embedding);
            }
        }
    }
}

void HopfieldDiffusion::set_config(const Config& config) {
    impl_->config = config;
}

const HopfieldDiffusion::Config& HopfieldDiffusion::get_config() const {
    return impl_->config;
}

HopfieldDiffusion::Stats HopfieldDiffusion::get_stats() const {
    return impl_->stats;
}

void HopfieldDiffusion::reset_stats() {
    impl_->stats = Stats();
}

// ============================================================================
// STANDALONE HELPER FUNCTIONS
// ============================================================================

void initialize_all_embeddings(Storage* storage, int embedding_dim) {
    // [Hopfield-Diffusion Upgrade]
    if (!storage) return;
    
    HopfieldDiffusion::Config config;
    config.embedding_dim = embedding_dim;
    HopfieldDiffusion hd(config);
    
    auto& nodes = storage->get_nodes_mut();
    for (auto& node : nodes) {
        if (node.embedding.empty()) {
            hd.initialize_embedding(node);
        }
    }
}

void ensure_embeddings(std::vector<Node>& nodes, int embedding_dim) {
    // [Hopfield-Diffusion Upgrade]
    HopfieldDiffusion::Config config;
    config.embedding_dim = embedding_dim;
    HopfieldDiffusion hd(config);
    
    for (auto& node : nodes) {
        if (node.embedding.empty()) {
            hd.initialize_embedding(node);
        }
    }
}

} // namespace hopfield
} // namespace melvin

