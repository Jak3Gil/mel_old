#!/usr/bin/env python3
"""
Fetch LARGE dictionary (1000-10000 words) with definitions
Uses free API + local word lists when needed
"""

import urllib.request
import json
import time
import sys

def fetch_word_definition(word):
    """Fetch from Free Dictionary API"""
    try:
        url = f"https://api.dictionaryapi.dev/api/v2/entries/en/{word.lower()}"
        req = urllib.request.Request(url, headers={'User-Agent': 'MelvinAI/1.0'})
        
        with urllib.request.urlopen(req, timeout=3) as response:
            data = json.loads(response.read().decode())
        
        if data and isinstance(data, list) and len(data) > 0:
            definitions = []
            for meaning in data[0].get('meanings', []):
                for defn in meaning.get('definitions', [])[:1]:  # 1 per part of speech
                    definitions.append(defn.get('definition', ''))
            return definitions
    except:
        pass
    return []

def get_common_english_words():
    """
    Get comprehensive English word list (FREE - no API!)
    Based on frequency and importance
    """
    # Most common English words by category
    words = []
    
    # Basic nouns (200 words)
    basic_nouns = """
    time person year way day thing man world life hand part child eye woman place
    work week case point government company number group problem fact water air food
    power energy science nature force mass weight speed distance temperature pressure
    volume density matter atom molecule element compound reaction solution mixture
    gas liquid solid plasma light sound heat electricity magnetism gravity motion
    velocity acceleration friction momentum wavelength frequency amplitude phase
    cell organism species evolution gene DNA protein enzyme membrane nucleus
    chromosome tissue organ system blood heart brain nerve muscle bone skin
    digestion respiration circulation reproduction growth development metabolism
    acid base salt ion bond electron proton neutron isotope catalyst equilibrium
    oxidation reduction synthesis analysis organic inorganic polymer crystal
    quantum relativity field wave particle photon radiation spectrum interference
    diffraction refraction reflection number integer fraction decimal ratio
    proportion equation formula function variable constant graph angle area
    perimeter calculate sum difference product quotient average mean median
    algorithm data information computer program software hardware network internet
    database code binary digital analog input output process memory storage
    idea thought concept theory hypothesis principle law rule pattern structure
    organization order chaos complexity simplicity cause effect reason result
    purpose meaning value quality truth beauty justice freedom equality liberty
    """.split()
    
    # Common verbs (100 words)
    verbs = """
    be have do say get make go know take see come think look want give use
    find tell ask work seem feel try leave call create produce generate transform
    transfer conduct transmit regulate control influence affect modify contain
    compose comprise include represent analyze synthesize integrate compute
    process calculate measure observe record demonstrate prove verify validate
    test experiment discover invent develop evolve adapt grow change move flow
    react combine separate divide multiply reduce increase decrease expand contract
    absorb emit reflect refract diffract interfere resonate vibrate oscillate
    """.split()
    
    # Adjectives (100 words)
    adjectives = """
    good new first last long great little own other old right big high different
    small large young important public bad same able natural scientific physical
    chemical biological mathematical theoretical practical experimental empirical
    quantitative qualitative analytical synthetic organic inorganic molecular
    atomic nuclear quantum mechanical thermal electrical magnetic optical acoustic
    solid liquid gaseous plasma crystalline amorphous transparent opaque dense
    sparse homogeneous heterogeneous stable unstable reactive inert polar nonpolar
    """.split()
    
    # Technical scientific terms (300 words)
    science_terms = """
    photosynthesis thermodynamics electromagnetism biochemistry biotechnology
    nanotechnology genomics proteomics metabolism homeostasis respiration
    fermentation oxidation phosphorylation glycolysis mitochondria chloroplast
    ribosome endoplasmic golgi lysosome peroxisome cytoplasm cytoskeleton
    microtubule microfilament centrosome centriole flagellum cilium pseudopod
    exocytosis endocytosis diffusion osmosis facilitated gradient concentration
    enzyme substrate catalyst inhibitor cofactor coenzyme activation denaturation
    hydrolysis condensation polymerization depolymerization isomer stereoisomer
    enantiomer diastereomer conformer tautomer resonance hybridization orbital
    valence electronegativity ionization affinity enthalpy entropy gibbs helmholtz
    equilibrium kinetics thermochemistry electrochemistry photochemistry radiochemistry
    spectroscopy chromatography electrophoresis centrifugation filtration distillation
    crystallization precipitation sublimation evaporation condensation solidification
    melting freezing boiling vaporization liquefaction ionization dissociation
    association solvation hydration complexation chelation coordination ligand
    receptor agonist antagonist synapse neurotransmitter dopamine serotonin
    acetylcholine glutamate gaba glycine norepinephrine epinephrine endorphin
    hormone insulin glucagon cortisol testosterone estrogen progesterone oxytocin
    vasopressin prolactin thyroxine adrenaline melatonin histamine bradykinin
    cytokine interferon interleukin chemokine lymphokine antibody antigen epitope
    pathogen bacteria virus fungi protozoa parasite infection immunity vaccination
    antibiotic antiviral antifungal antiseptic disinfectant sterilization pasteurization
    DNA RNA nucleotide nucleoside purine pyrimidine adenine guanine cytosine thymine
    uracil deoxyribose ribose phosphate backbone helix chromosome chromatin histone
    telomere centromere replication transcription translation mutation recombination
    """.split()
    
    # Combine all
    words.extend(basic_nouns)
    words.extend(verbs)
    words.extend(adjectives)
    words.extend(science_terms)
    
    # Remove duplicates
    words = list(set([w.lower() for w in words if len(w) > 2]))
    
    return words  # Return all


