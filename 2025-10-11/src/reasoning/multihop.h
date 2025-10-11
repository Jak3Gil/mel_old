#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

// Forward declarations
struct Node;
struct Edge;

namespace melvin {
namespace reasoning {

// Relation type enum (for priors)
enum class RelationType {
    ISA,        // Category (ARE, IS_A)
    PRODUCES,   // Creation/output  
    HAS,        // Property/possession
    REQUIRES,   // Dependency
    CAUSES,     // Causation
    PART_OF,    // Composition
    ASSOC,      // Generic association
    ENABLES,    // Capability
    BREATHE_WITH, // Method
    LIVE_IN,    // Location
    BUILD,      // Creation
    CARE_FOR,   // Relationship
    UNKNOWN
};

// Relation priors for different query types
struct RelationPriors {
    std::unordered_map<std::string, float> weights;
    
    // Get default priors for common query types
    static RelationPriors for_query_type(const std::string& question);
    
    // Get prior weight for a relation
    float get_prior(const std::string& rel) const;
};

// Path candidate for beam search
struct PathCandidate {
    std::vector<uint64_t> nodes;
    std::vector<std::string> relations;
    float score = 0.0f;
    int hop_count = 0;
    
    bool operator<(const PathCandidate& other) const {
        return score < other.score;  // For priority queue
    }
};

// Multi-hop reasoning options
struct MultiHopOptions {
    int max_hops = 3;
    int beam_width = 5;
    float min_edge_weight = 0.1f;
    bool use_relation_priors = true;
    bool use_rules = false;  // Virtual rule edges (future)
    bool verbose = false;
};

// Multi-hop result
struct MultiHopResult {
    std::string answer;
    std::vector<uint64_t> path;
    std::vector<std::string> relations_used;
    float confidence = 0.0f;
    int hop_count = 0;
    bool success = false;
};

// Multi-hop reasoning engine
class MultiHopReasoner {
public:
    // Answer query with k-hop search
    static MultiHopResult answer_query(
        const std::string& question,
        std::unordered_map<uint64_t, ::Node>& nodes,
        std::vector<::Edge>& edges,
        const MultiHopOptions& opts = MultiHopOptions()
    );
    
private:
    // Beam search over graph
    static std::vector<PathCandidate> beam_search(
        const std::vector<uint64_t>& start_nodes,
        std::unordered_map<uint64_t, ::Node>& nodes,
        std::vector<::Edge>& edges,
        const MultiHopOptions& opts,
        const RelationPriors& priors
    );
    
    // Score a path
    static float score_path(
        const PathCandidate& path,
        const std::vector<::Edge>& edges,
        const RelationPriors& priors,
        const MultiHopOptions& opts
    );
    
    // Extract query keywords → node IDs
    static std::vector<uint64_t> extract_query_nodes(
        const std::string& question,
        std::unordered_map<uint64_t, ::Node>& nodes
    );
    
    // Determine relation type from string
    static RelationType classify_relation(const std::string& rel);
    
    // Get edge between two nodes
    static const ::Edge* find_edge(
        uint64_t from,
        uint64_t to,
        const std::vector<::Edge>& edges
    );
    
    // Normalize text for matching
    static std::string normalize_text(const std::string& text);
};

} // namespace reasoning
} // namespace melvin

