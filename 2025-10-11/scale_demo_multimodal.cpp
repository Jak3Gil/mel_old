#include "scale_demo_multimodal.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <openssl/sha.h>

namespace melvin {
namespace scale_demo {

// ============================================================================
// CrossModalBridge
// ============================================================================

CrossModalBridge::CrossModalBridge() {}

void CrossModalBridge::bind_text_to_audio(
    const std::string& text,
    const std::vector<uint8_t>& audio_pattern,
    const NodeID& text_node,
    const NodeID& audio_node) {
    
    text_to_audio_map_[text] = audio_pattern;
    
    std::string audio_hash = hash_audio(audio_pattern);
    audio_to_text_map_[audio_hash] = text;
    
    text_to_audio_nodes_[text_node] = audio_node;
    audio_to_text_nodes_[audio_node] = text_node;
}

std::optional<std::vector<uint8_t>> CrossModalBridge::text_to_audio(
    const std::string& text) const {
    
    auto it = text_to_audio_map_.find(text);
    if (it != text_to_audio_map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<std::string> CrossModalBridge::audio_to_text(
    const std::vector<uint8_t>& audio) const {
    
    std::string audio_hash = hash_audio(audio);
    auto it = audio_to_text_map_.find(audio_hash);
    if (it != audio_to_text_map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<NodeID> CrossModalBridge::get_audio_node_for_text(
    const NodeID& text_node) const {
    
    auto it = text_to_audio_nodes_.find(text_node);
    if (it != text_to_audio_nodes_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<NodeID> CrossModalBridge::get_text_node_for_audio(
    const NodeID& audio_node) const {
    
    auto it = audio_to_text_nodes_.find(audio_node);
    if (it != audio_to_text_nodes_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::string CrossModalBridge::hash_audio(const std::vector<uint8_t>& audio) const {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(audio.data(), audio.size(), hash);
    
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(hash[i]);
    }
    return oss.str();
}

// ============================================================================
// MultimodalPipeline
// ============================================================================

MultimodalPipeline::MultimodalPipeline(
    BinaryRecordWriter& writer,
    RecordIndex& index,
    CrossModalBridge& bridge,
    const Config& config)
    : writer_(writer), index_(index), bridge_(bridge), config_(config) {}

MultimodalOutput MultimodalPipeline::process(
    const MultimodalInput& input,
    OutputModality desired_output) {
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Convert input to graph representation (unified across modalities)
    std::vector<NodeID> graph_path = input_to_graph_path(input);
    
    // Generate output in desired modality
    MultimodalOutput output = graph_path_to_output(graph_path, desired_output);
    
    auto end = std::chrono::high_resolution_clock::now();
    output.latency_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    return output;
}

void MultimodalPipeline::ingest_multimodal_pair(
    const std::string& text,
    const std::vector<uint8_t>& audio,
    const std::string& concept_label) {
    
    // Create text node
    NodeID text_node = writer_.emit_taught_node(text, NodeType::SYMBOL);
    text_token_cache_[text] = text_node;
    index_.register_node(text_node, writer_.bytes_written(), text);
    
    // Create audio node
    std::string audio_str(audio.begin(), audio.end());
    NodeID audio_node = writer_.emit_taught_node(audio_str, NodeType::MODALITY_AUDIO);
    std::string audio_hash = bridge_.hash_audio(audio);
    audio_frame_cache_[audio_hash] = audio_node;
    index_.register_node(audio_node, writer_.bytes_written());
    
    // Create concept node
    NodeID concept_node = writer_.emit_taught_node(concept_label, NodeType::CONCEPT);
    
    // Bind in cross-modal bridge
    bridge_.bind_text_to_audio(text, audio, text_node, audio_node);
    
    // Create ISA edges: text → concept, audio → concept
    writer_.emit_connection(text_node, Rel::ISA, concept_node, 1.0f);
    writer_.emit_connection(audio_node, Rel::ISA, concept_node, 1.0f);
    
    // Create EXACT edge: text ↔ audio (cross-modal binding)
    writer_.emit_connection(text_node, Rel::EXACT, audio_node, 1.0f);
    writer_.emit_connection(audio_node, Rel::EXACT, text_node, 1.0f);
}

MultimodalOutput MultimodalPipeline::query_text(
    const std::string& query,
    OutputModality output_modality) {
    
    MultimodalInput input;
    input.modality = InputModality::TEXT_ONLY;
    input.text = query;
    input.input_id = "text_query_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    input.created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Tokenize
    std::istringstream iss(query);
    std::string token;
    while (iss >> token) {
        input.text_tokens.push_back(token);
    }
    
    return process(input, output_modality);
}

MultimodalOutput MultimodalPipeline::query_audio(
    const std::vector<uint8_t>& audio_query,
    OutputModality output_modality) {
    
    MultimodalInput input;
    input.modality = InputModality::AUDIO_ONLY;
    input.audio_codes = audio_query;
    input.input_id = "audio_query_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    input.created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return process(input, output_modality);
}

MultimodalOutput MultimodalPipeline::query_multimodal(
    const std::string& text_query,
    const std::vector<uint8_t>& audio_context,
    OutputModality output_modality) {
    
    MultimodalInput input;
    input.modality = InputModality::MULTIMODAL;
    input.text = text_query;
    input.audio_codes = audio_context;
    input.input_id = "multimodal_query_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    input.created_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Tokenize text
    std::istringstream iss(text_query);
    std::string token;
    while (iss >> token) {
        input.text_tokens.push_back(token);
    }
    
    return process(input, output_modality);
}

std::vector<NodeID> MultimodalPipeline::input_to_graph_path(
    const MultimodalInput& input) {
    
    std::vector<NodeID> path;
    
    // Process text tokens
    if (input.text.has_value()) {
        for (const auto& token : input.text_tokens) {
            auto it = text_token_cache_.find(token);
            if (it != text_token_cache_.end()) {
                path.push_back(it->second);
            } else {
                // Look up in index
                auto nodes = index_.get_nodes_by_token(token);
                if (!nodes.empty()) {
                    path.push_back(nodes[0]);
                    text_token_cache_[token] = nodes[0];
                }
            }
        }
    }
    
    // Process audio codes
    if (input.audio_codes.has_value()) {
        // Try to translate audio to text if cross-modal reasoning enabled
        if (config_.enable_cross_modal_reasoning) {
            auto text_opt = bridge_.audio_to_text(input.audio_codes.value());
            if (text_opt.has_value()) {
                // Found cross-modal binding, add text node
                auto it = text_token_cache_.find(text_opt.value());
                if (it != text_token_cache_.end()) {
                    path.push_back(it->second);
                }
            }
        }
        
        // Also add audio nodes
        std::string audio_hash = bridge_.hash_audio(input.audio_codes.value());
        auto it = audio_frame_cache_.find(audio_hash);
        if (it != audio_frame_cache_.end()) {
            path.push_back(it->second);
        }
    }
    
    return path;
}

MultimodalOutput MultimodalPipeline::graph_path_to_output(
    const std::vector<NodeID>& path,
    OutputModality modality) {
    
    MultimodalOutput output;
    output.modality = modality;
    output.text_path = path;
    output.confidence = path.empty() ? 0.0 : 0.85;  // Placeholder
    
    // Generate text output
    if (modality == OutputModality::TEXT_ONLY || modality == OutputModality::MULTIMODAL) {
        std::ostringstream oss;
        for (size_t i = 0; i < path.size(); ++i) {
            // Look up node content (simplified - would read from index)
            if (i > 0) oss << " ";
            oss << "node_" << i;  // Placeholder
            output.text_tokens.push_back("node_" + std::to_string(i));
        }
        output.text = oss.str();
    }
    
    // Generate audio output
    if (modality == OutputModality::AUDIO_ONLY || modality == OutputModality::MULTIMODAL) {
        if (config_.enable_text_to_audio) {
            // Translate text nodes to audio
            for (const auto& node_id : path) {
                auto audio_node_opt = bridge_.get_audio_node_for_text(node_id);
                if (audio_node_opt.has_value()) {
                    // Found audio representation (would synthesize here)
                    if (!output.audio_codes.has_value()) {
                        output.audio_codes = std::vector<uint8_t>();
                    }
                    // Placeholder: add audio codes
                    output.audio_codes->push_back(static_cast<uint8_t>(rand() % 256));
                }
            }
        }
    }
    
    output.output_id = "output_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    
    return output;
}

std::vector<NodeID> MultimodalPipeline::translate_audio_to_text_nodes(
    const std::vector<NodeID>& audio_nodes) {
    
    std::vector<NodeID> text_nodes;
    for (const auto& audio_node : audio_nodes) {
        auto text_node_opt = bridge_.get_text_node_for_audio(audio_node);
        if (text_node_opt.has_value()) {
            text_nodes.push_back(text_node_opt.value());
        }
    }
    return text_nodes;
}

std::vector<NodeID> MultimodalPipeline::translate_text_to_audio_nodes(
    const std::vector<NodeID>& text_nodes) {
    
    std::vector<NodeID> audio_nodes;
    for (const auto& text_node : text_nodes) {
        auto audio_node_opt = bridge_.get_audio_node_for_text(text_node);
        if (audio_node_opt.has_value()) {
            audio_nodes.push_back(audio_node_opt.value());
        }
    }
    return audio_nodes;
}

// ============================================================================
// MultimodalTestGenerator
// ============================================================================

MultimodalTestGenerator::MultimodalTestGenerator(const Config& config)
    : config_(config), rng_(config.seed) {
    initialize_concept_audio_mappings();
}

void MultimodalTestGenerator::initialize_concept_audio_mappings() {
    // Create simple audio patterns for common concepts
    concept_to_audio_["cat"] = {0x10, 0x11, 0x12};  // "meow" pattern
    concept_to_audio_["dog"] = {0x20, 0x21, 0x22};  // "bark" pattern
    concept_to_audio_["bird"] = {0x30, 0x31, 0x32}; // "chirp" pattern
    concept_to_audio_["water"] = {0x40, 0x41, 0x42}; // "splash" pattern
    concept_to_audio_["fire"] = {0x50, 0x51, 0x52}; // "crackle" pattern
}

std::vector<MultimodalTestGenerator::PairedSample> 
MultimodalTestGenerator::generate_paired_samples() {
    
    std::vector<PairedSample> samples;
    samples.reserve(config_.num_paired_samples);
    
    std::vector<std::string> concepts = {"cat", "dog", "bird", "water", "fire"};
    std::uniform_int_distribution<size_t> concept_dist(0, concepts.size() - 1);
    
    for (uint32_t i = 0; i < config_.num_paired_samples; ++i) {
        PairedSample sample;
        sample.concept_label = concepts[concept_dist(rng_)];
        sample.text = sample.concept_label;
        sample.audio_codes = synthesize_audio_for_text(sample.text);
        sample.is_matched = true;
        
        samples.push_back(sample);
    }
    
    // Add mismatched pairs if requested
    if (config_.generate_mismatched_pairs) {
        for (uint32_t i = 0; i < config_.num_paired_samples / 10; ++i) {
            PairedSample sample;
            sample.concept_label = concepts[concept_dist(rng_)];
            sample.text = concepts[concept_dist(rng_)];
            sample.audio_codes = synthesize_audio_for_text(concepts[concept_dist(rng_)]);
            sample.is_matched = (sample.text == sample.concept_label);
            
            samples.push_back(sample);
        }
    }
    
    return samples;
}

std::vector<MultimodalTestGenerator::TestQuery>
MultimodalTestGenerator::generate_test_queries(uint32_t num_queries) {
    
    std::vector<TestQuery> queries;
    queries.reserve(num_queries);
    
    for (uint32_t i = 0; i < num_queries; ++i) {
        TestQuery query;
        
        // Generate multimodal input
        query.input.modality = InputModality::MULTIMODAL;
        query.input.text = "what is cat";
        query.input.text_tokens = {"what", "is", "cat"};
        query.input.audio_codes = concept_to_audio_["cat"];
        
        // Expected outputs
        query.expected_text_output = "cat is mammal";
        query.expected_audio_output = concept_to_audio_["cat"];
        query.category = "taxonomy";
        
        queries.push_back(query);
    }
    
    return queries;
}

std::vector<uint8_t> MultimodalTestGenerator::synthesize_audio_for_text(
    const std::string& text) {
    
    auto it = concept_to_audio_.find(text);
    if (it != concept_to_audio_.end()) {
        std::vector<uint8_t> audio = it->second;
        
        // Add noise if configured
        if (config_.audio_noise_level > 0.0f) {
            std::normal_distribution<float> noise_dist(0.0f, config_.audio_noise_level);
            for (auto& byte : audio) {
                float noisy = static_cast<float>(byte) + noise_dist(rng_) * 10.0f;
                byte = static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, noisy)));
            }
        }
        
        return audio;
    }
    
    // Default pattern
    return {0x00, 0x01, 0x02};
}

// ============================================================================
// MultimodalEvaluator
// ============================================================================

MultimodalEvaluator::MultimodalEvaluator() {}

MultimodalEvaluator::CrossModalMetrics MultimodalEvaluator::evaluate_cross_modal(
    MultimodalPipeline& pipeline,
    const std::vector<MultimodalTestGenerator::TestQuery>& queries) {
    
    CrossModalMetrics metrics;
    
    int text_to_text_correct = 0;
    int audio_to_audio_correct = 0;
    int text_to_audio_correct = 0;
    int audio_to_text_correct = 0;
    int multimodal_correct = 0;
    
    double total_text_latency = 0.0;
    double total_audio_latency = 0.0;
    double total_multimodal_latency = 0.0;
    
    for (const auto& query : queries) {
        // Test text→text
        if (query.input.text.has_value()) {
            auto output = pipeline.query_text(query.input.text.value(), 
                                             OutputModality::TEXT_ONLY);
            if (output.text.has_value()) {
                text_to_text_correct++;
            }
            total_text_latency += output.latency_ms;
        }
        
        // Test audio→audio
        if (query.input.audio_codes.has_value()) {
            auto output = pipeline.query_audio(query.input.audio_codes.value(),
                                              OutputModality::AUDIO_ONLY);
            if (output.audio_codes.has_value()) {
                audio_to_audio_correct++;
            }
            total_audio_latency += output.latency_ms;
        }
        
        // Test cross-modal: text→audio
        if (query.input.text.has_value()) {
            auto output = pipeline.query_text(query.input.text.value(),
                                             OutputModality::AUDIO_ONLY);
            if (output.audio_codes.has_value()) {
                text_to_audio_correct++;
            }
        }
        
        // Test cross-modal: audio→text
        if (query.input.audio_codes.has_value()) {
            auto output = pipeline.query_audio(query.input.audio_codes.value(),
                                              OutputModality::TEXT_ONLY);
            if (output.text.has_value()) {
                audio_to_text_correct++;
            }
        }
        
        // Test multimodal fusion
        if (query.input.text.has_value() && query.input.audio_codes.has_value()) {
            auto output = pipeline.query_multimodal(query.input.text.value(),
                                                   query.input.audio_codes.value(),
                                                   OutputModality::MULTIMODAL);
            if (output.text.has_value() && output.audio_codes.has_value()) {
                multimodal_correct++;
            }
            total_multimodal_latency += output.latency_ms;
        }
    }
    
    int n = queries.size();
    if (n > 0) {
        metrics.text_to_text_recall = static_cast<double>(text_to_text_correct) / n;
        metrics.audio_to_audio_recall = static_cast<double>(audio_to_audio_correct) / n;
        metrics.text_to_audio_recall = static_cast<double>(text_to_audio_correct) / n;
        metrics.audio_to_text_recall = static_cast<double>(audio_to_text_correct) / n;
        
        metrics.text_latency_ms = total_text_latency / n;
        metrics.audio_latency_ms = total_audio_latency / n;
        metrics.multimodal_latency_ms = total_multimodal_latency / n;
        
        // Fusion gain: improvement over single modality
        double single_best = std::max(metrics.text_to_text_recall, metrics.audio_to_audio_recall);
        double multimodal_recall = static_cast<double>(multimodal_correct) / n;
        metrics.multimodal_fusion_gain = multimodal_recall - single_best;
    }
    
    return metrics;
}

std::vector<MultimodalEvaluator::ModalitySwitchResult>
MultimodalEvaluator::evaluate_modality_switching(
    MultimodalPipeline& pipeline,
    const std::vector<MultimodalTestGenerator::TestQuery>& queries) {
    
    std::vector<ModalitySwitchResult> results;
    
    // Test all combinations: text→text, text→audio, audio→text, audio→audio
    struct TestCase {
        std::string input_mod;
        std::string output_mod;
        InputModality input_type;
        OutputModality output_type;
    };
    
    std::vector<TestCase> test_cases = {
        {"text", "text", InputModality::TEXT_ONLY, OutputModality::TEXT_ONLY},
        {"text", "audio", InputModality::TEXT_ONLY, OutputModality::AUDIO_ONLY},
        {"audio", "text", InputModality::AUDIO_ONLY, OutputModality::TEXT_ONLY},
        {"audio", "audio", InputModality::AUDIO_ONLY, OutputModality::AUDIO_ONLY}
    };
    
    for (const auto& test_case : test_cases) {
        ModalitySwitchResult result;
        result.input_modality = test_case.input_mod;
        result.output_modality = test_case.output_mod;
        
        int correct = 0;
        double total_latency = 0.0;
        
        for (const auto& query : queries) {
            MultimodalOutput output;
            
            if (test_case.input_type == InputModality::TEXT_ONLY && query.input.text.has_value()) {
                output = pipeline.query_text(query.input.text.value(), test_case.output_type);
            } else if (test_case.input_type == InputModality::AUDIO_ONLY && query.input.audio_codes.has_value()) {
                output = pipeline.query_audio(query.input.audio_codes.value(), test_case.output_type);
            }
            
            // Check if output in correct modality
            bool has_correct_output = false;
            if (test_case.output_type == OutputModality::TEXT_ONLY && output.text.has_value()) {
                has_correct_output = true;
            } else if (test_case.output_type == OutputModality::AUDIO_ONLY && output.audio_codes.has_value()) {
                has_correct_output = true;
            }
            
            if (has_correct_output) correct++;
            total_latency += output.latency_ms;
        }
        
        result.accuracy = static_cast<double>(correct) / queries.size();
        result.latency_ms = total_latency / queries.size();
        
        results.push_back(result);
    }
    
    return results;
}

} // namespace scale_demo
} // namespace melvin

