/**
 * Vocal Synthesis - Simulates human vocal cord production
 * 
 * MELVIN learns to control virtual vocal cords to generate his own unique voice.
 * Instead of replaying learned audio, he produces speech by controlling:
 * - Glottal source (pitch, voicing)
 * - Vocal tract (formants)
 * - Articulation (phoneme transitions)
 */

#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <cmath>

namespace melvin {
namespace audio {

// ============================================================
// FORMANT STRUCTURE
// ============================================================

/**
 * Single formant (vocal tract resonance)
 */
struct Formant {
    float frequency;    // Center frequency in Hz
    float bandwidth;    // Bandwidth in Hz (damping)
    float amplitude;    // Relative amplitude 0-1
    
    Formant(float f = 500.0f, float b = 50.0f, float a = 1.0f)
        : frequency(f), bandwidth(b), amplitude(a) {}
};

// ============================================================
// VOCAL CONFIGURATION
// ============================================================

/**
 * Complete vocal tract configuration for speech production
 */
struct VocalConfiguration {
    // Glottal source (voice generation)
    float f0;                      // Fundamental frequency (pitch) in Hz
    float voicing_strength;        // 0 = whisper, 1 = clear voice
    float aspiration_level;        // Breathiness 0-1
    float open_quotient;           // Fraction of pitch cycle glottis is open (0.3-0.7)
    
    // Vocal tract formants (F1-F5)
    std::vector<Formant> formants;
    
    // Articulation modifiers
    float nasal_coupling;          // Nasal resonance 0-1
    float lip_radiation;           // High-frequency boost 0-1
    
    // Prosody
    float duration_ms;             // How long to sustain this config
    float amplitude;               // Loudness 0-1
    
    VocalConfiguration();
    
    /**
     * Get typical vocal configuration for a phoneme
     * Uses IPA-like phoneme codes: AA, IY, UW, SH, etc.
     */
    static VocalConfiguration for_phoneme(const std::string& phoneme);
    
    /**
     * Interpolate between two configurations (for smooth transitions)
     */
    static VocalConfiguration interpolate(
        const VocalConfiguration& a,
        const VocalConfiguration& b,
        float t  // 0 = all a, 1 = all b
    );
};

// ============================================================
// VOCAL SYNTHESIZER
// ============================================================

/**
 * Core vocal synthesis engine using formant synthesis
 */
class VocalSynthesizer {
public:
    VocalSynthesizer(int sample_rate = 16000);
    ~VocalSynthesizer() = default;
    
    /**
     * Generate audio from a single vocal configuration
     */
    std::vector<float> synthesize(const VocalConfiguration& config);
    
    /**
     * Generate speech from sequence of phonemes with pitch contour
     */
    std::vector<float> synthesize_phonemes(
        const std::vector<std::string>& phonemes,
        const std::vector<float>& pitch_contour,
        const std::vector<float>& durations_ms
    );
    
    /**
     * Generate speech with smooth formant transitions
     */
    std::vector<float> synthesize_with_transitions(
        const std::vector<VocalConfiguration>& configs,
        float transition_time_ms = 50.0f
    );
    
    /**
     * Generate speech from text using default phoneme-to-vocal mapping
     */
    std::vector<float> synthesize_text(
        const std::string& text,
        float base_pitch = 150.0f
    );
    
    int get_sample_rate() const { return sample_rate_; }
    
private:
    int sample_rate_;
    
    /**
     * Generate glottal pulse using LF (Liljencrants-Fant) model
     */
    std::vector<float> generate_glottal_pulse(
        float f0,
        float open_quotient,
        float voicing_strength,
        float aspiration_level,
        int num_samples
    );
    
    /**
     * Apply formant filtering to source signal
     */
    std::vector<float> apply_formants(
        const std::vector<float>& source,
        const std::vector<Formant>& formants
    );
    
    /**
     * Apply lip radiation (6dB/octave high-pass)
     */
    std::vector<float> apply_lip_radiation(const std::vector<float>& signal);
    
    /**
     * Generate white noise for aspiration
     */
    std::vector<float> generate_noise(int num_samples);
    
    /**
     * Simple text to phoneme conversion (placeholder)
     */
    std::vector<std::string> text_to_phonemes(const std::string& text);
};

// ============================================================
// VOCAL PARAMETER LEARNER
// ============================================================

/**
 * Learns mapping from semantic concepts to vocal parameters
 * This is how MELVIN discovers his voice!
 */
class VocalParameterLearner {
public:
    VocalParameterLearner();
    ~VocalParameterLearner() = default;
    
