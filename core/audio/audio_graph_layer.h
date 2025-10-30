/**
 * Audio Graph Layer - Manages audio-semantic learning
 * 
 * This layer sits alongside the semantic graph and learns bidirectional
 * mappings between audio patterns (from Whisper/TTS) and concept nodes.
 * 
 * Now includes VOCAL SYNTHESIS: MELVIN learns to control virtual vocal cords!
 * Over time, he develops his own unique voice through practice.
 */

#pragma once

#include "audio_node.h"
#include "vocal_synthesis.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace melvin {
namespace audio {

class AudioGraphLayer {
public:
    AudioGraphLayer();
    ~AudioGraphLayer() = default;
    
    // ============================================================
    // LEARNING FROM INPUT (Whisper)
    // ============================================================
    
    /**
     * Learn audio-semantic associations from user speech
     * 
     * @param transcribed_text What Whisper heard
     * @param audio_embedding Whisper encoder embedding (512-1024 dim)
     * @param mel_features Mel spectrogram features (80 bins)
     * @param activated_concept_ids Concepts activated by semantic processing
     * @param temporal_proximity How temporally close audio and concepts were (1.0 = simultaneous)
     */
    void learn_from_speech_input(
        const std::string& transcribed_text,
        const std::vector<float>& audio_embedding,
        const std::vector<float>& mel_features,
        const std::vector<uint64_t>& activated_concept_ids,
        float temporal_proximity = 1.0f
    );
    
    // ============================================================
    // LEARNING FROM OUTPUT (TTS)
    // ============================================================
    
    /**
     * Learn audio-semantic associations from MELVIN's speech
     * 
     * @param spoken_text What MELVIN said
     * @param mel_features Mel spectrogram extracted from TTS audio
     * @param concept_ids_that_triggered_speech Concepts that triggered this response
     * @param temporal_proximity How close in time (1.0 = immediate)
     */
    void learn_from_speech_output(
        const std::string& spoken_text,
        const std::vector<float>& mel_features,
        const std::vector<uint64_t>& concept_ids_that_triggered_speech,
        float temporal_proximity = 1.0f
    );
    
    // ============================================================
    // PHONEME-LEVEL LEARNING
    // ============================================================
    
    /**
     * Learn phoneme patterns from text
     * Converts text → phoneme sequence → mel features
     */
    void learn_phoneme_pattern(
        const std::string& text,
        const std::vector<float>& mel_features,
        const std::vector<uint64_t>& concept_ids
    );
    
    // ============================================================
    // AUDIO GENERATION (Future - Phase 5)
    // ============================================================
    
    /**
     * Generate audio features from concept activations
     * Returns mel spectrogram that can be converted to waveform via vocoder
     * 
     * @param concept_ids Active concepts to generate audio for
     * @param energy_threshold Minimum association strength to include
     * @return Mel spectrogram (80 bins) ready for vocoder
     */
    std::vector<float> generate_audio_from_concepts(
        const std::vector<uint64_t>& concept_ids,
        float energy_threshold = 0.5f
    );
    
    /**
     * Check if we have enough learned associations to generate audio
     */
    bool can_self_generate_audio(const std::vector<uint64_t>& concept_ids);
    
    // ============================================================
    // VOCAL SYNTHESIS (NEW! Phase 6)
    // ============================================================
    
    /**
     * Learn vocal parameters from TTS audio
     * Extracts pitch, formants, voicing and associates with concepts
     */
    void learn_vocal_parameters_from_tts(
        const std::string& text,
        const std::vector<float>& tts_audio,
        const std::vector<uint64_t>& active_concepts,
        int sample_rate = 16000
    );
    
    /**
     * Generate speech using virtual vocal cords
     * Mode determines TTS vs self-generated blend
     */
    std::vector<float> generate_with_vocal_cords(
        const std::string& text,
        const std::vector<float>& tts_audio,
        const std::vector<uint64_t>& concept_ids,
        HybridVocalGenerator::Mode mode = HybridVocalGenerator::BALANCED
    );
    
    /**
     * DUAL OUTPUT: Generate both TTS and self-generated in parallel
     * Perfect for monitoring learning progress!
     */
    struct DualOutput {
        std::vector<float> tts_audio;           // Primary (reliable)
        std::vector<float> self_generated;      // Attempt (learning)
        float similarity_score;                 // How close they are (0-1)
        float vocal_confidence;                 // Confidence in self-generated
        HybridVocalGenerator::Mode current_mode; // Current learning mode
        bool ready_for_solo;                    // true when can go pure self
    };
    
    DualOutput generate_dual_output(
        const std::string& text,
        const std::vector<float>& tts_audio,
        const std::vector<uint64_t>& concept_ids
    );
    
