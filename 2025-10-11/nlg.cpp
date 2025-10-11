#include "nlg.hpp"
#include "storage.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>

namespace melvin {

// ===== PATH-TO-PHRASE ALGORITHM IMPLEMENTATION =====

/**
 * Path-to-Phrase Algorithm
 * Converts graph reasoning paths directly to natural language
 * Follows the organic design: output = replay of path walked
 */
std::string path_to_phrase(const BeamPath& path, Store* store) {
    if (path.edges.empty() || !store) {
        return "";
    }
    
    std::string phrase = "";
    NodeID last_emitted_node = {};
    
    // Traverse path edges
    for (size_t i = 0; i < path.edges.size(); ++i) {
        const EdgeRec& edge = path.edges[i];
        melvin::Rel relation = static_cast<melvin::Rel>(edge.rel);
        
        NodeID src_node, dst_node;
        std::copy(edge.src, edge.src + 32, src_node.begin());
        std::copy(edge.dst, edge.dst + 32, dst_node.begin());
        
        // 1. Emit NodeA (if not just emitted)
        if (i == 0 || src_node != last_emitted_node) {
            std::string src_text = get_node_text(src_node, store);
            if (!src_text.empty()) {
                if (!phrase.empty()) {
                    phrase += " ";
                }
                phrase += src_text;
                last_emitted_node = src_node;
            }
        }
        
        // 2. Handle Relation Code (2-bit)
        std::string relation_text = handle_relation_code(relation);
        if (!relation_text.empty()) {
            phrase += " " + relation_text;
        }
        
        // 3. Emit NodeB
        std::string dst_text = get_node_text(dst_node, store);
        if (!dst_text.empty()) {
            phrase += " " + dst_text;
            last_emitted_node = dst_node;
        }
    }
    
    // 3. Clean up
    return clean_phrase(phrase);
}

/**
 * Get text content from a node
 */
std::string get_node_text(const NodeID& node_id, Store* store) {
    if (!store) return "";
    
    NodeRecHeader header;
    std::vector<uint8_t> payload;
    
    if (!store->get_node(node_id, header, payload)) {
        return "";
    }
    
    // Extract text from payload
    if (payload.empty()) return "";
    
    std::string text(payload.begin(), payload.end());
    
    // Basic cleanup
    text.erase(std::remove(text.begin(), text.end(), '\0'), text.end());
    
    // Remove common artifacts
    if (text == "<UNK>" || text == "<PAD>" || text == "<BOS>" || text == "<EOS>") {
        return "";
    }
    
    return text;
}

/**
 * Handle relation codes and return appropriate connecting words
 */
std::string handle_relation_code(melvin::Rel relation) {
    switch (relation) {
        case Rel::EXACT:        // 00 - Identity/equivalence
            return "is";         // Default to "is" for identity
            
        case Rel::TEMPORAL:     // 01 - Sequential/temporal
            return "";          // Flow naturally, words connect themselves
            
        case Rel::LEAP:         // 10 - Inference/hypothesis
            return "might relate to";  // Soft inference indicator
            
        case Rel::GENERALIZATION: // 11 - Category/abstraction  
            return "is a type of";    // Category membership
            
        default:
            return "";          // Unknown relation, skip connector
    }
}

/**
 * Clean up the generated phrase for natural language output
 */
std::string clean_phrase(const std::string& raw_phrase) {
    if (raw_phrase.empty()) return "";
    
    std::string phrase = raw_phrase;
    
    // Remove multiple spaces
    phrase = std::regex_replace(phrase, std::regex("\\s+"), " ");
    
    // Remove leading/trailing whitespace
    phrase.erase(0, phrase.find_first_not_of(" \t\r\n"));
    phrase = phrase.substr(0, phrase.find_last_not_of(" \t\r\n") + 1);
    
    // Merge small words with neighbors
    std::vector<std::string> merge_words = {"a", "the", "an", "is", "are", "of", "in", "at", "on"};
    for (const std::string& word : merge_words) {
        std::string pattern = " " + word + " ";
        std::string replacement = " " + word + " ";
        phrase = std::regex_replace(phrase, std::regex(pattern), replacement);
    }
    
    // Capitalize first letter
    if (!phrase.empty() && std::islower(phrase[0])) {
        phrase[0] = std::toupper(phrase[0]);
    }
    
    // Add period if no ending punctuation
    if (!phrase.empty() && !std::strchr(".!?", phrase.back())) {
        phrase += ".";
    }
    
    return phrase;
}

// ===== END PATH-TO-PHRASE ALGORITHM =====

// PathToClauseConverter implementation
void PathToClauseConverter::initialize_templates() {
    // Definitional templates
    templates_.emplace_back("[X] are [Y]", std::vector<Rel>{Rel::EXACT, Rel::GENERALIZATION}, 1.0);
    templates_.emplace_back("[X] is [Y]", std::vector<Rel>{Rel::EXACT, Rel::GENERALIZATION}, 1.0);
    templates_.emplace_back("[X] means [Y]", std::vector<Rel>{Rel::EXACT}, 0.9);
    
    // Temporal/causal templates
    templates_.emplace_back("[X] leads to [Y]", std::vector<Rel>{Rel::TEMPORAL}, 1.0);
    templates_.emplace_back("[X] causes [Y]", std::vector<Rel>{Rel::TEMPORAL}, 1.0);
    templates_.emplace_back("[X] happens before [Y]", std::vector<Rel>{Rel::TEMPORAL}, 0.8);
    
    // Leap/inference templates
    templates_.emplace_back("[X] suggests [Y]", std::vector<Rel>{Rel::LEAP}, 0.7);
    templates_.emplace_back("[X] implies [Y]", std::vector<Rel>{Rel::LEAP}, 0.8);
    templates_.emplace_back("[X] might be related to [Y]", std::vector<Rel>{Rel::LEAP}, 0.6);
    
    // Generalization templates
    templates_.emplace_back("[X] is a type of [Y]", std::vector<Rel>{Rel::GENERALIZATION}, 1.0);
    templates_.emplace_back("[X] is an example of [Y]", std::vector<Rel>{Rel::GENERALIZATION}, 0.9);
}

Clause PathToClauseConverter::convert_path_to_clause(const BeamPath& path) const {
    if (path.edges.empty()) return Clause();
    
    Clause clause;
    clause.confidence = path.confidence;
    
    // Find best template for this path
    ClauseTemplate best_template = find_best_template(path);
    
    // Extract components
    clause.subject = extract_subject(path);
    clause.object = extract_object(path);
    clause.predicate = get_relation_text(static_cast<Rel>(path.edges[0].rel));
    
    // Set relation type from first edge
    clause.relation_type = static_cast<Rel>(path.edges[0].rel);
    
    // Apply template if available
    if (!best_template.pattern.empty()) {
        // Simple template substitution
        std::string formatted = best_template.pattern;
        std::replace(formatted.begin(), formatted.end(), '[', ' ');
        std::replace(formatted.begin(), formatted.end(), ']', ' ');
        
        // Replace placeholders (simplified)
        size_t x_pos = formatted.find("X");
        if (x_pos != std::string::npos) {
            formatted.replace(x_pos, 1, clause.subject);
        }
        
        size_t y_pos = formatted.find("Y");
        if (y_pos != std::string::npos) {
            formatted.replace(y_pos, 1, clause.object);
        }
        
        // Clean up the result
        clause.subject = clause.subject; // Keep original for now
        clause.object = clause.object;
        clause.predicate = formatted;
    }
    
    return clause;
}

std::vector<Clause> PathToClauseConverter::convert_paths_to_clauses(const std::vector<BeamPath>& paths) const {
    std::vector<Clause> clauses;
    
    for (const auto& path : paths) {
        Clause clause = convert_path_to_clause(path);
        if (NLGUtils::is_valid_clause(clause) && clause.confidence >= config_.min_clause_confidence) {
            clauses.push_back(clause);
        }
    }
    
    return clauses;
}

ClauseTemplate PathToClauseConverter::find_best_template(const BeamPath& path) const {
    if (path.edges.empty()) return ClauseTemplate();
    
    Rel primary_relation = static_cast<Rel>(path.edges[0].rel);
    ClauseTemplate best_template;
    double best_score = 0.0;
    
    for (const auto& clause_template : templates_) {
        // Check if template supports this relation type
        bool supports_relation = false;
        for (Rel rel : clause_template.supported_relations) {
            if (rel == primary_relation) {
                supports_relation = true;
                break;
            }
        }
        
        if (!supports_relation) continue;
        
        double score = compute_template_match_score(path, clause_template);
        if (score > best_score) {
            best_score = score;
            best_template = clause_template;
        }
    }
    
    return best_template;
}

double PathToClauseConverter::compute_template_match_score(const BeamPath& path, const ClauseTemplate& clause_template) const {
    // Simple scoring based on path length and relation match
    double score = clause_template.priority;
    
    // Prefer shorter paths for cleaner templates
    if (path.edges.size() <= 2) {
        score *= 1.2;
    }
    
    // Check if all edges match the template's supported relations
    bool all_edges_match = true;
    for (const auto& edge : path.edges) {
        Rel edge_rel = static_cast<Rel>(edge.rel);
        bool edge_matches = false;
        
        for (Rel supported_rel : clause_template.supported_relations) {
            if (edge_rel == supported_rel) {
                edge_matches = true;
                break;
            }
        }
        
        if (!edge_matches) {
            all_edges_match = false;
            break;
        }
    }
    
    if (all_edges_match) {
        score *= 1.5;
    }
    
    return score;
}

std::string PathToClauseConverter::extract_subject(const BeamPath& path) const {
    if (path.nodes.empty()) return "";
    
    return get_node_content(path.nodes[0]);
}

std::string PathToClauseConverter::extract_predicate(const BeamPath& path) const {
    if (path.edges.empty()) return "";
    
    return get_relation_text(static_cast<Rel>(path.edges[0].rel));
}

std::string PathToClauseConverter::extract_object(const BeamPath& path) const {
    if (path.nodes.size() < 2) return "";
    
    return get_node_content(path.nodes.back());
}

std::string PathToClauseConverter::get_node_content(const NodeID& node) const {
    if (!store_) return "";
    
    NodeRecHeader header;
    std::vector<uint8_t> payload;
    
    if (store_->get_node(node, header, payload)) {
        return std::string(reinterpret_cast<const char*>(payload.data()), payload.size());
    }
    
    return "";
}

std::string PathToClauseConverter::get_relation_text(Rel relation) const {
    switch (relation) {
        case Rel::EXACT: return "is";
        case Rel::TEMPORAL: return "leads to";
        case Rel::LEAP: return "suggests";
        case Rel::GENERALIZATION: return "is a type of";
        default: return "relates to";
    }
}

// ConnectorSelector implementation
void ConnectorSelector::initialize_rules() {
    // Causal connectors
    rules_.emplace_back("because", std::vector<std::string>{"causal", "explanation"}, 1.0);
    rules_.emplace_back("since", std::vector<std::string>{"causal", "explanation"}, 0.9);
    
    // Additive connectors
    rules_.emplace_back("and", std::vector<std::string>{"additional", "similar"}, 1.0);
    rules_.emplace_back("also", std::vector<std::string>{"additional", "similar"}, 0.8);
    rules_.emplace_back("furthermore", std::vector<std::string>{"additional", "formal"}, 0.7);
    
    // Contrastive connectors
    rules_.emplace_back("but", std::vector<std::string>{"contrast", "contradiction"}, 1.0);
    rules_.emplace_back("however", std::vector<std::string>{"contrast", "formal"}, 0.9);
    rules_.emplace_back("although", std::vector<std::string>{"contrast", "concession"}, 0.8);
    
    // Conclusive connectors
    rules_.emplace_back("therefore", std::vector<std::string>{"conclusion", "inference"}, 1.0);
    rules_.emplace_back("thus", std::vector<std::string>{"conclusion", "formal"}, 0.9);
    rules_.emplace_back("consequently", std::vector<std::string>{"conclusion", "causal"}, 0.8);
}

std::string ConnectorSelector::select_connector(const Clause& prev_clause, const Clause& current_clause) const {
    if (!config_.use_connectors) return "";
    
    std::string best_connector;
    double best_weight = 0.0;
    
    for (const auto& rule : rules_) {
        double weight = compute_rule_weight(prev_clause, current_clause, rule);
        if (weight > best_weight) {
            best_weight = weight;
            best_connector = rule.connector;
        }
    }
    
    return best_connector;
}

std::string ConnectorSelector::select_initial_connector(const Clause& clause) const {
    // For the first clause, use simple connectors
    if (clause.relation_type == Rel::LEAP) {
        return "Based on available information, ";
    }
    
    return "";
}

double ConnectorSelector::compute_rule_weight(const Clause& prev, const Clause& current, const ConnectorRule& rule) const {
    double weight = rule.weight;
    
    // Check conditions
    bool conditions_met = true;
    for (const std::string& condition : rule.conditions) {
        if (!evaluate_conditions(current, {condition})) {
            conditions_met = false;
            break;
        }
    }
    
    if (!conditions_met) return 0.0;
    
    // Boost weight for relation type matches
    if (rule.connector == "because" && current.relation_type == Rel::TEMPORAL) {
        weight *= 1.5;
    } else if (rule.connector == "and" && prev.relation_type == current.relation_type) {
        weight *= 1.2;
    } else if (rule.connector == "therefore" && current.relation_type == Rel::LEAP) {
        weight *= 1.3;
    }
    
    return weight;
}

bool ConnectorSelector::evaluate_conditions(const Clause& clause, const std::vector<std::string>& conditions) const {
    // Simplified condition evaluation
    for (const std::string& condition : conditions) {
        if (condition == "causal" && clause.relation_type == Rel::TEMPORAL) {
            return true;
        } else if (condition == "additional" && clause.relation_type == Rel::EXACT) {
            return true;
        } else if (condition == "conclusion" && clause.relation_type == Rel::LEAP) {
            return true;
        }
    }
    
    return false;
}

// TextFormatter implementation
std::string TextFormatter::format_clause(const Clause& clause) const {
    if (clause.subject.empty() || clause.object.empty()) return "";
    
    std::string result = clause.subject + " " + clause.predicate + " " + clause.object;
    return apply_final_formatting(result);
}

std::string TextFormatter::format_response(const std::vector<Clause>& clauses) const {
    if (clauses.empty()) return "I don't have enough information to answer that question.";
    
    std::ostringstream response;
    ConnectorSelector selector(config_);
    
    for (size_t i = 0; i < clauses.size(); ++i) {
        const Clause& clause = clauses[i];
        
        // Add connector for non-first clauses
        if (i > 0 && config_.use_connectors) {
            std::string connector = selector.select_connector(clauses[i-1], clause);
            if (!connector.empty()) {
                response << " " << connector << " ";
            }
        } else if (i == 0) {
            // Add initial connector
            std::string initial_connector = selector.select_initial_connector(clause);
            if (!initial_connector.empty()) {
                response << initial_connector;
            }
        }
        
        // Format the clause
        std::string formatted_clause = format_clause(clause);
        response << formatted_clause;
        
        // Add confidence hedge if needed
        if (config_.use_hedges && clause.confidence < config_.high_confidence_threshold) {
            response << add_confidence_hedge("", clause.confidence);
        }
        
        // Add period for last clause
        if (i == clauses.size() - 1) {
            response << ".";
        }
    }
    
    std::string result = response.str();
    return apply_final_formatting(result);
}

std::string TextFormatter::add_confidence_hedge(const std::string& text, double confidence) const {
    if (!config_.use_hedges) return text;
    
    std::string hedge;
    if (confidence < 0.4) {
        hedge = " (uncertain)";
    } else if (confidence < 0.7) {
        hedge = " (likely)";
    }
    
    return text + hedge;
}

std::string TextFormatter::apply_final_formatting(const std::string& text) const {
    std::string result = text;
    
    // Normalize whitespace
    result = normalize_whitespace(result);
    
    // Capitalize first letter
    result = capitalize_first_letter(result);
    
    // Add punctuation if missing
    result = add_punctuation(result);
    
    return result;
}

std::string TextFormatter::capitalize_first_letter(const std::string& text) const {
    if (text.empty()) return text;
    
    std::string result = text;
    result[0] = std::toupper(result[0]);
    return result;
}

std::string TextFormatter::add_punctuation(const std::string& text) const {
    if (text.empty()) return text;
    
    char last_char = text.back();
    if (last_char != '.' && last_char != '!' && last_char != '?') {
        return text + ".";
    }
    
    return text;
}

std::string TextFormatter::normalize_whitespace(const std::string& text) const {
    std::regex multiple_spaces("\\s+");
    std::string result = std::regex_replace(text, multiple_spaces, " ");
    
    // Trim leading/trailing whitespace
    result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](int ch) {
        return !std::isspace(ch);
    }));
    result.erase(std::find_if(result.rbegin(), result.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), result.end());
    
    return result;
}

