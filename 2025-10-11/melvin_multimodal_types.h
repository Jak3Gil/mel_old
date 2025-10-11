/*
 * MELVIN MULTIMODAL BRAIN TYPES
 * 
 * Extended data structures to support the complete sensory brain loop
 * with text, audio, and image processing capabilities.
 */

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <array>

namespace melvin {

// ==================== CORE BRAIN LAYOUT ====================

// Node Types (from "Melvin Brain Layout 9/17")
enum class NodeType : uint8_t {
    TAUGHT_NODE = 0x01,        // 0001 - Raw sensory fragment
    THOUGHT_NODE = 0x0E,       // 1110 - Stored reasoning path
    CONNECTION_WRAPPER = 0x05  // 0101 - Edge with relation + weight
};

// Sensory Data Types
enum class SensoryType : uint8_t {
    TEXT = 0,      // UTF-8 text fragments
    AUDIO = 1,     // Quantized spectrogram frames
    IMAGE = 2,     // CLIP/ResNet embeddings
    SENSOR = 3     // General sensor readings
};

// Relation Types (extended for multimodal)
enum class RelationType : uint8_t {
    NEXT = 0,           // Temporal sequence
    LEAP = 1,           // Reasoning jump
    SIMILAR_TO = 2,     // Similarity connection
    ISA = 3,            // Is-a relationship
    CONSUMES = 4,       // Consumption relationship
    TEMPORAL = 5,       // Temporal relationship
    AUDIO_PHONEME = 6,  // Phoneme-to-word mapping
    IMAGE_SEMANTIC = 7, // Image-to-concept binding
    CROSS_MODAL = 8     // Cross-modal association
};

// ==================== EXTENDED NODE RECORD ====================

struct NodeRec {
    NodeType node_type;           // 0001, 1110, or 0101
    SensoryType sensory_type;     // 0=text, 1=audio, 2=image, 3=sensor
    uint32_t id;                  // Unique node identifier
    uint32_t payload_len;         // Length of payload data
    uint8_t payload[];            // Variable-length payload
    
    // Helper methods
    bool is_taught_node() const { return node_type == NodeType::TAUGHT_NODE; }
    bool is_thought_node() const { return node_type == NodeType::THOUGHT_NODE; }
    bool is_connection() const { return node_type == NodeType::CONNECTION_WRAPPER; }
    
    bool is_text() const { return sensory_type == SensoryType::TEXT; }
    bool is_audio() const { return sensory_type == SensoryType::AUDIO; }
    bool is_image() const { return sensory_type == SensoryType::IMAGE; }
    bool is_sensor() const { return sensory_type == SensoryType::SENSOR; }
};

// ==================== AUDIO DATA STRUCTURES ====================

struct AudioFrame {
    static constexpr size_t FRAME_SIZE = 512;  // Quantized spectrogram frame size
    std::array<uint8_t, FRAME_SIZE> quantized_data;
    uint32_t sample_rate;
    uint32_t timestamp_ms;
    float confidence;
    
    // Phoneme mapping (if available)
    std::string phoneme_sequence;
    std::string word_boundaries;  // "0,1,0,1,0" for word starts/ends
};

struct AudioNodePayload {
    AudioFrame frame;
    std::string transcript;       // Human-readable transcript
    std::vector<uint32_t> word_ids; // Associated word node IDs
};

// ==================== IMAGE DATA STRUCTURES ====================

struct ImageEmbedding {
    static constexpr size_t EMBEDDING_SIZE = 1024;  // CLIP embedding size
    std::array<float, EMBEDDING_SIZE> features;
    uint32_t width;
    uint32_t height;
    std::string format;           // "RGB", "RGBA", etc.
    float confidence;
    
    // Semantic labels
    std::vector<std::string> detected_objects;
    std::vector<std::string> scene_description;
};

struct ImageNodePayload {
    ImageEmbedding embedding;
    std::string filename;         // Source filename
    std::vector<uint32_t> concept_ids; // Associated concept node IDs
    std::string caption;          // Human-readable description
};

// ==================== SENSOR DATA STRUCTURES ====================

struct SensorReading {
    uint32_t sensor_id;
    uint32_t timestamp_ms;
    float value;
    std::string unit;             // "degrees", "meters", "pressure", etc.
    std::string sensor_type;      // "temperature", "position", "touch", etc.
    float confidence;
};

struct SensorNodePayload {
    SensorReading reading;
    std::vector<uint32_t> context_ids; // Associated context node IDs
    std::string interpretation;   // Human-readable interpretation
};

// ==================== THOUGHT NODE STRUCTURES ====================

struct ThoughtPath {
    std::vector<uint32_t> node_sequence;
    std::vector<RelationType> relation_sequence;
    float success_score;
    uint32_t timestamp_created;
    uint32_t replay_count;
    
    // Multimodal fusion
    bool is_multimodal() const {
        // Check if path contains multiple sensory types
        // This would be determined by examining the nodes in the sequence
        return true; // Simplified for now
    }
};

struct ThoughtNodePayload {
    ThoughtPath path;
    std::string description;      // Human-readable description of the thought
    std::vector<std::string> keywords; // Searchable keywords
    float utility_score;          // How useful this thought has been
};

// ==================== CONNECTION WRAPPER STRUCTURES ====================

struct ConnectionData {
    uint32_t node_a_id;
    uint32_t node_b_id;
    RelationType relation_type;
    float weight;
    uint32_t frequency;
    uint32_t last_used;
    
