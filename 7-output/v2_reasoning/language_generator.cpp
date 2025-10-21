#include "language_generator.h"
#include <algorithm>
#include <sstream>
#include <cctype>

namespace melvin::v2::reasoning {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

LanguageGenerator::LanguageGenerator(
    const memory::SemanticBridge& semantic_bridge,
    const GenerationConfig& config
)
    : semantic_bridge_(semantic_bridge)
    , config_(config)
    , rng_(std::random_device{}())
{
    initialize_default_templates();
    stats_ = GenerationStats{};
}

LanguageGenerator::~LanguageGenerator() {
}

// ============================================================================
// MAIN GENERATION API
// ============================================================================

std::string LanguageGenerator::generate_from_nodes(
    const std::vector<BridgeNodeID>& concept_nodes,
    const std::string& context
) const {
    if (concept_nodes.empty()) {
        return generate_unknown_response(context);
    }
    
    std::vector<std::string> sentences;
    
    // Generate sentence for each concept
    for (BridgeNodeID node_id : concept_nodes) {
        // Get label
        std::string label = semantic_bridge_.get_label(node_id);
        if (label.empty()) {
            continue;  // Skip unlabeled nodes
        }
        
        // Get facts about this concept
        auto facts = semantic_bridge_.describe_concept(label);
        
        // Generate sentences from facts
        for (const auto& fact : facts) {
            std::string sent = generate_sentence(fact);
            if (!sent.empty()) {
                sentences.push_back(sent);
            }
            
            // Limit based on verbosity
            if (sentences.size() >= (size_t)(3 * config_.verbosity + 1)) {
                break;
            }
        }
        
        if (sentences.size() >= 5) {
            break;  // Don't overwhelm with too many facts
        }
    }
    
    if (sentences.empty()) {
        return generate_unknown_response(context);
    }
    
    stats_.total_generations++;
    stats_.sentences_generated += sentences.size();
    stats_.facts_used += sentences.size();
    
    return combine_sentences(sentences);
}

std::string LanguageGenerator::generate_from_facts(
    const std::vector<memory::SemanticFact>& facts
) const {
    if (facts.empty()) {
        return "I don't have any information about that.";
    }
    
    std::vector<std::string> sentences;
    for (const auto& fact : facts) {
        std::string sent = generate_sentence(fact);
        if (!sent.empty()) {
            sentences.push_back(sent);
        }
    }
    
    stats_.total_generations++;
    stats_.sentences_generated += sentences.size();
    stats_.facts_used += facts.size();
    
    return combine_sentences(sentences);
}

std::string LanguageGenerator::generate_sentence(const memory::SemanticFact& fact) const {
    // Find template for this relation type
    auto it = templates_.find(fact.predicate);
    if (it == templates_.end()) {
        stats_.template_misses++;
        // Fallback: simple format
        return fact.subject_label + " " + 
               relation_to_string(fact.predicate) + " " + 
               fact.object_label + ".";
    }
    
    stats_.template_hits++;
    return apply_template(fact, it->second);
}

std::string LanguageGenerator::describe_concept(const std::string& concept_label) const {
    // Find concept in graph
    NodeID concept_id = semantic_bridge_.find_concept(concept_label);
    if (concept_id == 0) {
        return generate_unknown_response(concept_label);
    }
    
    // Get description facts
    auto facts = semantic_bridge_.describe_concept(concept_label);
    if (facts.empty()) {
        return "I know about " + concept_label + ", but I don't have detailed information.";
    }
    
    // Generate description
    std::vector<std::string> sentences;
    
    // Start with IS_A / PART_OF (definitions)
    for (const auto& fact : facts) {
        if (fact.predicate == Rel::PART_OF || fact.predicate == Rel::OBSERVED_AS) {
            sentences.push_back(generate_sentence(fact));
            if (sentences.size() >= 1) break;  // One definition is enough
        }
    }
    
    // Then USED_FOR (function)
    for (const auto& fact : facts) {
        if (fact.predicate == Rel::USED_FOR) {
            sentences.push_back(generate_sentence(fact));
            if (sentences.size() >= 2) break;
        }
    }
    
    // Then CO_OCCURS_WITH (context)
    if (sentences.size() < 3 && config_.verbosity > 0.5f) {
        for (const auto& fact : facts) {
            if (fact.predicate == Rel::CO_OCCURS_WITH) {
                sentences.push_back(generate_sentence(fact));
                if (sentences.size() >= 3) break;
            }
        }
    }
    
    stats_.total_generations++;
    stats_.sentences_generated += sentences.size();
    
    return combine_sentences(sentences);
}

std::string LanguageGenerator::explain_relation(
    const std::string& subject,
    const std::string& object
) const {
    // Find nodes
    NodeID subj_id = semantic_bridge_.find_concept(subject);
    NodeID obj_id = semantic_bridge_.find_concept(object);
    
    if (subj_id == 0 || obj_id == 0) {
        return "I don't know enough about " + subject + " or " + object + ".";
    }
    
    // Check for direct relation
    auto relations = semantic_bridge_.get_all_relations(subj_id);
    for (const auto& rel : relations) {
        if (rel.to == obj_id) {
            memory::SemanticFact fact;
            fact.subject = subj_id;
            fact.predicate = rel.relation_type;
            fact.object = obj_id;
            fact.subject_label = subject;
            fact.object_label = object;
            fact.confidence = rel.strength;
            
            return generate_sentence(fact);
        }
    }
    
    // No direct relation found
    return "I don't see a clear connection between " + subject + " and " + object + ".";
}

std::string LanguageGenerator::generate_unknown_response(const std::string& query) const {
    stats_.unknown_responses++;
    
    if (config_.use_first_person) {
        return "I don't know about " + query + ". I haven't encountered that yet.";
    } else {
        return "No information available about: " + query;
    }
}

// ============================================================================
// EMOTIONAL/CONTEXTUAL GENERATION
// ============================================================================

std::string LanguageGenerator::generate_with_emotion(
    const std::string& content,
    const NeuromodState& neuromod_state
) const {
    // Compute tone from neuromodulator state
    EmotionalTone tone = compute_tone_from_neuromod(neuromod_state);
    
    return add_emotional_wrapper(content, tone);
}

std::string LanguageGenerator::add_emotional_wrapper(
    const std::string& content,
    EmotionalTone tone
) const {
    switch (tone) {
        case EmotionalTone::CURIOUS:
            return "That's interesting! " + content;
        
        case EmotionalTone::CONFIDENT:
            return "I'm confident that " + content;
        
        case EmotionalTone::UNCERTAIN:
            return "I think " + content + " But I'm not entirely sure.";
        
        case EmotionalTone::EXCITED:
            return "Wow! " + content;
        
        case EmotionalTone::NEUTRAL:
        default:
            return content;
    }
}

// ============================================================================
// TEMPLATE MANAGEMENT
// ============================================================================

void LanguageGenerator::add_template(Rel relation, const std::vector<std::string>& patterns) {
    LanguageTemplate tmpl;
    tmpl.relation_type = relation;
    tmpl.patterns = patterns;
    templates_[relation] = tmpl;
}

bool LanguageGenerator::load_templates(const std::string& template_file) {
    // TODO: Implement JSON/YAML template loading
    return false;
}

bool LanguageGenerator::save_templates(const std::string& template_file) const {
    // TODO: Implement template saving
    return false;
}

// ============================================================================
// STATISTICS
// ============================================================================

LanguageGenerator::GenerationStats LanguageGenerator::get_stats() const {
    return stats_;
}

void LanguageGenerator::reset_stats() {
    stats_ = GenerationStats{};
}

// ============================================================================
// TEMPLATE APPLICATION
// ============================================================================

std::string LanguageGenerator::apply_template(
    const memory::SemanticFact& fact,
    const LanguageTemplate& tmpl
) const {
    // Select random pattern variant
    std::string pattern = select_template_variant(tmpl.patterns);
    
    // Fill template
    std::string sentence = fill_template(pattern, fact.subject_label, fact.object_label);
    
    // Apply style
    sentence = apply_style(sentence);
    
    return sentence;
}

std::string LanguageGenerator::fill_template(
    const std::string& pattern,
    const std::string& subject,
    const std::string& object
) const {
    std::string result = pattern;
    
    // Replace {subject}
    size_t pos = result.find("{subject}");
    if (pos != std::string::npos) {
        result.replace(pos, 9, subject);
    }
    
    // Replace {object}
    pos = result.find("{object}");
    if (pos != std::string::npos) {
        result.replace(pos, 8, object);
    }
    
    return result;
}

std::string LanguageGenerator::select_template_variant(
    const std::vector<std::string>& patterns
) const {
    if (patterns.empty()) {
        return "{subject} relates to {object}";
    }
    
    // Select randomly
    std::uniform_int_distribution<size_t> dist(0, patterns.size() - 1);
    return patterns[dist(rng_)];
}

// ============================================================================
// GENERATION HELPERS
// ============================================================================

std::string LanguageGenerator::combine_sentences(const std::vector<std::string>& sentences) const {
    if (sentences.empty()) {
        return "";
    }
    
    if (sentences.size() == 1) {
        return ensure_period(capitalize(sentences[0]));
    }
    
    // Add connectives for multi-sentence responses
    std::vector<std::string> connected = add_connectives(sentences);
    
    std::string result;
    for (const auto& sent : connected) {
        result += sent + " ";
    }
    
    return clean_text(result);
}

std::string LanguageGenerator::add_connectives(const std::vector<std::string>& sentences) const {
    std::vector<std::string> result;
    
    for (size_t i = 0; i < sentences.size(); i++) {
        std::string sent = sentences[i];
        
        // Add connective for subsequent sentences
        if (i == 1 && config_.verbosity > 0.3f) {
            sent = "It's " + sent;
        } else if (i == 2 && config_.verbosity > 0.5f) {
            sent = "Also, " + sent;
        } else if (i > 2 && config_.verbosity > 0.7f) {
            sent = "Additionally, " + sent;
        }
        
        result.push_back(ensure_period(sent));
    }
    
    return result;
}

std::string LanguageGenerator::apply_style(const std::string& sentence) const {
    std::string result = sentence;
    
    switch (config_.style) {
        case GenerationStyle::CONCISE:
            // Remove unnecessary words
            // (simple implementation - could be more sophisticated)
            break;
        
        case GenerationStyle::DETAILED:
            // Add qualifiers
            // (could expand with more detail)
            break;
        
        case GenerationStyle::CONVERSATIONAL:
            // Natural phrasing (default)
            break;
        
        case GenerationStyle::SCIENTIFIC:
            // Formal language
            // (could make more precise)
            break;
    }
    
    return result;
}

EmotionalTone LanguageGenerator::compute_tone_from_neuromod(const NeuromodState& state) const {
    // High dopamine → excited/curious
    if (state.dopamine > 0.7f) {
        return EmotionalTone::EXCITED;
    }
    
    // High norepinephrine → curious/alert
    if (state.norepinephrine > 0.7f) {
        return EmotionalTone::CURIOUS;
    }
    
    // High serotonin → confident/stable
    if (state.serotonin > 0.6f) {
        return EmotionalTone::CONFIDENT;
    }
    
    // Low serotonin + moderate NE → uncertain
    if (state.serotonin < 0.4f && state.norepinephrine > 0.5f) {
        return EmotionalTone::UNCERTAIN;
    }
    
    return EmotionalTone::NEUTRAL;
}

void LanguageGenerator::initialize_default_templates() {
    // USED_FOR templates
    add_template(Rel::USED_FOR, {
        "{subject} is used for {object}",
        "{subject} serves to {object}",
        "You can use {subject} to {object}",
        "{subject} helps with {object}"
    });
    
    // PART_OF templates
    add_template(Rel::PART_OF, {
        "{subject} is a type of {object}",
        "{subject} is part of {object}",
        "{subject} belongs to {object}"
    });
    
    // CO_OCCURS_WITH templates
    add_template(Rel::CO_OCCURS_WITH, {
        "{subject} often appears with {object}",
        "{subject} is found near {object}",
        "{subject} and {object} occur together"
    });
    
    // CAUSES templates
    add_template(Rel::CAUSES, {
        "{subject} causes {object}",
        "{subject} leads to {object}",
        "{subject} results in {object}"
    });
    
    // EXPECTS templates
    add_template(Rel::EXPECTS, {
        "{subject} usually leads to {object}",
        "After {subject}, you can expect {object}",
        "{subject} typically precedes {object}"
    });
    
    // OBSERVED_AS templates
    add_template(Rel::OBSERVED_AS, {
        "{subject} is observed as {object}",
        "{subject} appears as {object}",
        "{subject} looks like {object}"
    });
    
    // NAMED_AS templates
    add_template(Rel::NAMED_AS, {
        "{subject} is called {object}",
        "{subject} is named {object}",
        "{subject} is known as {object}"
    });
    
    // SPATIALLY_NEAR templates
    add_template(Rel::SPATIALLY_NEAR, {
        "{subject} is near {object}",
        "{subject} is close to {object}",
        "{subject} is next to {object}"
    });
    
    // SIMILAR_FEATURES templates
    add_template(Rel::SIMILAR_FEATURES, {
        "{subject} is similar to {object}",
        "{subject} resembles {object}",
        "{subject} looks like {object}"
    });
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

std::string capitalize(const std::string& str) {
    if (str.empty()) return str;
    std::string result = str;
    result[0] = std::toupper(result[0]);
    return result;
}

std::string ensure_period(const std::string& str) {
    if (str.empty()) return str;
    if (str.back() == '.' || str.back() == '!' || str.back() == '?') {
        return str;
    }
    return str + ".";
}

std::string clean_text(const std::string& str) {
    std::string result = str;
    
    // Remove double spaces
    size_t pos;
    while ((pos = result.find("  ")) != std::string::npos) {
        result.replace(pos, 2, " ");
    }
    
    // Trim trailing space
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    
    return result;
}

} // namespace melvin::v2::reasoning