// ConfidenceCalibrator implementation
double ConfidenceCalibrator::compute_response_confidence(const std::vector<Clause>& clauses, 
                                                        const std::vector<BeamPath>& paths) const {
    if (clauses.empty()) return 0.0;
    
    // Average clause confidence
    double avg_clause_confidence = 0.0;
    for (const auto& clause : clauses) {
        avg_clause_confidence += clause.confidence;
    }
    avg_clause_confidence /= clauses.size();
    
    // Path confidence bonus
    double path_confidence_bonus = 0.0;
    if (!paths.empty()) {
        double avg_path_confidence = 0.0;
        for (const auto& path : paths) {
            avg_path_confidence += path.confidence;
        }
        avg_path_confidence /= paths.size();
        path_confidence_bonus = avg_path_confidence * 0.3;
    }
    
    // Evidence consistency bonus
    double consistency_bonus = compute_evidence_consistency(clauses);
    
    double final_confidence = avg_clause_confidence + path_confidence_bonus + consistency_bonus;
    return std::min(1.0, std::max(0.0, final_confidence));
}

ConfidenceCalibrator::ConfidenceLevel ConfidenceCalibrator::categorize_confidence(double confidence) const {
    if (confidence >= config_.high_confidence_threshold) {
        return HIGH;
    } else if (confidence >= config_.min_path_confidence) {
        return MEDIUM;
    } else {
        return LOW;
    }
}

