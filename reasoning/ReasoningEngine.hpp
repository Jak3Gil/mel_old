#pragma once

#include "melvin_types.h"
#include "uca_types.h"
#include "../input/PerceptionEngine.hpp"
#include "../graph_reasoning.hpp"
#include <string>
#include <memory>
#include <vector>
#include <chrono>

namespace melvin {

/**
 * Thought - Represents the result of reasoning
 */
struct Thought {
    std::string text;           // The reasoning result
    float confidence;           // Path score/confidence
    OutputType output_type;     // Inferred modality
    std::vector<NodeID> path;    // Reasoning path taken
    std::vector<Rel> relations;  // Relations used
    uint64_t timestamp;         // When this thought occurred
    std::string query;          // Original query that led to this thought
    
    Thought() : confidence(0.0f), output_type(OutputType::TEXT), timestamp(0) {}
    Thought(const std::string& t, float c, OutputType ot) 
        : text(t), confidence(c), output_type(ot) {
        timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

/**
 * ReasoningEngine - Uses graph reasoning to generate thoughts
 * 
 * Integrates with existing graph reasoning system:
 * - Beam search for path finding
 * - Iterative deepening for exploration
 * - Anchor selection for stability
 * - Confidence scoring
 */
class ReasoningEngine {
public:
    ReasoningEngine(std::shared_ptr<Storage> storage);
    ~ReasoningEngine() = default;

    // Core reasoning methods
    Thought reason(const InputConcept& input);
    std::vector<Thought> reason_batch(const std::vector<InputConcept>& inputs);
    
    // Query processing
    Thought process_query(const std::string& query);
    Thought process_concept_query(const InputConcept& input_concept);
    
    // Reasoning strategies
    Thought beam_search_reasoning(const InputConcept& input);
    Thought iterative_deepening_reasoning(const InputConcept& input);
    Thought anchor_based_reasoning(const InputConcept& input);
    
    // Output type inference
    OutputType infer_output_type(const InputConcept& input, const std::vector<NodeID>& path);
    std::string generate_response_text(const std::vector<NodeID>& path, OutputType type);
    
    // Configuration
    void set_graph_reasoning_config(const GraphReasoningConfig& config);
    void set_confidence_threshold(float threshold);
    void set_max_depth(int max_depth);
    void set_beam_width(int beam_width);
    
    // Statistics
    size_t get_reasoning_count() const;
    float get_average_confidence() const;
    size_t get_average_path_length() const;
    void reset_statistics();

private:
    std::shared_ptr<Storage> storage_;
    std::unique_ptr<GraphReasoningSystem> graph_system_;
    
    // Configuration
    GraphReasoningConfig config_;
    float confidence_threshold_;
    int max_depth_;
    int beam_width_;
    
    // Statistics
    std::atomic<size_t> reasoning_count_;
    std::atomic<float> total_confidence_;
    std::atomic<size_t> total_path_length_;
    
    // Internal methods
    std::vector<NodeID> find_anchor_nodes(const InputConcept& input);
    std::vector<NodeID> expand_from_anchors(const std::vector<NodeID>& anchors, int depth);
    float compute_path_confidence(const std::vector<NodeID>& path, const std::vector<Rel>& relations);
    std::string path_to_text(const std::vector<NodeID>& path, const std::vector<Rel>& relations);
    bool should_abstain(float confidence);
    void update_statistics(const Thought& thought);
    
    // Helper methods
    std::vector<std::string> tokenize(const std::string& text);
    NodeID get_or_create_node(const std::string& text);
    
    // Query templates
    bool is_definition_query(const InputConcept& input);
    bool is_factual_query(const InputConcept& input);
    bool is_arithmetic_query(const InputConcept& input);
    bool is_creative_query(const InputConcept& input);
};

} // namespace melvin