    /**
     * Check if MELVIN can speak independently for these concepts
     */
    bool can_speak_independently(
        const std::vector<uint64_t>& concept_ids,
        float confidence_threshold = 0.7f
    );
    
    /**
     * Get current vocal learning mode based on experience
     */
    HybridVocalGenerator::Mode get_recommended_mode(
        const std::vector<uint64_t>& concept_ids,
        size_t conversation_count
    );
    
    /**
     * Get vocal synthesis stats
     */
    struct VocalStats {
        size_t vocal_configs_learned;
        float vocal_confidence;
        size_t tts_samples_observed;
        size_t practice_attempts;
        bool can_self_generate;
        float average_similarity;  // How close self-gen is to TTS (0-1)
    };
    VocalStats get_vocal_stats() const;
    
    /**
     * Save dual output for comparison/analysis
     * @param save_tts If false, only saves self-generated (saves space!)
     */
    void save_dual_output_for_analysis(
        const DualOutput& output,
        const std::string& text,
        size_t conversation_number,
        const std::string& output_dir = "/tmp/melvin_vocal_progress",
        bool save_tts = false  // Default: only save self-generated
    );
    
    // ============================================================
    // QUERY & INSPECTION
    // ============================================================
    
    /**
     * Get audio nodes associated with a concept
     */
    std::vector<uint64_t> get_audio_for_concept(uint64_t concept_id);
    
    /**
     * Get concepts associated with an audio pattern
     */
    std::vector<uint64_t> get_concepts_for_audio(uint64_t audio_node_id);
    
    /**
     * Get association strength between audio and concept
     */
    float get_association_strength(uint64_t audio_node_id, uint64_t concept_id);
    
    /**
     * Get learning statistics
     */
    struct LearningStats {
        size_t total_audio_nodes;
        size_t total_phoneme_patterns;
        size_t total_associations;
        float average_confidence;
        size_t inputs_processed;
        size_t outputs_processed;
    };
    LearningStats get_stats() const;
    
    // ============================================================
    // PERSISTENCE
    // ============================================================
    
    void save_to_file(const std::string& filepath);
    void load_from_file(const std::string& filepath);
    
private:
    // ============================================================
    // INTERNAL DATA STRUCTURES
    // ============================================================
    
    // Audio nodes (by ID)
    std::unordered_map<uint64_t, AudioNode> audio_nodes_;
    
    // Bidirectional mappings
    std::unordered_map<uint64_t, std::vector<uint64_t>> concept_to_audio_;  // concept → audio nodes
    std::unordered_map<uint64_t, std::vector<uint64_t>> audio_to_concepts_; // audio → concepts
    
    // Association strength matrix
    std::unordered_map<uint64_t, std::unordered_map<uint64_t, float>> association_matrix_;
    
    // Phoneme patterns (text → phoneme → mel features)
    std::unordered_map<std::string, PhonemePattern> phoneme_patterns_;
    
    // Learning statistics
    size_t inputs_processed_;
    size_t outputs_processed_;
    uint64_t next_audio_node_id_;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Vocal synthesis components (Phase 6)
    VocalSynthesizer vocal_synthesizer_;
    VocalParameterLearner vocal_learner_;
    std::unique_ptr<HybridVocalGenerator> hybrid_generator_;
    
    // Dual output tracking
    std::vector<float> similarity_history_;  // Track similarity over time
    size_t conversation_count_;
    
    // ============================================================
    // INTERNAL HELPER METHODS
    // ============================================================
    
    /**
     * Create a new audio node
     */
    uint64_t create_audio_node(
        AudioNodeType type,
        const std::vector<float>& audio_embedding,
        const std::vector<float>& mel_features,
        const std::string& phoneme_sequence = ""
    );
    
    /**
     * Strengthen audio-semantic link (STDP-like)
     */
    void strengthen_link(
        uint64_t audio_node_id,
        uint64_t concept_id,
        float temporal_proximity,
        float learning_rate = 0.01f
    );
    
    /**
     * Decay weak associations over time
     */
    void decay_weak_links();
    
    /**
     * Convert text to phoneme sequence (simplified for now)
     */
    std::string text_to_phonemes(const std::string& text);
    
    /**
     * Find or create audio node for phoneme pattern
     */
    uint64_t get_or_create_phoneme_node(const std::string& phoneme_sequence);
    
    /**
     * Compute similarity between two audio signals (0-1)
     * Uses normalized cross-correlation
     */
    float compute_audio_similarity(
        const std::vector<float>& audio1,
        const std::vector<float>& audio2
    );
};

} // namespace audio
} // namespace melvin