std::string ConfidenceCalibrator::get_confidence_hedge(double confidence) const {
    ConfidenceLevel level = categorize_confidence(confidence);
    
    switch (level) {
        case HIGH: return "";
        case MEDIUM: return "likely ";
        case LOW: return "possibly ";
        default: return "";
    }
}

double ConfidenceCalibrator::compute_evidence_consistency(const std::vector<Clause>& clauses) const {
    if (clauses.size() <= 1) return 0.0;
    
    // Check for contradictions
    int contradictions = 0;
    for (size_t i = 0; i < clauses.size(); ++i) {
        for (size_t j = i + 1; j < clauses.size(); ++j) {
            if (NLGUtils::clauses_are_contradictory(clauses[i], clauses[j])) {
                contradictions++;
            }
        }
    }
    
    // Consistency bonus decreases with contradictions
    double consistency_score = 1.0 - (static_cast<double>(contradictions) / clauses.size());
    return consistency_score * 0.2; // Max 20% bonus
}

// SafetyFallback implementation
bool SafetyFallback::needs_fallback(const std::vector<Clause>& clauses, double confidence) const {
    if (confidence < config_.min_path_confidence) return true;
    if (clauses.empty()) return true;
    
    // Check if we have any high-confidence clauses
    bool has_high_confidence = false;
    for (const auto& clause : clauses) {
        if (clause.confidence >= config_.high_confidence_threshold) {
            has_high_confidence = true;
            break;
        }
    }
    
    return !has_high_confidence;
}

