#pragma once

#include "../core/types_v2.h"
#include "../core/global_workspace.h"
#include "../core/neuromodulators.h"
#include "../memory/semantic_bridge.h"
#include "language_generator.h"
#include "../../include/melvin_types.h"
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <chrono>

namespace melvin::v2::reasoning {

// Import types
using melvin::v2::memory::SemanticFact;
using melvin::v2::memory::SemanticNode;
using melvin::v2::memory::BridgeNodeID;

/**
 * @file conversation_engine.h
 * @brief ChatGPT-style conversational interface grounded in semantic graph
 * 
 * Purpose:
 * - Natural dialogue with users
 * - Query understanding (text → intent)
 * - Memory retrieval (semantic + episodic)
 * - Reasoning chains (graph traversal)
 * - Response generation (concepts → text)
 * 
 * Key principles:
 * - Every response grounded in graph knowledge
 * - Multi-turn conversation memory
 * - Context-aware (uses Global Workspace)
 * - Emotionally modulated (via Neuromodulators)
 * - Honest about uncertainty
 * 
 * Biological analog: Prefrontal cortex + language areas (Broca/Wernicke)
 */

// ============================================================================
// QUERY TYPES & INTENTS
// ============================================================================

/**
 * Types of user queries
 */
enum class QueryType {
    WHAT_IS,           // "What is X?" - definition
    DESCRIBE,          // "Describe X" - detailed description
    WHY,               // "Why X?" - explanation/causation
    HOW,               // "How X?" - process/mechanism
    WHERE,             // "Where X?" - location
    WHEN,              // "When X?" - temporal
    WHO,               // "Who X?" - agent
    RECALL,            // "What did you see?" - episodic memory
    RELATION,          // "How does X relate to Y?" - relation query
    EXPECTATION,       // "What happens if X?" - prediction
    COMPARISON,        // "Compare X and Y" - similarity/difference
    GENERAL,           // General conversation
    GREETING,          // "Hello", "Hi"
    GOODBYE,           // "Bye", "Goodbye"
    UNKNOWN            // Unrecognized pattern
};

/**
 * Parsed query intent
 */
struct QueryIntent {
    QueryType type;
    std::string raw_text;               // Original query
    std::vector<std::string> keywords;  // Extracted keywords
    std::string subject;                // Main subject (if any)
    std::string object;                 // Secondary object (if any)
    std::string temporal_context;       // Time context ("earlier", "yesterday")
    float confidence;                   // Parse confidence (0-1)
    
    QueryIntent() : type(QueryType::UNKNOWN), confidence(0.0f) {}
};

// ============================================================================
// CONVERSATION TURN
// ============================================================================

/**
 * Single turn in conversation (user message + Melvin response)
 */
struct ConversationTurn {
    uint64_t turn_id;
    std::string user_message;
    std::string melvin_response;
    QueryIntent parsed_intent;
    std::vector<BridgeNodeID> relevant_nodes;     // Nodes used in reasoning
    std::vector<SemanticFact> facts_used;  // Facts used
    NeuromodState neuromod_state;           // Emotional state during turn
    Timestamp timestamp;
    
    ConversationTurn() : turn_id(0), timestamp(0) {}
};

// ============================================================================
// CONVERSATION CONTEXT
// ============================================================================

/**
 * Context for current conversation
 */
struct ConversationContext {
    std::string conversation_id;
    std::deque<ConversationTurn> history;   // Recent turns
    std::vector<BridgeNodeID> mentioned_concepts; // All concepts mentioned
    std::string current_topic;              // Current topic (if any)
    size_t turn_count;
    Timestamp started_at;
    
    ConversationContext() : turn_count(0), started_at(0) {}
};

// ============================================================================
// CONVERSATION ENGINE
// ============================================================================

/**
 * ConversationEngine - ChatGPT-style dialogue system
 * 
 * Conversation flow:
 * 1. User message → parse intent
 * 2. Search semantic memory for relevant concepts
 * 3. Retrieve facts/episodes from graph
 * 4. Reason about the query (graph traversal)
 * 5. Generate response (grounded in facts)
 * 6. Store turn in conversation history
 * 7. Update Global Workspace with new concepts
 * 
 * Features:
 * - Multi-turn memory (remembers conversation)
 * - Context-aware (references previous turns)
 * - Graph-grounded (never hallucinates)
 * - Emotionally aware (uses neuromodulators)
 * - Honest about uncertainty
 */
class ConversationEngine {
public:
    struct Config {
        size_t max_history_turns;      // Max turns to remember (default: 20)
        size_t max_facts_per_response;  // Max facts to include (default: 5)
        float relevance_threshold;      // Min relevance for facts (default: 0.3)
        bool use_episodic_memory;       // Query episodic snapshots (default: true)
        bool verbose_reasoning;         // Show reasoning process (default: false)
        GenerationStyle default_style;  // Default language style
        
