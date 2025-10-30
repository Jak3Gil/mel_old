/**
 * Audio Node - Represents audio patterns learned from Whisper and TTS
 * 
 * These nodes live alongside semantic concepts in the unified graph.
 * Over time, MELVIN learns to generate audio directly from concept activations.
 */

#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

namespace melvin {
namespace audio {

enum class AudioNodeType {
    PHONEME,          // Individual sound unit: /h/, /ɛ/, /l/, /oʊ/
    WORD_AUDIO,       // Complete word audio pattern
    PROSODY,          // Pitch contour, rhythm, stress pattern
    MEL_SPECTROGRAM,  // Raw audio features (80-dim mel bins)
    WHISPER_EMBEDDING // High-level audio embedding from Whisper encoder
};

struct AudioNode {
    uint64_t node_id;
    AudioNodeType type;
    
    // Audio features
    std::vector<float> audio_embedding;    // High-level representation (512-1024 dim)
    std::vector<float> mel_features;       // Mel spectrogram (80 bins, averaged over time)
    std::string phoneme_sequence;          // e.g., "HH EH L OW" for "hello"
    
    // Temporal properties
    float duration_ms;                     // How long this audio lasts
    float pitch_mean;                      // Average F0 in Hz
    float energy_mean;                     // Average RMS energy
    
    // Semantic links (bidirectional with concept nodes)
    std::unordered_map<uint64_t, float> linked_concepts;  // concept_id → association strength
    
    // Learning metadata
    uint32_t co_activation_count;          // How many times heard/spoken
    float confidence;                      // How confident in this association (0-1)
    uint64_t timestamp_us;                 // When this was learned
    
    AudioNode() 
        : node_id(0), type(AudioNodeType::PHONEME), 
          duration_ms(0.0f), pitch_mean(0.0f), energy_mean(0.0f),
          co_activation_count(0), confidence(0.0f), timestamp_us(0) {}
};

/**
 * AudioSemanticLink - Represents learned association between audio and meaning
 */
struct AudioSemanticLink {
    uint64_t audio_node_id;
    uint64_t concept_node_id;
    float association_strength;      // 0-1, increased via STDP-like learning
    float temporal_correlation;      // How often they co-activate
    uint32_t co_activation_count;    // Number of times they activated together
    
    AudioSemanticLink()
        : audio_node_id(0), concept_node_id(0),
          association_strength(0.0f), temporal_correlation(0.0f),
          co_activation_count(0) {}
};

/**
 * PhonemePattern - Represents a learned phoneme sequence
 */
struct PhonemePattern {
    std::string phoneme_sequence;    // e.g., "HH EH L OW"
    std::vector<float> mel_template; // Average mel spectrogram for this pattern
    std::vector<uint64_t> word_concepts; // Concepts that use this phoneme pattern
    float confidence;                // How well-learned this pattern is
    
    PhonemePattern() : confidence(0.0f) {}
};

} // namespace audio
} // namespace melvin

