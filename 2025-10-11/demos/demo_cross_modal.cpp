/*
 * MELVIN CROSS-MODAL DEMO
 * 
 * Demonstrates cross-modal LEAP reasoning:
 * - Image → text connections (LOOKS_LIKE)
 * - Audio → text connections (SOUND)
 * - Cross-modal LEAPs (e.g., "what sound does a fox make?")
 * - Uses existing EXACT+LEAP infrastructure
 */

#include "../include/melvin_core.h"
#include "../src/sensory/clip_adapter.h"
#include "../src/sensory/audio_adapter.h"
#include "../src/util/telemetry.h"
#include "../src/util/config.h"
#include "../src/util/explain.h"
#include <iostream>

using namespace melvin_core;
using namespace melvin_clip;
using namespace melvin_audio;
using namespace melvin_telemetry;
using namespace melvin_config;
using namespace melvin_explain;

// ==================== DEMO FUNCTIONS ====================

void teach_cross_modal_pairs(bool explain) {
    std::cout << "\n📚 PHASE 1: Teaching Cross-Modal Pairs\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    // Teach image-label pairs
    std::cout << "Teaching visual concepts...\n";
    ingest_image("dog.jpg", "dog");
    ingest_image("cat.jpg", "cat");
    ingest_image("fox.jpg", "fox");
    ingest_image("wolf.jpg", "wolf");
    std::cout << "  ✅ Ingested 4 images\n\n";
    
    // Teach audio-label pairs
    std::cout << "Teaching audio concepts...\n";
    ingest_audio("bark.wav", "bark");
    ingest_audio("meow.wav", "meow");
    ingest_audio("howl.wav", "howl");
    std::cout << "  ✅ Ingested 3 audio files\n\n";
    
    // Create SOUND associations
    std::cout << "Connecting sounds to animals...\n";
    connect_sound("dog", "bark");
    connect_sound("cat", "meow");
    connect_sound("wolf", "howl");
    std::cout << "  ✅ Created 3 sound associations\n\n";
}

void test_exact_cross_modal_queries() {
    std::cout << "\n📖 PHASE 2: EXACT Cross-Modal Queries\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    std::cout << "Q1: What sound does a dog make?\n";
    std::cout << "   " << query_sound("dog") << "\n\n";
    
    std::cout << "Q2: What looks like a cat?\n";
    std::cout << "   " << query_visual_similarity("cat") << "\n\n";
    
    std::cout << "Q3: What sound does a wolf make?\n";
    std::cout << "   " << query_sound("wolf") << "\n\n";
}

void test_cross_modal_leaps(UnifiedBrain& brain, bool explain) {
    std::cout << "\n🔮 PHASE 3: Cross-Modal LEAPs\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    if (explain) {
        EXPLAIN_SECTION("Cross-Modal LEAP Reasoning");
    }
    
    std::cout << "Q4: What sound does a fox make?\n";
    std::cout << "Analysis:\n";
    std::cout << "  • Fox is known (image ingested)\n";
    std::cout << "  • No direct sound association\n";
    std::cout << "  • Crowd: dog→bark, cat→meow, wolf→howl\n";
    std::cout << "  • Should create LEAP: fox ~SOUND~> ?\n\n";
    
    brain.think("what sound does a fox make");
    
    if (explain) {
        EXPLAIN_END_SECTION();
    }
    
    std::cout << "\n";
}

void test_visual_similarity_leaps(UnifiedBrain& brain) {
    std::cout << "\n🖼️  PHASE 4: Visual Similarity LEAPs\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    std::cout << "Q5: What looks like a fox?\n";
    std::cout << "Analysis:\n";
    std::cout << "  • Fox image known\n";
    std::cout << "  • Similar animals: dog, wolf\n";
    std::cout << "  • Should find visual similarity\n\n";
    
    brain.think("what looks like a fox");
    std::cout << "\n";
}

void test_multi_modal_reasoning(UnifiedBrain& brain) {
    std::cout << "\n🌐 PHASE 5: Multi-Modal Reasoning\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    // Add some semantic knowledge
    brain.learn("dogs are mammals");
    brain.learn("cats are mammals");
    brain.learn("foxes are mammals");
    brain.learn("wolves are mammals");
    
    std::cout << "Q6: Are foxes mammals?\n";
    brain.think("are foxes mammals");
    std::cout << "\n";
    
    std::cout << "Q7: What sound does a mammal make?\n";
    brain.think("what sound does a mammal make");
    std::cout << "\n";
}

