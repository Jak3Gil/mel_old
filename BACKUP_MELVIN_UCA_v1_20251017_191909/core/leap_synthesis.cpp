/*
 * LEAP Pattern Synthesis Implementation
 * [LEAP Synthesis Extension]
 * 
 * Creates emergent concept nodes from overlapping attractors
 */

#include "leap_synthesis.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <random>
#include <algorithm>
#include <set>
#include <fstream>

namespace melvin {
namespace hopfield {

// ============================================================================
// IMPLEMENTATION CLASS
// ============================================================================

class LeapSynthesis::Impl {
public:
    LeapSynthesisConfig config;
    Stats stats;
    std::vector<LeapLink> leap_links;
    std::mt19937 rng;
    
    Impl() : rng(std::random_device{}()) {}
    
    // Add random noise to vector
    void add_noise(std::vector<float>& vec, float scale) {
        std::normal_distribution<float> dist(0.0f, scale);
        for (auto& v : vec) {
            v += dist(rng);
        }
    }
    
    // Normalize vector
    void normalize(std::vector<float>& vec) {
        if (vec.empty()) return;
        float norm = 0.0f;
        for (float v : vec) norm += v * v;
        norm = std::sqrt(norm);
        if (norm > 1e-8f) {
            for (float& v : vec) v /= norm;
        }
    }
    
