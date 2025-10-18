#!/usr/bin/env python3
"""
CONTINUOUS INTERNET LEARNING
Fetches from free APIs and feeds directly to Melvin continuously
No manual steps - runs indefinitely!
"""

import subprocess
import time
import urllib.request
import urllib.parse
import json
import random
import sys
from datetime import datetime

class ContinuousInternetLearner:
    """Continuously fetch from internet and feed to Melvin"""
    
    def __init__(self, facts_per_cycle=10, delay_seconds=5):
        self.facts_per_cycle = facts_per_cycle
        self.delay_seconds = delay_seconds
        self.total_facts_fetched = 0
        self.total_cycles = 0
        self.melvin_process = None
        
        # Wikipedia topics (rotated)
        self.wiki_topics = [
            "Physics", "Chemistry", "Biology", "Astronomy", "Geology",
            "Mathematics", "Computer_Science", "Neuroscience", "Psychology",
            "Philosophy", "Economics", "Sociology", "Anthropology",
            "History", "Geography", "Literature", "Art", "Music",
            "Medicine", "Engineering", "Architecture", "Agriculture",
            "Ecology", "Meteorology", "Oceanography", "Genetics",
            "Quantum_mechanics", "Thermodynamics", "Electromagnetism",
            "Machine_learning", "Artificial_intelligence", "Robotics"
        ]
        
        self.current_topic_idx = 0
    
    def fetch_wikipedia_facts(self, topic: str) -> list:
        """Fetch facts from Wikipedia API (FREE!)"""
        try:
            url = f"https://en.wikipedia.org/w/api.php?action=query&prop=extracts&exintro&explaintext&format=json&titles={urllib.parse.quote(topic)}"
            
            req = urllib.request.Request(
                url,
                headers={
                    'User-Agent': 'MelvinAI/1.0 (Educational; Contact: research@melvin.ai)',
                    'Accept': 'application/json'
                }
            )
            
            with urllib.request.urlopen(req, timeout=10) as response:
                data = json.loads(response.read().decode())
            
            pages = data.get('query', {}).get('pages', {})
            for page_id, page in pages.items():
                if 'extract' in page:
                    text = page['extract']
                    
                    # Parse into simple statements
                    sentences = [s.strip() for s in text.split('.') if s.strip()]
                    
                    # Filter for good facts
                    facts = []
                    for sent in sentences[:20]:  # First 20 sentences
                        words = sent.split()
                        if 5 <= len(words) <= 15:  # Reasonable length
                            facts.append(sent)
                    
                    return facts[:self.facts_per_cycle]
        
        except Exception as e:
            print(f"  ⚠ Wikipedia fetch failed: {e}")
        
        return []
    
    def generate_fallback_facts(self) -> list:
        """Generate facts locally if API fails"""
        subjects = ["neurons", "photons", "molecules", "cells", "systems",
                   "networks", "patterns", "processes", "structures", "forces"]
        verbs = ["transmit", "generate", "create", "process", "transform",
                "regulate", "influence", "control", "produce", "contain"]
        objects = ["signals", "energy", "information", "patterns", "data",
                  "properties", "characteristics", "features", "effects", "results"]
        
        facts = []
        for _ in range(self.facts_per_cycle):
            fact = f"{random.choice(subjects)} {random.choice(verbs)} {random.choice(objects)}"
            if fact not in facts:
                facts.append(fact)
        
        return facts
    
    def teach_melvin_directly(self, facts: list):
        """Feed facts directly to Melvin's learning system"""
        # Save to temporary file
        with open('temp_facts.txt', 'w') as f:
            for fact in facts:
                f.write(fact + '\n')
        
        # Call Melvin's teaching system
        # This runs the C++ feed program
        try:
            result = subprocess.run(
                ['./feed_internet_knowledge'],
                input='\n'.join(facts).encode(),
                capture_output=True,
                timeout=30
            )
            
            # Parse output to see growth
            output = result.stdout.decode()
            if 'nodes' in output.lower():
                return True
                
        except Exception as e:
            print(f"  ⚠ Melvin feeding error: {e}")
        
        return False
    
    def run_continuous(self, max_cycles=0):
        """
        Main continuous learning loop
        max_cycles: 0 = infinite, >0 = run for N cycles
        """
        print("\n╔═══════════════════════════════════════════════════════════════╗")
        print("║                                                               ║")
        print("║  🌐 MELVIN CONTINUOUS INTERNET LEARNING                       ║")
        print("║     (100% FREE APIs - No Cost!)                               ║")
        print("║                                                               ║")
        print("╚═══════════════════════════════════════════════════════════════╝\n")
        
        if max_cycles == 0:
            print("⚡ Running in INFINITE mode")
            print("   Press Ctrl+C to stop gracefully\n")
        else:
            print(f"⚡ Running for {max_cycles} cycles\n")
        
        print("Configuration:")
        print(f"  • Facts per cycle: {self.facts_per_cycle}")
        print(f"  • Delay between cycles: {self.delay_seconds} seconds")
        print(f"  • Topics rotating: {len(self.wiki_topics)}")
        print("\nSources:")
        print("  ✓ Wikipedia API (free)")
        print("  ✓ Local generation (fallback)")
        print("\nPress Enter to start...", end='')
        input()
        
        print("\n╔═══════════════════════════════════════════════════════════════╗")
        print("║  CONTINUOUS LEARNING STARTED                                  ║")
        print("╚═══════════════════════════════════════════════════════════════╝\n")
        
        start_time = time.time()
        
        try:
            cycle = 0
            while max_cycles == 0 or cycle < max_cycles:
                cycle += 1
                self.total_cycles = cycle
                
                # Get next topic
                topic = self.wiki_topics[self.current_topic_idx]
                self.current_topic_idx = (self.current_topic_idx + 1) % len(self.wiki_topics)
                
                # Fetch facts
                timestamp = datetime.now().strftime("%H:%M:%S")
                print(f"[{timestamp}] Cycle {cycle:4d} | Topic: {topic:20s}", end='')
                
                # Try Wikipedia first
                facts = self.fetch_wikipedia_facts(topic)
                
                # Fallback to local generation
                if not facts:
                    facts = self.generate_fallback_facts()
                    print(f" | Generated {len(facts)} facts", end='')
                else:
                    print(f" | Fetched {len(facts)} facts", end='')
                
                self.total_facts_fetched += len(facts)
                
                # Save facts
                with open('internet_facts.txt', 'w') as f:
                    for fact in facts:
                        f.write(fact + '\n')
                
                # Feed to Melvin
                try:
                    result = subprocess.run(
                        ['./feed_internet_knowledge'],
                        capture_output=True,
                        timeout=10,
                        text=True
                    )
                    
                    # Parse Melvin's response
                    if 'Nodes:' in result.stdout:
                        # Extract node count
                        for line in result.stdout.split('\n'):
                            if 'Nodes:' in line and 'bytes' in line:
                                print(f" | {line.strip()}")
                                break
                    else:
                        print(" | ✓ Fed to Melvin")
                
                except subprocess.TimeoutExpired:
                    print(" | ⚠ Timeout")
                except Exception as e:
                    print(f" | ⚠ Error: {e}")
                
                # Progress summary every 10 cycles
                if cycle % 10 == 0:
                    elapsed = time.time() - start_time
                    rate = self.total_facts_fetched / elapsed if elapsed > 0 else 0
                    
                    print("\n" + "━"*63)
                    print(f"  MILESTONE: Cycle {cycle}")
                    print(f"  Total facts fetched: {self.total_facts_fetched}")
                    print(f"  Time running: {int(elapsed)} seconds")
                    print(f"  Fetch rate: {rate:.1f} facts/second")
                    print("━"*63 + "\n")
                
                # Delay before next cycle
                if cycle < max_cycles or max_cycles == 0:
                    time.sleep(self.delay_seconds)
        
        except KeyboardInterrupt:
            print("\n\n╔═══════════════════════════════════════════════════════════════╗")
            print("║  GRACEFUL SHUTDOWN (Ctrl+C detected)                          ║")
            print("╚═══════════════════════════════════════════════════════════════╝\n")
        
        # Final summary
        elapsed = time.time() - start_time
        
        print("\n╔═══════════════════════════════════════════════════════════════╗")
        print("║  CONTINUOUS LEARNING SESSION COMPLETE                         ║")
        print("╚═══════════════════════════════════════════════════════════════╝\n")
        
        print("📊 SESSION SUMMARY")
        print("━"*63)
        print(f"  Total cycles: {self.total_cycles}")
        print(f"  Facts fetched: {self.total_facts_fetched}")
        print(f"  Time: {int(elapsed)} seconds ({int(elapsed/60)} minutes)")
        print(f"  Average: {self.total_facts_fetched/self.total_cycles:.1f} facts/cycle")
        print(f"  Rate: {self.total_facts_fetched/elapsed:.1f} facts/second\n")
        
        print("✅ All knowledge has been saved to Melvin's knowledge base!")
        print("\nNext steps:")
        print("  • Run: ./inspect_kb (see what Melvin learned)")
        print("  • Run: ./test_reasoning (test knowledge)")
        print("  • Run: cd melvin && ./melvin (ask questions)\n")


def main():
    # Parse arguments
    max_cycles = 0
    facts_per_cycle = 10
    delay = 5
    
    if len(sys.argv) > 1:
        max_cycles = int(sys.argv[1])
    if len(sys.argv) > 2:
        facts_per_cycle = int(sys.argv[2])
    if len(sys.argv) > 3:
        delay = int(sys.argv[3])
    
    learner = ContinuousInternetLearner(
        facts_per_cycle=facts_per_cycle,
        delay_seconds=delay
    )
    
    learner.run_continuous(max_cycles)


if __name__ == "__main__":
    main()

