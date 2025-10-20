#include "conversation_engine.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <regex>
#include <chrono>

namespace melvin::v2::reasoning {

using melvin::v2::memory::SemanticFact;
using melvin::v2::memory::SemanticNode;

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

ConversationEngine::ConversationEngine(
    const memory::SemanticBridge& semantic_bridge,
    GlobalWorkspace& workspace,
    Neuromodulators& neuromod,
    const Config& config
)
    : semantic_bridge_(semantic_bridge)
    , workspace_(workspace)
    , neuromod_(neuromod)
    , config_(config)
{
    // Create language generator
    GenerationConfig gen_config;
    gen_config.style = config_.default_style;
    gen_config.use_first_person = true;
    language_gen_ = std::make_unique<LanguageGenerator>(semantic_bridge_, gen_config);
    
    // Initialize conversation context
    start_new_conversation();
    
    stats_ = ConversationStats{};
}

ConversationEngine::~ConversationEngine() {
}

// ============================================================================
// MAIN CONVERSATION API
// ============================================================================

std::string ConversationEngine::respond(const std::string& user_message) {
    std::string reasoning_output;
    return respond_with_reasoning(user_message, reasoning_output);
}

std::string ConversationEngine::respond_with_reasoning(
    const std::string& user_message,
    std::string& reasoning_output
) {
    // Create new turn
    ConversationTurn turn;
    turn.turn_id = generate_turn_id();
    turn.user_message = user_message;
    turn.timestamp = get_timestamp();
    turn.neuromod_state = neuromod_.get_state();
    
    // Step 1: Parse query intent
    turn.parsed_intent = parse_query(user_message);
    
    // Step 2: Route to appropriate handler based on query type
    std::string response;
    
    switch (turn.parsed_intent.type) {
        case QueryType::WHAT_IS:
        case QueryType::DESCRIBE:
            response = handle_what_is(turn.parsed_intent);
            break;
        
        case QueryType::WHY:
            response = handle_why(turn.parsed_intent);
            break;
        
        case QueryType::HOW:
            response = handle_how(turn.parsed_intent);
            break;
        
        case QueryType::RECALL:
            response = handle_recall(turn.parsed_intent);
            break;
        
        case QueryType::RELATION:
            response = handle_relation(turn.parsed_intent);
            break;
        
        case QueryType::EXPECTATION:
            response = handle_expectation(turn.parsed_intent);
            break;
        
        case QueryType::GREETING:
            response = handle_greeting(turn.parsed_intent);
            break;
        
        case QueryType::GOODBYE:
            response = handle_goodbye(turn.parsed_intent);
            break;
        
        case QueryType::GENERAL:
        case QueryType::UNKNOWN:
        default:
            response = handle_general(turn.parsed_intent);
            break;
    }
    
    // Apply emotional tone
    EmotionalTone tone = get_current_emotional_tone();
    if (tone != EmotionalTone::NEUTRAL) {
        response = language_gen_->add_emotional_wrapper(response, tone);
    }
    
    // Store turn
    turn.melvin_response = response;
    store_turn(turn);
    
    // Update Global Workspace
    update_workspace(turn.relevant_nodes);
    
    // Build reasoning output if verbose
    if (config_.verbose_reasoning) {
        reasoning_output = build_reasoning_chain(
            turn.parsed_intent,
            turn.relevant_nodes,
            turn.facts_used
        );
    }
    
    // Update stats
    stats_.total_turns++;
    if (!response.empty() && response != "I don't know.") {
        stats_.successful_responses++;
    } else {
        stats_.unknown_queries++;
    }
    stats_.query_type_counts[turn.parsed_intent.type]++;
    
    return response;
}

// ============================================================================
// CONVERSATION MANAGEMENT
// ============================================================================

void ConversationEngine::start_new_conversation() {
    context_ = ConversationContext{};
    context_.conversation_id = "conv_" + std::to_string(get_timestamp());
    context_.started_at = get_timestamp();
    context_.turn_count = 0;
}

std::vector<ConversationTurn> ConversationEngine::get_recent_turns(size_t n) const {
    std::vector<ConversationTurn> recent;
    size_t start = context_.history.size() > n ? context_.history.size() - n : 0;
    
    for (size_t i = start; i < context_.history.size(); i++) {
        recent.push_back(context_.history[i]);
    }
    
    return recent;
}

void ConversationEngine::clear_history() {
    context_.history.clear();
    context_.mentioned_concepts.clear();
    context_.turn_count = 0;
}

// ============================================================================
// QUERY UNDERSTANDING
// ============================================================================

QueryIntent ConversationEngine::parse_query(const std::string& text) const {
    QueryIntent intent;
    intent.raw_text = text;
    intent.confidence = 0.7f;  // Default confidence
    
    // Normalize text
    std::string normalized = normalize_text(text);
    
    // Detect query type
    intent.type = detect_query_type(normalized);
    
    // Extract keywords
    intent.keywords = extract_keywords(normalized);
    
    // Extract subject/object based on query type
    intent.subject = extract_subject(normalized, intent.type);
    intent.object = extract_object(normalized);
    intent.temporal_context = extract_temporal_context(normalized);
    
    return intent;
}

std::vector<std::string> ConversationEngine::extract_keywords(const std::string& text) const {
    // Tokenize
    auto tokens = tokenize(text);
    
    // Remove stop words
    tokens = remove_stop_words(tokens);
    
    return tokens;
}

QueryType ConversationEngine::detect_query_type(const std::string& text) const {
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    // Greetings
    if (matches_pattern(lower, "hello") || matches_pattern(lower, "hi") || 
        matches_pattern(lower, "hey")) {
        return QueryType::GREETING;
    }
    
    // Goodbyes
    if (matches_pattern(lower, "bye") || matches_pattern(lower, "goodbye") ||
        matches_pattern(lower, "see you")) {
        return QueryType::GOODBYE;
    }
    
    // What is / Describe
    if (matches_pattern(lower, "what is") || matches_pattern(lower, "what's") ||
        matches_pattern(lower, "define")) {
        return QueryType::WHAT_IS;
    }
    
    if (matches_pattern(lower, "describe") || matches_pattern(lower, "tell me about")) {
        return QueryType::DESCRIBE;
    }
    
    // Why
    if (matches_pattern(lower, "why")) {
        return QueryType::WHY;
    }
    
    // How
    if (matches_pattern(lower, "how does") || matches_pattern(lower, "how do")) {
        return QueryType::HOW;
    }
    
    // Recall (episodic memory)
    if (matches_pattern(lower, "what did you") || matches_pattern(lower, "did you see") ||
        matches_pattern(lower, "did you hear") || matches_pattern(lower, "remember when")) {
        return QueryType::RECALL;
    }
    
    // Relation
    if (matches_pattern(lower, "relate") || matches_pattern(lower, "connection") ||
        matches_pattern(lower, "relationship between")) {
        return QueryType::RELATION;
    }
    
    // Expectation / Prediction
    if (matches_pattern(lower, "what happens if") || matches_pattern(lower, "what will") ||
        matches_pattern(lower, "predict")) {
        return QueryType::EXPECTATION;
    }
    
    // Where
    if (matches_pattern(lower, "where")) {
        return QueryType::WHERE;
    }
    
    // When
    if (matches_pattern(lower, "when")) {
        return QueryType::WHEN;
    }
    
    // Who
    if (matches_pattern(lower, "who")) {
        return QueryType::WHO;
    }
    
    // Default to general
    return QueryType::GENERAL;
}

// ============================================================================
// REASONING & RETRIEVAL
// ============================================================================

std::vector<BridgeNodeID> ConversationEngine::find_relevant_concepts(const QueryIntent& intent) const {
    std::vector<BridgeNodeID> concepts;
    
    // Search for subject
    if (!intent.subject.empty()) {
        BridgeNodeID subj_id = semantic_bridge_.find_concept(intent.subject);
        if (subj_id != 0) {
            concepts.push_back(subj_id);
        }
    }
    
    // Search for object
    if (!intent.object.empty()) {
        BridgeNodeID obj_id = semantic_bridge_.find_concept(intent.object);
        if (obj_id != 0) {
            concepts.push_back(obj_id);
        }
    }
    
    // Search for keywords
    for (const auto& keyword : intent.keywords) {
        BridgeNodeID kw_id = semantic_bridge_.find_concept(keyword);
        if (kw_id != 0 && std::find(concepts.begin(), concepts.end(), kw_id) == concepts.end()) {
            concepts.push_back(kw_id);
        }
    }
    
    return concepts;
}

std::vector<SemanticFact> ConversationEngine::retrieve_facts(
    const std::vector<BridgeNodeID>& concepts,
    const QueryIntent& intent
) const {
    std::vector<SemanticFact> facts;
    
    for (BridgeNodeID concept_node : concepts) {
        std::string label = semantic_bridge_.get_label(concept_node);
        if (label.empty()) continue;
        
        // Get description facts
        auto desc_facts = semantic_bridge_.describe_concept(label);
        
        // Add relevant facts
        for (const auto& fact : desc_facts) {
            if (fact.confidence >= config_.relevance_threshold) {
                facts.push_back(fact);
            }
            
            if (facts.size() >= config_.max_facts_per_response) {
                break;
            }
        }
        
        if (facts.size() >= config_.max_facts_per_response) {
            break;
        }
    }
    
    return facts;
}

std::vector<BridgeNodeID> ConversationEngine::reason_about_query(
    const QueryIntent& intent,
    const std::vector<BridgeNodeID>& concepts,
    const std::vector<SemanticFact>& facts
) const {
    // For now, simple reasoning: return input concepts
    // Future: add graph traversal, inference chains, etc.
    return concepts;
}

// ============================================================================
// SPECIALIZED QUERY HANDLERS
// ============================================================================

std::string ConversationEngine::handle_what_is(const QueryIntent& intent) const {
    if (intent.subject.empty()) {
        return "What would you like to know about?";
    }
    
    // Generate description
    std::string response = language_gen_->describe_concept(intent.subject);
    
    return response;
}

std::string ConversationEngine::handle_why(const QueryIntent& intent) const {
    if (intent.subject.empty()) {
        return "Why what?";
    }
    
    // Look for causal relations
    auto causes = semantic_bridge_.query_causes(intent.subject);
    
    if (causes.empty()) {
        return "I don't know why " + intent.subject + " happens. I haven't learned about its causes yet.";
    }
    
    // Generate explanation
    std::vector<std::string> explanations;
    for (const auto& cause : causes) {
        explanations.push_back(cause.label + " leads to " + intent.subject);
    }
    
    return explanations[0] + ".";
}

std::string ConversationEngine::handle_how(const QueryIntent& intent) const {
    if (intent.subject.empty()) {
        return "How does what work?";
    }
    
    // Look for process/mechanism relations
    // For now, provide general description
    return handle_what_is(intent);
}

std::string ConversationEngine::handle_recall(const QueryIntent& intent) const {
    // Query episodic memory (future: integrate with episodic system)
    // For now, check recent conversation
    if (context_.history.size() < 2) {
        return "I don't have much to recall yet. We just started talking!";
    }
    
    // Recall recent concepts
    std::string response = "Recently, we discussed ";
    
    size_t n = std::min((size_t)3, context_.mentioned_concepts.size());
    for (size_t i = 0; i < n; i++) {
        BridgeNodeID concept_node = context_.mentioned_concepts[context_.mentioned_concepts.size() - 1 - i];
        std::string label = semantic_bridge_.get_label(concept_node);
        if (!label.empty()) {
            response += label;
            if (i < n - 1) response += ", ";
        }
    }
    
    response += ".";
    return response;
}

std::string ConversationEngine::handle_relation(const QueryIntent& intent) const {
    if (intent.subject.empty() || intent.object.empty()) {
        return "Which two things would you like me to relate?";
    }
    
    return language_gen_->explain_relation(intent.subject, intent.object);
}

std::string ConversationEngine::handle_expectation(const QueryIntent& intent) const {
    if (intent.subject.empty()) {
        return "What scenario would you like me to predict?";
    }
    
    // Query expectations
    auto expectations = semantic_bridge_.query_expectations(intent.subject);
    
    if (expectations.empty()) {
        return "I don't have predictions about what happens after " + intent.subject + ".";
    }
    
    // Generate prediction
    std::string response = "After " + intent.subject + ", I'd expect ";
    
    for (size_t i = 0; i < std::min((size_t)3, expectations.size()); i++) {
        response += expectations[i].label;
        if (i < expectations.size() - 1) response += ", ";
    }
    
    response += ".";
    return response;
}

std::string ConversationEngine::handle_greeting(const QueryIntent& intent) const {
    // Vary greeting based on neuromodulator state
    float dopamine = neuromod_.get_dopamine();
    
    if (dopamine > 0.7f) {
        return "Hello! I'm excited to chat with you!";
    } else if (dopamine > 0.5f) {
        return "Hello! How can I help you?";
    } else {
        return "Hello.";
    }
}

std::string ConversationEngine::handle_goodbye(const QueryIntent& intent) const {
    return "Goodbye! It was nice talking with you.";
}

std::string ConversationEngine::handle_general(const QueryIntent& intent) const {
    // Try to extract concepts and describe them
    auto concepts = find_relevant_concepts(intent);
    
    if (!concepts.empty()) {
        return language_gen_->generate_from_nodes(concepts, intent.raw_text);
    }
    
    // Fallback
    return "I'm not sure how to respond to that. Could you ask about a specific concept?";
}

// ============================================================================
// INTEGRATION WITH OTHER SYSTEMS
// ============================================================================

void ConversationEngine::update_workspace(const std::vector<BridgeNodeID>& concepts) {
    // Post concepts as thoughts to Global Workspace
    for (BridgeNodeID concept_node : concepts) {
        Thought thought;
        thought.type = "concept";
        thought.salience = 0.8f;  // High salience for conversation topics
        thought.concept_refs.push_back(static_cast<melvin::v2::NodeID>(concept_node & 0xFFFF));
        
        workspace_.post(thought);
    }
}

EmotionalTone ConversationEngine::get_current_emotional_tone() const {
    auto state = neuromod_.get_state();
    
    // High dopamine → excited
    if (state.dopamine > 0.7f) {
        return EmotionalTone::EXCITED;
    }
    
    // High norepinephrine → curious
    if (state.norepinephrine > 0.7f) {
        return EmotionalTone::CURIOUS;
    }
    
    // High serotonin → confident
    if (state.serotonin > 0.6f) {
        return EmotionalTone::CONFIDENT;
    }
    
    // Low serotonin → uncertain
    if (state.serotonin < 0.4f) {
        return EmotionalTone::UNCERTAIN;
    }
    
    return EmotionalTone::NEUTRAL;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void ConversationEngine::set_language_style(GenerationStyle style) {
    auto config = language_gen_->get_config();
    config.style = style;
    language_gen_->set_config(config);
}

void ConversationEngine::set_verbosity(float v) {
    auto config = language_gen_->get_config();
    config.verbosity = v;
    language_gen_->set_config(config);
}

// ============================================================================
// STATISTICS
// ============================================================================

ConversationEngine::ConversationStats ConversationEngine::get_stats() const {
    auto lang_stats = language_gen_->get_stats();
    stats_.facts_retrieved = lang_stats.facts_used;
    stats_.concepts_discussed = context_.mentioned_concepts.size();
    
    return stats_;
}

void ConversationEngine::reset_stats() {
    stats_ = ConversationStats{};
    language_gen_->reset_stats();
}

// ============================================================================
// QUERY PARSING HELPERS
// ============================================================================

bool ConversationEngine::matches_pattern(const std::string& text, const std::string& pattern) const {
    return text.find(pattern) != std::string::npos;
}

std::string ConversationEngine::extract_subject(const std::string& text, QueryType type) const {
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    // Simple extraction based on patterns
    size_t pos;
    
    switch (type) {
        case QueryType::WHAT_IS:
            pos = lower.find("what is ");
            if (pos != std::string::npos) {
                std::string rest = text.substr(pos + 8);
                // Remove question mark and "a"
                rest.erase(std::remove(rest.begin(), rest.end(), '?'), rest.end());
                if (rest.substr(0, 2) == "a ") rest = rest.substr(2);
                return rest;
            }
            break;
        
        case QueryType::DESCRIBE:
            pos = lower.find("describe ");
            if (pos != std::string::npos) {
                return text.substr(pos + 9);
            }
            break;
        
        default:
            // Extract first keyword as subject
            auto keywords = extract_keywords(text);
            if (!keywords.empty()) {
                return keywords[0];
            }
            break;
    }
    
    return "";
}

std::string ConversationEngine::extract_object(const std::string& text) const {
    // Look for "and", "to", "with" patterns
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    size_t pos = lower.find(" and ");
    if (pos != std::string::npos) {
        std::string rest = text.substr(pos + 5);
        rest.erase(std::remove(rest.begin(), rest.end(), '?'), rest.end());
        return rest;
    }
    
    return "";
}

std::string ConversationEngine::extract_temporal_context(const std::string& text) const {
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    if (matches_pattern(lower, "earlier")) return "earlier";
    if (matches_pattern(lower, "yesterday")) return "yesterday";
    if (matches_pattern(lower, "before")) return "before";
    if (matches_pattern(lower, "recently")) return "recently";
    
    return "";
}

std::string ConversationEngine::normalize_text(const std::string& text) const {
    std::string normalized = text;
    
    // Trim whitespace
    normalized.erase(0, normalized.find_first_not_of(" \t\n\r"));
    normalized.erase(normalized.find_last_not_of(" \t\n\r") + 1);
    
    return normalized;
}

// ============================================================================
// REASONING HELPERS
// ============================================================================

float ConversationEngine::score_concept_relevance(BridgeNodeID concept_node, const QueryIntent& intent) const {
    // Simple relevance scoring
    std::string label = semantic_bridge_.get_label(concept_node);
    
    if (label == intent.subject) return 1.0f;
    if (label == intent.object) return 0.9f;
    
    for (const auto& keyword : intent.keywords) {
        if (label == keyword) return 0.7f;
    }
    
    return 0.3f;
}

std::string ConversationEngine::build_reasoning_chain(
    const QueryIntent& intent,
    const std::vector<BridgeNodeID>& concepts,
    const std::vector<SemanticFact>& facts
) const {
    std::stringstream ss;
    
    ss << "[Reasoning Chain]\n";
    ss << "Query Type: " << query_type_to_string(intent.type) << "\n";
    ss << "Subject: " << intent.subject << "\n";
    ss << "Concepts Found: " << concepts.size() << "\n";
    ss << "Facts Retrieved: " << facts.size() << "\n";
    
    return ss.str();
}

// ============================================================================
// CONVERSATION HELPERS
// ============================================================================

void ConversationEngine::store_turn(const ConversationTurn& turn) {
    // Add to history
    context_.history.push_back(turn);
    
    // Update mentioned concepts
    for (BridgeNodeID concept_node : turn.relevant_nodes) {
        if (!was_recently_mentioned(concept_node)) {
            context_.mentioned_concepts.push_back(concept_node);
        }
    }
    
    // Limit history size
    while (context_.history.size() > config_.max_history_turns) {
        context_.history.pop_front();
    }
    
    context_.turn_count++;
}

bool ConversationEngine::was_recently_mentioned(BridgeNodeID concept_node) const {
    return std::find(context_.mentioned_concepts.begin(), 
                    context_.mentioned_concepts.end(), 
                    concept_node) != context_.mentioned_concepts.end();
}

uint64_t ConversationEngine::generate_turn_id() {
    static uint64_t next_id = 1;
    return next_id++;
}

melvin::v2::Timestamp ConversationEngine::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

std::string query_type_to_string(QueryType type) {
    switch (type) {
        case QueryType::WHAT_IS: return "WHAT_IS";
        case QueryType::DESCRIBE: return "DESCRIBE";
        case QueryType::WHY: return "WHY";
        case QueryType::HOW: return "HOW";
        case QueryType::WHERE: return "WHERE";
        case QueryType::WHEN: return "WHEN";
        case QueryType::WHO: return "WHO";
        case QueryType::RECALL: return "RECALL";
        case QueryType::RELATION: return "RELATION";
        case QueryType::EXPECTATION: return "EXPECTATION";
        case QueryType::COMPARISON: return "COMPARISON";
        case QueryType::GENERAL: return "GENERAL";
        case QueryType::GREETING: return "GREETING";
        case QueryType::GOODBYE: return "GOODBYE";
        case QueryType::UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

std::string turn_to_string(const ConversationTurn& turn) {
    std::stringstream ss;
    ss << "Turn #" << turn.turn_id << "\n";
    ss << "User: " << turn.user_message << "\n";
    ss << "Melvin: " << turn.melvin_response << "\n";
    return ss.str();
}

std::string intent_to_string(const QueryIntent& intent) {
    std::stringstream ss;
    ss << "QueryIntent{\n";
    ss << "  type: " << query_type_to_string(intent.type) << "\n";
    ss << "  subject: " << intent.subject << "\n";
    ss << "  confidence: " << intent.confidence << "\n";
    ss << "}";
    return ss.str();
}

std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::stringstream ss(text);
    std::string token;
    
    while (ss >> token) {
        // Remove punctuation
        token.erase(std::remove_if(token.begin(), token.end(), ::ispunct), token.end());
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

std::vector<std::string> remove_stop_words(const std::vector<std::string>& words) {
    static const std::vector<std::string> stop_words = {
        "a", "an", "the", "is", "are", "was", "were", "be", "been",
        "what", "how", "why", "when", "where", "who", "which",
        "do", "does", "did", "can", "could", "would", "should",
        "i", "you", "me", "my", "your", "it", "its"
    };
    
    std::vector<std::string> filtered;
    
    for (const auto& word : words) {
        std::string lower = word;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        if (std::find(stop_words.begin(), stop_words.end(), lower) == stop_words.end()) {
            filtered.push_back(word);
        }
    }
    
    return filtered;
}

} // namespace melvin::v2::reasoning

