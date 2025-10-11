#include "scale_demo_multimodal.h"
#include "scale_demo_writer.h"
#include <iostream>
#include <iomanip>

using namespace melvin::scale_demo;

void print_header(const std::string& title) {
    std::cout << "\n═══════════════════════════════════════════════════════════════════\n";
    std::cout << "  " << title << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n\n";
}

void print_multimodal_output(const MultimodalOutput& output) {
    std::cout << "  Modality: ";
    switch (output.modality) {
        case OutputModality::TEXT_ONLY:
            std::cout << "TEXT_ONLY\n";
            break;
        case OutputModality::AUDIO_ONLY:
            std::cout << "AUDIO_ONLY\n";
            break;
        case OutputModality::MULTIMODAL:
            std::cout << "MULTIMODAL (Text + Audio)\n";
            break;
    }
    
    if (output.text.has_value()) {
        std::cout << "  Text:     " << output.text.value() << "\n";
    }
    
    if (output.audio_codes.has_value()) {
        std::cout << "  Audio:    [" << output.audio_codes.value().size() << " bytes] ";
        for (size_t i = 0; i < std::min(size_t(8), output.audio_codes.value().size()); ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                     << static_cast<int>(output.audio_codes.value()[i]) << " ";
        }
        if (output.audio_codes.value().size() > 8) {
            std::cout << "...";
        }
        std::cout << std::dec << "\n";
    }
    
    std::cout << "  Confidence: " << std::fixed << std::setprecision(2) 
             << output.confidence << "\n";
    std::cout << "  Latency:    " << std::fixed << std::setprecision(2) 
             << output.latency_ms << " ms\n";
}

