#pragma once

#include "speech_intent.h"
#include "../core/atomic_graph.h"
#include <string>
#include <fstream>

namespace melvin {

// Forward declarations
namespace audio {
    class AudioPipeline;
}

namespace io {

/**
 * TextToSpeechGraph - Bridge between cognitive speech and TTS output
 * 
 * Responsibilities:
 * - Convert graph-based speech intent to actual audio
 * - Record speech output in shared file for Python TTS
 * - Create feedback loop (speech output → audio input)
 * - Enable self-recognition
 */
class TextToSpeechGraph {
public:
    /**
     * Configuration for TTS graph bridge
     */
    struct Config {
        std::string output_file = "melvin_last_speech.txt";
        std::string tts_script = "melvin/io/text_to_speech.py";
        bool enable_self_recognition = true;
        bool blocking = true;  // Wait for speech to complete
        float speech_delay = 0.5f;  // Delay before self-recognition (seconds)
    };
    
    TextToSpeechGraph(SpeechIntent& speech_intent);
    explicit TextToSpeechGraph(SpeechIntent& speech_intent, const Config& config);
    ~TextToSpeechGraph();
    
    // ========================================================================
    // SPEECH GENERATION
    // ========================================================================
    
    /**
     * Speak text and create graph nodes
     * 
     * @param text Text to speak
     * @param concept_root Optional concept that prompted this speech
     * @return true if successful
     */
    bool speak(const std::string& text, uint64_t concept_root = 0);
    
    /**
     * Speak with explicit cause
     * 
     * @param text Text to speak
     * @param cause_node Node that caused this utterance
     * @return true if successful
     */
    bool speak_with_cause(const std::string& text, uint64_t cause_node);
    
    /**
     * Speak and wait for self-recognition
     * 
     * @param text Text to speak
     * @param audio_pipeline Pipeline to monitor for self-recognition
     * @return true if successful
     */
    bool speak_and_monitor(const std::string& text, 
                          audio::AudioPipeline* audio_pipeline = nullptr);
    
    // ========================================================================
    // TTS EXECUTION
    // ========================================================================
    
    /**
     * Execute TTS system (calls Python or system command)
     * 
     * @param text Text to synthesize
     * @return true if successful
     */
    bool execute_tts(const std::string& text);
    
    /**
     * Write text to output file for Python TTS
     * 
     * @param text Text to write
     */
    void write_speech_output(const std::string& text);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    size_t get_speech_count() const { return speech_count_; }
    
    void print_stats() const;
    
private:
    SpeechIntent& speech_intent_;
    Config config_;
    
    size_t speech_count_ = 0;
    
    // Helper methods
    bool call_python_tts(const std::string& text);
    bool call_system_tts(const std::string& text);
};

} // namespace io
} // namespace melvin