std::string SafetyFallback::generate_fallback_response(const std::vector<Clause>& clauses, 
                                                      const std::vector<BeamPath>& paths) const {
    std::ostringstream response;
    response << config_.fallback_prefix;
    
    std::vector<std::string> top_facts = extract_top_facts(clauses);
    
    if (top_facts.empty()) {
        response << "I don't have enough information to provide a confident answer.";
    } else {
        response << top_facts[0];
        
        for (size_t i = 1; i < top_facts.size() && i < 2; ++i) {
            response << " Additionally, " << top_facts[i];
        }
    }
    
    return response.str();
}

std::vector<std::string> SafetyFallback::extract_top_facts(const std::vector<Clause>& clauses) const {
    std::vector<std::string> facts;
    
    for (const auto& clause : clauses) {
        if (is_high_confidence_fact(clause)) {
            facts.push_back(format_fact(clause));
        }
    }
    
    // Sort by confidence
    std::sort(facts.begin(), facts.end(), [&clauses](const std::string& a, const std::string& b) {
        // This is simplified - in practice you'd track confidence with facts
        return a.length() > b.length(); // Placeholder sorting
    });
    
    return facts;
}

std::string SafetyFallback::format_fact(const Clause& clause) const {
    return clause.subject + " " + clause.predicate + " " + clause.object;
}

