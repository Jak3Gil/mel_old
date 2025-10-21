#pragma once

#include "types.h"
#include "storage.h"
#include "hopfield_diffusion.h"
#include <vector>
#include <string>
#include <ctime>

namespace melvin {
namespace hopfield {

/**
 * 🌌 LEAP Pattern Synthesis System
 * 
 * Creates new conceptual nodes when overlapping attractors are detected
 * during Hopfield-Diffusion reasoning. These "LEAP nodes" represent emergent
 * patterns that bridge distant concepts.
 * 
 * [LEAP Synthesis Extension]
 */

// ============================================================================
// LEAP LINK STRUCT
// ============================================================================

struct LeapLink {
    NodeID source_a;              // First source node
    NodeID source_b;              // Second source node
    NodeID leap_node;             // Synthesized LEAP node
    float strength;               // Link strength (0.0-1.0)
    float novelty;                // How novel this pattern is
    float energy_improvement;     // ΔE when created (positive = more stable)
    uint64_t timestamp;           // When created
    std::vector<float> blended_embedding;  // Stored embedding
    
    LeapLink() 
        : source_a(0), source_b(0), leap_node(0), 
          strength(0.0f), novelty(0.0f), energy_improvement(0.0f),
          timestamp(0) {}
};

// ============================================================================
// ATTRACTOR CLUSTER
// ============================================================================

struct AttractorCluster {
    std::vector<NodeID> active_nodes;     // Nodes in this cluster
    NodeID representative;                 // Most active node
    float avg_activation;                  // Average activation
    std::vector<float> centroid_embedding; // Average embedding
    
    AttractorCluster() : representative(0), avg_activation(0.0f) {}
};

// ============================================================================
// LEAP SYNTHESIS CONFIG
// ============================================================================

struct LeapSynthesisConfig {
    // Cluster detection
    float activation_threshold = 0.4f;     // Min activation to be in cluster
    float overlap_threshold = 0.5f;        // Min overlap to trigger synthesis
    int min_cluster_size = 3;              // Min nodes per cluster
    
    // Synthesis parameters
    float noise_scale = 0.05f;             // Random noise added to blend
    float link_weight_min = 0.3f;          // Min connection strength
    float link_weight_max = 0.6f;          // Max connection strength
    
    // Triggering conditions
    float curiosity_threshold = 0.6f;      // Min curiosity to attempt
    float stability_threshold = 0.01f;     // Max ΔE per step (stable system)
    bool require_energy_improvement = true; // Only keep if ΔE > 0
    
    // Limits
    int max_leaps_per_cycle = 3;           // Max LEAPs created per diffusion
    int max_total_leaps = 100;             // Max LEAPs stored total
    
    bool verbose = false;                  // Print LEAP creation logs
    
    LeapSynthesisConfig() {}
};

// ============================================================================
// LEAP SYNTHESIS ENGINE
// ============================================================================

class LeapSynthesis {
public:
    explicit LeapSynthesis(const LeapSynthesisConfig& config = LeapSynthesisConfig());
    ~LeapSynthesis();
    
    // ========================================================================
    // CLUSTER DETECTION
    // ========================================================================
    
    // Detect clusters of active nodes
    std::vector<AttractorCluster> detect_attractors(
        const std::vector<Node>& nodes
    );
    
    // Check if two clusters overlap significantly
    bool clusters_overlap(
        const AttractorCluster& a,
        const AttractorCluster& b,
        float threshold
    );
    
    // Compute overlap ratio between two clusters
    float compute_overlap(
        const AttractorCluster& a,
        const AttractorCluster& b
    );
    
    // ========================================================================
    // PATTERN SYNTHESIS
    // ========================================================================
    
    // Synthesize a new LEAP node from two overlapping clusters
    // Returns: node ID of created LEAP, or 0 if failed
    NodeID synthesize_leap_node(
        const AttractorCluster& cluster_a,
        const AttractorCluster& cluster_b,
        Storage* storage,
        HopfieldDiffusion* hopfield
    );
    
    // Blend embeddings from two nodes
    std::vector<float> blend_embeddings(
        const Node& node_a,
        const Node& node_b,
        float noise_scale
    );
    
    // Generate name for LEAP node
    std::string generate_leap_name(
        const Node& node_a,
        const Node& node_b,
        Storage* storage
    );
    
    // ========================================================================
    // ENERGY VALIDATION
    // ========================================================================
    
    // Compute energy change from adding a LEAP node
    float compute_energy_change(
        const std::vector<Node>& nodes_before,
        const std::vector<Node>& nodes_after,
        HopfieldDiffusion* hopfield
    );
    
    // Validate that LEAP improves stability
    bool validate_leap(
        NodeID leap_node,
        const std::vector<Node>& nodes,
        HopfieldDiffusion* hopfield
    );
    
    // ========================================================================
    // GRAPH LINKING
    // ========================================================================
    
    // Create bidirectional links between source nodes and LEAP node
    void create_leap_links(
        NodeID source_a,
        NodeID source_b,
        NodeID leap_node,
        Storage* storage
    );
    
    // Store LEAP link record
    void record_leap_link(
        NodeID source_a,
        NodeID source_b,
        NodeID leap_node,
        float strength,
        float novelty,
        float energy_delta
    );
    
    // ========================================================================
    // MAIN SYNTHESIS ENTRY POINT
    // ========================================================================
    
    // Detect overlaps and create LEAP nodes
    // Returns: number of LEAPs created
    int attempt_leap_synthesis(
        std::vector<Node>& nodes,
        Storage* storage,
        HopfieldDiffusion* hopfield,
        float curiosity,
        float energy_change_per_step
    );
    
    // ========================================================================
    // PERSISTENCE
    // ========================================================================
    
    // Save LEAP links to file
    bool save_leap_links(const std::string& path);
    
    // Load LEAP links from file
    bool load_leap_links(const std::string& path);
    
    // ========================================================================
    // CONFIGURATION & STATS
    // ========================================================================
    
    void set_config(const LeapSynthesisConfig& config);
    const LeapSynthesisConfig& get_config() const;
    
    // Get all LEAP links
    const std::vector<LeapLink>& get_leap_links() const;
    
    // Get latest LEAP
    const LeapLink* get_latest_leap() const;
    
    // Clear all LEAPs
    void clear_leaps();
    
    struct Stats {
        uint64_t attempts = 0;
        uint64_t successful = 0;
        uint64_t rejected_energy = 0;
        uint64_t rejected_curiosity = 0;
        uint64_t rejected_stability = 0;
        float avg_energy_improvement = 0.0f;
        float avg_novelty = 0.0f;
    };
    
    Stats get_stats() const;
    void reset_stats();
    
    // Print summary
    void print_summary() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace hopfield
} // namespace melvin

