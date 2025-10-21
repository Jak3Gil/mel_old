/*
 * Audio Input Adapter (Whisper Integration - Stub)
 */

#include "../core/types.h"
#include <string>

namespace melvin {
namespace io {

/**
 * Process audio input via Whisper (optional)
 * 
 * To enable: Install whisper.cpp and link against it
 * For now, this is a stub that returns an error
 */
Query process_audio_input(const std::string& audio_path) {
    Query query;
    query.text = "[Audio processing not enabled]";
    query.metadata["modality"] = "audio";
    query.metadata["audio_path"] = audio_path;
    
    // TODO: Integrate whisper.cpp for speech-to-text
    // TODO: Extract audio embeddings
    
    return query;
}

} // namespace io
} // namespace melvin

