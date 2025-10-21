#pragma once

#include "../core/atomic_graph.h"
#include <vector>
#include <string>
#include <cstdint>

namespace melvin {
namespace audio {

/**
 * AudioToken - A chunk of audio represented as a graph node
 * 
 * Instead of converting audio to text, we store the audio features directly.
 * Each token represents a distinctive sound pattern (like a "word" of audio).
 */
struct AudioToken {
    uint64_t id;                    // Node ID in graph
    std::vector<float> features;    // Audio features (MFCC, mel spectrogram, etc.)
    float duration_ms;              // Length of audio chunk
    float timestamp;                // When it occurred
    
    // Audio signature (for matching)
    float energy;                   // RMS energy
    float pitch;                    // Dominant frequency
    float spectral_centroid;        // Brightness
    
    AudioToken() : id(0), duration_ms(0), timestamp(0), 
                   energy(0), pitch(0), spectral_centroid(0) {}
};

/**
 * AudioTokenizer - Converts raw audio into graph-native tokens
 * 
 * Philosophy:
 * - Audio IS the language (not text)
 * - Sound patterns are the vocabulary
 * - Temporal sequences are the grammar
 * - Graph learns audio→audio connections directly
 * 
 * No APIs, no text conversion, pure audio cognition.
 */
class AudioTokenizer {
public:
    /**
     * Configuration for audio tokenization
     */
    struct Config {
        size_t frame_size_ms = 100;          // Size of each audio token (ms)
        size_t hop_size_ms = 50;             // Overlap between tokens (ms)
        uint32_t sample_rate = 16000;        // Audio sample rate
        size_t num_mfcc = 13;                // Number of MFCC coefficients
        float similarity_threshold = 0.8f;   // Threshold for matching tokens
        bool enable_deduplication = true;    // Merge similar tokens
    };
    
    AudioTokenizer();
    explicit AudioTokenizer(const Config& config);
    ~AudioTokenizer();
    
    // ========================================================================
    // TOKENIZATION
    // ========================================================================
    
    /**
     * Tokenize audio into chunks and create graph nodes
     * 
     * @param samples Raw audio samples
     * @param graph Target graph
     * @return Vector of created token IDs
     */
    std::vector<uint64_t> tokenize(const std::vector<float>& samples, 
                                   AtomicGraph& graph);
    
    /**
     * Extract single audio token from samples
     * 
     * @param samples Audio samples
     * @param start_idx Start index
     * @param end_idx End index
     * @return Audio token with features
     */
    AudioToken extract_token(const std::vector<float>& samples,
                            size_t start_idx,
                            size_t end_idx);
    
    /**
     * Create graph node from audio token
     * 
     * @param token Audio token
     * @param graph Target graph
     * @return Node ID
     */
    uint64_t create_node(const AudioToken& token, AtomicGraph& graph);
    
    // ========================================================================
    // FEATURE EXTRACTION
    // ========================================================================
    
    /**
     * Extract acoustic features from audio chunk
     * 
     * @param samples Audio samples
     * @return Feature vector (MFCC, spectral features, etc.)
     */
    std::vector<float> extract_features(const std::vector<float>& samples);
    
    /**
     * Extract MFCC (Mel-Frequency Cepstral Coefficients)
     * 
     * @param samples Audio samples
     * @return MFCC coefficients
     */
    std::vector<float> extract_mfcc(const std::vector<float>& samples);
    
    /**
     * Extract spectral features
     * 
     * @param samples Audio samples
     * @return Spectral features (centroid, rolloff, flux)
     */
    std::vector<float> extract_spectral_features(const std::vector<float>& samples);
    
    /**
     * Compute energy (RMS)
     */
    float compute_energy(const std::vector<float>& samples);
    
    /**
     * Estimate pitch (fundamental frequency)
     */
    float estimate_pitch(const std::vector<float>& samples);
    
    // ========================================================================
    // SIMILARITY & MATCHING
    // ========================================================================
    
    /**
     * Compute similarity between two audio tokens
     * 
     * @param token1 First token
     * @param token2 Second token
     * @return Similarity score (0.0 to 1.0)
     */
    float compute_similarity(const AudioToken& token1, const AudioToken& token2);
    
    /**
     * Find matching token in graph
     * 
     * @param token Audio token to match
     * @param graph Graph to search
     * @return Node ID of match (0 if no match)
     */
    uint64_t find_match(const AudioToken& token, AtomicGraph& graph);
    
    // ========================================================================
    // PATTERN LEARNING
    // ========================================================================
    
    /**
     * Link consecutive audio tokens (temporal sequence)
     * 
     * @param token_ids Sequence of token IDs
     * @param graph Target graph
     */
    void link_sequence(const std::vector<uint64_t>& token_ids, 
                      AtomicGraph& graph);
    
    /**
     * Find and reinforce repeated patterns
     * 
     * @param token_ids Recent tokens
     * @param graph Target graph
     */
    void learn_patterns(const std::vector<uint64_t>& token_ids,
                       AtomicGraph& graph);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    size_t get_token_count() const { return token_count_; }
    size_t get_unique_tokens() const { return unique_tokens_; }
    
    void print_stats() const;
    
private:
    Config config_;
    
    // Statistics
    size_t token_count_ = 0;
    size_t unique_tokens_ = 0;
    
    // Token cache (for deduplication)
    std::vector<AudioToken> recent_tokens_;
    
    // Helper methods
    std::string generate_token_id(const AudioToken& token);
    std::vector<float> compute_fft(const std::vector<float>& samples);
    std::vector<float> mel_filterbank(const std::vector<float>& fft);
    std::vector<float> dct(const std::vector<float>& mel);
};

} // namespace audio
} // namespace melvin

