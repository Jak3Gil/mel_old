#pragma once

#include "../core/types_v2.h"
#include "../core/neuromodulators.h"
#include "../memory/semantic_bridge.h"
#include "../../include/melvin_types.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <random>

namespace melvin::v2::reasoning {

// Import types from memory bridge
using melvin::v2::memory::SemanticFact;
using melvin::v2::memory::SemanticNode;
using melvin::v2::memory::BridgeNodeID;
using melvin::Rel;

/**
 * @file language_generator.h
 * @brief Graph-grounded language generation (NOT token prediction)
 * 
 * Purpose:
 * - Convert internal graph concepts → natural language
 * - Template-based generation (expandable to neural later)
 * - NEVER hallucinates - only speaks from graph knowledge
 * - Emotional tone modulated by neuromodulators
 * 
 * Key principle:
 * - Every sentence must be traceable to graph facts
 * - "I don't know" is always an acceptable answer
 * - Clarity > fluency
 * 
 * Biological analog: Broca's area + motor planning
 */

// ============================================================================
// GENERATION CONFIGURATION
// ============================================================================

enum class GenerationStyle {
    CONCISE,       // Short, factual ("Cup is a container.")
    DETAILED,      // Elaborate ("A cup is a type of container typically used for beverages.")
    CONVERSATIONAL,// Natural dialogue ("Well, a cup is basically a container you drink from.")
    SCIENTIFIC     // Precise, formal ("Entity 'cup' classified as container with beverage storage function.")
};

enum class EmotionalTone {
    NEUTRAL,       // No emotional coloring
    CURIOUS,       // Interested, engaged ("That's interesting!")
    CONFIDENT,     // Certain ("I'm sure that...")
    UNCERTAIN,     // Hesitant ("I think maybe...")
    EXCITED        // High energy ("Wow!")
};

struct GenerationConfig {
    GenerationStyle style;
    EmotionalTone tone;
    int max_sentence_length;    // Max words per sentence
    bool use_first_person;      // "I saw..." vs "System saw..."
    bool cite_sources;          // Add "(based on X)" citations
    float verbosity;            // 0-1 (0=minimal, 1=verbose)
    
    GenerationConfig()
        : style(GenerationStyle::CONVERSATIONAL)
        , tone(EmotionalTone::NEUTRAL)
        , max_sentence_length(20)
        , use_first_person(true)
        , cite_sources(false)
        , verbosity(0.5f) {}
};

// ============================================================================
// LANGUAGE TEMPLATES
// ============================================================================

/**
 * Template for generating sentences from graph patterns
 */
struct LanguageTemplate {
    Rel relation_type;              // Which relation this template handles
    std::vector<std::string> patterns;  // Alternative phrasings
    
    // Example:
    // relation_type = Rel::USED_FOR
    // patterns = {"{subject} is used for {object}",
    //             "{subject} serves to {object}",
    //             "You can use {subject} to {object}"}
};

// ============================================================================
// LANGUAGE GENERATOR
// ============================================================================

/**
 * LanguageGenerator converts graph knowledge into natural text
 * 
 * Core design:
 * 1. Input: concept nodes from reasoning
 * 2. Query semantic bridge for facts
 * 3. Select templates based on relation types
 * 4. Generate sentences
 * 5. Verify all sentences are grounded in graph
 * 6. Apply emotional tone (from neuromodulators)
 * 7. Output: natural language response
 * 
 * Never:
 * - Generate facts not in graph (hallucination)
 * - Use uncertain language without graph support
 * - Claim knowledge it doesn't have
 * 
 * Always:
 * - Traceable to graph paths
 * - Honest about uncertainty
 * - Clear and direct
 */
class LanguageGenerator {
public:
    LanguageGenerator(
        const memory::SemanticBridge& semantic_bridge,
        const GenerationConfig& config = GenerationConfig()
    );
    ~LanguageGenerator();
    
    // ========================================================================
    // MAIN GENERATION API
    // ========================================================================
    
    /**
     * Generate response from concept nodes
     * @param concept_nodes Nodes to describe/explain
     * @param context Optional context for generation
     * @return Natural language text
     * 
     * Example:
     *   nodes = [cup_node_id]
     *   → "A cup is a container. It's typically used for drinking."
     */
    std::string generate_from_nodes(
        const std::vector<BridgeNodeID>& concept_nodes,
        const std::string& context = ""
    ) const;
    
    /**
     * Generate response from facts
     * @param facts Semantic facts to verbalize
     * @return Natural language text
     */
    std::string generate_from_facts(
        const std::vector<SemanticFact>& facts
    ) const;
    
    /**
     * Generate single sentence from fact
     * @param fact Single semantic triple
     * @return Sentence
     * 
     * Example:
     *   fact = {cup, USED_FOR, drinking}
     *   → "A cup is used for drinking."
     */
    std::string generate_sentence(const SemanticFact& fact) const;
    
