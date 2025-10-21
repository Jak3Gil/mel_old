#include "vocal_engine.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <cstdlib>

namespace melvin {
namespace audio {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

VocalEngine::VocalEngine()
    : VocalEngine(Config()) {
}

VocalEngine::VocalEngine(const Config& config)
    : config_(config)
    , phoneme_graph_(new PhonemeGraph())
    , owns_graph_(true) {
    
    std::cout << "🎙️ VocalEngine initialized (biological-style synthesis)" << std::endl;
    std::cout << "   Base pitch: " << config_.base_pitch << " Hz" << std::endl;
    std::cout << "   Sample rate: " << config_.sample_rate << " Hz" << std::endl;
}

VocalEngine::VocalEngine(PhonemeGraph& phoneme_graph)
    : VocalEngine(phoneme_graph, Config()) {
}

VocalEngine::VocalEngine(PhonemeGraph& phoneme_graph, const Config& config)
    : config_(config)
    , phoneme_graph_(&phoneme_graph)
    , owns_graph_(false) {
    
    std::cout << "🎙️ VocalEngine initialized (with existing phoneme graph)" << std::endl;
}

VocalEngine::~VocalEngine() {
    if (owns_graph_) {
        delete phoneme_graph_;
    }
}

// ============================================================================
// SPEECH SYNTHESIS
// ============================================================================

std::vector<float> VocalEngine::speak(const std::string& text) {
    std::cout << "\n🎙️ Synthesizing: \"" << text << "\"" << std::endl;
    
    // Get phoneme sequence
    auto phonemes = phoneme_graph_->get_sequence(text);
    
    if (phonemes.empty()) {
        std::cerr << "⚠️  No phonemes found for: " << text << std::endl;
        return {};
    }
    
    std::cout << "   Phoneme sequence: " << phonemes.size() << " phonemes" << std::endl;
    
    // Synthesize with coarticulation
    auto audio = synthesize_sequence(phonemes);
    
    synthesis_count_++;
    
    std::cout << "   ✅ Generated " << audio.size() << " samples ("
              << audio.size() / config_.sample_rate << " seconds)" << std::endl;
    
    return audio;
}

bool VocalEngine::speak_to_file(const std::string& text, const std::string& output_file) {
    auto audio = speak(text);
    
    if (audio.empty()) {
        return false;
    }
    
    return write_wav_file(output_file, audio, config_.sample_rate);
}

bool VocalEngine::speak_and_play(const std::string& text, bool blocking) {
    auto audio = speak(text);
    
    if (audio.empty()) {
        return false;
    }
    
    return play_audio(audio, blocking);
}

// ============================================================================
// PHONEME SYNTHESIS
// ============================================================================

std::vector<float> VocalEngine::synthesize_phoneme(const PhonemeNode& phoneme) {
    // Calculate number of samples for this phoneme
    size_t num_samples = static_cast<size_t>(
        (phoneme.duration_ms / 1000.0f) * config_.sample_rate
    );
    
    // Generate glottal source
    float pitch = config_.base_pitch;
    auto source = generate_glottal_source(pitch, num_samples);
    
    // Apply formant filtering (vocal tract resonances)
    auto filtered = apply_formant_filter(source, 
                                         phoneme.formants[0],
                                         phoneme.formants[1],
                                         phoneme.formants[2]);
    
    // Apply amplitude envelope
    if (!phoneme.envelope.empty()) {
        filtered = apply_amplitude_envelope(filtered, phoneme.envelope);
    }
    
    // Apply amplitude scaling
    for (float& sample : filtered) {
        sample *= phoneme.amplitude;
    }
    
    return filtered;
}

std::vector<float> VocalEngine::synthesize_sequence(const std::vector<PhonemeNode>& phonemes) {
    std::vector<float> result;
    
    for (size_t i = 0; i < phonemes.size(); i++) {
        // Synthesize current phoneme
        auto phoneme_audio = synthesize_phoneme(phonemes[i]);
        
        // Apply coarticulation with previous phoneme
        if (i > 0 && config_.enable_coarticulation && !result.empty()) {
            // Smooth transition between phonemes
            float blend = config_.transition_smoothing;
            size_t overlap_samples = std::min(
                static_cast<size_t>(phoneme_audio.size() * blend),
                static_cast<size_t>(200)  // Max 200 samples overlap
            );
            
            if (overlap_samples > 0 && result.size() > overlap_samples) {
                // Blend the overlap region
                for (size_t j = 0; j < overlap_samples; j++) {
                    float blend_factor = static_cast<float>(j) / overlap_samples;
                    size_t result_idx = result.size() - overlap_samples + j;
                    result[result_idx] = result[result_idx] * (1.0f - blend_factor) +
                                        phoneme_audio[j] * blend_factor;
                }
                
                // Append remaining phoneme audio
                result.insert(result.end(), 
                            phoneme_audio.begin() + overlap_samples,
                            phoneme_audio.end());
            } else {
                // No overlap, just append
                result.insert(result.end(), phoneme_audio.begin(), phoneme_audio.end());
            }
        } else {
            // First phoneme or coarticulation disabled
            result.insert(result.end(), phoneme_audio.begin(), phoneme_audio.end());
        }
    }
    
    return result;
}

// ============================================================================
// SYNTHESIS COMPONENTS
// ============================================================================

std::vector<float> VocalEngine::generate_glottal_source(float pitch, size_t num_samples) {
    std::vector<float> source(num_samples);
    
    // Generate glottal pulse train (simplified Rosenberg model)
    float samples_per_cycle = config_.sample_rate / pitch;
    
    for (size_t i = 0; i < num_samples; i++) {
        float phase = std::fmod(i, samples_per_cycle) / samples_per_cycle;
        
        // Glottal waveform (approximation)
        if (phase < 0.5f) {
            source[i] = std::sin(phase * 2.0f * M_PI);
        } else {
            source[i] = -std::sin((phase - 0.5f) * 2.0f * M_PI) * 0.5f;
        }
        
        // Add breathiness (noise)
        if (config_.breathiness > 0.0f) {
            float noise = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
            source[i] = source[i] * (1.0f - config_.breathiness) + 
                       noise * config_.breathiness;
        }
    }
    
    return source;
}

std::vector<float> VocalEngine::apply_formant_filter(const std::vector<float>& source,
                                                     float f1, float f2, float f3) {
    // Simple resonant filter approximation
    // In production, use proper formant synthesis (Klatt synthesizer, etc.)
    
    std::vector<float> output = source;
    
    // Apply resonances at formant frequencies
    // This is a simplified version - real formant synthesis is more complex
    
    // For now, just apply amplitude modulation based on formants
    // (Placeholder for proper formant filtering)
    
    for (size_t i = 0; i < output.size(); i++) {
        float t = static_cast<float>(i) / config_.sample_rate;
        
        // Add formant resonances (very simplified)
        float resonance1 = std::sin(2.0f * M_PI * f1 * t) * 0.3f;
        float resonance2 = std::sin(2.0f * M_PI * f2 * t) * 0.2f;
        float resonance3 = std::sin(2.0f * M_PI * f3 * t) * 0.1f;
        
        output[i] = output[i] * (1.0f + resonance1 + resonance2 + resonance3);
        
        // Normalize
        output[i] = std::max(-1.0f, std::min(1.0f, output[i]));
    }
    
    return output;
}

std::vector<float> VocalEngine::apply_amplitude_envelope(const std::vector<float>& audio,
                                                        const std::vector<float>& envelope) {
    std::vector<float> output = audio;
    
    for (size_t i = 0; i < output.size() && i < envelope.size(); i++) {
        output[i] *= envelope[i];
    }
    
    return output;
}

std::vector<float> VocalEngine::smooth_transition(const std::vector<float>& audio1,
                                                  const std::vector<float>& audio2,
                                                  float blend_factor) {
    size_t overlap = std::min(audio1.size(), audio2.size());
    overlap = static_cast<size_t>(overlap * blend_factor);
    
    std::vector<float> result = audio1;
    
    for (size_t i = 0; i < overlap; i++) {
        float factor = static_cast<float>(i) / overlap;
        size_t idx = audio1.size() - overlap + i;
        result[idx] = result[idx] * (1.0f - factor) + audio2[i] * factor;
    }
    
    result.insert(result.end(), audio2.begin() + overlap, audio2.end());
    
    return result;
}

// ============================================================================
// LEARNING
// ============================================================================

void VocalEngine::learn_phoneme(const std::string& symbol,
                               const std::vector<float>& samples,
                               uint32_t sample_rate) {
    std::cout << "🎓 Learning phoneme: " << symbol << std::endl;
    
    // Extract formants from audio
    auto formants = extract_formants(samples, sample_rate);
    
    if (formants.size() >= 3) {
        // Create phoneme node
        PhonemeNode node;
        node.symbol = symbol;
        node.formants[0] = formants[0];
        node.formants[1] = formants[1];
        node.formants[2] = formants[2];
        node.duration_ms = (samples.size() / static_cast<float>(sample_rate)) * 1000.0f;
        
        // Add to graph
        phoneme_graph_->add_phoneme(symbol, node);
        
        std::cout << "   ✅ Learned: F1=" << formants[0] 
                  << " F2=" << formants[1] 
                  << " F3=" << formants[2] << " Hz" << std::endl;
    }
}

std::vector<float> VocalEngine::extract_formants(const std::vector<float>& samples,
                                                 uint32_t sample_rate) {
    // Placeholder for formant extraction
    // In production, use:
    // - LPC (Linear Predictive Coding)
    // - Praat formant tracker
    // - WORLD vocoder
    
    // Return default formants for now
    return {500.0f, 1500.0f, 2500.0f};
}

// ============================================================================
// AUDIO UTILITIES
// ============================================================================

bool VocalEngine::write_wav_file(const std::string& filename,
                                 const std::vector<float>& samples,
                                 uint32_t sample_rate) {
    // Simple WAV file writer
    std::ofstream file(filename, std::ios::binary);
    
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << filename << std::endl;
        return false;
    }
    