bool SafetyFallback::is_high_confidence_fact(const Clause& clause) const {
    return clause.confidence >= config_.min_clause_confidence;
}

// GraphNLG implementation
void GraphNLG::set_config(const NLGConfig& config) {
    config_ = config;
    converter_.set_config(config);
    connector_selector_.set_config(config);
    formatter_.set_config(config);
    calibrator_.set_config(config);
    fallback_.set_config(config);
}

std::string GraphNLG::generate_response(const std::vector<BeamPath>& paths, const Query& query) const {
    return generate_response_with_config(paths, query, config_);
}

std::string GraphNLG::generate_response_with_config(const std::vector<BeamPath>& paths, 
                                                   const Query& query,
                                                   const NLGConfig& custom_config) const {
    if (paths.empty()) {
        return "I don't have enough information to answer that question.";
    }
    
    // ===== NEW PATH-TO-PHRASE APPROACH =====
    // Try direct path conversion first (organic replay)
    std::string direct_response = try_path_to_phrase(paths);
    if (!direct_response.empty()) {
        return direct_response;
    }
    
    // ===== FALLBACK TO EXISTING TEMPLATE-BASED APPROACH =====
    // Create clauses from paths
    std::vector<Clause> clauses = create_clauses(paths);
    
    // Filter and rank clauses
    std::vector<Clause> ranked_clauses = filter_and_rank_clauses(clauses);
    
    // Limit number of clauses
    if (ranked_clauses.size() > custom_config.max_clauses) {
        ranked_clauses.resize(custom_config.max_clauses);
    }
    
    // Combine clauses into response
    std::string response = combine_clauses(ranked_clauses);
    
    // Compute confidence
    double confidence = calibrator_.compute_response_confidence(ranked_clauses, paths);
    
    // Apply safety checks and fallback if needed
    response = apply_safety_checks(response, ranked_clauses, confidence);
    
    return response;
}