    /**
     * Describe a concept (answer "what is X?")
     * @param concept_label Concept to describe
     * @return Description
     * 
     * Example:
     *   describe_concept("cup")
     *   → "A cup is a container. It's used for drinking beverages. Cups are typically found in kitchens."
     */
    std::string describe_concept(const std::string& concept_label) const;
    
    /**
     * Explain a relation (answer "why X relates to Y?")
     * @param subject Subject concept
     * @param object Object concept
     * @return Explanation
     */
    std::string explain_relation(const std::string& subject, const std::string& object) const;
    
    /**
     * Generate "I don't know" response with explanation
     * @param query What was asked
     * @return Honest "I don't know" + reason
     * 
     * Example:
     *   → "I don't know about that. I haven't encountered 'quantum_physics' yet."
     */
    std::string generate_unknown_response(const std::string& query) const;
    
    // ========================================================================
    // EMOTIONAL/CONTEXTUAL GENERATION
    // ========================================================================
    
    /**
     * Generate with neuromodulator-driven tone
     * @param content Base content to generate
     * @param neuromod_state Current neuromodulator levels
     * @return Text with emotional coloring
     * 
     * High DA → enthusiastic
     * High NE → alert/focused
     * High 5-HT → calm/confident
     * Low 5-HT + High NE → uncertain
     */
    std::string generate_with_emotion(
        const std::string& content,
        const NeuromodState& neuromod_state
    ) const;
    
    /**
     * Add emotional prefix/suffix
     */
    std::string add_emotional_wrapper(
        const std::string& content,
        EmotionalTone tone
    ) const;
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void set_config(const GenerationConfig& config) { config_ = config; }
    GenerationConfig get_config() const { return config_; }
    
    void set_style(GenerationStyle style) { config_.style = style; }
    void set_tone(EmotionalTone tone) { config_.tone = tone; }
    void set_verbosity(float v) { config_.verbosity = v; }
    
    // ========================================================================
    // TEMPLATE MANAGEMENT
    // ========================================================================
    
    /**
     * Add custom template for a relation type
     */
    void add_template(Rel relation, const std::vector<std::string>& patterns);
    
    /**
     * Load templates from file
     */
    bool load_templates(const std::string& template_file);
    
    /**
     * Save templates to file
     */
    bool save_templates(const std::string& template_file) const;
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct GenerationStats {
        size_t total_generations;
        size_t sentences_generated;
        size_t unknown_responses;
        size_t facts_used;
        size_t template_hits;
        size_t template_misses;
    };
    
    GenerationStats get_stats() const;
    void reset_stats();
    
private:
    // ========================================================================
    // INTERNAL STATE
    // ========================================================================
    
    const memory::SemanticBridge& semantic_bridge_;
    GenerationConfig config_;
    
    // Template library (relation_type → templates)
    std::unordered_map<Rel, LanguageTemplate> templates_;
    
    // Statistics
    mutable GenerationStats stats_;
    
    // Random number generator (for template selection)
    mutable std::mt19937 rng_;
    
    // ========================================================================
    // TEMPLATE APPLICATION
    // ========================================================================
    
    /**
     * Apply template to fact
     * @param fact Semantic triple
     * @param template Template to apply
     * @return Generated sentence
     */
    std::string apply_template(
        const SemanticFact& fact,
        const LanguageTemplate& tmpl
    ) const;
    
    /**
     * Fill template slots
     * @param pattern Template pattern with {subject}, {object} placeholders
     * @param subject Subject label
     * @param object Object label
     * @return Filled sentence
     */
    std::string fill_template(
        const std::string& pattern,
        const std::string& subject,
        const std::string& object
    ) const;
    
    /**
     * Select random template variant
     */
    std::string select_template_variant(const std::vector<std::string>& patterns) const;
    
    // ========================================================================
    // GENERATION HELPERS
    // ========================================================================
    
    /**
     * Combine multiple sentences into paragraph
     */
    std::string combine_sentences(const std::vector<std::string>& sentences) const;
    
    /**
     * Add connective words ("also", "additionally", etc.)
     */
    std::string add_connectives(const std::vector<std::string>& sentences) const;
    
    /**
     * Adjust sentence for style
     */
    std::string apply_style(const std::string& sentence) const;
    
    /**
     * Compute emotional tone from neuromodulators
     */
    EmotionalTone compute_tone_from_neuromod(const NeuromodState& state) const;
    
    /**
     * Initialize default templates
     */
    void initialize_default_templates();
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Capitalize first letter
 */
std::string capitalize(const std::string& str);

/**
 * Add period if missing
 */
std::string ensure_period(const std::string& str);

/**
 * Clean up generated text (spacing, punctuation)
 */
std::string clean_text(const std::string& str);

} // namespace melvin::v2::reasoning

