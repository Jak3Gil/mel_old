#include "generator.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace melvin {
namespace generator{

// ============================================================================
// Config Implementation
// ============================================================================

Generator::Config::Config() {}

// ============================================================================
// Implementation Class
// ============================================================================

class Generator::Impl {
public:
    Config config_;
    hybrid::HybridPredictor* predictor_;
    tokenizer::Tokenizer* tokenizer_;
    Storage* storage_;
    
    Stats stats_;
    
    Impl(hybrid::HybridPredictor* predictor, tokenizer::Tokenizer* tokenizer,
         Storage* storage, const Config& config)
        : config_(config), predictor_(predictor), tokenizer_(tokenizer), storage_(storage) {}
    
    std::string generate(const std::string& prompt) {
        if (!predictor_ || !tokenizer_) {
            return "";
        }
        
        // Encode prompt
        auto tokens = tokenizer_->encode(prompt);
        
        // Generate based on strategy
        std::vector<uint32_t> generated;
        switch (config_.strategy) {
            case DecodingStrategy::GREEDY:
                generated = decode_greedy(tokens);
                break;
            case DecodingStrategy::BEAM_SEARCH:
                generated = decode_beam_search(tokens);
                break;
            case DecodingStrategy::NUCLEUS_SAMPLING:
                generated = decode_nucleus(tokens);
                break;
            case DecodingStrategy::GRAPH_CONSTRAINED:
                generated = decode_graph_constrained(tokens);
                break;
        }
        
        // Decode to text
        std::string result = tokenizer_->decode(generated);
        
        // Update stats
        stats_.generations++;
        stats_.tokens_generated += generated.size();
        stats_.avg_length = stats_.avg_length * 0.99f + generated.size() * 0.01f;
        
        return result;
    }
    
    std::string generate_with_concepts(
        const std::string& prompt,
        const std::vector<std::string>& required_concepts
    ) {
        if (!predictor_ || !tokenizer_ || !storage_) {
            return "";
        }
        
        // Convert concept strings to node IDs
        std::vector<NodeID> concept_nodes;
        for (const auto& concept : required_concepts) {
            auto nodes = storage_->find_nodes(concept);
            if (!nodes.empty()) {
                concept_nodes.push_back(nodes[0].id);
            }
        }
        
        // Generate with concept constraints
        auto tokens = predictor_->generate_with_concepts(prompt, concept_nodes, config_.max_length);
        return tokenizer_->decode(tokens);
    }
    
    std::string answer_question(const std::string& question, bool include_reasoning) {
        // Generate answer
        std::string answer = generate(question);
        
        if (include_reasoning && storage_) {
            // Try to find reasoning path
            auto question_tokens = tokenizer_->encode(question);
            std::vector<NodeID> question_nodes;
            for (auto token : question_tokens) {
                if (tokenizer_->has_token_mapping(token)) {
                    question_nodes.push_back(tokenizer_->token_to_node(token));
                }
            }
            
            if (question_nodes.size() >= 2) {
                auto paths = storage_->find_paths(question_nodes[0], 
                                                 question_nodes[question_nodes.size()-1], 5);
                
                if (!paths.empty()) {
                    answer += "\n\nReasoning: ";
                    for (NodeID node_id : paths[0].nodes) {
                        answer += storage_->get_node_content(node_id) + " → ";
                    }
                    answer = answer.substr(0, answer.size() - 3);  // Remove last arrow
                }
            }
        }
        
        return answer;
    }
    
    std::string complete(const std::string& partial_text) {
        return generate(partial_text);
    }
    
    std::string generate_from_path(const ReasoningPath& path, const std::string& prompt) {
        if (!storage_ || !tokenizer_) {
            return "";
        }
        
        std::string result = prompt;
        
        // Generate text following the path
        for (NodeID node_id : path.nodes) {
            std::string content = storage_->get_node_content(node_id);
            if (!content.empty()) {
                result += " " + content;
            }
        }
        
        return result;
    }
    
