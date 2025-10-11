#include "multihop.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <queue>

// Full Node and Edge definitions
struct Node {
    uint64_t id = 0;
    std::string text;
    uint32_t type = 0;
    int freq = 0;
    bool pinned = false;
    std::vector<float> emb;
    std::vector<float> embedding;
    float attention_weight = 0.0f;
    uint64_t last_accessed = 0;
    float semantic_strength = 1.0f;
    float activation = 0.0f;
};

struct Edge {
    uint64_t u, v;
    uint64_t locB;
    float weight;
    float w_core;
    float w_ctx;
    uint32_t count;
    
    void update_frequency(uint64_t total_nodes) { (void)total_nodes; }
};

namespace melvin {
namespace reasoning {

// Normalize text for matching
std::string MultiHopReasoner::normalize_text(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // Trim
    size_t start = result.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = result.find_last_not_of(" \t\r\n");
    result = result.substr(start, end - start + 1);
    
    // Remove punctuation
    std::string cleaned;
    for (char c : result) {
        if (std::isalnum(c) || std::isspace(c)) {
            cleaned += c;
        }
    }
    
    return cleaned;
}

// Classify relation type
RelationType MultiHopReasoner::classify_relation(const std::string& rel) {
    std::string norm = normalize_text(rel);
    
    if (norm == "are" || norm == "is a" || norm == "isa") return RelationType::ISA;
    if (norm == "produces" || norm == "produce") return RelationType::PRODUCES;
    if (norm == "has" || norm == "have") return RelationType::HAS;
    if (norm == "requires" || norm == "require") return RelationType::REQUIRES;
    if (norm == "causes" || norm == "cause") return RelationType::CAUSES;
    if (norm == "part of" || norm == "partof") return RelationType::PART_OF;
    if (norm == "enables" || norm == "enable") return RelationType::ENABLES;
    if (norm == "breathe with" || norm == "breathewith") return RelationType::BREATHE_WITH;
    if (norm == "live in" || norm == "livein") return RelationType::LIVE_IN;
    if (norm == "build" || norm == "builds") return RelationType::BUILD;
    if (norm == "care for" || norm == "carefor") return RelationType::CARE_FOR;
    if (norm == "assoc" || norm == "association") return RelationType::ASSOC;
    
    return RelationType::UNKNOWN;
}

// Get relation priors for query type
RelationPriors RelationPriors::for_query_type(const std::string& question) {
    RelationPriors priors;
    std::string norm_q = MultiHopReasoner::normalize_text(question);
    
    // "What do X produce?" - favor PRODUCES
    if (norm_q.find("produce") != std::string::npos || norm_q.find("make") != std::string::npos) {
        priors.weights["PRODUCES"] = 1.5f;
        priors.weights["ARE"] = 1.2f;  // ISA helps reach categories
        priors.weights["HAS"] = 0.8f;
    }
    // "What is X?" or "What are X?" - favor ISA, properties
    else if (norm_q.find("what is") != std::string::npos || norm_q.find("what are") != std::string::npos) {
        priors.weights["ARE"] = 1.5f;
        priors.weights["IS_A"] = 1.5f;
        priors.weights["HAS"] = 1.2f;
    }
    // "What do X have?" - favor HAS
    else if (norm_q.find("have") != std::string::npos || norm_q.find("has") != std::string::npos) {
        priors.weights["HAS"] = 1.5f;
        priors.weights["ARE"] = 1.2f;
    }
    // "What do X require?" - favor REQUIRES
    else if (norm_q.find("require") != std::string::npos || norm_q.find("need") != std::string::npos) {
        priors.weights["REQUIRES"] = 1.5f;
        priors.weights["ARE"] = 1.2f;
    }
    // "Where do X live?" - favor LIVE_IN
    else if (norm_q.find("where") != std::string::npos || norm_q.find("live") != std::string::npos) {
        priors.weights["LIVE_IN"] = 1.5f;
        priors.weights["ARE"] = 1.0f;
    }
    // Default: equal priors
    else {
        priors.weights["ARE"] = 1.0f;
        priors.weights["HAS"] = 1.0f;
        priors.weights["PRODUCES"] = 1.0f;
    }
    
    return priors;
}

float RelationPriors::get_prior(const std::string& rel) const {
    auto it = weights.find(rel);
    if (it != weights.end()) {
        return it->second;
    }
    return 0.5f;  // Default for unknown relations
}

// Find edge between two nodes
const ::Edge* MultiHopReasoner::find_edge(
    uint64_t from,
    uint64_t to,
    const std::vector<::Edge>& edges
) {
    for (const auto& edge : edges) {
        if (edge.u == from && edge.v == to) {
            return &edge;
        }
    }
    return nullptr;
}

// Extract query nodes from question keywords
std::vector<uint64_t> MultiHopReasoner::extract_query_nodes(
    const std::string& question,
    std::unordered_map<uint64_t, ::Node>& nodes
) {
    std::vector<uint64_t> query_nodes;
    std::string norm_question = normalize_text(question);
    
    for (const auto& [id, node] : nodes) {
        std::string norm_node_text = normalize_text(node.text);
        // Match if node text appears in question and is substantial
        if (norm_question.find(norm_node_text) != std::string::npos && norm_node_text.size() > 2) {
            query_nodes.push_back(id);
        }
    }
    
    return query_nodes;
}

// Score a path
float MultiHopReasoner::score_path(
    const PathCandidate& path,
    const std::vector<::Edge>& edges,
    const RelationPriors& priors,
    const MultiHopOptions& opts
) {
    if (path.nodes.empty()) return 0.0f;
    
    float score = 1.0f;
    
    // Edge weight product (geometric mean to avoid vanishing)
    float weight_product = 1.0f;
    for (size_t i = 0; i + 1 < path.nodes.size(); i++) {
        const ::Edge* edge = find_edge(path.nodes[i], path.nodes[i+1], edges);
        if (edge) {
            weight_product *= edge->weight;
        }
    }
    score *= std::pow(weight_product, 1.0f / std::max(1.0f, static_cast<float>(path.hop_count)));
    
    // Relation prior bonus
    if (opts.use_relation_priors) {
        for (const auto& rel : path.relations) {
            float prior = priors.get_prior(rel);
            score *= prior;
        }
    }
    
    // Hop penalty (prefer shorter paths when tied)
    float hop_penalty = 1.0f / (1.0f + path.hop_count * 0.2f);
    score *= hop_penalty;
    
    return score;
}

// Beam search over graph
std::vector<PathCandidate> MultiHopReasoner::beam_search(
    const std::vector<uint64_t>& start_nodes,
    std::unordered_map<uint64_t, ::Node>& nodes,
    std::vector<::Edge>& edges,
    const MultiHopOptions& opts,
    const RelationPriors& priors
) {
    std::vector<PathCandidate> beam;
    
    // Initialize beam with start nodes
    for (uint64_t start_id : start_nodes) {
        PathCandidate initial;
        initial.nodes.push_back(start_id);
        initial.hop_count = 0;
        initial.score = 1.0f;
        beam.push_back(initial);
    }
    
    std::vector<PathCandidate> final_candidates;
    
    // Expand beam for k hops
    for (int hop = 0; hop < opts.max_hops; hop++) {
        std::vector<PathCandidate> expanded;
        
        for (const auto& candidate : beam) {
            uint64_t current_node = candidate.nodes.back();
            
            // Find all outgoing edges from current node
            for (const auto& edge : edges) {
                if (edge.u == current_node && edge.weight >= opts.min_edge_weight) {
                    // Create new candidate by extending path
                    PathCandidate new_cand = candidate;
                    new_cand.nodes.push_back(edge.v);
                    new_cand.hop_count++;
                    
                    // Determine relation (placeholder - would need edge metadata)
                    std::string rel = "ASSOC";  // Default
                    new_cand.relations.push_back(rel);
                    
                    // Score the new path
                    new_cand.score = score_path(new_cand, edges, priors, opts);
                    
                    expanded.push_back(new_cand);
                }
            }
            
            // Also add current candidate to final set (it's a complete path)
            final_candidates.push_back(candidate);
        }
        
        // Keep top beam_width candidates
        std::sort(expanded.rbegin(), expanded.rend());  // Descending by score
        if (expanded.size() > static_cast<size_t>(opts.beam_width)) {
            expanded.resize(opts.beam_width);
        }
        
        beam = expanded;
        
        if (beam.empty()) {
            break;  // No more paths to explore
        }
    }
    
    // Combine all candidates and return top ones
    final_candidates.insert(final_candidates.end(), beam.begin(), beam.end());
    std::sort(final_candidates.rbegin(), final_candidates.rend());
    
    return final_candidates;
}

// Main query answering function
MultiHopResult MultiHopReasoner::answer_query(
    const std::string& question,
    std::unordered_map<uint64_t, ::Node>& nodes,
    std::vector<::Edge>& edges,
    const MultiHopOptions& opts
) {
    MultiHopResult result;
    
    // Extract start nodes from question
    auto start_nodes = extract_query_nodes(question, nodes);
    
    if (start_nodes.empty()) {
        if (opts.verbose) {
            std::cout << "[MULTIHOP] No query nodes found in question\n";
        }
        return result;
    }
    
    if (opts.verbose) {
        std::cout << "[MULTIHOP] Found " << start_nodes.size() << " start nodes\n";
    }
    
    // Get relation priors for this query type
    RelationPriors priors = RelationPriors::for_query_type(question);
    
    // Beam search
    auto candidates = beam_search(start_nodes, nodes, edges, opts, priors);
    
    if (candidates.empty()) {
        if (opts.verbose) {
            std::cout << "[MULTIHOP] No paths found\n";
        }
        return result;
    }
    
    // Select best candidate
    const PathCandidate& best = candidates[0];
    
    if (!best.nodes.empty() && best.nodes.back() < nodes.size()) {
        uint64_t answer_node_id = best.nodes.back();
        if (nodes.find(answer_node_id) != nodes.end()) {
            result.answer = nodes[answer_node_id].text;
            result.path = best.nodes;
            result.relations_used = best.relations;
            result.confidence = best.score;
            result.hop_count = best.hop_count;
            result.success = true;
            
            if (opts.verbose) {
                std::cout << "[MULTIHOP] Best path (" << best.hop_count << " hops): ";
                for (size_t i = 0; i < best.nodes.size(); i++) {
                    if (i > 0) std::cout << " → ";
                    std::cout << nodes[best.nodes[i]].text;
                }
                std::cout << " (score: " << best.score << ")\n";
            }
        }
    }
    
    return result;
}

} // namespace reasoning
} // namespace melvin

