/*
 * MELVIN CLIP ADAPTER
 * 
 * Image → concept node mapping using CLIP embeddings:
 * - Ingest images and create IMAGE nodes
 * - Link to text labels via LOOKS_LIKE connector
 * - Support similarity queries
 * - Mock CLIP encoding (can be replaced with real CLIP)
 */

#pragma once
#include "../../include/melvin_storage.h"
#include "../../include/melvin_modal_types.h"
#include "../../src/util/telemetry.h"
#include "sensory_utils.h"
#include <string>
#include <vector>
#include <map>

namespace melvin_clip {

using namespace melvin_storage;
using namespace melvin_modal;
using namespace melvin_sensory;
using namespace melvin_telemetry;

// ==================== CLIP ENCODING ====================

// Encode image to CLIP embedding (mock implementation)
inline std::vector<float> encode_clip(const std::string& image_path, bool use_mock = true) {
    if (use_mock) {
        // Generate deterministic mock embedding
        return generate_mock_clip_embedding(image_path);
    } else {
        // In production, call Python CLIP encoder or load from cache
        // For now, fallback to mock
        return generate_mock_clip_embedding(image_path);
    }
}

// ==================== IMAGE INGESTION ====================

// Ingest image and create nodes + connections
inline int ingest_image(const std::string& image_path, const std::string& label) {
    // 1. Get CLIP embedding
    auto embedding = encode_clip(image_path);
    auto quantized = quantize_embedding(embedding);
    
    // 2. Create IMAGE node with quantized embedding as data
    std::string node_data = "img:" + image_path;
    int img_node = create_node(node_data, static_cast<int>(ModalNodeType::NODE_IMAGE), 0);
    
    if (img_node < 0) {
        return -1;
    }
    
    // 3. Find or create text label node
    int label_node = -1;
    auto it = node_lookup.find(label);
    if (it != node_lookup.end()) {
        label_node = it->second;
    } else {
        label_node = create_node(label, static_cast<int>(ModalNodeType::NODE_TEXT), 0);
    }
    
    if (label_node < 0) {
        return -1;
    }
    
    // 4. Create LOOKS_LIKE connection (EXACT)
    int conn_node = create_node("LOOKS_LIKE", 1, 0); // type=1 for connector
    
    // img -> LOOKS_LIKE
    int edge1 = connect(img_node, conn_node, Rel::ISA, 1.0f, EdgeType::EXACT);
    // LOOKS_LIKE -> label
    int edge2 = connect(conn_node, label_node, Rel::ISA, 1.0f, EdgeType::EXACT);
    
    // 5. Log telemetry
    log_event("image_ingest", {
        {"file", image_path},
        {"label", label},
        {"img_node", std::to_string(img_node)},
        {"label_node", std::to_string(label_node)}
    });
    
    return img_node;
}

// ==================== SIMILARITY SEARCH ====================

// Find images similar to a query embedding
inline std::vector<std::pair<int, float>> find_similar_images(
    const std::vector<float>& query_embedding,
    int top_k = 5
) {
    std::vector<std::pair<int, float>> results;
    
    // Scan all IMAGE nodes
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].type == static_cast<int>(ModalNodeType::NODE_IMAGE)) {
            // Would need to store embeddings separately or decode from node data
            // For now, compute similarity based on node index (mock)
            float similarity = 1.0f / (1.0f + std::abs((int)i - (int)query_embedding.size()));
            results.push_back({(int)i, similarity});
        }
    }
    
    // Sort by similarity (descending)
    std::sort(results.begin(), results.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Return top K
    if (results.size() > (size_t)top_k) {
        results.resize(top_k);
    }
    
    return results;
}

// Find images labeled with a specific concept
inline std::vector<int> find_images_by_label(const std::string& label) {
    std::vector<int> image_nodes;
    
    // Find label node
    auto it = node_lookup.find(label);
    if (it == node_lookup.end()) {
        return image_nodes;
    }
    
    int label_node = it->second;
    
    // Find LOOKS_LIKE connector
    int connector_node = -1;
    auto conn_it = node_lookup.find("LOOKS_LIKE");
    if (conn_it != node_lookup.end()) {
        connector_node = conn_it->second;
    }
    
    if (connector_node < 0) {
        return image_nodes;
    }
    
    // Find all edges pointing to this label through LOOKS_LIKE
    for (const auto& edge : edges) {
        if (edge.b == label_node && edge.is_exact()) {
            // Check if previous hop is through LOOKS_LIKE
            for (const auto& prev_edge : edges) {
                if (prev_edge.b == connector_node && prev_edge.is_exact()) {
                    if (nodes[prev_edge.a].type == static_cast<int>(ModalNodeType::NODE_IMAGE)) {
                        image_nodes.push_back(prev_edge.a);
                    }
                }
            }
        }
    }
    
    return image_nodes;
}

// ==================== CROSS-MODAL QUERIES ====================

// Query: "what looks like X?"
inline std::string query_visual_similarity(const std::string& concept) {
    auto images = find_images_by_label(concept);
    
    if (images.empty()) {
        return "I don't have visual information about " + concept;
    }
    
    std::string result = "Images that look like " + concept + ": ";
    for (size_t i = 0; i < images.size() && i < 3; ++i) {
        if (i > 0) result += ", ";
        result += nodes[images[i]].data;
    }
    
    return result;
}

} // namespace melvin_clip