    /**
     * Learn vocal parameters by analyzing TTS audio
     * Extracts F0, formants, voicing from audio + associates with concepts
     */
    void learn_from_tts(
        const std::string& text,
        const std::vector<float>& tts_audio,
        const std::vector<uint64_t>& active_concepts,
        int sample_rate = 16000
    );
    
    /**
     * Learn from self-generated speech + feedback
     * Reinforcement learning: adjust params based on similarity to target
     */
    void learn_from_practice(
        const VocalConfiguration& attempted,
        const VocalConfiguration& target,
        float similarity_score,
        const std::vector<uint64_t>& active_concepts
    );
    
    /**
     * Retrieve learned vocal configuration for active concepts
     */
    VocalConfiguration get_config_for_concepts(
        const std::vector<uint64_t>& concept_ids
    );
    
    /**
     * Check if confident enough to self-generate for these concepts
     */
    bool can_self_generate(
        const std::vector<uint64_t>& concept_ids,
        float confidence_threshold = 0.7f
    );
    
    /**
     * Get learning statistics
     */
    struct LearningStats {
        size_t total_vocal_configs;
        size_t total_concept_mappings;
        float average_confidence;
        size_t tts_samples_observed;
        size_t practice_attempts;
    };
    LearningStats get_stats() const;
    
private:
    // Concept → Vocal Configuration mapping
    std::unordered_map<uint64_t, VocalConfiguration> concept_to_vocal_;
    std::unordered_map<uint64_t, float> vocal_confidence_;  // 0-1
    
    // Learning statistics
    size_t tts_samples_observed_;
    size_t practice_attempts_;
    
    /**
     * Extract vocal parameters from audio
     * Simplified version - estimates F0 and formants
     */
    VocalConfiguration extract_parameters_from_audio(
        const std::vector<float>& audio,
        int sample_rate
    );
    
    /**
     * STDP-like strengthening of concept → vocal mapping
     */
    void strengthen_association(
        uint64_t concept_id,
        const VocalConfiguration& config,
        float learning_rate = 0.02f
    );
    
    /**
     * Estimate pitch (F0) from audio
     */
    float estimate_pitch(const std::vector<float>& audio, int sample_rate);
    
    /**
     * Estimate formant frequencies from audio spectrum
     */
    std::vector<Formant> estimate_formants(
        const std::vector<float>& audio,
        int sample_rate
    );
};

// ============================================================
// HYBRID VOCAL GENERATOR
// ============================================================

/**
 * Manages transition from TTS to self-generated speech
 * Like training wheels on a bike!
 */
class HybridVocalGenerator {
public:
    enum Mode {
        PURE_TTS,           // 100% TTS (early learning, 0-50 convos)
        MOSTLY_TTS,         // 80% TTS, 20% self (50-150 convos)
        BALANCED,           // 50% TTS, 50% self (150-300 convos)
        MOSTLY_SELF,        // 20% TTS, 80% self (300-500 convos)
        PURE_SELF           // 100% self (500+ convos, mastery!)
    };
    
    HybridVocalGenerator(
        VocalSynthesizer& synthesizer,
        VocalParameterLearner& learner
    );
    
    /**
     * Generate speech, blending TTS and self-generated
     */
    std::vector<float> generate_speech(
        const std::string& text,
        const std::vector<float>& tts_audio,
        const std::vector<uint64_t>& active_concepts,
        Mode mode
    );
    
    /**
     * Automatically determine mode based on confidence
     */
    Mode auto_select_mode(
        const std::vector<uint64_t>& concept_ids,
        size_t conversation_count
    );
    
    /**
     * Get current proficiency (0-1)
     */
    float get_proficiency(const std::vector<uint64_t>& concept_ids);
    
private:
    VocalSynthesizer& synthesizer_;
    VocalParameterLearner& learner_;
    
    /**
     * Blend TTS and self-generated audio
     */
    std::vector<float> blend_audio(
        const std::vector<float>& tts_audio,
        const std::vector<float>& self_audio,
        float tts_weight
    );
    
    /**
     * Get TTS weight based on mode
     */
    float get_tts_weight(Mode mode);
};

} // namespace audio
} // namespace melvin

