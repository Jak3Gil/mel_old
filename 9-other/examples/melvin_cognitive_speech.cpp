/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN COGNITIVE SPEECH DEMO                                             ║
 * ║  Speech as a cognitive process with full graph integration                ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "../core/atomic_graph.h"
#include "../io/speech_intent.h"
#include "../io/text_to_speech_graph.h"
#include "../audio/audio_pipeline.h"
#include "../audio/audio_bridge.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace melvin;
using namespace melvin::io;
using namespace melvin::audio;

// ============================================================================
// DEMO 1: Basic Cognitive Speech
// ============================================================================

void demo_basic_cognitive_speech() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 1: Basic Cognitive Speech                          ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    SpeechIntent intent(graph);
    TextToSpeechGraph tts(intent);
    
    std::cout << "📝 Melvin will speak and remember what he said...\n" << std::endl;
    
    // Speak and create graph
    tts.speak("Hello, I am Melvin");
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    tts.speak("I can now remember everything I say");
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    tts.speak("My speech is part of my memory");
    
    // Show results
    std::cout << "\n📊 Results:" << std::endl;
    graph.print_stats();
    intent.print_stats();
    
    std::cout << "\n💾 Saving speech memory..." << std::endl;
    graph.save("cognitive_speech_nodes.bin", "cognitive_speech_edges.bin");
}

// ============================================================================
// DEMO 2: Speech with Concept Linking
// ============================================================================

void demo_concept_linked_speech() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 2: Speech Linked to Concepts                       ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    SpeechIntent intent(graph);
    TextToSpeechGraph tts(intent);
    
    std::cout << "📝 Creating concepts and speaking about them...\n" << std::endl;
    
    // Create concepts first
    uint64_t cooking_concept = graph.get_or_create_node("cooking", 0);
    uint64_t learning_concept = graph.get_or_create_node("learning", 0);
    
    // Speak about concepts
    tts.speak("I am learning about cooking", learning_concept);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    tts.speak("Cooking involves heat and ingredients", cooking_concept);
    
    // Analyze connections
    std::cout << "\n🔍 Analyzing concept connections..." << std::endl;
    
    auto cooking_neighbors = graph.neighbors(cooking_concept);
    std::cout << "   'cooking' connected to " << cooking_neighbors.size() << " nodes" << std::endl;
    
    auto speech_about_learning = intent.find_speech_about("learning");
    std::cout << "   Found " << speech_about_learning.size() << " utterances about learning" << std::endl;
    
    graph.print_stats();
}

// ============================================================================
// DEMO 3: Self-Recognition Loop
// ============================================================================

void demo_self_recognition() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 3: Self-Recognition Feedback Loop                  ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    SpeechIntent intent(graph);
    TextToSpeechGraph tts(intent);
    AudioBridge audio_bridge;
    
    std::cout << "🔄 Demonstrating self-recognition loop...\n" << std::endl;
    
    // Melvin speaks
    std::cout << "1️⃣  Melvin speaks:" << std::endl;
    uint64_t speech_id = tts.speak("I can recognize my own voice");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Simulate audio recognition of own speech
    std::cout << "\n2️⃣  Audio pipeline hears (simulated):" << std::endl;
    AudioEvent self_audio;
    self_audio.id = 100;
    self_audio.timestamp = 1.5f;
    self_audio.label = "I can recognize my own voice";
    self_audio.type = "speech";
    self_audio.confidence = 0.93f;
    self_audio.source = "mic";
    
    // Check for self-recognition
    bool is_self = intent.is_self_speech(self_audio.label, self_audio.timestamp);
    
    std::cout << "\n3️⃣  Self-recognition check:" << std::endl;
    if (is_self) {
        std::cout << "   ✅ Recognized as own speech!" << std::endl;
        
        // Create audio node
        uint64_t audio_id = graph.get_or_create_node("audio:" + self_audio.label, audio::AUDIO_PHRASE);
        
        // Link speech output → audio input
        intent.link_self_recognition(speech_id, audio_id);
        
        std::cout << "   🔁 Feedback loop created: speech(" << speech_id 
                  << ") ↔ audio(" << audio_id << ")" << std::endl;
    }
    
    std::cout << "\n📊 Results:" << std::endl;
    graph.print_stats();
    intent.print_stats();
}

// ============================================================================
// DEMO 4: Conversation Memory
// ============================================================================

