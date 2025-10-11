#pragma once

#include "melvin_types.h"
#include "scale_demo_writer.h"
#include "scale_demo_generators.h"
#include <string>
#include <vector>
#include <variant>
#include <optional>

namespace melvin {
namespace scale_demo {

// Unified input modality
enum class InputModality {
    TEXT_ONLY,
    AUDIO_ONLY,
    MULTIMODAL  // Both audio and text
};

// Unified output modality
enum class OutputModality {
    TEXT_ONLY,
    AUDIO_ONLY,
    MULTIMODAL  // Both audio and text
};

// Multimodal input container
struct MultimodalInput {
    InputModality modality;
    
    // Text component (if present)
    std::optional<std::string> text;
    std::vector<std::string> text_tokens;
    
    // Audio component (if present)
    std::optional<std::vector<uint8_t>> audio_codes;
    std::vector<std::string> audio_motifs;
    
    // Timing information (for synchronized multimodal)
    std::vector<uint64_t> timestamps_ms;
    
    // Metadata
    std::string input_id;
    uint64_t created_at_ms;
};

// Multimodal output container
struct MultimodalOutput {
    OutputModality modality;
    
    // Text component (if generated)
    std::optional<std::string> text;
    std::vector<std::string> text_tokens;
    std::vector<NodeID> text_path;
    
    // Audio component (if generated)
    std::optional<std::vector<uint8_t>> audio_codes;
    std::vector<std::string> audio_motifs;
    
    // Metadata
    double confidence = 0.0;
    double latency_ms = 0.0;
    std::string output_id;
};

// Cross-modal bridge: links text concepts to audio patterns
class CrossModalBridge {
public:
    CrossModalBridge();
    
    // Register a text-audio binding (e.g., "cat" ↔ [audio pattern for "cat"])
    void bind_text_to_audio(const std::string& text, 
                           const std::vector<uint8_t>& audio_pattern,
                           const NodeID& text_node,
                           const NodeID& audio_node);
    
    // Find audio representation of text
    std::optional<std::vector<uint8_t>> text_to_audio(const std::string& text) const;
    
    // Find text representation of audio
    std::optional<std::string> audio_to_text(const std::vector<uint8_t>& audio) const;
    
    // Get cross-modal node binding
    std::optional<NodeID> get_audio_node_for_text(const NodeID& text_node) const;
    std::optional<NodeID> get_text_node_for_audio(const NodeID& audio_node) const;
    
private:
    // Bidirectional mappings
    std::unordered_map<std::string, std::vector<uint8_t>> text_to_audio_map_;
    std::unordered_map<std::string, std::string> audio_to_text_map_;  // audio hash → text
    std::unordered_map<NodeID, NodeID, NodeIDHash> text_to_audio_nodes_;
    std::unordered_map<NodeID, NodeID, NodeIDHash> audio_to_text_nodes_;
    
    std::string hash_audio(const std::vector<uint8_t>& audio) const;
};

// Unified multimodal pipeline
class MultimodalPipeline {
public:
    struct Config {
        bool enable_audio_to_text = true;
        bool enable_text_to_audio = true;
        bool enable_cross_modal_reasoning = true;  // Reason across modalities
        OutputModality default_output = OutputModality::TEXT_ONLY;
    };
    
    MultimodalPipeline(BinaryRecordWriter& writer, 
                      RecordIndex& index,
                      CrossModalBridge& bridge,
                      const Config& config = Config{});
    
    // Main pipeline: unified input → reasoning → unified output
    MultimodalOutput process(const MultimodalInput& input, 
                            OutputModality desired_output = OutputModality::TEXT_ONLY);
    
    // Ingest training data (builds cross-modal bindings)
    void ingest_multimodal_pair(const std::string& text,
                               const std::vector<uint8_t>& audio,
                               const std::string& concept_label);
    
    // Query with text, get output in any modality
    MultimodalOutput query_text(const std::string& query,
                               OutputModality output_modality = OutputModality::TEXT_ONLY);
    