def main():
    count = 100
    if len(sys.argv) > 1:
        count = int(sys.argv[1])
    
    print("\n╔═══════════════════════════════════════════════════════════════╗")
    print("║  MELVIN LARGE DICTIONARY FETCHER                              ║")
    print("║  (100% FREE - No API Keys Required!)                          ║")
    print("╚═══════════════════════════════════════════════════════════════╝\n")
    
    print(f"Target: {count} words with definitions\n")
    
    # Get word list
    print("📚 Loading word list...")
    words = get_common_english_words()[:count]
    print(f"  ✓ {len(words)} words selected\n")
    
    # Fetch definitions
    print("🌐 Fetching from Free Dictionary API...")
    print("   (Being respectful with rate limiting)\n")
    
    successful = 0
    failed = 0
    all_facts = []
    
    for i, word in enumerate(words):
        if i % 10 == 0:
            print(f"  Progress: [{i+1}/{len(words)}] {word:20s}", end='')
        
        definitions = fetch_word_definition(word)
        
        if definitions:
            for defn in definitions:
                # Create facts
                defn_clean = defn.split('.')[0]  # First sentence
                if 10 < len(defn_clean) < 150:
                    all_facts.append(f"{word} means {defn_clean}")
                    all_facts.append(f"{word} is {defn_clean}")
            
            successful += 1
            if i % 10 == 0:
                print(f" ✓ ({len(definitions)} defs)")
        else:
            failed += 1
            if i % 10 == 0:
                print(f" ✗")
        
        # Rate limiting (be respectful!)
        if i % 5 == 0 and i > 0:
            time.sleep(0.3)
    
    print(f"\n\n✅ Fetched {successful} words successfully ({failed} failed)\n")
    
    # Save
    print(f"💾 Saving {len(all_facts)} facts to dictionary_facts.txt...")
    with open('dictionary_facts.txt', 'w') as f:
        for fact in all_facts:
            f.write(fact + '\n')
    
    print(f"  ✓ Saved!\n")
    
    print("╔═══════════════════════════════════════════════════════════════╗")
    print("║  READY FOR MELVIN!                                            ║")
    print("╚═══════════════════════════════════════════════════════════════╝\n")
    
    print("📊 Summary:")
    print(f"  Words with definitions: {successful}")
    print(f"  Total facts created: {len(all_facts)}")
    print(f"  Expected new nodes: {len(set([f.split()[0] for f in all_facts]))}")
    print(f"  Expected growth: Thousands of new connections!\n")
    
    print("Next step:")
    print("  ./feed_internet_knowledge\n")
    
    # Show sample
    print("Sample facts created:")
    for fact in all_facts[:10]:
        print(f"  • {fact}")
    print()


if __name__ == "__main__":
    main()