    // WAV header
    uint32_t data_size = samples.size() * sizeof(int16_t);
    uint32_t file_size = 36 + data_size;
    uint16_t channels = 1;
    uint16_t bits_per_sample = 16;
    uint32_t byte_rate = sample_rate * channels * (bits_per_sample / 8);
    uint16_t block_align = channels * (bits_per_sample / 8);
    
    // Write RIFF header
    file.write("RIFF", 4);
    file.write(reinterpret_cast<const char*>(&file_size), 4);
    file.write("WAVE", 4);
    
    // Write fmt chunk
    file.write("fmt ", 4);
    uint32_t fmt_size = 16;
    uint16_t audio_format = 1; // PCM
    file.write(reinterpret_cast<const char*>(&fmt_size), 4);
    file.write(reinterpret_cast<const char*>(&audio_format), 2);
    file.write(reinterpret_cast<const char*>(&channels), 2);
    file.write(reinterpret_cast<const char*>(&sample_rate), 4);
    file.write(reinterpret_cast<const char*>(&byte_rate), 4);
    file.write(reinterpret_cast<const char*>(&block_align), 2);
    file.write(reinterpret_cast<const char*>(&bits_per_sample), 2);
    
    // Write data chunk
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&data_size), 4);
    
    // Convert float samples to int16
    for (float sample : samples) {
        int16_t value = static_cast<int16_t>(sample * 32767.0f);
        file.write(reinterpret_cast<const char*>(&value), 2);
    }
    
    file.close();
    
    std::cout << "💾 Saved to: " << filename << std::endl;
    
    return true;
}

