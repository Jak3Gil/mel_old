#include "phoneme_cluster.h"
#include "phoneme_graph.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <sstream>

namespace melvin {
namespace audio {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

PhonemeClusterer::PhonemeClusterer()
    : PhonemeClusterer(Config()) {
}

PhonemeClusterer::PhonemeClusterer(const Config& config)
    : config_(config) {
    
    std::cout << "🧬 PhonemeClusterer initialized" << std::endl;
    std::cout << "   Target clusters: " << config_.num_clusters << std::endl;
    std::cout << "   Learning phoneme prototypes from audio..." << std::endl;
}

PhonemeClusterer::~PhonemeClusterer() {
}

// ============================================================================
// CLUSTERING
// ============================================================================

std::vector<uint64_t> PhonemeClusterer::cluster_tokens(
    const std::vector<AudioToken>& tokens,
    AtomicGraph& graph) {
    
    std::cout << "\n🧬 Clustering " << tokens.size() << " audio tokens..." << std::endl;
    
    std::vector<uint64_t> assignments;
    
    // Online clustering: assign each token to nearest cluster or create new
    for (const auto& token : tokens) {
        uint64_t cluster_id = assign_to_cluster(token);
        
        if (cluster_id == 0 && clusters_.size() < config_.num_clusters) {
            // Create new cluster
            cluster_id = create_cluster(token, graph);
        }
        
        if (cluster_id > 0) {
            // Update cluster with this token
            if (config_.enable_online_learning) {
                update_cluster(cluster_id, token);
            }
            
            assignments.push_back(cluster_id);
        }
    }
    
    std::cout << "   ✅ Formed " << clusters_.size() << " phoneme clusters" << std::endl;
    
    return assignments;
}

uint64_t PhonemeClusterer::assign_to_cluster(const AudioToken& token) {
    if (clusters_.empty()) {
        return 0;
    }
    
    float best_distance = std::numeric_limits<float>::max();
    uint64_t best_cluster = 0;
    
    // Find nearest cluster
    for (const auto& cluster : clusters_) {
        float dist = compute_distance(token.features, cluster.centroid);
        
        // Convert distance to similarity (inverse)
        float similarity = 1.0f / (1.0f + dist);
        
        if (similarity >= config_.similarity_threshold && dist < best_distance) {
            best_distance = dist;
            best_cluster = cluster.cluster_id;
        }
    }
    
    return best_cluster;
}

uint64_t PhonemeClusterer::create_cluster(const AudioToken& token, AtomicGraph& graph) {
    PhonemeCluster cluster;
    cluster.cluster_id = next_cluster_id_++;
    cluster.symbol = generate_cluster_symbol(clusters_.size());
    cluster.centroid = token.features;
    cluster.variance = std::vector<float>(token.features.size(), 0.1f);
    cluster.members.push_back(token.id);
    cluster.occurrence_count = 1;
    
    // Create graph node for this phoneme cluster
    uint64_t node_id = graph.get_or_create_node("phoneme_cluster:" + cluster.symbol, 23);
    cluster.cluster_id = node_id;
    
    clusters_.push_back(cluster);
    
    std::cout << "   📚 New cluster: " << cluster.symbol << " (node " << node_id << ")" << std::endl;
    
    return node_id;
}

void PhonemeClusterer::update_cluster(uint64_t cluster_id, const AudioToken& token) {
    // Find cluster
    for (auto& cluster : clusters_) {
        if (cluster.cluster_id == cluster_id) {
            // Add member
            cluster.members.push_back(token.id);
            cluster.occurrence_count++;
            
            // Update centroid (running average)
            for (size_t i = 0; i < cluster.centroid.size() && i < token.features.size(); i++) {
                cluster.centroid[i] = (cluster.centroid[i] * (cluster.occurrence_count - 1) +
                                      token.features[i]) / cluster.occurrence_count;
            }
            
            break;
        }
    }
}

// ============================================================================
// ANALYSIS
// ============================================================================

std::vector<uint64_t> PhonemeClusterer::get_most_common(size_t top_n) {
    // Sort clusters by occurrence count
    std::vector<PhonemeCluster> sorted = clusters_;
    std::sort(sorted.begin(), sorted.end(),
             [](const PhonemeCluster& a, const PhonemeCluster& b) {
                 return a.occurrence_count > b.occurrence_count;
             });
    
    std::vector<uint64_t> result;
    for (size_t i = 0; i < std::min(top_n, sorted.size()); i++) {
        result.push_back(sorted[i].cluster_id);
    }
    
    return result;
}

float PhonemeClusterer::compute_cluster_quality(uint64_t cluster_id) {
    const PhonemeCluster* cluster = get_cluster(cluster_id);
    
    if (!cluster || cluster->members.empty()) {
        return 0.0f;
    }
    
    // Quality = inverse of variance (tight cluster = high quality)
    float avg_variance = 0.0f;
    for (float v : cluster->variance) {
        avg_variance += v;
    }
    avg_variance /= cluster->variance.size();
    
    return 1.0f / (1.0f + avg_variance);
}

// ========================================================================
// GRAPH INTEGRATION
// ========================================================================

void PhonemeClusterer::link_to_graph(AtomicGraph& graph) {
    std::cout << "\n🔗 Linking audio tokens to phoneme clusters..." << std::endl;
    
    size_t links_created = 0;
    
    for (const auto& cluster : clusters_) {
        for (uint64_t member_id : cluster.members) {
            // Link token → cluster [INSTANCE_OF]
            graph.add_edge(member_id, cluster.cluster_id, Relation::INSTANCE_OF, 1.0f);
            links_created++;
        }
    }
    
    std::cout << "   ✅ Created " << links_created << " token→phoneme links" << std::endl;
}

void PhonemeClusterer::export_to_phonemes(PhonemeGraph& phoneme_graph) {
    std::cout << "\n📤 Exporting clusters to PhonemeGraph..." << std::endl;
    
    for (const auto& cluster : clusters_) {
        // Create phoneme node
        PhonemeNode phoneme;
        phoneme.symbol = cluster.symbol;
        phoneme.id = cluster.cluster_id;
        
        // Estimate formants from spectral features (simplified)
        if (cluster.centroid.size() >= 3) {
            phoneme.formants[0] = 500.0f + cluster.centroid[0] * 100.0f;
            phoneme.formants[1] = 1500.0f + cluster.centroid[1] * 100.0f;
            phoneme.formants[2] = 2500.0f + cluster.centroid[2] * 100.0f;
        }
        
        phoneme.duration_ms = 100.0f;  // Average duration
        
        phoneme_graph.add_phoneme(cluster.symbol, phoneme);
    }
    
    std::cout << "   ✅ Exported " << clusters_.size() << " learned phonemes" << std::endl;
}

// ============================================================================
// STATISTICS
// ============================================================================

const PhonemeCluster* PhonemeClusterer::get_cluster(uint64_t cluster_id) const {
    for (const auto& cluster : clusters_) {
        if (cluster.cluster_id == cluster_id) {
            return &cluster;
        }
    }
    return nullptr;
}

size_t PhonemeClusterer::total_tokens_clustered() const {
    size_t total = 0;
    for (const auto& cluster : clusters_) {
        total += cluster.members.size();
    }
    return total;
}

void PhonemeClusterer::print_stats() const {
    std::cout << "\n📊 PhonemeClusterer Statistics:" << std::endl;
    std::cout << "   Total clusters: " << clusters_.size() << std::endl;
    std::cout << "   Tokens clustered: " << total_tokens_clustered() << std::endl;
    std::cout << "   Avg tokens/cluster: " 
              << (clusters_.empty() ? 0 : total_tokens_clustered() / clusters_.size()) 
              << std::endl;
}

void PhonemeClusterer::print_cluster_details() const {
    std::cout << "\n📚 Phoneme Clusters:" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    for (const auto& cluster : clusters_) {
        std::cout << "   " << cluster.symbol << ": "
                  << cluster.occurrence_count << " occurrences, "
                  << cluster.members.size() << " members" << std::endl;
    }
}

// ============================================================================
// HELPER METHODS
// ============================================================================

float PhonemeClusterer::compute_distance(const std::vector<float>& f1, 
                                        const std::vector<float>& f2) {
    if (f1.empty() || f2.empty()) {
        return std::numeric_limits<float>::max();
    }
    
    // Euclidean distance
    float sum = 0.0f;
    size_t dim = std::min(f1.size(), f2.size());
    
    for (size_t i = 0; i < dim; i++) {
        float diff = f1[i] - f2[i];
        sum += diff * diff;
    }
    
    return std::sqrt(sum);
}

std::string PhonemeClusterer::generate_cluster_symbol(size_t cluster_index) {
    // Generate symbol like "ph0", "ph1", etc.
    std::ostringstream oss;
    oss << "ph" << cluster_index;
    return oss.str();
}

} // namespace audio
} // namespace melvin