    // Query with audio, get output in any modality
    MultimodalOutput query_audio(const std::vector<uint8_t>& audio_query,
                                OutputModality output_modality = OutputModality::TEXT_ONLY);
    
    // Query with both (e.g., "what is" [audio: "cat meow sound"])
    MultimodalOutput query_multimodal(const std::string& text_query,
                                     const std::vector<uint8_t>& audio_context,
                                     OutputModality output_modality = OutputModality::MULTIMODAL);
    
private:
    BinaryRecordWriter& writer_;
    RecordIndex& index_;
    CrossModalBridge& bridge_;
    Config config_;
    
    // Internal token cache for both modalities
    std::unordered_map<std::string, NodeID> text_token_cache_;
    std::unordered_map<std::string, NodeID> audio_frame_cache_;  // audio hash → NodeID
    
    // Process input to unified graph representation
    std::vector<NodeID> input_to_graph_path(const MultimodalInput& input);
    
    // Generate output from graph path in desired modality
    MultimodalOutput graph_path_to_output(const std::vector<NodeID>& path,
                                         OutputModality modality);
    
    // Cross-modal translation
    std::vector<NodeID> translate_audio_to_text_nodes(const std::vector<NodeID>& audio_nodes);
    std::vector<NodeID> translate_text_to_audio_nodes(const std::vector<NodeID>& text_nodes);
};

// Multimodal test generator: creates paired audio+text training data
class MultimodalTestGenerator {
public:
    struct Config {
        uint32_t num_paired_samples = 5000;
        float audio_noise_level = 0.1f;
        bool generate_mismatched_pairs = false;  // For negative examples
        uint64_t seed = 42;
    };
    
    MultimodalTestGenerator(const Config& config);
    
    struct PairedSample {
        std::string text;
        std::vector<uint8_t> audio_codes;
        std::string concept_label;
        bool is_matched = true;  // false for negative examples
    };
    
    // Generate paired audio-text samples
    std::vector<PairedSample> generate_paired_samples();
    
    // Generate test queries in both modalities
    struct TestQuery {
        MultimodalInput input;
        std::string expected_text_output;
        std::vector<uint8_t> expected_audio_output;
        std::string category;
    };
    
    std::vector<TestQuery> generate_test_queries(uint32_t num_queries = 500);
    
private:
    Config config_;
    std::mt19937 rng_;
    
    // Concept → audio pattern mapping
    std::unordered_map<std::string, std::vector<uint8_t>> concept_to_audio_;
    
    void initialize_concept_audio_mappings();
    std::vector<uint8_t> synthesize_audio_for_text(const std::string& text);
};

// Multimodal evaluation: test cross-modal retrieval
class MultimodalEvaluator {
public:
    struct CrossModalMetrics {
        // Text→Text retrieval
        double text_to_text_recall = 0.0;
        
        // Audio→Audio retrieval
        double audio_to_audio_recall = 0.0;
        
        // Cross-modal retrieval
        double text_to_audio_recall = 0.0;    // Query text, retrieve audio
        double audio_to_text_recall = 0.0;    // Query audio, retrieve text
        
        // Multimodal fusion
        double multimodal_fusion_gain = 0.0;  // Improvement when using both
        
        // Latency by modality
        double text_latency_ms = 0.0;
        double audio_latency_ms = 0.0;
        double multimodal_latency_ms = 0.0;
    };
    
    MultimodalEvaluator();
    
    // Evaluate cross-modal retrieval
    CrossModalMetrics evaluate_cross_modal(
        MultimodalPipeline& pipeline,
        const std::vector<MultimodalTestGenerator::TestQuery>& queries);
    
    // Test modality switching: input in one modality, output in another
    struct ModalitySwitchResult {
        std::string input_modality;
        std::string output_modality;
        double accuracy = 0.0;
        double latency_ms = 0.0;
    };
    
    std::vector<ModalitySwitchResult> evaluate_modality_switching(
        MultimodalPipeline& pipeline,
        const std::vector<MultimodalTestGenerator::TestQuery>& queries);
};

} // namespace scale_demo
} // namespace melvin

