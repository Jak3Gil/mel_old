/*
 * MELVIN UNIFIED BRAIN - OUTPUT LAYER
 * Unified path-to-output generation for all modalities
 */

#pragma once
#include "melvin_storage.h"

namespace melvin_output {

using namespace melvin_storage;

// ==================== OUTPUT CONFIGURATION ====================

struct OutputConfig {
    bool generate_text = true;
    bool generate_audio = false;
    bool generate_image_desc = false;
    bool generate_motor = false;
};

struct MultimodalOutput {
    std::string text;
    std::vector<uint8_t> audio;
    std::string image_description;
    std::vector<std::string> motor_commands;
    float confidence = 0.0f;
    
    void print() const {
        if (!text.empty()) {
            std::cout << "📝 Text: " << text << "\n";
        }
        if (!audio.empty()) {
            std::cout << "🔊 Audio: [" << audio.size() << " bytes]\n";
        }
        if (!image_description.empty()) {
            std::cout << "🖼️  Image: " << image_description << "\n";
        }
        if (!motor_commands.empty()) {
            std::cout << "🤖 Motor: " << motor_commands.size() << " commands\n";
        }
        std::cout << "   Confidence: " << std::fixed << std::setprecision(2) << confidence << "\n";
    }
};

// ==================== PATH TO OUTPUT CONVERSION ====================

inline MultimodalOutput generate_output(const std::vector<int>& path, const OutputConfig& config) {
    MultimodalOutput output;
    
    if (path.empty()) {
        output.confidence = 0.0f;
        return output;
    }
    
    // Partition path by modality
    std::vector<int> text_nodes, audio_nodes, image_nodes, motor_nodes;
    
    for (int node_id : path) {
        if (node_id < 0 || node_id >= (int)nodes.size()) continue;
        
        switch (nodes[node_id].sensory_type) {
            case 0: text_nodes.push_back(node_id); break;
            case 1: audio_nodes.push_back(node_id); break;
            case 2: image_nodes.push_back(node_id); break;
            case 3: motor_nodes.push_back(node_id); break;
        }
    }
    
    // Generate text output
    if (config.generate_text) {
        std::string text_result;
        for (int node_id : text_nodes) {
            text_result += nodes[node_id].data + " ";
        }
        
        // If we have audio/image but no text, describe them
        if (text_result.empty()) {
            if (!audio_nodes.empty()) {
                text_result = "[Audio response: " + std::to_string(audio_nodes.size()) + " sounds]";
            } else if (!image_nodes.empty()) {
                text_result = "[Image: " + nodes[image_nodes[0]].data + "]";
            }
        }
        
        output.text = text_result;
    }
    
    // Generate audio output
    if (config.generate_audio) {
        if (!audio_nodes.empty()) {
            // For now, encode node labels
            for (int node_id : audio_nodes) {
                std::string label = nodes[node_id].data;
                output.audio.insert(output.audio.end(), label.begin(), label.end());
            }
        } else if (!text_nodes.empty()) {
            // Text-to-speech fallback (placeholder)
            std::string tts_text = "[TTS: " + nodes[text_nodes[0]].data + "]";
            output.audio.insert(output.audio.end(), tts_text.begin(), tts_text.end());
        }
    }
    
    // Generate image description
    if (config.generate_image_desc) {
        if (!image_nodes.empty()) {
            output.image_description = nodes[image_nodes[0]].data;
            
            // Enhance with text context
            if (!text_nodes.empty()) {
                output.image_description += " (" + nodes[text_nodes[0]].data + ")";
            }
        }
    }
    
    // Generate motor commands
    if (config.generate_motor) {
        for (int node_id : motor_nodes) {
            output.motor_commands.push_back(nodes[node_id].data);
        }
    }
    
    // Compute confidence from path weights
    float total_confidence = 0.0f;
    int edge_count = 0;
    
    for (size_t i = 0; i + 1 < path.size(); ++i) {
        // Find edge from path[i] to path[i+1]
        auto it = adjacency.find(path[i]);
        if (it != adjacency.end()) {
            for (int edge_id : it->second) {
                if (edges[edge_id].b == path[i+1]) {
                    total_confidence += edges[edge_id].w;
                    edge_count++;
                }
            }
        }
    }
    
    output.confidence = edge_count > 0 ? total_confidence / edge_count : 0.0f;
    
    return output;
}

// ==================== CONVENIENCE FUNCTIONS ====================

inline std::string path_to_text(const std::vector<int>& path) {
    OutputConfig config;
    config.generate_text = true;
    config.generate_audio = false;
    auto output = generate_output(path, config);
    return output.text;
}

inline void speak(const std::string& text) {
    std::cout << "🗣️  " << text << std::endl;
}

inline void display_multimodal(const MultimodalOutput& output) {
    std::cout << "\n🌐 Multimodal Output:\n";
    output.print();
    std::cout << "\n";
}

} // namespace melvin_output