    // Multimodal connection metadata
    bool is_cross_modal() const {
        // This would be determined by checking node types
        return false; // Simplified for now
    }
};

// ==================== MULTIMODAL METRICS ====================

struct MultimodalMetrics {
    // Audio metrics
    float avg_audio_entropy = 0.0f;
    float audio_phoneme_accuracy = 0.0f;
    float audio_word_boundary_accuracy = 0.0f;
    
    // Image metrics
    float avg_image_similarity = 0.0f;
    float image_object_detection_accuracy = 0.0f;
    float image_caption_quality = 0.0f;
    
    // Cross-modal metrics
    float multi_modal_coherence_score = 0.0f;
    float cross_modal_association_strength = 0.0f;
    float sensory_fusion_quality = 0.0f;
    
    // Sensor metrics
    float sensor_data_consistency = 0.0f;
    float sensor_prediction_accuracy = 0.0f;
    
    std::string to_string() const {
        std::stringstream ss;
        ss << "MultimodalMetrics["
           << "audio_entropy=" << std::fixed << std::setprecision(3) << avg_audio_entropy
           << ", image_sim=" << avg_image_similarity
           << ", coherence=" << multi_modal_coherence_score
           << ", fusion=" << sensory_fusion_quality << "]";
        return ss.str();
    }
};

// ==================== DATA PARSING HELPERS ====================

class NodePayloadParser {
public:
    // Parse text payload
    static std::string parse_text_payload(const NodeRec* node) {
        if (!node->is_text()) return "";
        return std::string(reinterpret_cast<const char*>(node->payload), node->payload_len);
    }
    
    // Parse audio payload
    static AudioNodePayload parse_audio_payload(const NodeRec* node) {
        AudioNodePayload payload;
        if (!node->is_audio() || node->payload_len < sizeof(AudioFrame)) {
            return payload;
        }
        
        // Deserialize audio frame
        const AudioFrame* frame = reinterpret_cast<const AudioFrame*>(node->payload);
        payload.frame = *frame;
        
        // Parse additional data if available
        size_t frame_size = sizeof(AudioFrame);
        if (node->payload_len > frame_size) {
            const char* transcript_start = reinterpret_cast<const char*>(node->payload + frame_size);
            payload.transcript = std::string(transcript_start);
        }
        
        return payload;
    }
    
    // Parse image payload
    static ImageNodePayload parse_image_payload(const NodeRec* node) {
        ImageNodePayload payload;
        if (!node->is_image() || node->payload_len < sizeof(ImageEmbedding)) {
            return payload;
        }
        
        // Deserialize image embedding
        const ImageEmbedding* embedding = reinterpret_cast<const ImageEmbedding*>(node->payload);
        payload.embedding = *embedding;
        
        // Parse additional metadata
        size_t embedding_size = sizeof(ImageEmbedding);
        if (node->payload_len > embedding_size) {
            const char* metadata_start = reinterpret_cast<const char*>(node->payload + embedding_size);
            payload.filename = std::string(metadata_start);
        }
        
        return payload;
    }
    
    // Parse sensor payload
    static SensorNodePayload parse_sensor_payload(const NodeRec* node) {
        SensorNodePayload payload;
        if (!node->is_sensor() || node->payload_len < sizeof(SensorReading)) {
            return payload;
        }
        
        // Deserialize sensor reading
        const SensorReading* reading = reinterpret_cast<const SensorReading*>(node->payload);
        payload.reading = *reading;
        
        return payload;
    }
    
    // Parse thought payload
    static ThoughtNodePayload parse_thought_payload(const NodeRec* node) {
        ThoughtNodePayload payload;
        if (!node->is_thought_node()) {
            return payload;
        }
        
        // Deserialize thought path
        const ThoughtPath* path = reinterpret_cast<const ThoughtPath*>(node->payload);
        payload.path = *path;
        
        return payload;
    }
};

// ==================== UTILITY FUNCTIONS ====================

// Create a taught node for text
NodeRec* create_text_taught_node(uint32_t id, const std::string& text);

// Create a taught node for audio
NodeRec* create_audio_taught_node(uint32_t id, const AudioFrame& frame, const std::string& transcript = "");

// Create a taught node for image
NodeRec* create_image_taught_node(uint32_t id, const ImageEmbedding& embedding, const std::string& filename = "");

// Create a taught node for sensor data
NodeRec* create_sensor_taught_node(uint32_t id, const SensorReading& reading, const std::string& interpretation = "");

// Create a thought node
NodeRec* create_thought_node(uint32_t id, const ThoughtPath& path, const std::string& description = "");

// Create a connection wrapper
NodeRec* create_connection_wrapper(uint32_t id, uint32_t node_a, uint32_t node_b, 
                                   RelationType relation, float weight = 1.0f);

// Serialize node to binary format for append-only log
std::vector<uint8_t> serialize_node(const NodeRec* node);

// Deserialize node from binary format
NodeRec* deserialize_node(const uint8_t* data, size_t data_len);

} // namespace melvin