std::string GraphNLG::generate_simple_response(const BeamPath& best_path, const Query& query) const {
    std::vector<BeamPath> paths = {best_path};
    return generate_response(paths, query);
}

std::string GraphNLG::try_path_to_phrase(const std::vector<BeamPath>& paths) const {
    // Try path-to-phrase algorithm on best path
    if (paths.empty() || !store_) {
        return "";
    }
    
    const BeamPath& best_path = paths[0];
    
    // Try the new organic path-to-phrase algorithm
    std::string organic_response = path_to_phrase(best_path, store_);
    
    // Validate response quality
    if (!organic_response.empty() && organic_response.length() > 5 && organic_response != ".") {
        return organic_response;
    }
    
    return "";  // Signal to fallback to template-based approach
}

std::vector<Clause> GraphNLG::create_clauses(const std::vector<BeamPath>& paths) const {
    return converter_.convert_paths_to_clauses(paths);
}

std::vector<Clause> GraphNLG::filter_and_rank_clauses(const std::vector<Clause>& clauses) const {
    std::vector<Clause> filtered;
    
    // Filter by confidence
    for (const auto& clause : clauses) {
        if (clause.confidence >= config_.min_clause_confidence) {
            filtered.push_back(clause);
        }
    }
    
    // Sort by confidence (descending)
    std::sort(filtered.begin(), filtered.end(),
              [](const Clause& a, const Clause& b) { return a.confidence > b.confidence; });
    
    return filtered;
}

std::string GraphNLG::combine_clauses(const std::vector<Clause>& clauses) const {
    return formatter_.format_response(clauses);
}

std::string GraphNLG::apply_safety_checks(const std::string& response, 
                                         const std::vector<Clause>& clauses,
                                         double confidence) const {
    // Check if fallback is needed
    if (fallback_.needs_fallback(clauses, confidence)) {
        std::vector<BeamPath> empty_paths; // Would pass actual paths in real implementation
        return fallback_.generate_fallback_response(clauses, empty_paths);
    }
    
    return response;
}

