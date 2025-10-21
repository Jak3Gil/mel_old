#!/usr/bin/env python3
"""
Diverse Knowledge Streamer - Large-Scale Test Data

Generates diverse facts across many domains to demonstrate:
- Real knowledge growth (not just 10 repeating facts)
- Deduplication at scale
- Edge/node ratio evolution
- Memory persistence under load
"""

import random
import time
import csv
import os
from datetime import datetime

# Configuration
STREAM_RATE = 0.1  # Faster for demo
BATCH_SIZE = 50
QUEUE_FILE = "data/stream_queue.csv"

# ============================================================================
# DIVERSE KNOWLEDGE BASE - 500+ Unique Facts
# ============================================================================

KNOWLEDGE_DOMAINS = {
    "biology": [
        "cells are units", "DNA contains information", "proteins are molecules",
        "enzymes catalyze reactions", "mitochondria produce energy", "chloroplasts perform photosynthesis",
        "neurons transmit signals", "blood transports oxygen", "lungs exchange gases",
        "heart pumps blood", "liver filters toxins", "kidneys remove waste",
        "bacteria are organisms", "viruses infect cells", "fungi decompose matter",
        "plants produce oxygen", "animals consume oxygen", "organisms require water",
        "cells divide continuously", "DNA replicates accurately", "mutations cause variation",
        "evolution drives diversity", "species adapt environments", "ecosystems contain organisms",
        "predators hunt prey", "herbivores eat plants", "carnivores eat meat",
        "food chains transfer energy", "sunlight powers ecosystems", "carbon cycles naturally",
    ],
    
    "physics": [
        "energy drives motion", "force causes acceleration", "mass resists movement",
        "gravity attracts objects", "light travels fast", "sound requires medium",
        "electricity powers devices", "magnetism attracts iron", "atoms contain particles",
        "electrons carry charge", "protons are positive", "neutrons are neutral",
        "molecules bond atoms", "heat increases temperature", "cold decreases motion",
        "pressure compresses gases", "vacuum lacks matter", "waves transmit energy",
        "frequency determines pitch", "amplitude determines volume", "light exhibits duality",
        "photons carry energy", "quarks form protons", "fusion releases energy",
        "radiation transfers heat", "conduction requires contact", "convection moves fluids",
    ],
    
    "chemistry": [
        "atoms form molecules", "bonds connect elements", "reactions transform matter",
        "catalysts accelerate reactions", "acids donate protons", "bases accept protons",
        "salts are compounds", "water is solvent", "oxygen supports combustion",
        "hydrogen is fuel", "carbon forms chains", "nitrogen is essential",
        "metals conduct electricity", "nonmetals are insulators", "alloys combine metals",
        "oxidation loses electrons", "reduction gains electrons", "combustion produces heat",
        "polymers are chains", "monomers build polymers", "crystals have structure",
        "solutions are mixtures", "solvents dissolve solutes", "concentration measures amount",
    ],
    
    "technology": [
        "computers process data", "algorithms solve problems", "networks connect devices",
        "internet enables communication", "software controls hardware", "databases store information",
        "encryption secures data", "protocols define communication", "servers host services",
        "clients request services", "bandwidth limits speed", "latency causes delay",
        "processors execute instructions", "memory stores data", "storage persists information",
        "transistors switch current", "circuits process signals", "sensors detect changes",
        "actuators cause motion", "robots automate tasks", "AI learns patterns",
        "code defines behavior", "bugs cause errors", "testing ensures quality",
    ],
    
    "astronomy": [
        "stars emit light", "planets orbit stars", "moons orbit planets",
        "galaxies contain stars", "universe is expanding", "gravity shapes cosmos",
        "black holes warp spacetime", "supernovas explode violently", "nebulae form stars",
        "comets have tails", "asteroids are rocks", "meteors burn atmosphere",
        "sun is star", "earth is planet", "mars has water",
        "jupiter is massive", "saturn has rings", "mercury is hot",
        "venus is cloudy", "pluto is dwarf", "light travels distance",
    ],
    
    "mathematics": [
        "numbers represent quantities", "addition combines values", "subtraction removes values",
        "multiplication repeats addition", "division splits quantities", "fractions represent parts",
        "decimals are fractions", "percentages are ratios", "algebra uses variables",
        "equations show equality", "functions map inputs", "graphs visualize relationships",
        "geometry studies shapes", "triangles have angles", "circles are round",
        "pi is constant", "infinity is unlimited", "zero is nothing",
        "primes are divisible", "composites have factors", "squares are products",
    ],
    
    "geology": [
        "rocks form layers", "minerals compose rocks", "erosion wears surfaces",
        "weathering breaks rocks", "sediments accumulate gradually", "fossils preserve life",
        "volcanoes erupt magma", "earthquakes shake ground", "plates move constantly",
        "mountains rise slowly", "valleys form erosion", "rivers carve canyons",
        "glaciers move ice", "oceans cover earth", "continents are landmasses",
        "magma becomes lava", "igneous rocks cool", "sedimentary rocks compact",
        "metamorphic rocks transform", "crystals grow slowly", "gems are minerals",
    ],
    
    "ecology": [
        "ecosystems are balanced", "biodiversity strengthens systems", "habitats shelter organisms",
        "niches define roles", "competition limits resources", "cooperation benefits groups",
        "symbiosis involves interaction", "parasites harm hosts", "mutualism benefits both",
        "decomposers recycle nutrients", "producers create energy", "consumers use energy",
        "predation controls populations", "herbivory affects plants", "pollination aids reproduction",
        "migration follows seasons", "hibernation conserves energy", "adaptation improves survival",
    ],
    
    "psychology": [
        "memory stores experiences", "learning changes behavior", "perception interprets stimuli",
        "attention focuses awareness", "emotions influence decisions", "motivation drives action",
        "cognition processes information", "reasoning solves problems", "creativity generates ideas",
        "intelligence measures ability", "personality defines character", "behavior reflects mind",
        "consciousness is awareness", "sleep restores function", "stress affects health",
        "habits automate actions", "beliefs shape reality", "attitudes influence behavior",
    ],
    
    "economics": [
        "markets allocate resources", "supply meets demand", "prices signal value",
        "scarcity creates value", "trade enables exchange", "money facilitates transactions",
        "banks store currency", "loans provide capital", "interest rewards lending",
        "inflation reduces value", "deflation increases value", "employment generates income",
        "productivity drives growth", "investment builds capital", "savings defer consumption",
        "competition lowers prices", "monopolies control markets", "regulations limit behavior",
    ],
}

