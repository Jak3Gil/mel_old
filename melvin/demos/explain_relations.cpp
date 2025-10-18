/*
 * Explain Relation Types - Show examples of each connection type
 */

#include "../core/storage.h"
#include "../core/types.h"
#include <iostream>
#include <map>
#include <vector>

using namespace melvin;

const char* get_relation_name(RelationType rel) {
    switch(rel) {
        case RelationType::SEMANTIC: return "SEMANTIC";
        case RelationType::CAUSAL: return "CAUSAL";
        case RelationType::ATTRIBUTE: return "ATTRIBUTE";
        case RelationType::SEQUENCE: return "SEQUENCE";
        case RelationType::ABSTRACTION: return "ABSTRACTION";
        case RelationType::REINFORCEMENT: return "REINFORCEMENT";
        case RelationType::MULTIMODAL: return "MULTIMODAL";
        default: return "UNKNOWN";
    }
}

void explain_relation_type(RelationType type, const std::string& description, 
                           const std::string& meaning, const std::string& use_case) {
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  " << get_relation_name(type) << "\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    std::cout << "  Definition: " << description << "\n\n";
    std::cout << "  Meaning:    " << meaning << "\n\n";
    std::cout << "  Use case:   " << use_case << "\n\n";
}

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  RELATION TYPES EXPLAINED                             ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    Storage storage;
    
    if (!storage.load("data/nodes.melvin", "data/edges.melvin")) {
        std::cerr << "✗ Failed to load\n";
        return 1;
    }
    
    // Organize edges by type
    std::map<RelationType, std::vector<Edge>> by_type;
    auto edges = storage.get_all_edges();
    
    for (const auto& edge : edges) {
        by_type[edge.relation].push_back(edge);
    }
    
    std::cout << "📊 YOUR KNOWLEDGE BASE HAS:\n";
    std::cout << "  " << by_type[RelationType::SEMANTIC].size() << " SEMANTIC connections\n";
    std::cout << "  " << by_type[RelationType::CAUSAL].size() << " CAUSAL connections\n";
    std::cout << "  " << by_type[RelationType::ATTRIBUTE].size() << " ATTRIBUTE connections\n\n";
    
    // Explain SEMANTIC
    explain_relation_type(
        RelationType::SEMANTIC,
        "General semantic relationship (related concepts)",
        "A is related to B in meaning/context",
        "Links concepts that co-occur or share semantic space"
    );
    
    std::cout << "  Examples from your data:\n";
    for (size_t i = 0; i < std::min(by_type[RelationType::SEMANTIC].size(), size_t(5)); ++i) {
        auto& e = by_type[RelationType::SEMANTIC][i];
        std::cout << "    • " << storage.get_node_content(e.from_id) 
                  << " → " << storage.get_node_content(e.to_id) << "\n";
    }
    std::cout << "\n  Reasoning: \"What is related to X?\"\n";
    std::cout << "  → Finds general associations\n\n";
    
    // Explain CAUSAL
    explain_relation_type(
        RelationType::CAUSAL,
        "Cause and effect relationship",
        "A causes B, A produces B, A leads to B",
        "Models how things happen, what produces what"
    );
    
    std::cout << "  Examples from your data:\n";
    for (size_t i = 0; i < std::min(by_type[RelationType::CAUSAL].size(), size_t(5)); ++i) {
        auto& e = by_type[RelationType::CAUSAL][i];
        std::cout << "    • " << storage.get_node_content(e.from_id) 
                  << " CAUSES → " << storage.get_node_content(e.to_id) << "\n";
    }
    std::cout << "\n  Reasoning: \"What happens if X?\"\n";
    std::cout << "  → Traces consequences and effects\n\n";
    
    // Explain ATTRIBUTE
    explain_relation_type(
        RelationType::ATTRIBUTE,
        "Property or requirement relationship",
        "A needs B, A requires B, A has property B",
        "Models requirements, properties, and dependencies"
    );
    
    std::cout << "  Examples from your data:\n";
    for (size_t i = 0; i < std::min(by_type[RelationType::ATTRIBUTE].size(), size_t(5)); ++i) {
        auto& e = by_type[RelationType::ATTRIBUTE][i];
        std::cout << "    • " << storage.get_node_content(e.from_id) 
                  << " NEEDS → " << storage.get_node_content(e.to_id) << "\n";
    }
    std::cout << "\n  Reasoning: \"What does X need?\"\n";
    std::cout << "  → Finds requirements and properties\n\n";
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  KEY DIFFERENCES\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    std::cout << "  SEMANTIC vs CAUSAL vs ATTRIBUTE:\n\n";
    
    std::cout << "  Example concept: FIRE\n\n";
    
    std::cout << "    SEMANTIC:   fire → smoke\n";
    std::cout << "      (smoke is semantically related to fire)\n\n";
    
    std::cout << "    CAUSAL:     fire → heat\n";
    std::cout << "      (fire CAUSES/PRODUCES heat)\n\n";
    
    std::cout << "    ATTRIBUTE:  plants → water\n";
    std::cout << "      (plants NEED water as a property/requirement)\n\n";
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  WHY TYPES MATTER\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    std::cout << "  1. DIFFERENT REASONING PATTERNS:\n";
    std::cout << "     CAUSAL → Predict consequences (\"What happens if?\")\n";
    std::cout << "     ATTRIBUTE → Find requirements (\"What does X need?\")\n";
    std::cout << "     SEMANTIC → Explore associations (\"What's related?\")\n\n";
    
    std::cout << "  2. WEIGHTED DIFFERENTLY:\n";
    std::cout << "     CAUSAL gets higher weight (stronger inference)\n";
    std::cout << "     ATTRIBUTE moderate weight\n";
    std::cout << "     SEMANTIC lower weight (looser connection)\n\n";
    
    std::cout << "  3. DIFFERENT LEARNING:\n";
    std::cout << "     CAUSAL reinforced when predictions work\n";
    std::cout << "     ATTRIBUTE reinforced when requirements met\n";
    std::cout << "     SEMANTIC reinforced by co-occurrence\n\n";
    
    std::cout << "✅ Relation types enable SMART reasoning!\n\n";
    
    return 0;
}

