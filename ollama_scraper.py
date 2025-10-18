#!/usr/bin/env python3
"""
Ollama-Powered Knowledge Scraper for Melvin
Uses local Llama 3.2 to generate high-quality educational facts
100% LOCAL - No internet needed!
"""

import requests
import json
import time
from datetime import datetime

class OllamaScraper:
    """Generate educational facts using local Ollama/Llama 3.2"""
    
    def __init__(self, model="llama3.2", ollama_url="http://localhost:11434"):
        self.model = model
        self.ollama_url = ollama_url
        self.facts_generated = 0
        
    def check_ollama(self):
        """Check if Ollama is running"""
        try:
            response = requests.get(f"{self.ollama_url}/api/tags", timeout=2)
            return response.status_code == 200
        except:
            return False
    
    def generate_facts(self, topic, count=20):
        """Generate facts about a specific topic using Ollama"""
        
        prompt = f"""Generate {count} educational facts about {topic}. 
Each fact should be:
- One complete sentence
- Scientifically accurate
- Educational and interesting
- Suitable for a knowledge base

Format: One fact per line, no numbering or bullets.

Facts:"""

        try:
            response = requests.post(
                f"{self.ollama_url}/api/generate",
                json={
                    "model": self.model,
                    "prompt": prompt,
                    "stream": False,
                    "options": {
                        "temperature": 0.8,
                        "top_p": 0.9,
                        "num_predict": 500
                    }
                },
                timeout=30
            )
            
            if response.status_code == 200:
                result = response.json()
                text = result.get('response', '')
                
                # Extract facts (one per line)
                facts = []
                for line in text.split('\n'):
                    line = line.strip()
                    # Remove numbering, bullets, etc.
                    line = line.lstrip('0123456789.-•*) ')
                    if len(line) > 20 and len(line) < 300:  # Reasonable length
                        facts.append(line)
                
                return facts[:count]
            
        except Exception as e:
            print(f"  ⚠️  Ollama error for {topic}: {e}")
        
        return []
    
    def generate_diverse_facts(self, total_count=100):
        """Generate diverse facts across multiple topics"""
        
        topics = [
            "physics and quantum mechanics",
            "biology and cellular processes",
            "chemistry and molecular science",
            "astronomy and cosmology",
            "computer science and AI",
            "mathematics and logic",
            "neuroscience and cognition",
            "geology and earth science",
            "history and civilization",
            "philosophy and ethics",
            "engineering and technology",
            "medicine and health",
            "ecology and environment",
            "linguistics and language",
            "psychology and behavior"
        ]
        
        all_facts = []
        facts_per_topic = max(1, total_count // len(topics))
        
        print(f"🧠 Generating {total_count} facts using Ollama ({self.model})...\n")
        
        for i, topic in enumerate(topics):
            if len(all_facts) >= total_count:
                break
            
            print(f"  [{i+1}/{len(topics)}] {topic}...", end=' ', flush=True)
            
            facts = self.generate_facts(topic, facts_per_topic)
            if facts:
                all_facts.extend(facts)
                self.facts_generated += len(facts)
                print(f"✓ {len(facts)} facts")
            else:
                print("✗ failed")
            
            time.sleep(0.1)  # Brief pause between topics
        
        return all_facts[:total_count]
    
    def save_facts(self, facts, filename="internet_facts.txt"):
        """Save facts to file"""
        with open(filename, 'w') as f:
            for fact in facts:
                f.write(fact + '\n')
        return True


def main():
    import sys
    
    facts_count = 100
    if len(sys.argv) > 1:
        facts_count = int(sys.argv[1])
    
    print("\n╔═══════════════════════════════════════════════════════════════╗")
    print("║  🧠 OLLAMA-POWERED KNOWLEDGE SCRAPER FOR MELVIN               ║")
    print("║     Using Llama 3.2 for HIGH-QUALITY Facts!                  ║")
    print("╚═══════════════════════════════════════════════════════════════╝\n")
    
    scraper = OllamaScraper()
    
    # Check if Ollama is running
    print("Checking Ollama status...")
    if not scraper.check_ollama():
        print("\n❌ ERROR: Ollama is not running!")
        print("\nTo start Ollama:")
        print("  1. Install: https://ollama.ai")
        print("  2. Run: ollama serve")
        print("  3. Pull model: ollama pull llama3.2\n")
        return 1
    
    print("✅ Ollama is running!\n")
    
    start_time = time.time()
    
    # Generate facts
    facts = scraper.generate_diverse_facts(facts_count)
    
    elapsed = time.time() - start_time
    
    print(f"\n✅ Generated {len(facts)} high-quality facts in {elapsed:.1f}s")
    print(f"   Speed: {len(facts)/elapsed:.1f} facts/sec\n")
    
    # Save
    scraper.save_facts(facts)
    print("💾 Saved to internet_facts.txt\n")
    
    # Show samples
    print("📄 Sample facts:")
    for i, fact in enumerate(facts[:10], 1):
        print(f"  {i}. {fact}")
    if len(facts) > 10:
        print(f"  ... and {len(facts) - 10} more\n")
    
    print("🎉 Ready to feed to Melvin!\n")
    
    return 0


if __name__ == "__main__":
    exit(main())