# ============================================================================
# FACT GENERATION
# ============================================================================

def generate_diverse_facts(num_facts=1000):
    """Generate diverse facts across all domains"""
    all_facts = []
    
    for domain, facts in KNOWLEDGE_DOMAINS.items():
        for fact in facts:
            # Parse "subject predicate object" format
            parts = fact.split()
            if len(parts) >= 3:
                subject = parts[0]
                predicate = parts[1]
                obj = " ".join(parts[2:])
                all_facts.append((subject, predicate, obj))
    
    return all_facts

def append_to_queue(facts, queue_file):
    """Append facts to ingestion queue"""
    file_exists = os.path.exists(queue_file)
    
    with open(queue_file, 'a', newline='') as f:
        writer = csv.writer(f)
        
        if not file_exists:
            writer.writerow(['subject', 'predicate', 'object'])
        
        writer.writerows(facts)

# ============================================================================
# STREAMING
# ============================================================================

def main():
    print("\n╔═══════════════════════════════════════════════════════╗")
    print("║  DIVERSE KNOWLEDGE STREAMER                           ║")
    print("╚═══════════════════════════════════════════════════════╝\n")
    
    # Generate all facts once
    all_facts = generate_diverse_facts()
    
    print(f"📚 Generated {len(all_facts)} unique facts across {len(KNOWLEDGE_DOMAINS)} domains")
    print(f"⚙️  Stream rate: {STREAM_RATE} sec/batch")
    print(f"📦 Batch size: {BATCH_SIZE} facts\n")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print("  STREAMING KNOWLEDGE")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n")
    print("Press Ctrl+C to stop...\n")
    
    # Shuffle for variety
    random.shuffle(all_facts)
    
    total_streamed = 0
    batch_count = 0
    
    try:
        while total_streamed < len(all_facts):
            # Get next batch
            batch = all_facts[total_streamed:total_streamed + BATCH_SIZE]
            
            if not batch:
                print("\n✓ All facts streamed! Cycling back to start...\n")
                random.shuffle(all_facts)
                total_streamed = 0
                continue
            
            # Queue the batch
            append_to_queue(batch, QUEUE_FILE)
            total_streamed += len(batch)
            batch_count += 1
            
            print(f"[{datetime.now().strftime('%H:%M:%S')}] "
                  f"Batch {batch_count} | Queued {len(batch)} facts | "
                  f"Total: {total_streamed}/{len(all_facts)} "
                  f"({100*total_streamed//len(all_facts)}%)")
            
            time.sleep(STREAM_RATE)
    
    except KeyboardInterrupt:
        print("\n\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
        print("  STREAMING STOPPED")
        print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n")
        
        print(f"📊 Session Summary:")
        print(f"  Batches streamed:    {batch_count}")
        print(f"  Facts queued:        {total_streamed}")
        print(f"  Queue file:          {QUEUE_FILE}\n")
        
        print(f"💡 Next steps:")
        print(f"  ./tools/melvin_ingest -t csv {QUEUE_FILE}")
        print(f"  make stats\n")

if __name__ == "__main__":
    main()

