#include "policy.hpp"
#include <regex>
#include <algorithm>
#include <sstream>
#include <cctype>

namespace melvin {

// Simple tokenization function
std::vector<std::string> tokenize_query(const std::string& text) {
    std::vector<std::string> tokens;
    std::string current_token;
    
    for (char c : text) {
        if (std::isalnum(c) || c == '\'') {
            current_token += c;
        } else {
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
        }
    }
    
    if (!current_token.empty()) {
        tokens.push_back(current_token);
    }
    
    return tokens;
}

// QueryIntentClassifier implementation
QueryIntentClassifier::QueryIntentClassifier() {
    initialize_keywords();
    initialize_patterns();
}

void QueryIntentClassifier::initialize_keywords() {
    // Define keywords
    intent_keywords_["what"] = QueryIntent::DEFINE;
    intent_keywords_["define"] = QueryIntent::DEFINE;
    intent_keywords_["meaning"] = QueryIntent::DEFINE;
    intent_keywords_["is"] = QueryIntent::DEFINE;
    intent_keywords_["are"] = QueryIntent::DEFINE;
    keyword_weights_["what"] = 1.0;
    keyword_weights_["define"] = 1.5;
    keyword_weights_["meaning"] = 1.3;
    keyword_weights_["is"] = 0.8;
    keyword_weights_["are"] = 0.8;
    
    // Why keywords
    intent_keywords_["why"] = QueryIntent::WHY;
    intent_keywords_["how"] = QueryIntent::WHY;
    intent_keywords_["explain"] = QueryIntent::WHY;
    intent_keywords_["reason"] = QueryIntent::WHY;
    keyword_weights_["why"] = 1.5;
    keyword_weights_["how"] = 1.3;
    keyword_weights_["explain"] = 1.2;
    keyword_weights_["reason"] = 1.0;
    
    // Compare keywords
    intent_keywords_["compare"] = QueryIntent::COMPARE;
    intent_keywords_["which"] = QueryIntent::COMPARE;
    intent_keywords_["better"] = QueryIntent::COMPARE;
    intent_keywords_["difference"] = QueryIntent::COMPARE;
    intent_keywords_["versus"] = QueryIntent::COMPARE;
    keyword_weights_["compare"] = 1.5;
    keyword_weights_["which"] = 1.2;
    keyword_weights_["better"] = 1.0;
    keyword_weights_["difference"] = 1.3;
    keyword_weights_["versus"] = 1.1;
    
    // Causal keywords
    intent_keywords_["cause"] = QueryIntent::CAUSAL;
    intent_keywords_["causes"] = QueryIntent::CAUSAL;
    intent_keywords_["leads"] = QueryIntent::CAUSAL;
    intent_keywords_["results"] = QueryIntent::CAUSAL;
    keyword_weights_["cause"] = 1.4;
    keyword_weights_["causes"] = 1.4;
    keyword_weights_["leads"] = 1.2;
    keyword_weights_["results"] = 1.1;
    
    // Temporal keywords
    intent_keywords_["before"] = QueryIntent::TEMPORAL;
    intent_keywords_["after"] = QueryIntent::TEMPORAL;
    intent_keywords_["next"] = QueryIntent::TEMPORAL;
    intent_keywords_["then"] = QueryIntent::TEMPORAL;
    intent_keywords_["sequence"] = QueryIntent::TEMPORAL;
    keyword_weights_["before"] = 1.2;
    keyword_weights_["after"] = 1.2;
    keyword_weights_["next"] = 1.0;
    keyword_weights_["then"] = 0.9;
    keyword_weights_["sequence"] = 1.3;
}

void QueryIntentClassifier::initialize_patterns() {
    // Define patterns
    intent_patterns_.push_back(std::regex(R"(what\s+is\s+(\w+))", std::regex_constants::icase));
    intent_patterns_.push_back(std::regex(R"(what\s+are\s+(\w+))", std::regex_constants::icase));
    intent_patterns_.push_back(std::regex(R"(define\s+(\w+))", std::regex_constants::icase));
    intent_patterns_.push_back(std::regex(R"(meaning\s+of\s+(\w+))", std::regex_constants::icase));
    
    // Why patterns
    intent_patterns_.push_back(std::regex(R"(why\s+does\s+(\w+))", std::regex_constants::icase));
    intent_patterns_.push_back(std::regex(R"(how\s+does\s+(\w+))", std::regex_constants::icase));
    intent_patterns_.push_back(std::regex(R"(explain\s+(\w+))", std::regex_constants::icase));
    
    // Compare patterns
    intent_patterns_.push_back(std::regex(R"(compare\s+(\w+)\s+and\s+(\w+))", std::regex_constants::icase));
    intent_patterns_.push_back(std::regex(R"(which\s+is\s+better)", std::regex_constants::icase));
    intent_patterns_.push_back(std::regex(R"(difference\s+between\s+(\w+)\s+and\s+(\w+))", std::regex_constants::icase));
    
    // Causal patterns
    intent_patterns_.push_back(std::regex(R"(what\s+causes\s+(\w+))", std::regex_constants::icase));
    intent_patterns_.push_back(std::regex(R"((\w+)\s+leads\s+to)", std::regex_constants::icase));
    intent_patterns_.push_back(std::regex(R"(results\s+in\s+(\w+))", std::regex_constants::icase));
    
    // Temporal patterns
    intent_patterns_.push_back(std::regex(R"(what\s+happens\s+after\s+(\w+))", std::regex_constants::icase));
    intent_patterns_.push_back(std::regex(R"(what\s+comes\s+before\s+(\w+))", std::regex_constants::icase));
    intent_patterns_.push_back(std::regex(R"(sequence\s+of\s+(\w+))", std::regex_constants::icase));
}

QueryClassification QueryIntentClassifier::classify(const Query& query) const {
    return classify_text(query.original_text);
}

QueryClassification QueryIntentClassifier::classify_text(const std::string& query_text) const {
    QueryClassification result;
    
    // Tokenize query
    std::vector<std::string> tokens = tokenize_query(query_text);
    
    // Match patterns first (higher confidence)
    QueryIntent pattern_intent = match_patterns(query_text);
    if (pattern_intent != QueryIntent::UNKNOWN) {
        result.intent = pattern_intent;
        result.confidence = 0.9;
    } else {
        // Fall back to keyword matching
        QueryIntent keyword_intent = match_keywords(tokens);
        result.intent = keyword_intent;
        result.confidence = 0.7;
    }
    
    // Extract entities and key terms
    result.entities = extract_entities(query_text);
    result.key_terms = extract_key_terms(query_text);
    
    // Analyze complexity
    result.complexity = analyze_complexity(query_text, result.entities);
    
    // Generate reasoning
    result.reasoning = "Basic reasoning for " + query_text;
    
    return result;
}

QueryIntent QueryIntentClassifier::match_patterns(const std::string& query) const {
    // This is simplified - in practice you'd have more sophisticated pattern matching
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    // Check for define patterns
    if (std::regex_search(lower_query, std::regex(R"(what\s+is)")) ||
        std::regex_search(lower_query, std::regex(R"(define)")) ||
        std::regex_search(lower_query, std::regex(R"(meaning\s+of)"))) {
        return QueryIntent::DEFINE;
    }
    
    // Check for why patterns
    if (std::regex_search(lower_query, std::regex(R"(why\s+does)")) ||
        std::regex_search(lower_query, std::regex(R"(how\s+does)")) ||
        std::regex_search(lower_query, std::regex(R"(explain)"))) {
        return QueryIntent::WHY;
    }
    
    // Check for compare patterns
    if (std::regex_search(lower_query, std::regex(R"(compare\s+\w+\s+and)")) ||
        std::regex_search(lower_query, std::regex(R"(which\s+is\s+better)")) ||
        std::regex_search(lower_query, std::regex(R"(difference\s+between)"))) {
        return QueryIntent::COMPARE;
    }
    
    // Check for causal patterns
    if (std::regex_search(lower_query, std::regex(R"(what\s+causes)")) ||
        std::regex_search(lower_query, std::regex(R"(\w+\s+leads\s+to)")) ||
        std::regex_search(lower_query, std::regex(R"(results\s+in)"))) {
        return QueryIntent::CAUSAL;
    }
    
    // Check for temporal patterns
    if (std::regex_search(lower_query, std::regex(R"(what\s+happens\s+after)")) ||
        std::regex_search(lower_query, std::regex(R"(what\s+comes\s+before)")) ||
        std::regex_search(lower_query, std::regex(R"(sequence\s+of)"))) {
        return QueryIntent::TEMPORAL;
    }
    
    return QueryIntent::UNKNOWN;
}

QueryIntent QueryIntentClassifier::match_keywords(const std::vector<std::string>& tokens) const {
    std::unordered_map<QueryIntent, double> intent_scores;
    
    for (const auto& token : tokens) {
        auto it = intent_keywords_.find(token);
        if (it != intent_keywords_.end()) {
            QueryIntent intent = it->second;
            double weight = keyword_weights_.at(token);
            intent_scores[intent] += weight;
        }
    }
    
    if (intent_scores.empty()) {
        return QueryIntent::GENERAL;
    }
    
    // Find intent with highest score
    QueryIntent best_intent = intent_scores.begin()->first;
    double best_score = intent_scores.begin()->second;
    
    for (const auto& [intent, score] : intent_scores) {
        if (score > best_score) {
            best_intent = intent;
            best_score = score;
        }
    }
    
    return best_intent;
}

QueryComplexity QueryIntentClassifier::analyze_complexity(const std::string& query, const std::vector<std::string>& entities) const {
    QueryComplexityAnalyzer analyzer;
    return analyzer.analyze_complexity(query);
}

std::vector<std::string> QueryIntentClassifier::extract_entities(const std::string& query) const {
    EntityExtractor extractor;
    return extractor.extract_entities(query);
}

std::vector<std::string> QueryIntentClassifier::extract_key_terms(const std::string& query) const {
    EntityExtractor extractor;
    return extractor.extract_key_terms(query);
}

QueryPolicy QueryIntentClassifier::get_policy_for_intent(QueryIntent intent) const {
    PolicyManager manager;
    return manager.get_policy_for_intent(intent);
}

void QueryIntentClassifier::add_intent_keyword(const std::string& keyword, QueryIntent intent, double weight) {
    intent_keywords_[keyword] = intent;
    keyword_weights_[keyword] = weight;
}

void QueryIntentClassifier::add_intent_pattern(const std::string& pattern, QueryIntent intent) {
    intent_patterns_.push_back(std::regex(pattern, std::regex_constants::icase));
}

// QueryComplexityAnalyzer implementation
QueryComplexityAnalyzer::QueryComplexityAnalyzer() {
    initialize_indicators();
    initialize_connectors();
}

void QueryComplexityAnalyzer::initialize_indicators() {
    complexity_indicators_ = {
        "complex", "complicated", "difficult", "advanced", "detailed",
        "multiple", "several", "various", "different", "diverse",
        "relationship", "connection", "interaction", "process", "mechanism"
    };
}

void QueryComplexityAnalyzer::initialize_connectors() {
    logical_connectors_ = {
        "and", "or", "but", "however", "therefore", "because", "since",
        "if", "then", "when", "while", "although", "unless", "until"
    };
}

QueryComplexity QueryComplexityAnalyzer::analyze_complexity(const std::string& query) const {
    double score = get_complexity_score(query);
    
    if (score < 0.3) {
        return QueryComplexity::SIMPLE;
    } else if (score < 0.7) {
        return QueryComplexity::MODERATE;
    } else {
        return QueryComplexity::COMPLEX;
    }
}

double QueryComplexityAnalyzer::get_complexity_score(const std::string& query) const {
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    double score = 0.0;
    
    // Count entities
    int entity_count = count_entities(query);
    score += std::min(0.3, entity_count * 0.1);
    
    // Count relations/connectors
    int relation_count = count_relations(query);
    score += std::min(0.3, relation_count * 0.15);
    
    // Check for conditional logic
    if (has_conditional_logic(query)) {
        score += 0.2;
    }
    
    // Check for comparison logic
    if (has_comparison_logic(query)) {
        score += 0.2;
    }
    
    // Check for complexity indicators
    for (const auto& indicator : complexity_indicators_) {
        if (lower_query.find(indicator) != std::string::npos) {
            score += 0.1;
        }
    }
    
    return std::min(1.0, score);
}

int QueryComplexityAnalyzer::count_entities(const std::string& query) const {
    // Simple entity counting - count capitalized words and quoted terms
    int count = 0;
    
    // Count quoted terms
    std::regex quoted_terms("\"([^\"]+)\"");
    std::sregex_iterator iter(query.begin(), query.end(), quoted_terms);
    std::sregex_iterator end;
    count += std::distance(iter, end);
    
    // Count capitalized words (simplified)
    std::regex capitalized_words("\\b[A-Z][a-z]+\\b");
    iter = std::sregex_iterator(query.begin(), query.end(), capitalized_words);
    count += std::distance(iter, end);
    
    return count;
}

int QueryComplexityAnalyzer::count_relations(const std::string& query) const {
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    int count = 0;
    for (const auto& connector : logical_connectors_) {
        size_t pos = 0;
        while ((pos = lower_query.find(connector, pos)) != std::string::npos) {
            count++;
            pos += connector.length();
        }
    }
    
    return count;
}

bool QueryComplexityAnalyzer::has_conditional_logic(const std::string& query) const {
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    return lower_query.find("if") != std::string::npos ||
           lower_query.find("when") != std::string::npos ||
           lower_query.find("unless") != std::string::npos;
}

bool QueryComplexityAnalyzer::has_comparison_logic(const std::string& query) const {
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    return lower_query.find("compare") != std::string::npos ||
           lower_query.find("versus") != std::string::npos ||
           lower_query.find("difference") != std::string::npos ||
           lower_query.find("better") != std::string::npos ||
           lower_query.find("worse") != std::string::npos;
}

void QueryComplexityAnalyzer::add_complexity_indicator(const std::string& indicator) {
    complexity_indicators_.insert(indicator);
}

void QueryComplexityAnalyzer::add_logical_connector(const std::string& connector) {
    logical_connectors_.insert(connector);
}

// EntityExtractor implementation
EntityExtractor::EntityExtractor() {
    initialize_stop_words();
    initialize_entity_patterns();
}

void EntityExtractor::initialize_stop_words() {
    stop_words_ = {
        "the", "a", "an", "and", "or", "but", "in", "on", "at", "to", "for",
        "of", "with", "by", "is", "are", "was", "were", "be", "been", "being",
        "have", "has", "had", "do", "does", "did", "will", "would", "could",
        "should", "may", "might", "can", "what", "how", "why", "when", "where",
        "who", "which", "that", "this", "these", "those"
    };
}

void EntityExtractor::initialize_entity_patterns() {
    // Simple entity patterns
    entity_patterns_.push_back(std::regex("\"([^\"]+)\"")); // Quoted strings
    entity_patterns_.push_back(std::regex("\\b[A-Z][a-z]+\\b")); // Capitalized words
    entity_patterns_.push_back(std::regex("\\b\\d+\\b")); // Numbers
}

std::vector<std::string> EntityExtractor::extract_entities(const std::string& query) const {
    std::vector<std::string> entities;
    
    for (const auto& pattern : entity_patterns_) {
        std::sregex_iterator iter(query.begin(), query.end(), pattern);
        std::sregex_iterator end;
        
        for (; iter != end; ++iter) {
            std::string entity = (*iter)[1].str();
            if (!entity.empty() && !is_stop_word(entity)) {
                entities.push_back(entity);
            }
        }
    }
    
    return entities;
}

std::vector<std::string> EntityExtractor::extract_key_terms(const std::string& query) const {
    std::vector<std::string> tokens = tokenize(query);
    std::vector<std::string> key_terms;
    
    for (const auto& token : tokens) {
        std::string normalized = normalize_term(token);
        if (!normalized.empty() && !is_stop_word(normalized)) {
            key_terms.push_back(normalized);
        }
    }
    
    return key_terms;
}

bool EntityExtractor::is_stop_word(const std::string& term) const {
    return stop_words_.find(term) != stop_words_.end();
}

void EntityExtractor::add_stop_word(const std::string& word) {
    stop_words_.insert(word);
}

void EntityExtractor::add_entity_pattern(const std::string& pattern) {
    entity_patterns_.push_back(std::regex(pattern));
}

std::vector<std::string> EntityExtractor::tokenize(const std::string& text) const {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::string EntityExtractor::normalize_term(const std::string& term) const {
    std::string normalized = term;
    
    // Remove punctuation
    normalized.erase(std::remove_if(normalized.begin(), normalized.end(), ::ispunct), normalized.end());
    
    // Convert to lowercase
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    return normalized;
}

// PolicyManager implementation
PolicyManager::PolicyManager() {
    initialize_default_policies();
}

void PolicyManager::initialize_default_policies() {
    intent_policies_[QueryIntent::DEFINE] = create_define_policy();
    intent_policies_[QueryIntent::WHY] = create_why_policy();
    intent_policies_[QueryIntent::COMPARE] = create_compare_policy();
    intent_policies_[QueryIntent::CAUSAL] = create_causal_policy();
    intent_policies_[QueryIntent::TEMPORAL] = create_temporal_policy();
    intent_policies_[QueryIntent::GENERAL] = create_general_policy();
}

QueryPolicy PolicyManager::create_define_policy() const {
    QueryPolicy policy;
    
    // Beam parameters
    policy.beam_params.beam_width = 16;
    policy.beam_params.max_depth = 3;
    policy.beam_params.top_k = 6;
    policy.beam_params.top_p = 0.9;
    policy.beam_params.stop_threshold = 0.1;
    
    // Scoring weights
    policy.scoring_weights.define_query.exact = 1.0;
    policy.scoring_weights.define_query.temporal = 0.6;
    policy.scoring_weights.define_query.leap = 0.2;
    policy.scoring_weights.define_query.generalize = 1.0;
    
    // Preferred relations
    policy.preferred_relations = {Rel::EXACT, Rel::GENERALIZATION};
    policy.max_depth_multiplier = 1;
    policy.confidence_threshold = 0.6;
    
    return policy;
}

QueryPolicy PolicyManager::create_why_policy() const {
    QueryPolicy policy;
    
    // Beam parameters
    policy.beam_params.beam_width = 24;
    policy.beam_params.max_depth = 5;
    policy.beam_params.top_k = 8;
    policy.beam_params.top_p = 0.85;
    policy.beam_params.stop_threshold = 0.05;
    
    // Scoring weights
    policy.scoring_weights.why_query.exact = 0.3;
    policy.scoring_weights.why_query.temporal = 1.0;
    policy.scoring_weights.why_query.leap = 0.6;
    policy.scoring_weights.why_query.generalize = 0.3;
    
    // Preferred relations
    policy.preferred_relations = {Rel::TEMPORAL, Rel::LEAP};
    policy.max_depth_multiplier = 2;
    policy.confidence_threshold = 0.5;
    
    return policy;
}

QueryPolicy PolicyManager::create_compare_policy() const {
    QueryPolicy policy;
    
    // Beam parameters
    policy.beam_params.beam_width = 32;
    policy.beam_params.max_depth = 4;
    policy.beam_params.top_k = 10;
    policy.beam_params.top_p = 0.8;
    policy.beam_params.stop_threshold = 0.08;
    
    // Scoring weights
    policy.scoring_weights.compare_query.exact = 0.8;
    policy.scoring_weights.compare_query.temporal = 0.4;
    policy.scoring_weights.compare_query.leap = 0.8;
    policy.scoring_weights.compare_query.generalize = 1.2;
    
    // Preferred relations
    policy.preferred_relations = {Rel::EXACT, Rel::LEAP, Rel::GENERALIZATION};
    policy.max_depth_multiplier = 1;
    policy.confidence_threshold = 0.7;
    
    return policy;
}

QueryPolicy PolicyManager::create_causal_policy() const {
    QueryPolicy policy;
    
    // Beam parameters
    policy.beam_params.beam_width = 20;
    policy.beam_params.max_depth = 6;
    policy.beam_params.top_k = 7;
    policy.beam_params.top_p = 0.9;
    policy.beam_params.stop_threshold = 0.06;
    
    // Scoring weights (same as why policy)
    policy.scoring_weights = create_why_policy().scoring_weights;
    
    // Preferred relations
    policy.preferred_relations = {Rel::TEMPORAL, Rel::LEAP};
    policy.max_depth_multiplier = 2;
    policy.confidence_threshold = 0.5;
    
    return policy;
}

QueryPolicy PolicyManager::create_temporal_policy() const {
    QueryPolicy policy;
    
    // Beam parameters
    policy.beam_params.beam_width = 16;
    policy.beam_params.max_depth = 4;
    policy.beam_params.top_k = 6;
    policy.beam_params.top_p = 0.9;
    policy.beam_params.stop_threshold = 0.1;
    
    // Scoring weights
    policy.scoring_weights.define_query.temporal = 1.0;
    policy.scoring_weights.define_query.exact = 0.5;
    policy.scoring_weights.define_query.leap = 0.3;
    policy.scoring_weights.define_query.generalize = 0.4;
    
    // Preferred relations
    policy.preferred_relations = {Rel::TEMPORAL};
    policy.max_depth_multiplier = 1;
    policy.confidence_threshold = 0.6;
    
    return policy;
}

QueryPolicy PolicyManager::create_general_policy() const {
    QueryPolicy policy;
    
    // Beam parameters (balanced)
    policy.beam_params.beam_width = 20;
    policy.beam_params.max_depth = 4;
    policy.beam_params.top_k = 8;
    policy.beam_params.top_p = 0.85;
    policy.beam_params.stop_threshold = 0.08;
    
    // Scoring weights (balanced)
    policy.scoring_weights.define_query.exact = 1.0;
    policy.scoring_weights.define_query.temporal = 0.8;
    policy.scoring_weights.define_query.leap = 0.6;
    policy.scoring_weights.define_query.generalize = 0.8;
    
    // Preferred relations (all)
    policy.preferred_relations = {Rel::EXACT, Rel::TEMPORAL, Rel::LEAP, Rel::GENERALIZATION};
    policy.max_depth_multiplier = 1;
    policy.confidence_threshold = 0.5;
    
    return policy;
}

QueryPolicy PolicyManager::get_policy(const Query& query) const {
    QueryClassification classification = classify_query(query);
    return get_policy_for_intent(classification.intent);
}

QueryPolicy PolicyManager::get_policy(const std::string& query_text) const {
    QueryClassification classification = classify_query(query_text);
    return get_policy_for_intent(classification.intent);
}

QueryPolicy PolicyManager::get_policy_for_intent(QueryIntent intent) const {
    auto it = intent_policies_.find(intent);
    if (it != intent_policies_.end()) {
        return it->second;
    }
    
    // Default to general policy
    return intent_policies_.at(QueryIntent::GENERAL);
}

void PolicyManager::set_policy_for_intent(QueryIntent intent, const QueryPolicy& policy) {
    intent_policies_[intent] = policy;
}

QueryClassification PolicyManager::classify_query(const Query& query) const {
    return classifier_.classify(query);
}

QueryClassification PolicyManager::classify_query(const std::string& query_text) const {
    return classifier_.classify_text(query_text);
}

void PolicyManager::set_default_policies() {
    initialize_default_policies();
}

void PolicyManager::customize_policy(QueryIntent intent, const BeamParams& beam_params, 
                                   const ScoringWeights& scoring_weights) {
    QueryPolicy policy;
    policy.beam_params = beam_params;
    policy.scoring_weights = scoring_weights;
    intent_policies_[intent] = policy;
}

// QueryPreprocessor implementation
QueryPreprocessor::QueryPreprocessor() {}

Query QueryPreprocessor::preprocess_query(const std::string& raw_query) const {
    Query query;
    query.original_text = raw_query;
    query.tokens = extract_tokens(normalize_query(raw_query));
    query.token_embeddings = compute_token_embeddings(query.tokens);
    query.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    return query;
}

std::string QueryPreprocessor::normalize_query(const std::string& query) const {
    std::string normalized = query;
    
    // Remove extra whitespace
    normalized = normalize_whitespace(normalized);
    
    // Remove punctuation (keep some for pattern matching)
    // This is simplified - in practice you'd want more sophisticated normalization
    
    return normalized;
}

std::vector<std::string> QueryPreprocessor::extract_tokens(const std::string& query) const {
    return entity_extractor_.tokenize(query);
}

std::vector<float> QueryPreprocessor::compute_token_embeddings(const std::vector<std::string>& tokens) const {
    std::vector<float> embeddings;
    
    for (const auto& token : tokens) {
        std::vector<float> token_embedding = compute_simple_embedding(token);
        embeddings.insert(embeddings.end(), token_embedding.begin(), token_embedding.end());
    }
    
    return embeddings;
}

std::string QueryPreprocessor::lowercase(const std::string& text) const {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string QueryPreprocessor::remove_punctuation(const std::string& text) const {
    std::string result = text;
    result.erase(std::remove_if(result.begin(), result.end(), ::ispunct), result.end());
    return result;
}

std::string QueryPreprocessor::normalize_whitespace(const std::string& text) const {
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

std::vector<float> QueryPreprocessor::compute_simple_embedding(const std::string& token) const {
    // Simple hash-based embedding (in practice you'd use a proper embedding model)
    std::hash<std::string> hasher;
    size_t hash = hasher(token);
    
    std::vector<float> embedding(32); // 32-dimensional embedding
    
    // Distribute hash bits across embedding dimensions
    for (size_t i = 0; i < embedding.size(); ++i) {
        embedding[i] = ((hash >> i) & 1) ? 1.0f : -1.0f;
    }
    
    return embedding;
}

// QueryRouter implementation
QueryRouter::QueryRouter() {}

QueryRouter::RoutingResult QueryRouter::route_query(const std::string& raw_query, Store* store) const {
    RoutingResult result;
    
    // Preprocess query
    result.processed_query = preprocessor_.preprocess_query(raw_query);
    
    // Classify query
    result.classification = policy_manager_.classify_query(result.processed_query);
    
    // Get base policy
    result.policy = policy_manager_.get_policy_for_intent(result.classification.intent);
    
    // Adapt policy for complexity
    result.policy = adapt_policy_for_complexity(result.policy, result.classification.complexity);
    
    // Select start nodes
    result.suggested_start_nodes = select_start_nodes(result.processed_query, result.classification.intent, store);
    
    return result;
}

std::vector<NodeID> QueryRouter::select_start_nodes(const Query& query, QueryIntent intent, Store* store) const {
    // This is a simplified implementation
    // In practice, you'd want more sophisticated node selection
    
    std::vector<NodeID> start_nodes;
    
    // For now, just return empty vector
    // In a real implementation, you'd search for nodes matching the query entities
    
    return start_nodes;
}

NodeID QueryRouter::find_best_start_node(const std::vector<std::string>& entities, Store* store) const {
    // This would implement sophisticated node matching
    // For now, return zero node
    return NodeID{};
}

QueryPolicy QueryRouter::adapt_policy_for_complexity(const QueryPolicy& base_policy, 
                                                    QueryComplexity complexity) const {
    QueryPolicy adapted_policy = base_policy;
    
    switch (complexity) {
        case QueryComplexity::SIMPLE:
            // Reduce beam width and depth for simple queries
            adapted_policy.beam_params.beam_width = std::max(8, static_cast<int>(base_policy.beam_params.beam_width * 0.5));
            adapted_policy.beam_params.max_depth = std::max(2, static_cast<int>(base_policy.beam_params.max_depth * 0.7));
            break;
            
        case QueryComplexity::MODERATE:
            // Keep base policy mostly unchanged
            break;
            
        case QueryComplexity::COMPLEX:
            // Increase beam width and depth for complex queries
            adapted_policy.beam_params.beam_width = static_cast<int>(base_policy.beam_params.beam_width * 1.5);
            adapted_policy.beam_params.max_depth = static_cast<int>(base_policy.beam_params.max_depth * 1.3);
            adapted_policy.beam_params.stop_threshold *= 0.8; // More thorough search
            break;
            
        default:
            break;
    }
    
    return adapted_policy;
}

// PolicyUtils implementation
namespace PolicyUtils {
    std::string intent_to_string(QueryIntent intent) {
        switch (intent) {
            case QueryIntent::DEFINE: return "DEFINE";
            case QueryIntent::WHY: return "WHY";
            case QueryIntent::COMPARE: return "COMPARE";
            case QueryIntent::CAUSAL: return "CAUSAL";
            case QueryIntent::TEMPORAL: return "TEMPORAL";
            case QueryIntent::GENERAL: return "GENERAL";
            case QueryIntent::UNKNOWN: return "UNKNOWN";
            default: return "UNKNOWN";
        }
    }
    
    std::string complexity_to_string(QueryComplexity complexity) {
        switch (complexity) {
            case QueryComplexity::SIMPLE: return "SIMPLE";
            case QueryComplexity::MODERATE: return "MODERATE";
            case QueryComplexity::COMPLEX: return "COMPLEX";
            case QueryComplexity::UNKNOWN: return "UNKNOWN";
            default: return "UNKNOWN";
        }
    }
    
    bool requires_multi_hop_reasoning(QueryIntent intent, QueryComplexity complexity) {
        switch (intent) {
            case QueryIntent::WHY:
            case QueryIntent::CAUSAL:
                return complexity != QueryComplexity::SIMPLE;
            case QueryIntent::COMPARE:
                return complexity == QueryComplexity::COMPLEX;
            default:
                return complexity == QueryComplexity::COMPLEX;
        }
    }
    
    size_t get_recommended_beam_width(QueryIntent intent, QueryComplexity complexity) {
        size_t base_width = 16;
        
        switch (intent) {
            case QueryIntent::DEFINE:
                base_width = 12;
                break;
            case QueryIntent::WHY:
            case QueryIntent::CAUSAL:
                base_width = 20;
                break;
            case QueryIntent::COMPARE:
                base_width = 24;
                break;
            case QueryIntent::TEMPORAL:
                base_width = 16;
                break;
            default:
                base_width = 16;
                break;
        }
        
        switch (complexity) {
            case QueryComplexity::SIMPLE:
                return base_width / 2;
            case QueryComplexity::MODERATE:
                return base_width;
            case QueryComplexity::COMPLEX:
                return base_width * 2;
            default:
                return base_width;
        }
    }
    
    size_t get_recommended_max_depth(QueryIntent intent, QueryComplexity complexity) {
        size_t base_depth = 4;
        
        switch (intent) {
            case QueryIntent::DEFINE:
                base_depth = 3;
                break;
            case QueryIntent::WHY:
            case QueryIntent::CAUSAL:
                base_depth = 5;
                break;
            case QueryIntent::COMPARE:
                base_depth = 4;
                break;
            case QueryIntent::TEMPORAL:
                base_depth = 4;
                break;
            default:
                base_depth = 4;
                break;
        }
        
        switch (complexity) {
            case QueryComplexity::SIMPLE:
                return std::max(2, static_cast<int>(base_depth * 0.7));
            case QueryComplexity::MODERATE:
                return base_depth;
            case QueryComplexity::COMPLEX:
                return static_cast<int>(base_depth * 1.5);
            default:
                return base_depth;
        }
    }
    
    bool has_definitive_answer(QueryIntent intent) {
        switch (intent) {
            case QueryIntent::DEFINE:
            case QueryIntent::TEMPORAL:
                return true;
            case QueryIntent::WHY:
            case QueryIntent::CAUSAL:
            case QueryIntent::COMPARE:
                return false;
            default:
                return false;
        }
    }
    
    double get_confidence_adjustment(QueryIntent intent) {
        switch (intent) {
            case QueryIntent::DEFINE:
                return 1.2; // Boost confidence for definitions
            case QueryIntent::WHY:
            case QueryIntent::CAUSAL:
                return 0.8; // Reduce confidence for causal questions
            case QueryIntent::COMPARE:
                return 0.9; // Slightly reduce for comparisons
            default:
                return 1.0; // No adjustment
        }
    }
}

} // namespace melvin
