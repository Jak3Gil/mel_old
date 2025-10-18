/*
 * Initialize knowledge base in AtomicGraph format for melvin_unified
 */

#include "melvin/core/atomic_graph.h"
#include <iostream>

using namespace melvin;

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  INITIALIZING KNOWLEDGE FOR UNIFIED MELVIN            ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    AtomicGraph graph;
    
    std::cout << "Creating base knowledge concepts...\n\n";
    
    // Node type: 0=SENSORY, 1=CONCEPT
    const uint8_t CONCEPT = 1;
    
    // Relation types from atomic_graph.h:
    // 0=INSTANCE_OF, 1=CO_OCCURS_WITH, 2=OBSERVED_AS, 3=NEAR, 4=TEMPORAL_NEXT, 5=NAMED
    const uint8_t RELATED = INSTANCE_OF;      // for general relationships
    const uint8_t TOGETHER = CO_OCCURS_WITH;   // for co-occurrence
    
    // Basic physics concepts
    auto fire = graph.get_or_create_node("fire", CONCEPT);
    auto heat = graph.get_or_create_node("heat", CONCEPT);
    auto light = graph.get_or_create_node("light", CONCEPT);
    auto energy = graph.get_or_create_node("energy", CONCEPT);
    auto warmth = graph.get_or_create_node("warmth", CONCEPT);
    
    graph.add_edge(fire, heat, RELATED, 1.0f);
    graph.add_edge(fire, light, RELATED, 1.0f);
    graph.add_edge(heat, warmth, RELATED, 1.0f);
    graph.add_edge(heat, energy, RELATED, 1.0f);
    
    std::cout << "  ✓ fire → heat, light\n";
    std::cout << "  ✓ heat → warmth, energy\n\n";
    
    // Water concepts
    auto water = graph.get_or_create_node("water", CONCEPT);
    auto liquid = graph.get_or_create_node("liquid", CONCEPT);
    auto vapor = graph.get_or_create_node("vapor", CONCEPT);
    auto clouds = graph.get_or_create_node("clouds", CONCEPT);
    auto rain = graph.get_or_create_node("rain", CONCEPT);
    
    graph.add_edge(water, liquid, RELATED, 1.0f);
    graph.add_edge(water, vapor, RELATED, 1.0f);
    graph.add_edge(vapor, clouds, RELATED, 1.0f);
    graph.add_edge(clouds, rain, RELATED, 1.0f);
    
    std::cout << "  ✓ water → liquid, vapor\n";
    std::cout << "  ✓ vapor → clouds → rain\n\n";
    
    // Life concepts
    auto sun = graph.get_or_create_node("sun", CONCEPT);
    auto plants = graph.get_or_create_node("plants", CONCEPT);
    auto oxygen = graph.get_or_create_node("oxygen", CONCEPT);
    auto animals = graph.get_or_create_node("animals", CONCEPT);
    auto life = graph.get_or_create_node("life", CONCEPT);
    
    graph.add_edge(sun, light, RELATED, 1.0f);
    graph.add_edge(sun, heat, RELATED, 1.0f);
    graph.add_edge(plants, oxygen, RELATED, 1.0f);
    graph.add_edge(oxygen, animals, RELATED, 1.0f);
    graph.add_edge(oxygen, life, RELATED, 1.0f);
    graph.add_edge(water, life, RELATED, 1.0f);
    
    std::cout << "  ✓ sun → light, heat\n";
    std::cout << "  ✓ plants → oxygen → animals\n";
    std::cout << "  ✓ oxygen, water → life\n\n";
    
    // Common objects (will link with vision)
    auto person = graph.get_or_create_node("person", CONCEPT);
    auto chair = graph.get_or_create_node("chair", CONCEPT);
    auto table = graph.get_or_create_node("table", CONCEPT);
    auto dog = graph.get_or_create_node("dog", CONCEPT);
    auto cat = graph.get_or_create_node("cat", CONCEPT);
    
    graph.add_edge(person, animals, RELATED, 1.0f);
    graph.add_edge(dog, animals, RELATED, 1.0f);
    graph.add_edge(cat, animals, RELATED, 1.0f);
    
    std::cout << "  ✓ person, dog, cat are animals\n\n";
    
    // Actions
    auto see = graph.get_or_create_node("see", CONCEPT);
    auto hear = graph.get_or_create_node("hear", CONCEPT);
    auto think = graph.get_or_create_node("think", CONCEPT);
    
    graph.add_edge(light, see, RELATED, 1.0f);
    graph.add_edge(see, think, RELATED, 1.0f);
    
    std::cout << "  ✓ light → see → think\n\n";
    
    // Save
    graph.save("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
    
    std::cout << "✅ Saved knowledge base:\n";
    std::cout << "   Nodes: " << graph.node_count() << "\n";
    std::cout << "   Edges: " << graph.edge_count() << "\n\n";
    
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  READY FOR MELVIN UNIFIED!                            ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Now run: ./melvin_unified\n\n";
    std::cout << "Melvin will:\n";
    std::cout << "  • Start with these 26 base concepts\n";
    std::cout << "  • Add vision percepts (person, chair, etc.)\n";
    std::cout << "  • Link visual concepts to base knowledge\n";
    std::cout << "  • Learn and grow continuously!\n\n";
    
    return 0;
}

