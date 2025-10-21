#pragma once

#include "../audio/audio_pipeline.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace melvin {
namespace logging {

/**
 * Audio Event Logger
 * 
 * Provides formatted logging for audio events
 */
class AudioLogger {
public:
    /**
     * Log an audio event to stdout
     */
    static void log_audio_event(const audio::AudioEvent& ev) {
        std::cout << format_audio_event(ev) << std::endl;
    }
    
    /**
     * Format an audio event as a string
     */
    static std::string format_audio_event(const audio::AudioEvent& ev) {
        std::ostringstream oss;
        
        // Type icon
        std::string icon = ev.type == "speech" ? "🎤" :
                          ev.type == "ambient" ? "🔊" :
                          ev.type == "system" ? "⚙️" : "🎵";
        
        // Format: [TIME] ICON [TYPE] "label" (confidence)
        oss << "[" << std::fixed << std::setprecision(2) << ev.timestamp << "s] "
            << icon << " [" << ev.type << "] "
            << "\"" << ev.label << "\" "
            << "(conf: " << std::setprecision(2) << ev.confidence << ")";
        
        return oss.str();
    }
    
    /**
     * Log audio event with context
     */
    static void log_with_context(const audio::AudioEvent& ev, const std::string& context) {
        std::cout << format_audio_event(ev) << " | " << context << std::endl;
    }
    
    /**
     * Log cross-modal connection
     */
    static void log_cross_modal(const std::string& audio_label, 
                                const std::string& visual_label,
                                float time_diff) {
        std::cout << "🔗 Cross-modal: audio:\"" << audio_label 
                  << "\" ↔ vision:\"" << visual_label 
                  << "\" (Δt=" << time_diff << "s)" << std::endl;
    }
    
    /**
     * Log graph integration
     */
    static void log_graph_integration(const audio::AudioEvent& ev, 
                                      size_t nodes_created, 
                                      size_t edges_created) {
        std::cout << "  ↳ 📊 Graph: +" << nodes_created << " nodes, +"
                  << edges_created << " edges" << std::endl;
    }
};

// Convenience functions
inline void log_audio_event(const audio::AudioEvent& ev) {
    AudioLogger::log_audio_event(ev);
}

inline void log_cross_modal(const std::string& audio, const std::string& visual, float dt) {
    AudioLogger::log_cross_modal(audio, visual, dt);
}

} // namespace logging
} // namespace melvin

