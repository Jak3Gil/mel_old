#pragma once

#include "phoneme_graph.h"
#include <vector>
#include <string>

namespace melvin {
namespace audio {

/**
 * VocalEngine - Biological-style speech synthesis
 * 
 * Simulates human vocal tract to generate speech from phonemes:
 * - Glottal source (vocal cords)
 * - Formant filtering (vocal tract resonances)
 * - Coarticulation (smooth transitions)
 * - Prosody (pitch, rhythm, emphasis)
 * 
 * Unlike traditional TTS, this:
 * - Generates speech from first principles
 * - Learns new sounds dynamically
 * - Adapts pronunciation based on phoneme graph
 * - Creates consistent voice timbre
 */
class VocalEngine {
public:
    /**
     * Configuration for vocal synthesis
     */
    struct Config {
        float base_pitch = 120.0f;        // Base frequency (Hz)
        float pitch_variance = 20.0f;     // Natural pitch variation
        float sample_rate = 22050.0f;     // Output sample rate
        float breathiness = 0.1f;         // Glottal noise (0.0 to 1.0)
        float formant_bandwidth = 50.0f;  // Formant filter width
        bool enable_coarticulation = true;
        float transition_smoothing = 0.3f; // Phoneme blend factor
        
        // Voice characteristics
        float vocal_tract_length = 17.0f; // cm (affects formants)
        float nasality_base = 0.05f;      // Base nasal resonance
    };
    
    VocalEngine();
    explicit VocalEngine(const Config& config);
    explicit VocalEngine(PhonemeGraph& phoneme_graph);
    VocalEngine(PhonemeGraph& phoneme_graph, const Config& config);
    ~VocalEngine();
    
    // ========================================================================
    // SPEECH SYNTHESIS
    // ========================================================================
    
    /**
     * Speak text using vocal synthesis
     * 
     * @param text Text to speak
     * @return Audio samples
     */
    std::vector<float> speak(const std::string& text);
    
    /**
     * Speak text and output to file
     * 
     * @param text Text to speak
     * @param output_file WAV file path
     * @return true if successful
     */
    bool speak_to_file(const std::string& text, const std::string& output_file);
    
    /**
     * Speak text and play through speakers
     * 
     * @param text Text to speak
     * @param blocking Wait for completion
     * @return true if successful
     */
    bool speak_and_play(const std::string& text, bool blocking = true);
    
    // ========================================================================
    // PHONEME SYNTHESIS
    // ========================================================================
    
    /**
     * Synthesize a single phoneme
     * 
     * @param phoneme Phoneme to synthesize
     * @return Audio samples
     */
    std::vector<float> synthesize_phoneme(const PhonemeNode& phoneme);
    
    /**
     * Synthesize phoneme sequence with coarticulation
     * 
     * @param phonemes Phoneme sequence
     * @return Audio samples
     */
    std::vector<float> synthesize_sequence(const std::vector<PhonemeNode>& phonemes);
    
    // ========================================================================
    // LEARNING
    // ========================================================================
    
    /**
     * Learn phoneme from audio sample
     * 
     * @param symbol Phoneme symbol
     * @param samples Audio samples
     * @param sample_rate Sample rate (Hz)
     */
    void learn_phoneme(const std::string& symbol,
                      const std::vector<float>& samples,
                      uint32_t sample_rate = 16000);
    
    /**
     * Extract formants from audio
     * 
     * @param samples Audio samples
     * @param sample_rate Sample rate
     * @return Formant frequencies [F1, F2, F3]
     */
    std::vector<float> extract_formants(const std::vector<float>& samples,
                                       uint32_t sample_rate);
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void set_pitch(float hz) { config_.base_pitch = hz; }
    void set_breathiness(float value) { config_.breathiness = value; }
    void set_vocal_tract_length(float cm) { config_.vocal_tract_length = cm; }
    
    const Config& get_config() const { return config_; }
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    size_t get_synthesis_count() const { return synthesis_count_; }
    
    void print_stats() const;
    
private:
    Config config_;
    PhonemeGraph* phoneme_graph_;
    bool owns_graph_;
    
    size_t synthesis_count_ = 0;
    
    // Synthesis components
    std::vector<float> generate_glottal_source(float pitch, size_t num_samples);
    std::vector<float> apply_formant_filter(const std::vector<float>& source,
                                           float f1, float f2, float f3);
    std::vector<float> apply_amplitude_envelope(const std::vector<float>& audio,
                                               const std::vector<float>& envelope);
    std::vector<float> smooth_transition(const std::vector<float>& audio1,
                                        const std::vector<float>& audio2,
                                        float blend_factor);
    
    // Audio utilities
    bool write_wav_file(const std::string& filename,
                       const std::vector<float>& samples,
                       uint32_t sample_rate);
    bool play_audio(const std::vector<float>& samples, bool blocking);
};

} // namespace audio
} // namespace melvin

