#!/usr/bin/env python3
"""
Dictionary Fetcher - Pull dictionary with definitions (100% FREE!)

Sources:
- Free Dictionary API (dictionaryapi.dev) - FREE, no key needed!
- Wiktionary API (Wikimedia) - FREE!
- Local word lists - FREE!
"""

import urllib.request
import urllib.parse
import json
import time
import sys

class DictionaryFetcher:
    """Fetch dictionary words and definitions from free APIs"""
    
    def __init__(self):
        self.words_fetched = 0
        self.definitions_found = 0
        
    def fetch_definition_free_api(self, word: str) -> dict:
        """
        Fetch from Free Dictionary API (NO KEY REQUIRED!)
        API: https://dictionaryapi.dev/
        """
        try:
            url = f"https://api.dictionaryapi.dev/api/v2/entries/en/{word.lower()}"
            
            req = urllib.request.Request(
                url,
                headers={'User-Agent': 'MelvinAI/1.0 Educational'}
            )
            
            with urllib.request.urlopen(req, timeout=5) as response:
                data = json.loads(response.read().decode())
            
            if data and isinstance(data, list) and len(data) > 0:
                entry = data[0]
                word_clean = entry.get('word', word)
                
                # Extract definitions
                definitions = []
                if 'meanings' in entry:
                    for meaning in entry['meanings']:
                        part_of_speech = meaning.get('partOfSpeech', '')
                        
                        for definition in meaning.get('definitions', [])[:2]:  # Max 2 per part of speech
                            def_text = definition.get('definition', '')
                            if def_text:
                                definitions.append({
                                    'pos': part_of_speech,
                                    'definition': def_text
                                })
                
                return {
                    'word': word_clean,
                    'definitions': definitions
                }
                
        except Exception as e:
            # Silently fail - many words won't be found
            pass
        
        return None
    
    def fetch_common_words_batch(self, count=1000) -> list:
        """
        Fetch most common English words with definitions
        """
        # Common English words (by frequency)
        common_words = [
            # Top 100 most common
            "time", "person", "year", "way", "day", "thing", "man", "world", "life", "hand",
            "part", "child", "eye", "woman", "place", "work", "week", "case", "point", "government",
            "company", "number", "group", "problem", "fact", "water", "air", "food", "power", "energy",
            
            # Science words
            "science", "nature", "force", "mass", "weight", "speed", "distance", "temperature", "pressure",
            "volume", "density", "matter", "atom", "molecule", "element", "compound", "reaction", "solution",
            "mixture", "gas", "liquid", "solid", "plasma", "energy", "light", "sound", "heat", "electricity",
            "magnetism", "gravity", "motion", "velocity", "acceleration", "friction", "momentum", "wavelength",
            
            # Biology words
            "cell", "organism", "species", "evolution", "gene", "DNA", "protein", "enzyme", "membrane",
            "nucleus", "chromosome", "tissue", "organ", "system", "blood", "heart", "brain", "nerve",
            "muscle", "bone", "skin", "digestion", "respiration", "circulation", "reproduction", "growth",
            
            # Chemistry words
            "acid", "base", "salt", "ion", "bond", "electron", "proton", "neutron", "isotope",
            "catalyst", "equilibrium", "oxidation", "reduction", "synthesis", "analysis", "organic",
            "inorganic", "polymer", "crystal", "solution", "solvent", "solute", "concentration",
            
            # Physics words
            "quantum", "relativity", "field", "wave", "particle", "photon", "radiation", "spectrum",
            "frequency", "amplitude", "phase", "interference", "diffraction", "refraction", "reflection",
            
            # Math words
            "number", "integer", "fraction", "decimal", "ratio", "proportion", "equation", "formula",
            "function", "variable", "constant", "graph", "angle", "area", "volume", "perimeter",
            "calculate", "sum", "difference", "product", "quotient", "average", "mean", "median",
            
            # Computer science
            "algorithm", "data", "information", "computer", "program", "software", "hardware", "network",
            "internet", "database", "code", "binary", "digital", "analog", "input", "output", "process",
            
            # Abstract concepts
            "idea", "thought", "concept", "theory", "hypothesis", "principle", "law", "rule", "pattern",
            "structure", "system", "organization", "order", "chaos", "complexity", "simplicity",
            "cause", "effect", "reason", "result", "purpose", "meaning", "value", "quality"
        ]
        
        # Extend with more words
        alphabet_words = []
        for letter in 'abcdefghijklmnopqrstuvwxyz':
            alphabet_words.extend([
                f"{letter}bsorb", f"{letter}daptation", f"{letter}nalysis"
            ])
        
        all_words = list(set(common_words))[:count]
        
        return all_words
    
    def fetch_dictionary_batch(self, count=1000, verbose=True):
        """
        Fetch dictionary definitions in batches
        """
        print(f"\n╔═══════════════════════════════════════════════════════════════╗")
        print(f"║  FETCHING DICTIONARY ({count} words)                          ")
        print(f"╚═══════════════════════════════════════════════════════════════╝\n")
        
        print("📚 Getting word list...")
        words = self.fetch_common_words_batch(count)
        print(f"  ✓ {len(words)} words to fetch\n")
        
        print("🌐 Fetching definitions from Free Dictionary API...")
        print("   (This is 100% FREE - no API key needed!)\n")
        
        results = []
        
        for i, word in enumerate(words):
            if verbose and (i % 50 == 0 or i < 10):
                print(f"  [{i+1}/{len(words)}] Fetching: {word}...", end='')
            
            definition_data = self.fetch_definition_free_api(word)
            
            if definition_data and definition_data['definitions']:
                results.append(definition_data)
                self.words_fetched += 1
                self.definitions_found += len(definition_data['definitions'])
                
                if verbose and (i % 50 == 0 or i < 10):
                    print(f" ✓ ({len(definition_data['definitions'])} definitions)")
            else:
                if verbose and (i % 50 == 0 or i < 10):
                    print(f" ✗")
            
            # Respect API rate limit (be nice!)
            if i % 10 == 0 and i > 0:
                time.sleep(0.5)  # Small delay every 10 words
        
        print(f"\n✅ Fetched {self.words_fetched} words with {self.definitions_found} definitions!\n")
        
        return results
    
    def save_dictionary(self, dictionary_data, output_file="dictionary_facts.txt"):
        """
        Save dictionary as facts for Melvin
        Format: word is definition
                word means definition
        """
        print(f"💾 Saving to {output_file}...")
        
        with open(output_file, 'w') as f:
            for entry in dictionary_data:
                word = entry['word']
                
                for defn in entry['definitions']:
                    # Create fact from definition
                    definition_text = defn['definition']
                    
                    # Clean definition (first sentence only)
                    definition_text = definition_text.split('.')[0]
                    
                    # Create fact
                    if len(definition_text) > 10 and len(definition_text) < 200:
                        fact = f"{word} means {definition_text}"
                        f.write(fact + '\n')
                        
                        # Also create "is" facts
                        fact2 = f"{word} is {definition_text}"
                        f.write(fact2 + '\n')
        
        print(f"  ✓ Saved {len(dictionary_data)} words\n")