int main() {
    print_header("MELVIN MULTIMODAL PIPELINE DEMO");
    
    std::cout << "This demo shows how Melvin handles multimodal input/output:\n";
    std::cout << "  • Same reasoning pipeline for audio AND text\n";
    std::cout << "  • Query in one modality, get answer in another\n";
    std::cout << "  • Cross-modal concept bindings\n\n";
    
    // Setup
    std::cout << "Setting up multimodal system...\n";
    
    BinaryRecordWriter::Config writer_config;
    writer_config.output_path = "multimodal_demo_memory.bin";
    BinaryRecordWriter writer(writer_config);
    
    RecordIndex index;
    CrossModalBridge bridge;
    
    MultimodalPipeline::Config pipeline_config;
    pipeline_config.enable_audio_to_text = true;
    pipeline_config.enable_text_to_audio = true;
    pipeline_config.enable_cross_modal_reasoning = true;
    
    MultimodalPipeline pipeline(writer, index, bridge, pipeline_config);
    
    std::cout << "✓ System ready\n";
    
    // ========================================================================
    // STEP 1: Ingest multimodal training data
    // ========================================================================
    
    print_header("STEP 1: Ingesting Multimodal Training Data");
    
    std::cout << "Creating cross-modal bindings (text ↔ audio):\n\n";
    
    struct TrainingPair {
        std::string text;
        std::vector<uint8_t> audio;
        std::string concept;
    };
    
    std::vector<TrainingPair> training_data = {
        {"cat", {0x10, 0x11, 0x12}, "cat_concept"},
        {"dog", {0x20, 0x21, 0x22}, "dog_concept"},
        {"bird", {0x30, 0x31, 0x32}, "bird_concept"},
        {"mammal", {0x40, 0x41, 0x42}, "mammal_concept"},
        {"meow", {0x10, 0x11, 0x12}, "cat_sound"}  // Same audio as "cat"
    };
    
    for (const auto& pair : training_data) {
        pipeline.ingest_multimodal_pair(pair.text, pair.audio, pair.concept);
        std::cout << "  ✓ Bound: \"" << pair.text << "\" ↔ [";
        for (size_t i = 0; i < pair.audio.size(); ++i) {
            if (i > 0) std::cout << " ";
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                     << static_cast<int>(pair.audio[i]);
        }
        std::cout << std::dec << "] → " << pair.concept << "\n";
    }
    
    writer.flush();
    
    // ========================================================================
    // STEP 2: Query with TEXT, get TEXT output
    // ========================================================================
    
    print_header("STEP 2: Text Input → Text Output");
    
    std::cout << "Query: \"what is cat\"\n";
    std::cout << "Expected: Text response\n\n";
    
    auto output1 = pipeline.query_text("what is cat", OutputModality::TEXT_ONLY);
    print_multimodal_output(output1);
    
    // ========================================================================
    // STEP 3: Query with TEXT, get AUDIO output (cross-modal)
    // ========================================================================
    
    print_header("STEP 3: Text Input → Audio Output (Cross-Modal)");
    
    std::cout << "Query: \"what is cat\"\n";
    std::cout << "Expected: AUDIO representation of 'cat'\n\n";
    
    auto output2 = pipeline.query_text("what is cat", OutputModality::AUDIO_ONLY);
    print_multimodal_output(output2);
    
    std::cout << "\n💡 Notice: Input was TEXT, output was AUDIO!\n";
    std::cout << "   The system translated through the graph:\n";
    std::cout << "   \"cat\" (text) → cat_concept → [audio pattern]\n";
    
    // ========================================================================
    // STEP 4: Query with AUDIO, get TEXT output (cross-modal)
    // ========================================================================
    
    print_header("STEP 4: Audio Input → Text Output (Cross-Modal)");
    
    std::vector<uint8_t> audio_query = {0x10, 0x11, 0x12};  // "cat" audio
    
    std::cout << "Query: [Audio: 10 11 12] (cat meow sound)\n";
    std::cout << "Expected: TEXT description\n\n";
    
    auto output3 = pipeline.query_audio(audio_query, OutputModality::TEXT_ONLY);
    print_multimodal_output(output3);
    
    std::cout << "\n💡 Notice: Input was AUDIO, output was TEXT!\n";
    std::cout << "   The system recognized the audio and translated to text.\n";
    
    // ========================================================================
    // STEP 5: Query with AUDIO, get AUDIO output (same modality)
    // ========================================================================
    
    print_header("STEP 5: Audio Input → Audio Output (Same Modality)");
    
    std::cout << "Query: [Audio: 10 11 12]\n";
    std::cout << "Expected: AUDIO response\n\n";
    
    auto output4 = pipeline.query_audio(audio_query, OutputModality::AUDIO_ONLY);
    print_multimodal_output(output4);
    
    // ========================================================================
    // STEP 6: Multimodal query (both audio and text)
    // ========================================================================
    
    print_header("STEP 6: Multimodal Input → Multimodal Output");
    
    std::cout << "Query: \"what is\" + [Audio: 10 11 12]\n";
    std::cout << "Expected: MULTIMODAL response (text + audio)\n\n";
    
    auto output5 = pipeline.query_multimodal("what is", audio_query, 
                                             OutputModality::MULTIMODAL);
    print_multimodal_output(output5);
    
    std::cout << "\n💡 Notice: Input combined TEXT + AUDIO!\n";
    std::cout << "   The system fused both modalities for reasoning.\n";
    std::cout << "   This is like saying \"what is\" and pointing to a cat.\n";
    
    // ========================================================================
    // STEP 7: Comprehensive evaluation
    // ========================================================================
    
    print_header("STEP 7: Comprehensive Cross-Modal Evaluation");
    
    MultimodalTestGenerator::Config gen_config;
    gen_config.num_paired_samples = 100;
    gen_config.generate_mismatched_pairs = false;
    
    MultimodalTestGenerator generator(gen_config);
    auto test_queries = generator.generate_test_queries(50);
    
    std::cout << "Running " << test_queries.size() << " test queries...\n\n";
    
    MultimodalEvaluator evaluator;
    auto metrics = evaluator.evaluate_cross_modal(pipeline, test_queries);
    
    std::cout << "Cross-Modal Retrieval Results:\n";
    std::cout << "─────────────────────────────────────────────────────────────\n";
    std::cout << "  Text → Text Recall:     " << std::fixed << std::setprecision(2) 
             << (metrics.text_to_text_recall * 100) << "%\n";
    std::cout << "  Audio → Audio Recall:   " << std::fixed << std::setprecision(2)
             << (metrics.audio_to_audio_recall * 100) << "%\n";
    std::cout << "  Text → Audio Recall:    " << std::fixed << std::setprecision(2)
             << (metrics.text_to_audio_recall * 100) << "%  (cross-modal!)\n";
    std::cout << "  Audio → Text Recall:    " << std::fixed << std::setprecision(2)
             << (metrics.audio_to_text_recall * 100) << "%  (cross-modal!)\n";
    std::cout << "  Multimodal Fusion Gain: " << std::fixed << std::setprecision(2)
             << (metrics.multimodal_fusion_gain * 100) << "%\n";
    std::cout << "\n";
    std::cout << "Latency by Modality:\n";
    std::cout << "─────────────────────────────────────────────────────────────\n";
    std::cout << "  Text queries:           " << std::fixed << std::setprecision(2)
             << metrics.text_latency_ms << " ms\n";
    std::cout << "  Audio queries:          " << std::fixed << std::setprecision(2)
             << metrics.audio_latency_ms << " ms\n";
    std::cout << "  Multimodal queries:     " << std::fixed << std::setprecision(2)
             << metrics.multimodal_latency_ms << " ms\n";
    
    // ========================================================================
    // STEP 8: Modality switching matrix
    // ========================================================================
    
    print_header("STEP 8: Modality Switching Matrix");
    
    auto switch_results = evaluator.evaluate_modality_switching(pipeline, test_queries);
    
    std::cout << "                    Output Modality\n";
    std::cout << "                 Text        Audio\n";
    std::cout << "              ┌─────────┬─────────┐\n";
    
    // Find results for each combination
    auto find_result = [&](const std::string& in, const std::string& out) -> double {
        for (const auto& r : switch_results) {
            if (r.input_modality == in && r.output_modality == out) {
                return r.accuracy * 100;
            }
        }
        return 0.0;
    };
    
    std::cout << "  Input   Text  │ " << std::setw(6) << std::fixed << std::setprecision(1)
             << find_result("text", "text") << "% │ "
             << std::setw(6) << find_result("text", "audio") << "% │\n";
    std::cout << " Modality       ├─────────┼─────────┤\n";
    std::cout << "         Audio  │ " << std::setw(6) << std::fixed << std::setprecision(1)
             << find_result("audio", "text") << "% │ "
             << std::setw(6) << find_result("audio", "audio") << "% │\n";
    std::cout << "              └─────────┴─────────┘\n";
    
    std::cout << "\n💡 The diagonal (Text→Text, Audio→Audio) shows same-modality performance.\n";
    std::cout << "   Off-diagonal shows CROSS-MODAL translation ability!\n";
    
    // ========================================================================
    // Summary
    // ========================================================================
    
    print_header("SUMMARY");
    
    std::cout << "✓ Multimodal pipeline successfully demonstrated!\n\n";
    
    std::cout << "Key Capabilities:\n";
    std::cout << "  1. ✓ Unified graph representation for all modalities\n";
    std::cout << "  2. ✓ Cross-modal concept bindings (text ↔ audio)\n";
    std::cout << "  3. ✓ Query in one modality, answer in another\n";
    std::cout << "  4. ✓ Multimodal fusion (combine text + audio)\n";
    std::cout << "  5. ✓ Same reasoning pipeline regardless of input\n";
    std::cout << "  6. ✓ Flexible output generation (text/audio/both)\n";
    
    std::cout << "\nHow It Works:\n";
    std::cout << "  • Text and audio both create nodes in the graph\n";
    std::cout << "  • Cross-modal edges link equivalent representations\n";
    std::cout << "  • Graph traversal works the same for any modality\n";
    std::cout << "  • Output generation adapts to requested modality\n";
    
    std::cout << "\nReal-World Example:\n";
    std::cout << "  User speaks: \"What is this sound?\" [plays cat meow]\n";
    std::cout << "  → Audio transcribed to text: \"what is this sound\"\n";
    std::cout << "  → Audio analyzed: [meow pattern]\n";
    std::cout << "  → Graph reasoning: meow → cat → mammal\n";
    std::cout << "  → Output (text): \"That's a cat. Cats are mammals.\"\n";
    std::cout << "  → Output (audio): [synthesized speech or cat meow]\n";
    
    std::cout << "\nMemory file: multimodal_demo_memory.bin\n";
    std::cout << "  Nodes:  " << writer.nodes_written() << "\n";
    std::cout << "  Edges:  " << writer.edges_written() << "\n";
    std::cout << "  Bytes:  " << writer.bytes_written() << "\n";
    
    std::cout << "\n";
    
    return 0;
}