void show_cross_modal_statistics() {
    std::cout << "\n📊 PHASE 6: Cross-Modal Statistics\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    // Count nodes by type
    int text_nodes = 0, audio_nodes = 0, image_nodes = 0;
    for (const auto& node : melvin_storage::nodes) {
        if (node.type == static_cast<int>(melvin_modal::ModalNodeType::NODE_TEXT)) {
            text_nodes++;
        } else if (node.type == static_cast<int>(melvin_modal::ModalNodeType::NODE_AUDIO)) {
            audio_nodes++;
        } else if (node.type == static_cast<int>(melvin_modal::ModalNodeType::NODE_IMAGE)) {
            image_nodes++;
        }
    }
    
    std::cout << "Node Distribution:\n";
    std::cout << "  TEXT nodes:  " << text_nodes << "\n";
    std::cout << "  AUDIO nodes: " << audio_nodes << "\n";
    std::cout << "  IMAGE nodes: " << image_nodes << "\n";
    std::cout << "  TOTAL:       " << melvin_storage::nodes.size() << "\n\n";
    
    // Count cross-modal edges
    int sound_edges = 0, looks_like_edges = 0, leap_edges = 0;
    for (const auto& edge : melvin_storage::edges) {
        if (melvin_storage::nodes[edge.a].type != melvin_storage::nodes[edge.b].type) {
            // Cross-modal edge
            if (edge.is_leap()) {
                leap_edges++;
            }
        }
    }
    
    std::cout << "Cross-Modal Connections:\n";
    std::cout << "  Cross-modal LEAPs: " << leap_edges << "\n";
    std::cout << "  Total edges:       " << melvin_storage::edges.size() << "\n\n";
}

// ==================== MAIN ====================

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN CROSS-MODAL LEAP DEMO                                ║\n";
    std::cout << "║  Image ↔ Text ↔ Audio Reasoning                              ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    
    // Parse arguments
    bool explain = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--explain") {
            explain = true;
            EXPLAIN_ENABLE(true);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "\nUsage: demo_cross_modal [OPTIONS]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --explain      Enable explain tracing\n";
            std::cout << "  --help, -h     Show this help\n\n";
            std::cout << "Environment Variables:\n";
            std::cout << "  EXPLAIN=1              Enable explain tracing\n";
            std::cout << "  ALLOW_MULTI_ATTR=1     Allow multi-attribute reasoning\n";
            std::cout << "  TH_SUPPORT=N           Crowd support threshold\n\n";
            std::cout << "Examples:\n";
            std::cout << "  ./demo_cross_modal\n";
            std::cout << "  ./demo_cross_modal --explain\n";
            std::cout << "  EXPLAIN=1 ALLOW_MULTI_ATTR=1 ./demo_cross_modal\n\n";
            return 0;
        }
    }
    
    // Load configuration
    load_config();
    
    // Enable telemetry
    enable_telemetry(true);
    set_telemetry_file("cross_modal_telemetry.jsonl");
    
    // Create brain
    system("rm -f cross_modal_demo.bin");
    UnifiedBrain brain("cross_modal_demo.bin");
    
    // Run demo phases
    teach_cross_modal_pairs(explain);
    test_exact_cross_modal_queries();
    test_cross_modal_leaps(brain, explain);
    test_visual_similarity_leaps(brain);
    test_multi_modal_reasoning(brain);
    show_cross_modal_statistics();
    
    // Print explain trace if enabled
    if (explain) {
        EXPLAIN_PRINT(std::cout);
    }
    
    // Print telemetry summary
    std::cout << "\n📊 Telemetry Summary:\n";
    std::cout << std::string(70, '=') << "\n";
    std::cout << "  image_ingest:      " << count_events("image_ingest") << "\n";
    std::cout << "  audio_ingest:      " << count_events("audio_ingest") << "\n";
    std::cout << "  leap_create:       " << count_events("leap_create") << "\n";
    std::cout << "  leap_reject:       " << count_events("leap_reject") << "\n";
    std::cout << "  crossmodal_leap:   " << count_events("crossmodal_leap") << "\n\n";
    
    // Show final stats
    std::cout << "📈 Final Brain State:\n";
    std::cout << std::string(70, '=') << "\n";
    brain.stats();
    
    // Save
    brain.save();
    std::cout << "\n✅ Brain saved to cross_modal_demo.bin\n";
    
    // Summary
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  DEMO COMPLETE                                               ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Features Demonstrated:\n";
    std::cout << "  ✅ Image ingestion with CLIP encoding (mock)\n";
    std::cout << "  ✅ Audio ingestion with phoneme quantization (mock)\n";
    std::cout << "  ✅ Cross-modal EXACT connections (SOUND, LOOKS_LIKE)\n";
    std::cout << "  ✅ Cross-modal LEAP reasoning\n";
    std::cout << "  ✅ Multi-modal knowledge integration\n";
    std::cout << "  ✅ Telemetry tracking for all modalities\n\n";
    
    std::cout << "Next Steps:\n";
    std::cout << "  • Replace mock encoders with real CLIP/audio models\n";
    std::cout << "  • Add data/images/ and data/audio/ directories\n";
    std::cout << "  • Use clip_encode.py for real image embeddings\n";
    std::cout << "  • Test with real multimodal queries\n\n";
    
    return 0;
}

