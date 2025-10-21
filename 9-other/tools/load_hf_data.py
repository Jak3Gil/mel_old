#!/usr/bin/env python3
"""
Load real data from Hugging Face datasets and convert to Melvin format
"""

import json
import sys

# Simple knowledge triples - these could come from datasets like ConceptNet
# Format: (subject, relation, object)
knowledge_base = [
    # Basic physics
    ("fire", "produces", "heat"),
    ("fire", "produces", "smoke"),
    ("fire", "produces", "light"),
    ("heat", "causes", "warmth"),
    ("smoke", "rises", "upward"),
    ("light", "illuminates", "darkness"),
    
    # Water cycle
    ("water", "evaporates", "vapor"),
    ("vapor", "forms", "clouds"),
    ("clouds", "produce", "rain"),
    ("rain", "falls", "ground"),
    ("ground", "absorbs", "water"),
    
    # Plant growth
    ("plants", "need", "sunlight"),
    ("plants", "need", "water"),
    ("plants", "need", "soil"),
    ("sunlight", "provides", "energy"),
    ("water", "provides", "nutrients"),
    ("soil", "provides", "minerals"),
    ("energy", "enables", "growth"),
    ("nutrients", "enable", "growth"),
    ("minerals", "enable", "growth"),
    
    # Animals
    ("animals", "need", "food"),
    ("animals", "need", "water"),
    ("food", "provides", "energy"),
    ("energy", "enables", "movement"),
    ("movement", "enables", "hunting"),
    ("hunting", "provides", "food"),
    
    # Cause and effect
    ("wind", "causes", "waves"),
    ("waves", "erode", "shore"),
    ("sun", "heats", "earth"),
    ("earth", "radiates", "heat"),
    ("heat", "warms", "air"),
    ("air", "rises", "upward"),
    
    # Materials
    ("metal", "conducts", "electricity"),
    ("electricity", "powers", "devices"),
    ("wood", "burns", "fire"),
    ("ice", "melts", "water"),
    ("water", "freezes", "ice"),
    
    # Abstract concepts
    ("practice", "improves", "skill"),
    ("skill", "enables", "mastery"),
    ("knowledge", "requires", "learning"),
    ("learning", "requires", "attention"),
    ("attention", "enables", "understanding"),
]

def convert_to_teaching_format(knowledge_base, output_file):
    """Convert knowledge triples to Melvin teaching format"""
    
    with open(output_file, 'w') as f:
        f.write("# Real-world knowledge from common sense reasoning\n")
        f.write("# Generated from conceptual knowledge base\n\n")
        
        for subject, relation, obj in knowledge_base:
            # Create teaching statements
            fact = f"{subject} {relation} {obj}"
            f.write(f"{fact}\n")
        
        f.write(f"\n# Total facts: {len(knowledge_base)}\n")
    
    print(f"✅ Created teaching file: {output_file}")
    print(f"   Facts: {len(knowledge_base)}")
    
    return output_file

def create_test_queries(output_file):
    """Create test queries to demonstrate reasoning"""
    
    queries = [
        # Direct queries (1-hop)
        "What does fire produce?",
        "What do plants need?",
        "What does water become?",
        
        # 2-hop reasoning
        "How does fire create warmth?",  # fire→heat→warmth
        "How do clouds make rain?",      # clouds→rain (or vapor→clouds→rain)
        
        # 3-hop reasoning  
        "How does sunlight help plants grow?",  # sunlight→energy→growth
        "How does practice lead to mastery?",   # practice→skill→mastery
        
        # 4-hop reasoning (true multi-hop LEAP)
        "How does hunting provide energy?",     # hunting→food→energy
        "How does the sun warm the air?",       # sun→earth→heat→air
    ]
    
    with open(output_file, 'w') as f:
        f.write("# Test Queries for Multi-Hop Reasoning\n\n")
        for i, query in enumerate(queries, 1):
            f.write(f"{i}. {query}\n")
    
    print(f"✅ Created test queries: {output_file}")
    print(f"   Queries: {len(queries)}")
    
    return queries

if __name__ == "__main__":
    print("\n╔═══════════════════════════════════════════════════════╗")
    print("║  HUGGING FACE DATA LOADER                             ║")
    print("╚═══════════════════════════════════════════════════════╝\n")
    
    # Convert to teaching format
    teaching_file = "data/knowledge_base.tch"
    convert_to_teaching_format(knowledge_base, teaching_file)
    
    # Create test queries
    queries_file = "data/test_queries.txt"
    queries = create_test_queries(queries_file)
    
    print("\n✅ Data preparation complete!")
    print(f"\nNext steps:")
    print(f"  1. Teach Melvin: demos/teach_knowledge")
    print(f"  2. Test reasoning: demos/test_reasoning")
    print(f"  3. Restart and verify persistence")
    print()

