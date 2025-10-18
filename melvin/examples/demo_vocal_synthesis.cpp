/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  DEMO - Biological-Style Vocal Synthesis                                  ║
 * ║  Melvin speaks through simulated vocal tract                              ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "../audio/vocal_engine.h"
#include "../audio/phoneme_graph.h"
#include "../core/atomic_graph.h"
#include "../io/speech_intent.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace melvin;
using namespace melvin::audio;
using namespace melvin::io;

// ============================================================================
// DEMO 1: Basic Vocal Synthesis
// ============================================================================

void demo_basic_vocal() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 1: Basic Vocal Synthesis                           ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    VocalEngine engine;
    
    std::cout << "🎙️ Melvin will speak using simulated vocal cords...\n" << std::endl;
    
    // Synthesize simple words
    engine.speak_and_play("hello", true);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    engine.speak_and_play("melvin", true);
    
    std::cout << "\n📊 Results:" << std::endl;
    engine.print_stats();
}

// ============================================================================
// DEMO 2: Phoneme-Level Control
// ============================================================================

void demo_phoneme_control() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 2: Phoneme-Level Control                           ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    PhonemeGraph pg;
    VocalEngine engine(pg);
    
    std::cout << "🔬 Demonstrating individual phoneme synthesis...\n" << std::endl;
    
    // Get and synthesize individual phonemes
    const PhonemeNode* m = pg.get_phoneme("m");
    if (m) {
        std::cout << "Synthesizing /m/ phoneme..." << std::endl;
        auto audio_m = engine.synthesize_phoneme(*m);
        std::cout << "   Generated " << audio_m.size() << " samples" << std::endl;
    }
    
    const PhonemeNode* eh = pg.get_phoneme("eh");
    if (eh) {
        std::cout << "Synthesizing /eh/ phoneme..." << std::endl;
        auto audio_eh = engine.synthesize_phoneme(*eh);
        std::cout << "   Generated " << audio_eh.size() << " samples" << std::endl;
    }
    
    pg.print_stats();
}

// ============================================================================
// DEMO 3: Learning New Words
// ============================================================================

void demo_learning() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 3: Learning New Words                              ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    PhonemeGraph pg;
    VocalEngine engine(pg);
    
    std::cout << "🎓 Teaching Melvin new pronunciations...\n" << std::endl;
    
    // Teach custom pronunciations
    pg.add_word_pronunciation("robot", {"r", "ow", "b", "aa", "t"});
    pg.add_word_pronunciation("neural", {"n", "uw", "r", "ae", "l"});
    
    std::cout << "\n🎙️ Melvin attempting to speak new words..." << std::endl;
    engine.speak("robot");
    engine.speak("neural");
    
    pg.print_stats();
}

// ============================================================================
// DEMO 4: Vocal Parameters
// ============================================================================

void demo_vocal_parameters() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 4: Vocal Parameters (Pitch, Breathiness)           ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    PhonemeGraph pg;
    
    std::cout << "🎚️ Testing different vocal parameters...\n" << std::endl;
    
    // Normal voice
    std::cout << "1. Normal voice (120 Hz):" << std::endl;
    VocalEngine::Config config1;
    config1.base_pitch = 120.0f;
    config1.breathiness = 0.1f;
    VocalEngine engine1(pg, config1);
    engine1.speak("normal");
    
    // Lower pitch
    std::cout << "\n2. Lower voice (100 Hz):" << std::endl;
    VocalEngine::Config config2;
    config2.base_pitch = 100.0f;
    config2.breathiness = 0.1f;
    VocalEngine engine2(pg, config2);
    engine2.speak("lower");
    
    // Higher pitch
    std::cout << "\n3. Higher voice (150 Hz):" << std::endl;
    VocalEngine::Config config3;
    config3.base_pitch = 150.0f;
    config3.breathiness = 0.05f;
    VocalEngine engine3(pg, config3);
    engine3.speak("higher");
    
    // Breathy voice
    std::cout << "\n4. Breathy voice:" << std::endl;
    VocalEngine::Config config4;
    config4.base_pitch = 120.0f;
    config4.breathiness = 0.5f;
    VocalEngine engine4(pg, config4);
    engine4.speak("breathy");
    
    std::cout << "\n✅ Demonstrated vocal parameter control" << std::endl;
}

// ============================================================================
// DEMO 5: Integrated Cognitive Speech
// ============================================================================

void demo_integrated_cognitive() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 5: Integrated Cognitive Speech                     ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    PhonemeGraph pg(graph);
    VocalEngine engine(pg);
    SpeechIntent intent(graph);
    
    std::cout << "🧠 Combining cognitive speech with vocal synthesis...\n" << std::endl;
    
    // Create concept
    uint64_t ai_concept = graph.get_or_create_node("artificial_intelligence", 0);
    
    // Speak with concept linking
    std::string text = "I am an AI";
    
    // Step 1: Create cognitive representation
    uint64_t speech_id = intent.process_output(text, ai_concept);
    
    // Step 2: Synthesize vocally
    auto audio = engine.speak(text);
    
    // Step 3: Mark as spoken
    intent.mark_spoken(speech_id);
    
    std::cout << "\n📊 Results:" << std::endl;
    std::cout << "   Graph nodes: " << graph.node_count() << std::endl;
    std::cout << "   Graph edges: " << graph.edge_count() << std::endl;
    std::cout << "   Audio samples: " << audio.size() << std::endl;
    std::cout << "   Phonemes used: " << pg.phoneme_count() << std::endl;
    
    std::cout << "\n✅ Cognitive speech + vocal synthesis integrated!" << std::endl;
}

// ============================================================================
// MAIN DEMO SELECTOR
// ============================================================================

void print_menu() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  🎙️ MELVIN VOCAL SYNTHESIS DEMO                          ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\nSelect a demo:" << std::endl;
    std::cout << "  1. Basic Vocal Synthesis" << std::endl;
    std::cout << "  2. Phoneme-Level Control" << std::endl;
    std::cout << "  3. Learning New Words" << std::endl;
    std::cout << "  4. Vocal Parameters" << std::endl;
    std::cout << "  5. Integrated Cognitive Speech" << std::endl;
    std::cout << "  6. Run All Demos" << std::endl;
    std::cout << "  0. Exit" << std::endl;
    std::cout << "\nChoice: ";
}

int main(int argc, char** argv) {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  🧠 MELVIN - Biological-Style Vocal Synthesis            ║" << std::endl;
    std::cout << "║  Speech generated from simulated vocal tract             ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\n";
    
    // If argument provided, run that demo directly
    if (argc > 1) {
        int choice = std::atoi(argv[1]);
        
        switch (choice) {
            case 1: demo_basic_vocal(); break;
            case 2: demo_phoneme_control(); break;
            case 3: demo_learning(); break;
            case 4: demo_vocal_parameters(); break;
            case 5: demo_integrated_cognitive(); break;
            case 6:
                demo_basic_vocal();
                demo_phoneme_control();
                demo_learning();
                demo_vocal_parameters();
                demo_integrated_cognitive();
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
            case 1: demo_basic_vocal(); break;
            case 2: demo_phoneme_control(); break;
            case 3: demo_learning(); break;
            case 4: demo_vocal_parameters(); break;
            case 5: demo_integrated_cognitive(); break;
            case 6:
                demo_basic_vocal();
                demo_phoneme_control();
                demo_learning();
                demo_vocal_parameters();
                demo_integrated_cognitive();
                break;
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
        }
    }
    
    return 0;
}