        Config()
            : max_history_turns(20)
            , max_facts_per_response(5)
            , relevance_threshold(0.3f)
            , use_episodic_memory(true)
            , verbose_reasoning(false)
            , default_style(GenerationStyle::CONVERSATIONAL) {}
    };
    
    ConversationEngine(
        const memory::SemanticBridge& semantic_bridge,
        GlobalWorkspace& workspace,
        Neuromodulators& neuromod,
        const Config& config = Config()
    );
    ~ConversationEngine();
    
    // ========================================================================
    // MAIN CONVERSATION API
    // ========================================================================
    
    /**
     * Main conversation method - respond to user message
     * @param user_message User's text input
     * @return Melvin's response
     * 
     * Example:
     *   respond("What is a cup?")
     *   → "A cup is a container. It's used for drinking beverages."
     */
    std::string respond(const std::string& user_message);
    
    /**
     * Respond with detailed reasoning shown
     * @param user_message User input
     * @param reasoning_output Output parameter for reasoning chain
     * @return Response text
     */
    std::string respond_with_reasoning(
        const std::string& user_message,
        std::string& reasoning_output
    );
    
    // ========================================================================
    // CONVERSATION MANAGEMENT
    // ========================================================================
    
    /**
     * Start new conversation (clears history)
     */
    void start_new_conversation();
    
    /**
     * Get current conversation context
     */
    const ConversationContext& get_context() const { return context_; }
    
    /**
     * Get conversation history
     */
    const std::deque<ConversationTurn>& get_history() const { return context_.history; }
    
    /**
     * Get last N turns
     */
    std::vector<ConversationTurn> get_recent_turns(size_t n) const;
    
    /**
     * Clear conversation history
     */
    void clear_history();
    
    // ========================================================================
    // QUERY UNDERSTANDING
    // ========================================================================
    
    /**
     * Parse user query into structured intent
     * @param text User message
     * @return Parsed intent
     * 
     * Example:
     *   parse_query("What is a cup?")
     *   → QueryIntent{type=WHAT_IS, subject="cup", confidence=0.9}
     */
    QueryIntent parse_query(const std::string& text) const;
    
    /**
     * Extract keywords from text
     */
    std::vector<std::string> extract_keywords(const std::string& text) const;
    
    /**
     * Detect query type from patterns
     */
    QueryType detect_query_type(const std::string& text) const;
    
    // ========================================================================
    // REASONING & RETRIEVAL
    // ========================================================================
    
    /**
     * Find relevant concepts for query
     * @param intent Parsed query intent
     * @return Relevant node IDs ranked by relevance
     */
    std::vector<BridgeNodeID> find_relevant_concepts(const QueryIntent& intent) const;
    
    /**
     * Retrieve facts for reasoning
     * @param concepts Relevant concept nodes
     * @param intent Original query intent
     * @return Semantic facts to use in response
     */
    std::vector<SemanticFact> retrieve_facts(
        const std::vector<BridgeNodeID>& concepts,
        const QueryIntent& intent
    ) const;
    
    /**
     * Reason about query (graph traversal + inference)
     * @param intent Query intent
     * @param concepts Relevant concepts
     * @param facts Retrieved facts
     * @return Reasoning conclusion (concepts to verbalize)
     */
    std::vector<BridgeNodeID> reason_about_query(
        const QueryIntent& intent,
        const std::vector<BridgeNodeID>& concepts,
        const std::vector<SemanticFact>& facts
    ) const;
    
    // ========================================================================
    // SPECIALIZED QUERY HANDLERS
    // ========================================================================
    
    /**
     * Handle "What is X?" queries
     */
    std::string handle_what_is(const QueryIntent& intent) const;
    
