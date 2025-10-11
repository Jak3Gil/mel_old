#include "teaching_ingest.h"
#include "../../storage.h"
#include <algorithm>
#include <cctype>
#include <iostream>

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
namespace teaching {

// Normalize text for edge key
std::string TeachingIngestor::normalize_for_key(const std::string& text) {
    std::string result = text;
    // Lowercase
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

// Find or create node by text
uint64_t TeachingIngestor::find_or_create_node(
    const std::string& text,
    std::unordered_map<uint64_t, ::Node>& nodes,
    IngestResult& result
) {
    std::string normalized = normalize_for_key(text);
    
    // Search for existing node
    for (const auto& [id, node] : nodes) {
        if (normalize_for_key(node.text) == normalized) {
            return id;  // Found existing
        }
    }
    
    // Create new node
    uint64_t max_id = 0;
    for (const auto& [id, node] : nodes) {
        if (id > max_id) max_id = id;
    }
    uint64_t new_id = max_id + 1;
    
    ::Node new_node;
    new_node.id = new_id;
    new_node.text = text;
    new_node.type = 0;
    new_node.freq = 1;
    new_node.pinned = false;
    new_node.embedding.resize(64, 0.0f);  // Initialize empty embedding
    new_node.emb = new_node.embedding;
    
    nodes[new_id] = new_node;
    result.nodes_added++;
    result.new_node_labels.push_back(text);
    
    return new_id;
}

// Create or update edge
bool TeachingIngestor::create_or_update_edge(
    uint64_t from_id,
    uint64_t to_id,
    const std::string& rel,
    float weight,
    std::vector<::Edge>& edges,
    std::unordered_set<EdgeKey>& seen_edges,
    IngestResult& result,
    bool allow_duplicates
) {
    // Check for duplicate
    if (!allow_duplicates) {
        EdgeKey key;
        key.from = std::to_string(from_id);
        key.rel = normalize_for_key(rel);
        key.to = std::to_string(to_id);
        
        if (seen_edges.find(key) != seen_edges.end()) {
            result.duplicates_skipped++;
            return false;
        }
        seen_edges.insert(key);
    }
    
    // Check if edge exists
    bool found = false;
    for (auto& edge : edges) {
        if (edge.u == from_id && edge.v == to_id) {
            // Update existing edge
            edge.count++;
            edge.weight = std::min(edge.weight + 0.1f, 1.0f);
            edge.w_core = edge.weight;
            result.edges_updated++;
            found = true;
            break;
        }
    }
    
    if (!found) {
        // Create new edge
        ::Edge new_edge;
        new_edge.u = from_id;
        new_edge.v = to_id;
        new_edge.locB = to_id;
        new_edge.weight = weight;
        new_edge.w_core = weight;
        new_edge.w_ctx = 0.0f;
        new_edge.count = 1;
        
        edges.push_back(new_edge);
        result.edges_added++;
    }
    
    return true;
}

// Ingest a FACT
void TeachingIngestor::ingest_fact(
    const TchFact& fact,
    std::unordered_map<uint64_t, ::Node>& nodes,
    std::vector<::Edge>& edges,
    std::unordered_set<EdgeKey>& seen_edges,
    IngestResult& result,
    const IngestOptions& opts
) {
    // Create/find nodes
    uint64_t subj_id = find_or_create_node(fact.subj, nodes, result);
    uint64_t obj_id = find_or_create_node(fact.obj, nodes, result);
    
    // Create edge
    float weight = fact.meta.confidence;
    create_or_update_edge(subj_id, obj_id, fact.rel, weight, edges, seen_edges, result, !opts.deduplicate);
    
    if (opts.verbose) {
        std::cout << "[INGEST] FACT: " << fact.subj << " " << fact.rel << " " << fact.obj << std::endl;
    }
}

// Ingest an ASSOCIATION
void TeachingIngestor::ingest_association(
    const TchAssoc& assoc,
    std::unordered_map<uint64_t, ::Node>& nodes,
    std::vector<::Edge>& edges,
    std::unordered_set<EdgeKey>& seen_edges,
    IngestResult& result,
    const IngestOptions& opts
) {
    // Create/find nodes
    uint64_t left_id = find_or_create_node(assoc.left, nodes, result);
    uint64_t right_id = find_or_create_node(assoc.right, nodes, result);
    
    // Create edge (bidirectional if marked)
    float weight = assoc.meta.confidence * 0.8f;  // Associations slightly weaker than facts
    std::string rel = assoc.rel.empty() ? "ASSOC" : assoc.rel;
    
    create_or_update_edge(left_id, right_id, rel, weight, edges, seen_edges, result, !opts.deduplicate);
    
    if (assoc.bidirectional) {
        create_or_update_edge(right_id, left_id, rel, weight, edges, seen_edges, result, !opts.deduplicate);
    }
    
    if (opts.verbose) {
        std::cout << "[INGEST] ASSOC: " << assoc.left << " " << rel << " " << assoc.right 
                  << (assoc.bidirectional ? " (bidirectional)" : "") << std::endl;
    }
}

// Main ingestion function
IngestResult TeachingIngestor::ingest(
    const TchDoc& doc,
    std::unordered_map<uint64_t, ::Node>& nodes,
    std::vector<::Edge>& edges,
    const IngestOptions& opts
) {
    IngestResult result;
    std::unordered_set<EdgeKey> seen_edges;
    
    uint32_t initial_nodes = nodes.size();
    uint32_t initial_edges = edges.size();
    
    if (opts.verbose) {
        std::cout << "\n[INGEST] Starting ingestion: " << doc.filepath << std::endl;
        std::cout << "  Initial: " << initial_nodes << " nodes, " << initial_edges << " edges\n" << std::endl;
    }
    
    // Process each block
    for (const auto& block : doc.blocks) {
        switch (block.type) {
            case BlockType::FACT:
                if (block.fact) {
                    ingest_fact(*block.fact, nodes, edges, seen_edges, result, opts);
                }
                break;
            
            case BlockType::ASSOCIATION:
                if (block.assoc) {
                    ingest_association(*block.assoc, nodes, edges, seen_edges, result, opts);
                }
                break;
            
            case BlockType::RULE:
                // Rules are stored as metadata for now (future: create schema nodes)
                if (block.rule) {
                    result.rules_added++;
                    if (opts.verbose) {
                        std::cout << "[INGEST] RULE: " << block.rule->pattern 
                                  << " → " << block.rule->implies << std::endl;
                    }
                }
                break;
            
            case BlockType::QUERY:
            case BlockType::EXPECT:
            case BlockType::TEST:
                // These are for verification, not ingestion
                break;
            
            default:
                break;
        }
        
        // Stop on first error if strict mode
        if (opts.strict && !result.errors.empty()) {
            result.success = false;
            break;
        }
    }
    
    if (opts.verbose) {
        std::cout << "\n[INGEST] Complete:" << std::endl;
        std::cout << "  Nodes added: " << result.nodes_added << std::endl;
        std::cout << "  Edges added: " << result.edges_added << std::endl;
        std::cout << "  Edges updated: " << result.edges_updated << std::endl;
        std::cout << "  Duplicates skipped: " << result.duplicates_skipped << std::endl;
        std::cout << "  Rules added: " << result.rules_added << std::endl;
        std::cout << "  Final: " << nodes.size() << " nodes, " << edges.size() << " edges\n" << std::endl;
    }
    
    // Snapshot if requested
    if (opts.snapshot_after && result.success) {
        if (melvin::save_brain_snapshot("melvin_brain.bin", nodes, edges)) {
            if (opts.verbose) {
                std::cout << "[SNAPSHOT] Saved brain state" << std::endl;
            }
        }
    }
    
    return result;
}

} // namespace teaching
} // namespace melvin