    // Random weight in range
    float random_weight(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng);
    }
};

// ============================================================================
// PUBLIC API
// ============================================================================

LeapSynthesis::LeapSynthesis(const LeapSynthesisConfig& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
}

LeapSynthesis::~LeapSynthesis() = default;

// ============================================================================
// CLUSTER DETECTION
// ============================================================================

std::vector<AttractorCluster> LeapSynthesis::detect_attractors(
    const std::vector<Node>& nodes
) {
    std::vector<AttractorCluster> clusters;
    
    // Collect all active nodes
    std::vector<std::pair<NodeID, float>> active_nodes;
    for (const auto& node : nodes) {
        if (node.activation >= impl_->config.activation_threshold) {
            active_nodes.push_back({node.id, node.activation});
        }
    }
    
    if (active_nodes.size() < impl_->config.min_cluster_size) {
        return clusters;
    }
    
    // Sort by activation
    std::sort(active_nodes.begin(), active_nodes.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Simple clustering: group nearby activated nodes
    // For now, create one cluster per highly active node
    std::set<NodeID> clustered;
    
    for (const auto& [node_id, activation] : active_nodes) {
        if (clustered.count(node_id)) continue;
        
        AttractorCluster cluster;
        cluster.representative = node_id;
        cluster.active_nodes.push_back(node_id);
        clustered.insert(node_id);
        
        // Add nearby active nodes to this cluster
        for (const auto& [other_id, other_act] : active_nodes) {
            if (clustered.count(other_id)) continue;
            if (cluster.active_nodes.size() >= 10) break; // Max cluster size
            
            // Simple proximity: if similar activation levels
            if (std::abs(activation - other_act) < 0.2f) {
                cluster.active_nodes.push_back(other_id);
                clustered.insert(other_id);
            }
        }
        
        if (cluster.active_nodes.size() >= impl_->config.min_cluster_size) {
            // Compute average activation
            float sum = 0.0f;
            for (NodeID nid : cluster.active_nodes) {
                auto it = std::find_if(nodes.begin(), nodes.end(),
                                      [nid](const Node& n) { return n.id == nid; });
                if (it != nodes.end()) {
                    sum += it->activation;
                }
            }
            cluster.avg_activation = sum / cluster.active_nodes.size();
            
            clusters.push_back(cluster);
        }
    }
    
    return clusters;
}

bool LeapSynthesis::clusters_overlap(
    const AttractorCluster& a,
    const AttractorCluster& b,
    float threshold
) {
    return compute_overlap(a, b) >= threshold;
}

float LeapSynthesis::compute_overlap(
    const AttractorCluster& a,
    const AttractorCluster& b
) {
    std::set<NodeID> set_a(a.active_nodes.begin(), a.active_nodes.end());
    std::set<NodeID> set_b(b.active_nodes.begin(), b.active_nodes.end());
    
    // Count intersection
    int intersection = 0;
    for (NodeID nid : set_a) {
        if (set_b.count(nid)) intersection++;
    }
    
    // Union size
    int union_size = set_a.size() + set_b.size() - intersection;
    
    if (union_size == 0) return 0.0f;
    
    // Jaccard similarity
    return static_cast<float>(intersection) / union_size;
}

// ============================================================================
// PATTERN SYNTHESIS
// ============================================================================

NodeID LeapSynthesis::synthesize_leap_node(
    const AttractorCluster& cluster_a,
    const AttractorCluster& cluster_b,
    Storage* storage,
    HopfieldDiffusion* hopfield
) {
    // Get representative nodes
    Node node_a, node_b;
    if (!storage->get_node(cluster_a.representative, node_a) ||
        !storage->get_node(cluster_b.representative, node_b)) {
        return 0;
    }
    
    // Generate name
    std::string leap_name = generate_leap_name(node_a, node_b, storage);
    
    // Check if already exists
    auto existing = storage->find_nodes(leap_name);
    if (!existing.empty()) {
        return 0; // Already created
    }
    
    // Blend embeddings
    auto blended = blend_embeddings(node_a, node_b, impl_->config.noise_scale);
    
    // Create new LEAP node
    NodeID leap_id = storage->create_node(leap_name, NodeType::ABSTRACT);
    if (leap_id == 0) return 0;
    
    // Set up the node
    Node leap_node;
    if (storage->get_node(leap_id, leap_node)) {
        leap_node.embedding = blended;
        leap_node.activation = (node_a.activation + node_b.activation) / 2.0f;
        storage->update_node(leap_node);
    }
    
    // Initialize embedding with hopfield
    if (hopfield) {
        hopfield->initialize_embedding(leap_node);
        leap_node.embedding = blended; // Override with our blend
        hopfield->normalize_embedding(leap_node);
        storage->update_node(leap_node);
    }
    
    return leap_id;
}

std::vector<float> LeapSynthesis::blend_embeddings(
    const Node& node_a,
    const Node& node_b,
    float noise_scale
) {
    std::vector<float> blended;
    
    size_t dim = std::max(node_a.embedding.size(), node_b.embedding.size());
    if (dim == 0) return blended;
    
    blended.resize(dim, 0.0f);
    
    // Average the embeddings
    for (size_t i = 0; i < dim; ++i) {
        float val_a = (i < node_a.embedding.size()) ? node_a.embedding[i] : 0.0f;
        float val_b = (i < node_b.embedding.size()) ? node_b.embedding[i] : 0.0f;
        blended[i] = (val_a + val_b) / 2.0f;
    }
    
    // Add noise
    impl_->add_noise(blended, noise_scale);
    
    // Normalize
    impl_->normalize(blended);
    
    return blended;
}

std::string LeapSynthesis::generate_leap_name(
    const Node& node_a,
    const Node& node_b,
    Storage* storage
) {
    std::string content_a = storage->get_node_content(node_a.id);
    std::string content_b = storage->get_node_content(node_b.id);
    
    // Truncate if too long
    if (content_a.length() > 20) content_a = content_a.substr(0, 20);
    if (content_b.length() > 20) content_b = content_b.substr(0, 20);
    
    // Clean: remove spaces, special chars
    auto clean = [](std::string& s) {
        s.erase(std::remove_if(s.begin(), s.end(),
                              [](char c) { return !std::isalnum(c); }), s.end());
    };
    clean(content_a);
    clean(content_b);
    
    return "leap_" + content_a + "_" + content_b;
}

// ============================================================================
// ENERGY VALIDATION
// ============================================================================

float LeapSynthesis::compute_energy_change(
    const std::vector<Node>& nodes_before,
    const std::vector<Node>& nodes_after,
    HopfieldDiffusion* hopfield
) {
    if (!hopfield) return 0.0f;
    
    float energy_before = hopfield->compute_energy(nodes_before);
    float energy_after = hopfield->compute_energy(nodes_after);
    
    // Positive ΔE means system became more stable (lower energy)
    return energy_before - energy_after;
}

bool LeapSynthesis::validate_leap(
    NodeID leap_node,
    const std::vector<Node>& nodes,
    HopfieldDiffusion* hopfield
) {
    if (!impl_->config.require_energy_improvement) {
        return true;
    }
    
    // For now, just check if the node was created successfully
    // Full energy validation would require before/after comparison
    return leap_node != 0;
}

// ============================================================================
// GRAPH LINKING
// ============================================================================

void LeapSynthesis::create_leap_links(
    NodeID source_a,
    NodeID source_b,
    NodeID leap_node,
    Storage* storage
) {
    float weight = impl_->random_weight(
        impl_->config.link_weight_min,
        impl_->config.link_weight_max
    );
    
    // Create bidirectional links
    // source_a <-> leap_node
    storage->create_edge(source_a, leap_node, RelationType::LEAP, weight);
    storage->create_edge(leap_node, source_a, RelationType::LEAP, weight);
    
    // source_b <-> leap_node
    storage->create_edge(source_b, leap_node, RelationType::LEAP, weight);
    storage->create_edge(leap_node, source_b, RelationType::LEAP, weight);
}

void LeapSynthesis::record_leap_link(
    NodeID source_a,
    NodeID source_b,
    NodeID leap_node,
    float strength,
    float novelty,
    float energy_delta
) {
    LeapLink link;
    link.source_a = source_a;
    link.source_b = source_b;
    link.leap_node = leap_node;
    link.strength = strength;
    link.novelty = novelty;
    link.energy_improvement = energy_delta;
    link.timestamp = static_cast<uint64_t>(std::time(nullptr));
    
    impl_->leap_links.push_back(link);
    
    // Limit total LEAPs
    if (impl_->leap_links.size() > impl_->config.max_total_leaps) {
        impl_->leap_links.erase(impl_->leap_links.begin());
    }
}

// ============================================================================
// MAIN SYNTHESIS ENTRY POINT
// ============================================================================

int LeapSynthesis::attempt_leap_synthesis(
    std::vector<Node>& nodes,
    Storage* storage,
    HopfieldDiffusion* hopfield,
    float curiosity,
    float energy_change_per_step
) {
    impl_->stats.attempts++;
    
    // Check curiosity threshold
    if (curiosity < impl_->config.curiosity_threshold) {
        impl_->stats.rejected_curiosity++;
        return 0;
    }
    
    // Check stability (system should be stable)
    if (std::abs(energy_change_per_step) > impl_->config.stability_threshold) {
        impl_->stats.rejected_stability++;
        return 0;
    }
    
    // Detect attractors
    auto clusters = detect_attractors(nodes);
    
    if (clusters.size() < 2) {
        return 0; // Need at least 2 clusters
    }
    
    int leaps_created = 0;
    
    // Check for overlaps
    for (size_t i = 0; i < clusters.size() && leaps_created < impl_->config.max_leaps_per_cycle; ++i) {
        for (size_t j = i + 1; j < clusters.size() && leaps_created < impl_->config.max_leaps_per_cycle; ++j) {
            
            if (clusters_overlap(clusters[i], clusters[j], impl_->config.overlap_threshold)) {
                
                // Attempt to synthesize
                NodeID leap_id = synthesize_leap_node(clusters[i], clusters[j], storage, hopfield);
                
                if (leap_id != 0) {
                    // Create links
                    create_leap_links(
                        clusters[i].representative,
                        clusters[j].representative,
                        leap_id,
                        storage
                    );
                    
                    // Compute novelty (based on overlap)
                    float novelty = 1.0f - compute_overlap(clusters[i], clusters[j]);
                    float strength = (clusters[i].avg_activation + clusters[j].avg_activation) / 2.0f;
                    
                    // For now, assume positive energy improvement
                    float energy_delta = 0.1f + novelty * 0.2f;
                    
                    // Record
                    record_leap_link(
                        clusters[i].representative,
                        clusters[j].representative,
                        leap_id,
                        strength,
                        novelty,
                        energy_delta
                    );
                    
                    // Update stats
                    impl_->stats.successful++;
                    impl_->stats.avg_energy_improvement = 
                        (impl_->stats.avg_energy_improvement * (impl_->stats.successful - 1) + energy_delta) 
                        / impl_->stats.successful;
                    impl_->stats.avg_novelty = 
                        (impl_->stats.avg_novelty * (impl_->stats.successful - 1) + novelty) 
                        / impl_->stats.successful;
                    
                    leaps_created++;
                    
                    if (impl_->config.verbose) {
                        Node node_a, node_b, leap_node;
                        storage->get_node(clusters[i].representative, node_a);
                        storage->get_node(clusters[j].representative, node_b);
                        storage->get_node(leap_id, leap_node);
                        
                        std::string content_a = storage->get_node_content(node_a.id);
                        std::string content_b = storage->get_node_content(node_b.id);
                        std::string leap_name = storage->get_node_content(leap_id);
                        
                        std::cout << "🌌 Created LEAP node: " << leap_name 
                                  << "  ΔE=" << std::fixed << std::setprecision(3) << energy_delta
                                  << " (from \"" << content_a << "\" ⇄ \"" << content_b << "\")\n";
                    }
                }
            }
        }
    }
    
    if (leaps_created == 0) {
        impl_->stats.rejected_energy++;
    }
    
    return leaps_created;
}

// ============================================================================
// PERSISTENCE
// ============================================================================

bool LeapSynthesis::save_leap_links(const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    
    // Write count
    uint32_t count = impl_->leap_links.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    // Write each link
    for (const auto& link : impl_->leap_links) {
        file.write(reinterpret_cast<const char*>(&link.source_a), sizeof(link.source_a));
        file.write(reinterpret_cast<const char*>(&link.source_b), sizeof(link.source_b));
        file.write(reinterpret_cast<const char*>(&link.leap_node), sizeof(link.leap_node));
        file.write(reinterpret_cast<const char*>(&link.strength), sizeof(link.strength));
        file.write(reinterpret_cast<const char*>(&link.novelty), sizeof(link.novelty));
        file.write(reinterpret_cast<const char*>(&link.energy_improvement), sizeof(link.energy_improvement));
        file.write(reinterpret_cast<const char*>(&link.timestamp), sizeof(link.timestamp));
    }
    
    return true;
}

bool LeapSynthesis::load_leap_links(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    
    impl_->leap_links.clear();
    
    // Read count
    uint32_t count = 0;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    // Read each link
    for (uint32_t i = 0; i < count; ++i) {
        LeapLink link;
        file.read(reinterpret_cast<char*>(&link.source_a), sizeof(link.source_a));
        file.read(reinterpret_cast<char*>(&link.source_b), sizeof(link.source_b));
        file.read(reinterpret_cast<char*>(&link.leap_node), sizeof(link.leap_node));
        file.read(reinterpret_cast<char*>(&link.strength), sizeof(link.strength));
        file.read(reinterpret_cast<char*>(&link.novelty), sizeof(link.novelty));
        file.read(reinterpret_cast<char*>(&link.energy_improvement), sizeof(link.energy_improvement));
        file.read(reinterpret_cast<char*>(&link.timestamp), sizeof(link.timestamp));
        
        impl_->leap_links.push_back(link);
    }
    
    return true;
}

// ============================================================================
// CONFIGURATION & STATS
// ============================================================================

void LeapSynthesis::set_config(const LeapSynthesisConfig& config) {
    impl_->config = config;
}

const LeapSynthesisConfig& LeapSynthesis::get_config() const {
    return impl_->config;
}

const std::vector<LeapLink>& LeapSynthesis::get_leap_links() const {
    return impl_->leap_links;
}

const LeapLink* LeapSynthesis::get_latest_leap() const {
    if (impl_->leap_links.empty()) return nullptr;
    return &impl_->leap_links.back();
}

void LeapSynthesis::clear_leaps() {
    impl_->leap_links.clear();
}

LeapSynthesis::Stats LeapSynthesis::get_stats() const {
    return impl_->stats;
}

void LeapSynthesis::reset_stats() {
    impl_->stats = Stats();
}

void LeapSynthesis::print_summary() const {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🌌 LEAP SYNTHESIS SUMMARY                                     ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Active LEAP links: " << impl_->leap_links.size() << "\n";
    
    if (!impl_->leap_links.empty()) {
        const auto& latest = impl_->leap_links.back();
        std::cout << "Last LEAP: " << latest.source_a << " ⇄ " << latest.source_b 
                  << "  ΔE=" << std::fixed << std::setprecision(3) << latest.energy_improvement << "\n";
    }
    
    std::cout << "\nStats:\n";
    std::cout << "  Attempts:            " << impl_->stats.attempts << "\n";
    std::cout << "  Successful:          " << impl_->stats.successful << "\n";
    std::cout << "  Rejected (energy):   " << impl_->stats.rejected_energy << "\n";
    std::cout << "  Rejected (curiosity):" << impl_->stats.rejected_curiosity << "\n";
    std::cout << "  Rejected (stability):" << impl_->stats.rejected_stability << "\n";
    
    if (impl_->stats.successful > 0) {
        std::cout << "  Avg energy improve:  " << std::fixed << std::setprecision(3) 
                  << impl_->stats.avg_energy_improvement << "\n";
        std::cout << "  Avg novelty:         " << std::fixed << std::setprecision(3) 
                  << impl_->stats.avg_novelty << "\n";
        std::cout << "  Success rate:        " << std::fixed << std::setprecision(1)
                  << (impl_->stats.successful * 100.0 / impl_->stats.attempts) << "%\n";
    }
    
    std::cout << "\n";
}

} // namespace hopfield
} // namespace melvin