    /**
     * Handle "Why X?" queries (causation/explanation)
     */
    std::string handle_why(const QueryIntent& intent) const;
    
    /**
     * Handle "How X?" queries (process/mechanism)
     */
    std::string handle_how(const QueryIntent& intent) const;
    
    /**
     * Handle "What did you see/hear?" (episodic recall)
     */
    std::string handle_recall(const QueryIntent& intent) const;
    
    /**
     * Handle "How does X relate to Y?"
     */
    std::string handle_relation(const QueryIntent& intent) const;
    
    /**
     * Handle "What happens if X?" (prediction)
     */
    std::string handle_expectation(const QueryIntent& intent) const;
    
    /**
     * Handle greetings
     */
    std::string handle_greeting(const QueryIntent& intent) const;
    
    /**
     * Handle goodbyes
     */
    std::string handle_goodbye(const QueryIntent& intent) const;
    
    /**
     * Handle general conversation (fallback)
     */
    std::string handle_general(const QueryIntent& intent) const;
    
    // ========================================================================
    // INTEGRATION WITH OTHER SYSTEMS
    // ========================================================================
    
    /**
     * Update Global Workspace with conversation concepts
     * Posts relevant concepts as thoughts
     */
    void update_workspace(const std::vector<BridgeNodeID>& concepts);
    
    /**
     * Get emotional tone from neuromodulators
     */
    EmotionalTone get_current_emotional_tone() const;
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void set_config(const Config& config) { config_ = config; }
    Config get_config() const { return config_; }
    
    void set_language_style(GenerationStyle style);
    void set_verbosity(float v);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct ConversationStats {
        size_t total_turns;
        size_t successful_responses;
        size_t unknown_queries;
        size_t facts_retrieved;
        size_t concepts_discussed;
        std::unordered_map<QueryType, size_t> query_type_counts;
    };
    
    ConversationStats get_stats() const;
    void reset_stats();
    
private:
    // ========================================================================
    // INTERNAL STATE
    // ========================================================================
    
    const memory::SemanticBridge& semantic_bridge_;
    GlobalWorkspace& workspace_;
    Neuromodulators& neuromod_;
    
    std::unique_ptr<LanguageGenerator> language_gen_;
    
    Config config_;
    ConversationContext context_;
    
    // Statistics
    mutable ConversationStats stats_;
    
    // ========================================================================
    // QUERY PARSING HELPERS
    // ========================================================================
    
    // Check if text matches pattern
    bool matches_pattern(const std::string& text, const std::string& pattern) const;
    
    // Extract subject from query
    std::string extract_subject(const std::string& text, QueryType type) const;
    
    // Extract object from query (for relation queries)
    std::string extract_object(const std::string& text) const;
    
    // Extract temporal context
    std::string extract_temporal_context(const std::string& text) const;
    
    // Normalize text (lowercase, trim, etc.)
    std::string normalize_text(const std::string& text) const;
    
    // ========================================================================
    // REASONING HELPERS
    // ========================================================================
    
    // Score concept relevance to query
    float score_concept_relevance(BridgeNodeID concept_node, const QueryIntent& intent) const;
    
    // Build reasoning chain (for verbose mode)
    std::string build_reasoning_chain(
        const QueryIntent& intent,
        const std::vector<BridgeNodeID>& concepts,
        const std::vector<SemanticFact>& facts
    ) const;
    
    // ========================================================================
    // CONVERSATION HELPERS
    // ========================================================================
    
    // Store turn in history
    void store_turn(const ConversationTurn& turn);
    
    // Check if concept was mentioned recently
    bool was_recently_mentioned(BridgeNodeID concept_node) const;
    
    // Generate turn ID
    uint64_t generate_turn_id();
    
    // Get current timestamp
    Timestamp get_timestamp() const;
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Convert QueryType to string
 */
std::string query_type_to_string(QueryType type);

/**
 * Pretty-print conversation turn
 */
std::string turn_to_string(const ConversationTurn& turn);

/**
 * Pretty-print query intent
 */
std::string intent_to_string(const QueryIntent& intent);

/**
 * Simple text tokenization
 */
std::vector<std::string> tokenize(const std::string& text);

/**
 * Remove stop words
 */
std::vector<std::string> remove_stop_words(const std::vector<std::string>& words);

} // namespace melvin::v2::reasoning

