#pragma once

#include "melvin_types.h"
#include "beam.hpp"
#include "scoring.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace melvin {

// Forward declarations
class Store;

// ===== PATH-TO-PHRASE ALGORITHM DECLARATIONS =====

/**
 * Core path-to-phrase function - converts BeamPaths to natural language
 */
std::string path_to_phrase(const BeamPath& path, Store*);

/**
 * Helper function to extract text from nodes
 */
std::string get_node_text(const NodeID&, Store*);

/**
 * Handle relation codes and return connecting words
 */
std::string handle_relation_code(Rel relation);

/**
 * Clean up generated phrases for natural language output
 */
std::string clean_phrase(const std::string& raw_phrase);

// ===== END PATH-TO-PHRASE DECLARATION =====
struct BeamPath;
struct Query;

// Clause structure for text generation
struct Clause {
    std::string subject;
    std::string predicate;
    std::string object;
    std::string connector;  // "and", "because", "therefore", etc.
    double confidence;
    Rel relation_type;      // The relation type that generated this clause
    
    Clause() : confidence(0.0), relation_type(Rel::EXACT) {}
};

// Template structure for clause generation
struct ClauseTemplate {
    std::string pattern;
    std::vector<Rel> supported_relations;
    double priority;
    
    ClauseTemplate() : priority(1.0) {}
    ClauseTemplate(const std::string& p, const std::vector<Rel>& rels, double prio = 1.0)
        : pattern(p), supported_relations(rels), priority(prio) {}
};

// Connector selection rules
struct ConnectorRule {
    std::string connector;
    std::vector<std::string> conditions;
    double weight;
    
    ConnectorRule() : weight(1.0) {}
    ConnectorRule(const std::string& conn, const std::vector<std::string>& conds, double w = 1.0)
        : connector(conn), conditions(conds), weight(w) {}
};

// NLG configuration
struct NLGConfig {
    // Confidence thresholds
    double min_clause_confidence = 0.3;
    double min_path_confidence = 0.4;
    double high_confidence_threshold = 0.7;
    
    // Output formatting
    bool use_hedges = true;              // Use "likely", "suggests", etc.
    bool use_connectors = true;          // Use connectors between clauses
    size_t max_clauses = 3;              // Maximum clauses per response
    size_t max_sentence_length = 100;    // Maximum characters per sentence
    
    // Template matching
    double template_similarity_threshold = 0.8;
    bool enable_template_fallback = true;
    
    // Safety and fallback
    bool enable_llm_fallback = false;    // Use LLM for fallback (requires external integration)
    std::string fallback_prefix = "Based on available information: ";
    
    NLGConfig() = default;
};

// Path to clause converter
class PathToClauseConverter {
private:
    Store* store_;
    NLGConfig config_;
    std::vector<ClauseTemplate> templates_;
    
public:
    PathToClauseConverter(Store* store, const NLGConfig& config = NLGConfig()) 
        : store_(store), config_(config) {
        initialize_templates();
    }
    
    // Convert a beam path to a clause
    Clause convert_path_to_clause(const BeamPath& path) const;
    
    // Convert multiple paths to clauses
    std::vector<Clause> convert_paths_to_clauses(const std::vector<BeamPath>& paths) const;
    
    // Configuration
    void set_config(const NLGConfig& config) { config_ = config; }
    const NLGConfig& get_config() const { return config_; }
    
private:
    // Template matching
    ClauseTemplate find_best_template(const BeamPath& path) const;
    double compute_template_match_score(const BeamPath& path, const ClauseTemplate& clause_template) const;
    
    // Clause construction
    std::string extract_subject(const BeamPath& path) const;
    std::string extract_predicate(const BeamPath& path) const;
    std::string extract_object(const BeamPath& path) const;
    
    // Node content extraction
    std::string get_node_content(const NodeID& node) const;
    std::string get_relation_text(Rel relation) const;
    
    // Template initialization
    void initialize_templates();
};

// Clause connector selector
class ConnectorSelector {
private:
    std::vector<ConnectorRule> rules_;
    NLGConfig config_;
    
public:
    ConnectorSelector(const NLGConfig& config = NLGConfig()) : config_(config) {
        initialize_rules();
    }
    
    // Select appropriate connector between clauses
    std::string select_connector(const Clause& prev_clause, const Clause& current_clause) const;
    
    // Select connector for the beginning of a sentence
    std::string select_initial_connector(const Clause& clause) const;
    
    // Configuration
    void set_config(const NLGConfig& config) { config_ = config; }
    
private:
    // Rule evaluation
    bool evaluate_conditions(const Clause& clause, const std::vector<std::string>& conditions) const;
    double compute_rule_weight(const Clause& prev, const Clause& current, const ConnectorRule& rule) const;
    
    // Rule initialization
    void initialize_rules();
};

// Text formatter
class TextFormatter {
private:
    NLGConfig config_;
    
public:
    TextFormatter(const NLGConfig& config = NLGConfig()) : config_(config) {}
    
    // Format a single clause into text
    std::string format_clause(const Clause& clause) const;
    
    // Format multiple clauses into a complete response
    std::string format_response(const std::vector<Clause>& clauses) const;
    
    // Add confidence hedges
    std::string add_confidence_hedge(const std::string& text, double confidence) const;
    
    // Apply final formatting (capitalization, punctuation, etc.)
    std::string apply_final_formatting(const std::string& text) const;
    
