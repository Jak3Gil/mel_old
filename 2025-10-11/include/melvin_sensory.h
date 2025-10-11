/*
 * MELVIN UNIFIED BRAIN - SENSORY LAYER
 * Input encoders for text, audio, image, and motor feedback
 */

#pragma once
#include "melvin_storage.h"
#include "melvin_reasoning.h"

namespace melvin_sensory {

using namespace melvin_storage;
using namespace melvin_reasoning;

// ==================== SENSORY TYPES ====================

enum SensoryModality : uint32_t {
    TEXT = 0,
    AUDIO = 1,
    IMAGE = 2,
    MOTOR = 3
};

// ==================== TEXT ENCODING ====================

inline std::vector<int> teach_text(const std::string& text) {
    std::stringstream ss(text);
    std::string word;
    std::vector<int> word_ids;
    int prev_id = -1;
    
    while (ss >> word) {
        // Clean word
        word.erase(std::remove_if(word.begin(), word.end(), 
                  [](char c) { return std::ispunct(c) && c != '-'; }), word.end());
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        
        if (word.length() < 2) continue;
        
        // Create node
        int word_id = create_node(word, 0, SensoryModality::TEXT);
        word_ids.push_back(word_id);
        
        // Connect to previous word (temporal)
        if (prev_id >= 0) {
            connect(prev_id, word_id, Rel::TEMPORAL);
        }
        
        prev_id = word_id;
    }
    
    // Extract relationships
    for (size_t i = 0; i + 2 < word_ids.size(); ++i) {
        std::string w1 = nodes[word_ids[i]].data;
        std::string w2 = nodes[word_ids[i+1]].data;
        std::string w3 = nodes[word_ids[i+2]].data;
        
        // Pattern: "X is Y"
        if (w2 == "is" || w2 == "are") {
            connect(word_ids[i], word_ids[i+2], Rel::ISA);
        }
        // Pattern: "X has Y"
        else if (w2 == "has" || w2 == "have") {
            connect(word_ids[i], word_ids[i+2], Rel::HAS);
        }
        // Pattern: "X can Y"
        else if (w2 == "can" || w2 == "could") {
            connect(word_ids[i], word_ids[i+2], Rel::CAN);
        }
        // Pattern: "X eats/drinks Y"
        else if (w2 == "eats" || w2 == "drinks" || w2 == "consumes") {
            connect(word_ids[i], word_ids[i+2], Rel::CONSUMES);
        }
    }
    
    return word_ids;
}

// ==================== AUDIO ENCODING ====================

inline int teach_audio(const std::vector<uint8_t>& audio_data, const std::string& label = "") {
    // Create audio node
    std::string audio_repr = "[audio:" + (label.empty() ? "unlabeled" : label) + "]";
    int audio_id = create_node(audio_repr, 0, SensoryModality::AUDIO);
    
    // If label provided, create cross-modal link
    if (!label.empty()) {
        int text_id = create_node(label, 0, SensoryModality::TEXT);
        connect(text_id, audio_id, Rel::CROSS_MODAL);
        connect(audio_id, text_id, Rel::CROSS_MODAL);
    }
    
    return audio_id;
}

inline std::vector<int> teach_audio_sequence(const std::vector<std::vector<uint8_t>>& frames, 
                                             const std::vector<std::string>& labels) {
    std::vector<int> audio_ids;
    int prev_id = -1;
    
    for (size_t i = 0; i < frames.size(); ++i) {
        std::string label = i < labels.size() ? labels[i] : "";
        int audio_id = teach_audio(frames[i], label);
        audio_ids.push_back(audio_id);
        
        // Temporal connection between frames
        if (prev_id >= 0) {
            connect(prev_id, audio_id, Rel::TEMPORAL);
        }
        
        prev_id = audio_id;
    }
    
    return audio_ids;
}

// ==================== IMAGE ENCODING ====================

inline int teach_image(const std::vector<float>& embedding, const std::string& label = "") {
    // Create image node
    std::string image_repr = "[image:" + (label.empty() ? "unlabeled" : label) + "]";
    int image_id = create_node(image_repr, 0, SensoryModality::IMAGE);
    
    // If label provided, create cross-modal link
    if (!label.empty()) {
        int text_id = create_node(label, 0, SensoryModality::TEXT);
        connect(text_id, image_id, Rel::CROSS_MODAL);
        connect(image_id, text_id, Rel::CROSS_MODAL);
    }
    
    return image_id;
}

// ==================== MOTOR ENCODING ====================

inline int teach_motor_state(const std::string& action, float value) {
    // Create motor state node
    std::string motor_repr = "[motor:" + action + "=" + std::to_string(value) + "]";
    int motor_id = create_node(motor_repr, 0, SensoryModality::MOTOR);
    
    // Link to action concept
    int concept_id = create_node(action, 0, SensoryModality::TEXT);
    connect(concept_id, motor_id, Rel::CROSS_MODAL);
    
    return motor_id;
}

// ==================== MULTIMODAL PAIRING ====================

inline void teach_multimodal_pair(const std::string& text, 
                                  const std::vector<uint8_t>& audio,
                                  const std::string& concept_label) {
    // Create text node
    std::vector<int> text_ids = teach_text(text);
    
    // Create audio node
    int audio_id = teach_audio(audio, concept_label);
    
    // Create concept node
    int concept_id = create_node(concept_label, 1, SensoryModality::TEXT);  // type=1 for concept
    
    // Cross-modal links
    for (int text_id : text_ids) {
        if (nodes[text_id].data == concept_label) {
            connect(text_id, audio_id, Rel::CROSS_MODAL, 0.8f);
            connect(audio_id, text_id, Rel::CROSS_MODAL, 0.8f);
        }
    }
    
    // Concept links
    connect(concept_id, audio_id, Rel::HAS);
    for (int text_id : text_ids) {
        if (nodes[text_id].data != concept_label) {
            connect(concept_id, text_id, Rel::HAS);
        }
    }
}

} // namespace melvin_sensory