    std::string explain_concept(const std::string& concept) {
        if (!storage_ || !tokenizer_) {
            return "";
        }
        
        // Find concept node
        auto nodes = storage_->find_nodes(concept);
        if (nodes.empty()) {
            return "Concept not found in knowledge graph.";
        }
        
        NodeID concept_node = nodes[0].id;
        
        // Get related nodes
        auto outgoing = storage_->get_edges_from(concept_node);
        auto incoming = storage_->get_edges_to(concept_node);
        
        std::string explanation = concept + " is connected to:\n";
        
        // Add outgoing connections
        for (const auto& edge : outgoing) {
            std::string target = storage_->get_node_content(edge.to_id);
            std::string relation_type = (edge.relation == RelationType::EXACT) ? "EXACT" : "LEAP";
            explanation += "  - " + target + " [" + relation_type + 
                          ", conf: " + std::to_string(edge.confidence) + "]\n";
        }
        
        // Add incoming connections
        for (const auto& edge : incoming) {
            std::string source = storage_->get_node_content(edge.from_id);
            std::string relation_type = (edge.relation == RelationType::EXACT) ? "EXACT" : "LEAP";
            explanation += "  - from " + source + " [" + relation_type + "]\n";
        }
        
        return explanation;
    }
    
    ValidationResult validate_text(const std::string& text) {
        ValidationResult result;
        result.is_valid = true;
        result.confidence = 1.0f;
        
        if (!storage_ || !tokenizer_) {
            return result;
        }
        
        // Tokenize
        auto tokens = tokenizer_->encode(text);
        
        // Check each token against graph
        float total_validity = 0.0f;
        int checked_tokens = 0;
        
        for (size_t i = 0; i < tokens.size(); i++) {
            uint32_t token_id = tokens[i];
            
            if (tokenizer_->has_token_mapping(token_id)) {
                NodeID node_id = tokenizer_->token_to_node(token_id);
                
                // Check if node exists
                Node node;
                if (storage_->get_node(node_id, node)) {
                    result.supporting_nodes.push_back(node_id);
                    total_validity += 1.0f;
                } else {
                    total_validity += 0.5f;  // Token exists but not in graph
                }
                checked_tokens++;
            }
        }
        
        if (checked_tokens > 0) {
            result.confidence = total_validity / checked_tokens;
            result.is_valid = result.confidence > config_.min_graph_validity;
        }
        
        result.explanation = result.is_valid ? 
            "Text is supported by " + std::to_string(result.supporting_nodes.size()) + " graph nodes" :
            "Text has insufficient graph support (confidence: " + std::to_string(result.confidence) + ")";
        
        return result;
    }
    
    bool verify_claim(const std::string& claim) {
        auto validation = validate_text(claim);
        return validation.is_valid && validation.confidence > 0.7f;
    }
    
    std::vector<uint32_t> decode_greedy(const std::vector<uint32_t>& prompt_tokens) {
        std::vector<uint32_t> generated = prompt_tokens;
        std::vector<NodeID> concept_context;
        
        // Build initial context
        for (auto token : prompt_tokens) {
            if (tokenizer_->has_token_mapping(token)) {
                concept_context.push_back(tokenizer_->token_to_node(token));
            }
        }
        
        for (size_t i = generated.size(); i < config_.max_length; i++) {
            auto [probs, _] = predictor_->predict_next(concept_context, generated);
            
            // Pick highest probability
            auto max_it = std::max_element(probs.begin(), probs.end());
            uint32_t next_token = std::distance(probs.begin(), max_it);
            
            if (next_token == tokenizer_->eos_token_id()) break;
            
            generated.push_back(next_token);
            
            // Update context
            if (tokenizer_->has_token_mapping(next_token)) {
                concept_context.push_back(tokenizer_->token_to_node(next_token));
            }
        }
        
        return generated;
    }
    