// NLGUtils implementation
namespace NLGUtils {
    std::string clean_text(const std::string& text) {
        std::string result = text;
        
        // Remove extra whitespace
        std::regex multiple_spaces("\\s+");
        result = std::regex_replace(result, multiple_spaces, " ");
        
        // Trim
        result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](int ch) {
            return !std::isspace(ch);
        }));
        result.erase(std::find_if(result.rbegin(), result.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(), result.end());
        
        return result;
    }
    
    std::string normalize_case(const std::string& text) {
        std::string result = text;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        
        // Capitalize first letter
        if (!result.empty()) {
            result[0] = std::toupper(result[0]);
        }
        
        return result;
    }
    
    std::string remove_redundancy(const std::string& text) {
        // Simple redundancy removal - in practice you'd want more sophisticated logic
        std::string result = text;
        
        // Remove repeated words
        std::regex repeated_words("\\b(\\w+)\\s+\\1\\b");
        result = std::regex_replace(result, repeated_words, "$1");
        
        return result;
    }
    
    bool is_valid_clause(const Clause& clause) {
        return !clause.subject.empty() && 
               !clause.object.empty() && 
               clause.confidence > 0.0;
    }
    
    bool clauses_are_contradictory(const Clause& clause1, const Clause& clause2) {
        // Simple contradiction detection
        if (clause1.subject == clause2.subject) {
            // Check for direct contradictions
            if ((clause1.predicate.find("is") != std::string::npos && 
                 clause2.predicate.find("is not") != std::string::npos) ||
                (clause1.predicate.find("is not") != std::string::npos && 
                 clause2.predicate.find("is") != std::string::npos)) {
                return true;
            }
        }
        
        return false;
    }
    
    bool path_has_definitional_pattern(const BeamPath& path) {
        if (path.edges.empty()) return false;
        
        for (const auto& edge : path.edges) {
            if (edge.rel == static_cast<uint32_t>(Rel::EXACT) || 
                edge.rel == static_cast<uint32_t>(Rel::GENERALIZATION)) {
                return true;
            }
        }
        
        return false;
    }
    
    bool path_has_causal_pattern(const BeamPath& path) {
        if (path.edges.empty()) return false;
        
        for (const auto& edge : path.edges) {
            if (edge.rel == static_cast<uint32_t>(Rel::TEMPORAL)) {
                return true;
            }
        }
        
        return false;
    }
    
    bool path_has_comparison_pattern(const BeamPath& path) {
        // Simplified - would need more sophisticated pattern matching
        return path.nodes.size() >= 2;
    }
    
    double compute_path_coherence(const BeamPath& path) {
        if (path.edges.empty()) return 1.0;
        
        // Simple coherence based on relation consistency
        Rel first_relation = static_cast<Rel>(path.edges[0].rel);
        int consistent_relations = 1;
        
        for (size_t i = 1; i < path.edges.size(); ++i) {
            if (static_cast<Rel>(path.edges[i].rel) == first_relation) {
                consistent_relations++;
            }
        }
        
        return static_cast<double>(consistent_relations) / path.edges.size();
    }
    
    double compute_evidence_strength(const std::vector<Clause>& clauses) {
        if (clauses.empty()) return 0.0;
        
        double total_confidence = 0.0;
        for (const auto& clause : clauses) {
            total_confidence += clause.confidence;
        }
        
        return total_confidence / clauses.size();
    }
    
    std::vector<std::string> extract_template_variables(const std::string& pattern) {
        std::vector<std::string> variables;
        std::regex var_regex("\\[([^\\]]+)\\]");
        std::sregex_iterator iter(pattern.begin(), pattern.end(), var_regex);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            variables.push_back((*iter)[1]);
        }
        
        return variables;
    }
    
    std::string substitute_template_variables(const std::string& pattern, 
                                            const std::unordered_map<std::string, std::string>& variables) {
        std::string result = pattern;
        
        for (const auto& [var, value] : variables) {
            std::string placeholder = "[" + var + "]";
            size_t pos = 0;
            while ((pos = result.find(placeholder, pos)) != std::string::npos) {
                result.replace(pos, placeholder.length(), value);
                pos += value.length();
            }
        }
        
        return result;
    }
}

} // namespace melvin