void demo_conversation_memory() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 4: Conversation Memory                             ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    SpeechIntent intent(graph);
    TextToSpeechGraph tts(intent);
    
    std::cout << "💬 Simulating a conversation...\n" << std::endl;
    
    // Conversation turns
    std::cout << "User: Hello Melvin" << std::endl;
    tts.speak("Hello! How can I help you today?");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "\nUser: What can you do?" << std::endl;
    tts.speak("I can listen, speak, and remember our conversations");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    std::cout << "\nUser: Thank you" << std::endl;
    tts.speak("You're welcome! I now remember everything I just said");
    
    // Show memory
    std::cout << "\n🧠 Melvin's speech memory:" << std::endl;
    auto recent = intent.get_recent_speech(60.0f);
    std::cout << "   Stored " << recent.size() << " recent utterances" << std::endl;
    std::cout << "   Total words spoken: " << intent.get_word_count() << std::endl;
    
    // Save conversation
    std::cout << "\n💾 Saving conversation to graph..." << std::endl;
    graph.save("conversation_memory_nodes.bin", "conversation_memory_edges.bin");
    
    graph.print_stats();
}

// ============================================================================
// DEMO 5: Reflection on Past Speech
// ============================================================================

void demo_reflection() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 5: Reflection on Past Speech                       ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    
    // Load previous conversation
    std::cout << "📂 Loading previous conversation..." << std::endl;
    try {
        graph.load("conversation_memory_nodes.bin", "conversation_memory_edges.bin");
        std::cout << "✅ Loaded conversation memory:" << std::endl;
        graph.print_stats();
        
        // Analyze what was said
        std::cout << "\n🔍 Analyzing what Melvin said..." << std::endl;
        
        auto utterances = graph.find_nodes("utterance:");
        std::cout << "   Found " << utterances.size() << " utterances" << std::endl;
        
        // Find specific topics
        auto spoken_words = graph.find_nodes("spoken:");
        std::cout << "   Found " << spoken_words.size() << " spoken words" << std::endl;
        
        // Show some utterances
        std::cout << "\n   Sample utterances:" << std::endl;
        for (size_t i = 0; i < std::min(utterances.size(), size_t(5)); i++) {
            const Node* node = graph.get_node(utterances[i]);
            if (node) {
                std::cout << "     - " << node->label << std::endl;
            }
        }
        
    } catch (...) {
        std::cout << "⚠️  No previous conversation found. Run demo 4 first." << std::endl;
    }
}

// ============================================================================
// MAIN DEMO SELECTOR
// ============================================================================

void print_menu() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  🧠 MELVIN COGNITIVE SPEECH DEMO                          ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\nSelect a demo:" << std::endl;
    std::cout << "  1. Basic Cognitive Speech" << std::endl;
    std::cout << "  2. Speech Linked to Concepts" << std::endl;
    std::cout << "  3. Self-Recognition Feedback Loop" << std::endl;
    std::cout << "  4. Conversation Memory" << std::endl;
    std::cout << "  5. Reflection on Past Speech" << std::endl;
    std::cout << "  6. Run All Demos" << std::endl;
    std::cout << "  0. Exit" << std::endl;
    std::cout << "\nChoice: ";
}

int main(int argc, char** argv) {
    std::cout << "\n";
    
    // If argument provided, run that demo directly
    if (argc > 1) {
        int choice = std::atoi(argv[1]);
        
        switch (choice) {
            case 1: demo_basic_cognitive_speech(); break;
            case 2: demo_concept_linked_speech(); break;
            case 3: demo_self_recognition(); break;
            case 4: demo_conversation_memory(); break;
            case 5: demo_reflection(); break;
            case 6:
                demo_basic_cognitive_speech();
                demo_concept_linked_speech();
                demo_self_recognition();
                demo_conversation_memory();
                demo_reflection();
                break;
            default:
                std::cerr << "Invalid demo number" << std::endl;
                return 1;
        }
        
        std::cout << "\n✨ Demo complete!\n" << std::endl;
        return 0;
    }
    
    // Interactive menu
    while (true) {
        print_menu();
        
        int choice;
        std::cin >> choice;
        
        if (choice == 0) {
            std::cout << "\n👋 Goodbye!\n" << std::endl;
            break;
        }
        
        switch (choice) {
            case 1: demo_basic_cognitive_speech(); break;
            case 2: demo_concept_linked_speech(); break;
            case 3: demo_self_recognition(); break;
            case 4: demo_conversation_memory(); break;
            case 5: demo_reflection(); break;
            case 6:
                demo_basic_cognitive_speech();
                demo_concept_linked_speech();
                demo_self_recognition();
                demo_conversation_memory();
                demo_reflection();
                break;
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
        }
    }
    
    return 0;
}