bool VocalEngine::play_audio(const std::vector<float>& samples, bool blocking) {
    // Save to temp file
    std::string temp_file = "/tmp/melvin_voice.wav";
    
    if (!write_wav_file(temp_file, samples, config_.sample_rate)) {
        return false;
    }
    
    // Play using system command
    #ifdef __APPLE__
    std::string command = "afplay " + temp_file;
    if (!blocking) {
        command += " &";
    }
    int result = std::system(command.c_str());
    return result == 0;
    #endif
    
    #ifdef __linux__
    std::string command = "aplay " + temp_file;
    if (!blocking) {
        command += " &";
    }
    int result = std::system(command.c_str());
    return result == 0;
    #endif
    
    return false;
}

// ============================================================================
// STATISTICS
// ============================================================================

void VocalEngine::print_stats() const {
    std::cout << "\n📊 VocalEngine Statistics:" << std::endl;
    std::cout << "   Total syntheses: " << synthesis_count_ << std::endl;
    std::cout << "   Base pitch: " << config_.base_pitch << " Hz" << std::endl;
    std::cout << "   Breathiness: " << config_.breathiness << std::endl;
    std::cout << "   Coarticulation: " << (config_.enable_coarticulation ? "enabled" : "disabled") << std::endl;
    
    if (phoneme_graph_) {
        phoneme_graph_->print_stats();
    }
}

} // namespace audio
} // namespace melvin