    std::vector<uint32_t> decode_beam_search(const std::vector<uint32_t>& prompt_tokens) {
        // Simplified beam search
        struct Beam {
            std::vector<uint32_t> tokens;
            std::vector<NodeID> context;
            float score;
        };
        
        std::vector<Beam> beams;
        Beam initial;
        initial.tokens = prompt_tokens;
        initial.score = 0.0f;
        
        // Build initial context
        for (auto token : prompt_tokens) {
            if (tokenizer_->has_token_mapping(token)) {
                initial.context.push_back(tokenizer_->token_to_node(token));
            }
        }
        
        beams.push_back(initial);
        
        for (size_t step = 0; step < config_.max_length - prompt_tokens.size(); step++) {
            std::vector<Beam> candidates;
            
            for (const auto& beam : beams) {
                auto [probs, _] = predictor_->predict_next(beam.context, beam.tokens);
                
                // Get top tokens
                std::vector<std::pair<float, uint32_t>> sorted_probs;
                for (size_t i = 0; i < probs.size(); i++) {
                    sorted_probs.push_back({probs[i], i});
                }
                std::sort(sorted_probs.begin(), sorted_probs.end(), std::greater<>());
                
                // Expand beam
                for (int k = 0; k < config_.beam_width && k < sorted_probs.size(); k++) {
                    Beam new_beam = beam;
                    uint32_t token = sorted_probs[k].second;
                    float prob = sorted_probs[k].first;
                    
                    new_beam.tokens.push_back(token);
                    new_beam.score += std::log(prob + 1e-10f);
                    
                    if (tokenizer_->has_token_mapping(token)) {
                        new_beam.context.push_back(tokenizer_->token_to_node(token));
                    }
                    
                    candidates.push_back(new_beam);
                }
            }
            
            // Keep top beams
            std::sort(candidates.begin(), candidates.end(),
                     [](const Beam& a, const Beam& b) { return a.score > b.score; });
            
            beams.clear();
            for (int k = 0; k < config_.beam_width && k < candidates.size(); k++) {
                beams.push_back(candidates[k]);
            }
            
            if (beams.empty()) break;
        }
        
        return beams.empty() ? prompt_tokens : beams[0].tokens;
    }
    
    std::vector<uint32_t> decode_nucleus(const std::vector<uint32_t>& prompt_tokens) {
        if (!predictor_) {
            return prompt_tokens;
        }
        
        // Use the predictor's generate method with nucleus sampling
        std::vector<NodeID> concept_context;
        for (auto token : prompt_tokens) {
            if (tokenizer_->has_token_mapping(token)) {
                concept_context.push_back(tokenizer_->token_to_node(token));
            }
        }
        
        auto tokens = predictor_->generate(
            tokenizer_->decode(prompt_tokens),
            config_.max_length,
            config_.temperature,
            !config_.graph_constrained
        );
        
        return tokens;
    }
    