    // Configuration
    void set_config(const NLGConfig& config) { config_ = config; }
    
private:
    // Text processing utilities
    std::string capitalize_first_letter(const std::string& text) const;
    std::string add_punctuation(const std::string& text) const;
    std::string normalize_whitespace(const std::string& text) const;
};

// Confidence calibrator
class ConfidenceCalibrator {
private:
    NLGConfig config_;
    
public:
    ConfidenceCalibrator(const NLGConfig& config = NLGConfig()) : config_(config) {}
    
    // Compute calibrated confidence for a response
    double compute_response_confidence(const std::vector<Clause>& clauses, 
                                     const std::vector<BeamPath>& paths) const;
    
    // Determine confidence level category
    enum ConfidenceLevel { LOW, MEDIUM, HIGH };
    ConfidenceLevel categorize_confidence(double confidence) const;
    
    // Get appropriate hedge for confidence level
    std::string get_confidence_hedge(double confidence) const;
    
    // Configuration
    void set_config(const NLGConfig& config) { config_ = config; }
    
private:
    // Confidence computation
    double compute_clause_confidence(const Clause& clause) const;
    double compute_path_confidence(const BeamPath& path) const;
    double compute_evidence_consistency(const std::vector<Clause>& clauses) const;
};

// Safety fallback system
class SafetyFallback {
private:
    NLGConfig config_;
    
public:
    SafetyFallback(const NLGConfig& config = NLGConfig()) : config_(config) {}
    
    // Check if fallback is needed
    bool needs_fallback(const std::vector<Clause>& clauses, double confidence) const;
    
    // Generate fallback response
    std::string generate_fallback_response(const std::vector<Clause>& clauses, 
                                         const std::vector<BeamPath>& paths) const;
    
    // Extract top facts for fallback
    std::vector<std::string> extract_top_facts(const std::vector<Clause>& clauses) const;
    
    // Configuration
    void set_config(const NLGConfig& config) { config_ = config; }
    
private:
    // Fact extraction
    std::string format_fact(const Clause& clause) const;
    bool is_high_confidence_fact(const Clause& clause) const;
};

// Main NLG orchestrator
class GraphNLG {
private:
    PathToClauseConverter converter_;
    ConnectorSelector connector_selector_;
    TextFormatter formatter_;
    ConfidenceCalibrator calibrator_;
    SafetyFallback fallback_;
    
    Store* store_;
    NLGConfig config_;
    
public:
    GraphNLG(Store* store, const NLGConfig& config = NLGConfig())
        : converter_(store, config), store_(store), config_(config) {}
    
    // Main text generation interface
    std::string generate_response(const std::vector<BeamPath>& paths, const Query& query) const;
    
    // Generate response with custom configuration
    std::string generate_response_with_config(const std::vector<BeamPath>& paths, 
                                            const Query& query,
                                            const NLGConfig& custom_config) const;
    
    // Generate single-sentence response
    std::string generate_simple_response(const BeamPath& best_path, const Query& query) const;
    
    // Try new path-to-phrase algorithm
    std::string try_path_to_phrase(const std::vector<BeamPath>& paths) const;
    
    // Configuration
    void set_config(const NLGConfig& config);
    const NLGConfig& get_config() const { return config_; }
    
private:
    // Response generation pipeline
    std::vector<Clause> create_clauses(const std::vector<BeamPath>& paths) const;
    std::vector<Clause> filter_and_rank_clauses(const std::vector<Clause>& clauses) const;
    std::string combine_clauses(const std::vector<Clause>& clauses) const;
    std::string apply_safety_checks(const std::string& response, 
                                   const std::vector<Clause>& clauses,
                                   double confidence) const;
};

// Template registry for clause patterns
class TemplateRegistry {
private:
    std::vector<ClauseTemplate> templates_;
    std::unordered_map<Rel, std::vector<ClauseTemplate*>> relation_templates_;
    
public:
    TemplateRegistry() { initialize_default_templates(); }
    
    // Template management
    void add_template(const ClauseTemplate& clause_template);
    void remove_template(const std::string& pattern);
    const std::vector<ClauseTemplate>& get_templates() const { return templates_; }
    
    // Template matching
    ClauseTemplate* find_best_template(Rel relation, const BeamPath& path) const;
    std::vector<ClauseTemplate*> get_templates_for_relation(Rel relation) const;
    
    // Template scoring
    double score_template_match(const BeamPath& path, const ClauseTemplate& clause_template) const;
    
private:
    void initialize_default_templates();
    void index_templates();
};

// Utility functions
namespace NLGUtils {
    // Text processing
    std::string clean_text(const std::string& text);
    std::string normalize_case(const std::string& text);
    std::string remove_redundancy(const std::string& text);
    
    // Clause validation
    bool is_valid_clause(const Clause& clause);
    bool clauses_are_contradictory(const Clause& clause1, const Clause& clause2);
    
    // Path analysis
    bool path_has_definitional_pattern(const BeamPath& path);
    bool path_has_causal_pattern(const BeamPath& path);
    bool path_has_comparison_pattern(const BeamPath& path);
    
    // Confidence computation
    double compute_path_coherence(const BeamPath& path);
    double compute_evidence_strength(const std::vector<Clause>& clauses);
    
    // Template matching
    std::vector<std::string> extract_template_variables(const std::string& pattern);
    std::string substitute_template_variables(const std::string& pattern, 
                                            const std::unordered_map<std::string, std::string>& variables);
}

} // namespace melvin