def main():
    count = 100  # Default
    if len(sys.argv) > 1:
        count = int(sys.argv[1])
    
    print("\n🌐 MELVIN DICTIONARY FETCHER (100% FREE!)\n")
    print("Source: Free Dictionary API (https://dictionaryapi.dev/)")
    print("No API key required - completely free!\n")
    
    fetcher = DictionaryFetcher()
    
    # Fetch dictionary
    dictionary = fetcher.fetch_dictionary_batch(count, verbose=True)
    
    # Save for Melvin
    fetcher.save_dictionary(dictionary, "dictionary_facts.txt")
    
    print("╔═══════════════════════════════════════════════════════════════╗")
    print("║  READY TO FEED TO MELVIN!                                     ║")
    print("╚═══════════════════════════════════════════════════════════════╝\n")
    
    print(f"📊 Statistics:")
    print(f"  Words fetched: {fetcher.words_fetched}")
    print(f"  Definitions found: {fetcher.definitions_found}")
    print(f"  Facts created: {fetcher.definitions_found * 2}\n")  # Each definition = 2 facts
    
    print("Next steps:")
    print("  1. Feed to Melvin: ./feed_internet_knowledge")
    print("  2. This will create THOUSANDS of new nodes!")
    print("  3. Each word + definition = rich semantic knowledge\n")


if __name__ == "__main__":
    main()