    std::vector<uint32_t> decode_graph_constrained(const std::vector<uint32_t>& prompt_tokens) {
        stats_.graph_constrained_generations++;
        
        // Similar to greedy but with graph validity check
        std::vector<uint32_t> generated = prompt_tokens;
        std::vector<NodeID> concept_context;
        
        for (auto token : prompt_tokens) {
            if (tokenizer_->has_token_mapping(token)) {
                concept_context.push_back(tokenizer_->token_to_node(token));
            }
        }
        
        for (size_t i = generated.size(); i < config_.max_length; i++) {
            auto [probs, _] = predictor_->predict_next(concept_context, generated);
            
            // Apply graph gating
            auto gated_probs = predictor_->apply_graph_gate(probs, concept_context);
            
            // Sample from gated distribution
            float r = static_cast<float>(rand()) / RAND_MAX;
            float cumsum = 0.0f;
            uint32_t next_token = 0;
            
            for (size_t j = 0; j < gated_probs.size(); j++) {
                cumsum += gated_probs[j];
                if (r <= cumsum) {
                    next_token = j;
                    break;
                }
            }
            
            if (next_token == tokenizer_->eos_token_id()) break;
            
            // Check validity
            float validity = predictor_->is_valid_token(next_token, concept_context);
            if (validity < config_.min_graph_validity) {
                stats_.hallucinations_prevented++;
                break;  // Stop if no valid continuation
            }
            
            generated.push_back(next_token);
            
            if (tokenizer_->has_token_mapping(next_token)) {
                concept_context.push_back(tokenizer_->token_to_node(next_token));
            }
        }
        
        return generated;
    }
};

// ============================================================================
// Public Interface
// ============================================================================

Generator::Generator(
    hybrid::HybridPredictor* predictor,
    tokenizer::Tokenizer* tokenizer,
    Storage* storage,
    const Config& config
) : impl_(std::make_unique<Impl>(predictor, tokenizer, storage, config)) {}

Generator::~Generator() = default;

std::string Generator::generate(const std::string& prompt) {
    return impl_->generate(prompt);
}

std::string Generator::generate_with_concepts(
    const std::string& prompt,
    const std::vector<std::string>& required_concepts
) {
    return impl_->generate_with_concepts(prompt, required_concepts);
}

std::string Generator::answer_question(const std::string& question, bool include_reasoning) {
    return impl_->answer_question(question, include_reasoning);
}

std::string Generator::complete(const std::string& partial_text) {
    return impl_->complete(partial_text);
}

std::string Generator::generate_from_path(const ReasoningPath& path, const std::string& prompt) {
    return impl_->generate_from_path(path, prompt);
}

std::string Generator::explain_concept(const std::string& concept) {
    return impl_->explain_concept(concept);
}

Generator::ValidationResult Generator::validate_text(const std::string& text) {
    return impl_->validate_text(text);
}

bool Generator::verify_claim(const std::string& claim) {
    return impl_->verify_claim(claim);
}

std::vector<uint32_t> Generator::decode_greedy(const std::vector<uint32_t>& prompt_tokens) {
    return impl_->decode_greedy(prompt_tokens);
}

std::vector<uint32_t> Generator::decode_beam_search(const std::vector<uint32_t>& prompt_tokens) {
    return impl_->decode_beam_search(prompt_tokens);
}

std::vector<uint32_t> Generator::decode_nucleus(const std::vector<uint32_t>& prompt_tokens) {
    return impl_->decode_nucleus(prompt_tokens);
}

std::vector<uint32_t> Generator::decode_graph_constrained(const std::vector<uint32_t>& prompt_tokens) {
    return impl_->decode_graph_constrained(prompt_tokens);
}

void Generator::set_config(const Config& config) {
    impl_->config_ = config;
}

const Generator::Config& Generator::get_config() const {
    return impl_->config_;
}

void Generator::set_strategy(DecodingStrategy strategy) {
    impl_->config_.strategy = strategy;
}

Generator::DecodingStrategy Generator::get_strategy() const {
    return impl_->config_.strategy;
}

Generator::Stats Generator::get_stats() const {
    return impl_->stats_;
}

void Generator::reset_stats() {
    impl_->stats_ = Stats();
}

void Generator::print_stats() const {
    auto stats = get_stats();
    std::cout << "\n=== Generator Statistics ===\n";
    std::cout << "Generations: " << stats.generations << "\n";
    std::cout << "Tokens generated: " << stats.tokens_generated << "\n";
    std::cout << "Avg length: " << stats.avg_length << " tokens\n";
    std::cout << "Avg confidence: " << stats.avg_confidence << "\n";
    std::cout << "Graph-constrained: " << stats.graph_constrained_generations << "\n";
    std::cout << "Hallucinations prevented: " << stats.hallucinations_prevented << "\n";
    std::cout << "============================\n\n";
}

} // namespace generator
} // namespace melvin

