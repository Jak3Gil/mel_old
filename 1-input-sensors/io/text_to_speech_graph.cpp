#include "text_to_speech_graph.h"
#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>

namespace melvin {
namespace io {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

TextToSpeechGraph::TextToSpeechGraph(SpeechIntent& speech_intent)
    : TextToSpeechGraph(speech_intent, Config()) {
}

TextToSpeechGraph::TextToSpeechGraph(SpeechIntent& speech_intent, const Config& config)
    : speech_intent_(speech_intent)
    , config_(config) {
    
    std::cout << "🎙️ TextToSpeechGraph initialized (cognitive TTS)" << std::endl;
}

TextToSpeechGraph::~TextToSpeechGraph() {
}

// ============================================================================
// SPEECH GENERATION
// ============================================================================

bool TextToSpeechGraph::speak(const std::string& text, uint64_t concept_root) {
    // Step 1: Create graph representation
    uint64_t speech_id = speech_intent_.process_output(text, concept_root);
    
    // Step 2: Execute TTS
    bool success = execute_tts(text);
    
    // Step 3: Mark as spoken
    if (success) {
        speech_intent_.mark_spoken(speech_id);
        speech_count_++;
    }
    
    return success;
}

bool TextToSpeechGraph::speak_with_cause(const std::string& text, uint64_t cause_node) {
    // Step 1: Create graph representation with cause
    uint64_t speech_id = speech_intent_.process_with_cause(text, cause_node);
    
    // Step 2: Execute TTS
    bool success = execute_tts(text);
    
    // Step 3: Mark as spoken
    if (success) {
        speech_intent_.mark_spoken(speech_id);
        speech_count_++;
    }
    
    return success;
}

bool TextToSpeechGraph::speak_and_monitor(const std::string& text, 
                                         audio::AudioPipeline* audio_pipeline) {
    // Speak normally
    bool success = speak(text);
    
    if (!success || !config_.enable_self_recognition || !audio_pipeline) {
        return success;
    }
    
    // Wait for speech delay (allow TTS to start)
    std::this_thread::sleep_for(
        std::chrono::milliseconds(static_cast<int>(config_.speech_delay * 1000))
    );
    
    // Monitor for self-recognition
    // (In production, AudioPipeline would detect Melvin's own voice)
    std::cout << "   🎧 Monitoring for self-recognition..." << std::endl;
    
    return success;
}

// ============================================================================
// TTS EXECUTION
// ============================================================================

bool TextToSpeechGraph::execute_tts(const std::string& text) {
    std::cout << "\n🎙️ Melvin speaking: \"" << text << "\"" << std::endl;
    
    // Write to output file (for Python TTS to read)
    write_speech_output(text);
    
    // Try system TTS first (fastest)
    if (call_system_tts(text)) {
        return true;
    }
    
    // Fallback to Python TTS
    if (call_python_tts(text)) {
        return true;
    }
    
    // Fallback to print
    std::cout << "   [SPEAK] " << text << std::endl;
    return false;
}

void TextToSpeechGraph::write_speech_output(const std::string& text) {
    std::ofstream ofs(config_.output_file);
    if (ofs.is_open()) {
        ofs << text;
        ofs.close();
    }
}

// ============================================================================
// TTS BACKEND CALLS
// ============================================================================

bool TextToSpeechGraph::call_system_tts(const std::string& text) {
    // Try macOS 'say' command
    #ifdef __APPLE__
    std::string command = "say \"" + text + "\"";
    if (config_.blocking) {
        command += " 2>/dev/null";
    } else {
        command += " &>/dev/null &";
    }
    
    int result = std::system(command.c_str());
    return result == 0;
    #endif
    
    // Try Linux espeak
    #ifdef __linux__
    std::string command = "espeak \"" + text + "\"";
    if (!config_.blocking) {
        command += " &";
    }
    
    int result = std::system(command.c_str());
    return result == 0;
    #endif
    
    return false;
}

bool TextToSpeechGraph::call_python_tts(const std::string& text) {
    std::string command = "python3 -c \"from " + config_.tts_script + 
                         " import speak; speak('" + text + "')\" 2>/dev/null";
    
    if (!config_.blocking) {
        command += " &";
    }
    
    int result = std::system(command.c_str());
    return result == 0;
}

// ============================================================================
// STATISTICS
// ============================================================================

void TextToSpeechGraph::print_stats() const {
    std::cout << "\n📊 TextToSpeechGraph Statistics:" << std::endl;
    std::cout << "   Total speech outputs: " << speech_count_ << std::endl;
}

} // namespace io
} // namespace melvin

