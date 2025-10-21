/*
 * Cross-Modal Demo - Test text + audio + image fusion (when enabled)
 */

#include "../core/melvin.h"
#include "../UCAConfig.h"
#include <iostream>

using namespace melvin;

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN CROSS-MODAL DEMO                              ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    // Create Melvin instance
    MelvinCore melvin;
    melvin.load_memory(config::STORAGE_PATH_NODES, config::STORAGE_PATH_EDGES);
    
    if (!config::ENABLE_AUDIO && !config::ENABLE_IMAGES) {
        std::cout << "⚠️  Audio and image processing are not enabled.\n";
        std::cout << "    To enable: set ENABLE_AUDIO and ENABLE_IMAGES in UCAConfig.h\n";
        std::cout << "    and install Whisper and CLIP models.\n\n";
    }
    
    // Test multimodal query
    std::cout << "Testing multimodal query...\n\n";
    
    std::string text = "What is in this scene?";
    std::string audio_path = "data/test_audio.wav";
    std::string image_path = "data/test_image.jpg";
    
    std::cout << "Text:  " << text << "\n";
    std::cout << "Audio: " << audio_path << "\n";
    std::cout << "Image: " << image_path << "\n\n";
    
    auto answer = melvin.query_multimodal(text, audio_path, image_path);
    
    std::cout << "Answer: " << answer.text << "\n";
    std::cout << "(confidence: " << answer.confidence << ")\n\n";
    
    return 0;
}

