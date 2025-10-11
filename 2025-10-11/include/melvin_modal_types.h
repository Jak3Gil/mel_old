/*
 * MELVIN MODAL TYPES
 * 
 * Multi-modal node and connector types for cross-modal reasoning:
 * - Text, audio, image, sensor nodes
 * - Specialized connectors for different modalities
 * - Integration with existing EXACT+LEAP system
 */

#pragma once
#include <cstdint>
#include <string>

namespace melvin_modal {

// ==================== NODE TYPES ====================

enum class ModalNodeType : uint8_t {
    NODE_TEXT = 0,      // Text/concept node
    NODE_AUDIO = 1,     // Audio/phoneme node
    NODE_IMAGE = 2,     // Image/visual node
    NODE_SENSOR = 3,    // Sensor data node
    NODE_THOUGHT = 4,   // Abstract thought/reasoning node
    NODE_MIXED = 5      // Multi-modal fusion node
};

// ==================== CONNECTOR TYPES ====================

enum class ModalConnectorType : uint8_t {
    CONN_BE = 0,           // General "is/are" (text-text)
    CONN_SOUND = 1,        // Audio relation (audio-text)
    CONN_LOOKS_LIKE = 2,   // Visual similarity (image-text)
    CONN_GENERAL = 3,      // Generic relation
    CONN_REPRESENTS = 4,   // Symbolic representation
    CONN_SOUNDS_LIKE = 5,  // Phonetic similarity
    CONN_CONTAINS = 6,     // Compositional relation
    CONN_SIMILAR_TO = 7    // Generic similarity
};

// ==================== MODALITY FLAGS ====================

enum class Modality : uint8_t {
    TEXT = 0,
    AUDIO = 1,
    IMAGE = 2,
    SENSOR = 3,
    ABSTRACT = 4
};

// ==================== HELPER FUNCTIONS ====================

inline std::string node_type_to_string(ModalNodeType type) {
    switch (type) {
        case ModalNodeType::NODE_TEXT: return "TEXT";
        case ModalNodeType::NODE_AUDIO: return "AUDIO";
        case ModalNodeType::NODE_IMAGE: return "IMAGE";
        case ModalNodeType::NODE_SENSOR: return "SENSOR";
        case ModalNodeType::NODE_THOUGHT: return "THOUGHT";
        case ModalNodeType::NODE_MIXED: return "MIXED";
        default: return "UNKNOWN";
    }
}

inline std::string connector_type_to_string(ModalConnectorType type) {
    switch (type) {
        case ModalConnectorType::CONN_BE: return "BE";
        case ModalConnectorType::CONN_SOUND: return "SOUND";
        case ModalConnectorType::CONN_LOOKS_LIKE: return "LOOKS_LIKE";
        case ModalConnectorType::CONN_GENERAL: return "GENERAL";
        case ModalConnectorType::CONN_REPRESENTS: return "REPRESENTS";
        case ModalConnectorType::CONN_SOUNDS_LIKE: return "SOUNDS_LIKE";
        case ModalConnectorType::CONN_CONTAINS: return "CONTAINS";
        case ModalConnectorType::CONN_SIMILAR_TO: return "SIMILAR_TO";
        default: return "UNKNOWN";
    }
}

inline Modality get_modality(ModalNodeType type) {
    switch (type) {
        case ModalNodeType::NODE_TEXT: return Modality::TEXT;
        case ModalNodeType::NODE_AUDIO: return Modality::AUDIO;
        case ModalNodeType::NODE_IMAGE: return Modality::IMAGE;
        case ModalNodeType::NODE_SENSOR: return Modality::SENSOR;
        case ModalNodeType::NODE_THOUGHT: return Modality::ABSTRACT;
        case ModalNodeType::NODE_MIXED: return Modality::ABSTRACT;
        default: return Modality::TEXT;
    }
}

// Check if connector is cross-modal
inline bool is_cross_modal_connector(ModalConnectorType type) {
    return type == ModalConnectorType::CONN_SOUND ||
           type == ModalConnectorType::CONN_LOOKS_LIKE ||
           type == ModalConnectorType::CONN_REPRESENTS ||
           type == ModalConnectorType::CONN_SOUNDS_LIKE;
}

// Get appropriate connector for modality pair
inline ModalConnectorType get_connector_for_modalities(Modality from, Modality to) {
    if (from == Modality::AUDIO || to == Modality::AUDIO) {
        return ModalConnectorType::CONN_SOUND;
    }
    if (from == Modality::IMAGE || to == Modality::IMAGE) {
        return ModalConnectorType::CONN_LOOKS_LIKE;
    }
    if (from == Modality::TEXT && to == Modality::TEXT) {
        return ModalConnectorType::CONN_BE;
    }
    return ModalConnectorType::CONN_GENERAL;
}

} // namespace melvin_modal

