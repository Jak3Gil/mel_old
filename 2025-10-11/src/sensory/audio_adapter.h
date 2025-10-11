/*
 * MELVIN AUDIO ADAPTER
 * 
 * Audio → concept node mapping using phoneme/word quantization:
 * - Ingest audio files and create AUDIO nodes
 * - Link to text labels via SOUND connector
 * - Support phonetic similarity
 * - Mock audio quantization (can be replaced with real VQ)
 */

#pragma once
#include "../../include/melvin_storage.h"
#include "../../include/melvin_modal_types.h"
#include "../../src/util/telemetry.h"
#include "sensory_utils.h"
#include <string>
#include <vector>
#include <map>

namespace melvin_audio {

using namespace melvin_storage;
using namespace melvin_modal;
using namespace melvin_sensory;
using namespace melvin_telemetry;

// ==================== AUDIO QUANTIZATION ====================

// Quantize audio to phoneme/frame codes (mock implementation)
inline std::vector<uint8_t> quantize_audio(const std::string& audio_path, bool use_mock = true) {
    if (use_mock) {
        // Generate deterministic mock codes
        return generate_mock_audio_codes(audio_path);
    } else {
        // In production, do log-mel spectrogram + vector quantization
        // For now, fallback to mock
        return generate_mock_audio_codes(audio_path);
    }
}

// ==================== AUDIO INGESTION ====================

// Ingest audio and create nodes + connections
inline int ingest_audio(const std::string& audio_path, const std::string& label) {
    // 1. Get audio quantization codes
    auto codes = quantize_audio(audio_path);
    
    // 2. Create AUDIO node with codes as data
    std::string node_data = "aud:" + audio_path;
    int aud_node = create_node(node_data, static_cast<int>(ModalNodeType::NODE_AUDIO), 0);
    
    if (aud_node < 0) {
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
    
    // 4. Create SOUND connection (EXACT)
    int conn_node = create_node("SOUND", 1, 0); // type=1 for connector
    
    // aud -> SOUND
    int edge1 = connect(aud_node, conn_node, Rel::ISA, 1.0f, EdgeType::EXACT);
    // SOUND -> label
    int edge2 = connect(conn_node, label_node, Rel::ISA, 1.0f, EdgeType::EXACT);
    
    // 5. Log telemetry
    log_event("audio_ingest", {
        {"file", audio_path},
        {"label", label},
        {"aud_node", std::to_string(aud_node)},
        {"label_node", std::to_string(label_node)}
    });
    
    return aud_node;
}

// ==================== PHONETIC SIMILARITY ====================

// Compute phonetic similarity between audio codes
inline float phonetic_similarity(const std::vector<uint8_t>& codes_a,
                                const std::vector<uint8_t>& codes_b) {
    if (codes_a.empty() || codes_b.empty()) return 0.0f;
    
    // Simple edit distance-based similarity
    size_t min_len = std::min(codes_a.size(), codes_b.size());
    size_t max_len = std::max(codes_a.size(), codes_b.size());
    
    int matches = 0;
    for (size_t i = 0; i < min_len; ++i) {
        if (codes_a[i] == codes_b[i]) {
            matches++;
        }
    }
    
    return static_cast<float>(matches) / max_len;
}

// Find audio files similar to query codes
inline std::vector<std::pair<int, float>> find_similar_audio(
    const std::vector<uint8_t>& query_codes,
    int top_k = 5
) {
    std::vector<std::pair<int, float>> results;
    
    // Scan all AUDIO nodes
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].type == static_cast<int>(ModalNodeType::NODE_AUDIO)) {
            // Would need to store codes separately or decode from node data
            // For now, compute similarity based on node index (mock)
            float similarity = 1.0f / (1.0f + std::abs((int)i - (int)query_codes.size()));
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

// Find audio labeled with a specific concept
inline std::vector<int> find_audio_by_label(const std::string& label) {
    std::vector<int> audio_nodes;
    
    // Find label node
    auto it = node_lookup.find(label);
    if (it == node_lookup.end()) {
        return audio_nodes;
    }
    
    int label_node = it->second;
    
    // Find SOUND connector
    int connector_node = -1;
    auto conn_it = node_lookup.find("SOUND");
    if (conn_it != node_lookup.end()) {
        connector_node = conn_it->second;
    }
    
    if (connector_node < 0) {
        return audio_nodes;
    }
    
    // Find all edges pointing to this label through SOUND
    for (const auto& edge : edges) {
        if (edge.b == label_node && edge.is_exact()) {
            // Check if previous hop is through SOUND
            for (const auto& prev_edge : edges) {
                if (prev_edge.b == connector_node && prev_edge.is_exact()) {
                    if (nodes[prev_edge.a].type == static_cast<int>(ModalNodeType::NODE_AUDIO)) {
                        audio_nodes.push_back(prev_edge.a);
                    }
                }
            }
        }
    }
    
    return audio_nodes;
}

// ==================== CROSS-MODAL QUERIES ====================

// Query: "what sound does X make?"
inline std::string query_sound(const std::string& concept) {
    auto audio = find_audio_by_label(concept);
    
    if (audio.empty()) {
        return "I don't have audio information about " + concept;
    }
    
    std::string result = "Sounds associated with " + concept + ": ";
    for (size_t i = 0; i < audio.size() && i < 3; ++i) {
        if (i > 0) result += ", ";
        result += nodes[audio[i]].data;
    }
    
    return result;
}

// Create sound association between concept and sound label
inline bool connect_sound(const std::string& concept, const std::string& sound) {
    // Find nodes
    auto concept_it = node_lookup.find(concept);
    auto sound_it = node_lookup.find(sound);
    
    if (concept_it == node_lookup.end() || sound_it == node_lookup.end()) {
        return false;
    }
    
    // Create connection through SOUND connector
    int conn_node = -1;
    auto conn_it = node_lookup.find("SOUND");
    if (conn_it != node_lookup.end()) {
        conn_node = conn_it->second;
    } else {
        conn_node = create_node("SOUND", 1, 0);
    }
    
    // concept -> SOUND -> sound
    connect(concept_it->second, conn_node, Rel::ISA, 1.0f, EdgeType::EXACT);
    connect(conn_node, sound_it->second, Rel::ISA, 1.0f, EdgeType::EXACT);
    
    return true;
}

} // namespace melvin_audio

