#!/usr/bin/env python3
"""
╔═══════════════════════════════════════════════════════════════════════════╗
║  MELVIN CONTINUOUS LEARNING MODE - OLLAMA POWERED                         ║
║  Ollama generates → Melvin processes → Saves to global storage            ║
║  100% LOCAL - No internet needed!                                         ║
╚═══════════════════════════════════════════════════════════════════════════╝

This script:
1. Connects to local Ollama instance
2. Generates continuous stream of educational facts
3. Parses facts into concepts and relationships
4. Creates nodes and edges in Melvin's brain
5. Saves to global storage (nodes.bin/edges.bin)
6. Runs 24/7 until stopped
"""

import requests
import json
import time
import subprocess
import sys
import os
import re
from datetime import datetime
from typing import List, Tuple, Set

class OllamaFeeder:
    """Generates continuous stream of knowledge using Ollama"""
    
    def __init__(self, model="llama3.2", ollama_url="http://localhost:11434"):
        self.model = model
        self.ollama_url = ollama_url
        self.topics = [
            "quantum physics", "molecular biology", "organic chemistry", 
            "astrophysics", "machine learning", "abstract mathematics",
            "neuroscience", "geology", "ancient history", "philosophy",
            "computer architecture", "genetics", "ecology", "linguistics",
            "psychology", "materials science", "thermodynamics", "evolution",
            "cosmology", "cryptography", "topology", "particle physics",
            "biochemistry", "cognitive science", "robotics", "game theory",
            "quantum computing", "synthetic biology", "climate science",
            "archeology", "logic", "epidemiology", "oceanography"
        ]
        self.topic_index = 0
        
    def check_ollama(self) -> bool:
        """Check if Ollama is running"""
        try:
            response = requests.get(f"{self.ollama_url}/api/tags", timeout=2)
            return response.status_code == 200
        except:
            return False
    
    def generate_batch(self, batch_size=20) -> List[str]:
        """Generate a batch of facts on rotating topics"""
        topic = self.topics[self.topic_index % len(self.topics)]
        self.topic_index += 1
        
        prompt = f"""Generate {batch_size} simple educational facts about {topic}.

CRITICAL: Each fact MUST follow these exact formats:
- "X is Y" (definition)
- "X has Y" (possession)
- "X produces Y" (creation)
- "X contains Y" (composition)
- "X causes Y" (causation)
- "X enables Y" (enablement)
- "X forms Y" (formation)

Requirements:
- Keep subjects and objects short (1-3 words max)
- Use simple, clear relationships
- Be scientifically accurate
- Do NOT use complex sentence structures

Good examples:
- "Fire produces heat"
- "Mitochondria produce energy"
- "DNA contains genes"
- "Photosynthesis converts light"
- "Neurons transmit signals"

Bad examples (too complex):
- "Quantum entanglement enables faster information processing across vast distances"
- "The mitochondria, which are cellular organelles, produce ATP"

Simple facts only:"""

        try:
            response = requests.post(
                f"{self.ollama_url}/api/generate",
                json={
                    "model": self.model,
                    "prompt": prompt,
                    "stream": False,
                    "options": {
                        "temperature": 0.7,
                        "top_p": 0.9,
                        "num_predict": 600
                    }
                },
                timeout=45
            )
            
            if response.status_code == 200:
                result = response.json()
                text = result.get('response', '')
                
                # Extract facts
                facts = []
                for line in text.split('\n'):
                    line = line.strip()
                    # Clean up numbering/bullets
                    line = re.sub(r'^[\d\.\-\*\•\)\]]+\s*', '', line)
                    # Must be reasonable length and contain relationship words
                    if (20 < len(line) < 250 and 
                        any(word in line.lower() for word in 
                            ['is', 'has', 'does', 'contains', 'produces', 'creates', 
                             'enables', 'causes', 'affects', 'controls', 'forms'])):
                        facts.append(line)
                
                return facts[:batch_size]
            
        except Exception as e:
            print(f"  ⚠️  Ollama error: {e}")
        
        return []


class MelvinBrain:
    """Interface to Melvin's C++ brain with optimized storage"""
    
    def __init__(self, workspace="/Users/jakegilbert/Desktop/Melvin/Melvin"):
        self.workspace = workspace
        self.temp_facts_file = os.path.join(workspace, "temp_ollama_facts.txt")
        self.feeder_binary = os.path.join(workspace, "direct_fact_feeder")
        self.nodes_file = os.path.join(workspace, "melvin/data/nodes.melvin")
        self.edges_file = os.path.join(workspace, "melvin/data/edges.melvin")
        
    def feed_facts(self, facts: List[str]) -> bool:
        """Feed facts to Melvin's brain using direct C++ feeder"""
        if not facts:
            return False
        
        # Write facts to temp file
        with open(self.temp_facts_file, 'w') as f:
            for fact in facts:
                f.write(fact + '\n')
        
        # Use C++ direct feeder
        try:
            # Call the direct fact feeder
            result = subprocess.run(
                [self.feeder_binary, '--quiet', self.temp_facts_file],
                cwd=self.workspace,
                capture_output=True,
                text=True,
                timeout=30
            )
            
            # Parse output (format: facts_read facts_processed nodes_added edges_added time)
            if result.returncode == 0 and result.stdout.strip():
                return True
            
            return result.returncode == 0
            
        except Exception as e:
            print(f"  ⚠️  Brain feed error: {e}")
            return False
    
    def get_stats(self) -> dict:
        """Get current brain statistics - READ ACTUAL COUNTS from storage"""
        stats = {
            'nodes': 0,
            'edges': 0,
            'nodes_size_mb': 0,
            'edges_size_mb': 0
        }
        
        try:
            # Get file sizes
            if os.path.exists(self.nodes_file):
                stats['nodes_size_mb'] = os.path.getsize(self.nodes_file) / (1024 * 1024)
            if os.path.exists(self.edges_file):
                stats['edges_size_mb'] = os.path.getsize(self.edges_file) / (1024 * 1024)
            
            # Get ACTUAL counts by running a quick C++ query
            # Create a simple stats checker
            result = subprocess.run(
                ['./melvin/melvin', '--stats-only'],
                cwd=self.workspace,
                capture_output=True,
                text=True,
                timeout=2
            )
            
            # If that doesn't work, try parsing the binary file directly
            # Read the nodes.melvin file format to get actual count
            if os.path.exists(self.nodes_file):
                with open(self.nodes_file, 'rb') as f:
                    # Skip string table count (4 bytes)
                    string_count_bytes = f.read(4)
                    if len(string_count_bytes) == 4:
                        import struct
                        string_count = struct.unpack('I', string_count_bytes)[0]
                        
                        # Skip string table
                        for _ in range(string_count):
                            str_len_bytes = f.read(2)
                            if len(str_len_bytes) < 2:
                                break
                            str_len = struct.unpack('H', str_len_bytes)[0]
                            f.read(str_len)  # Skip the string data
                        
                        # Read actual node count
                        node_count_bytes = f.read(4)
                        if len(node_count_bytes) == 4:
                            stats['nodes'] = struct.unpack('I', node_count_bytes)[0]
            
            if os.path.exists(self.edges_file):
                with open(self.edges_file, 'rb') as f:
                    # Read edge count (first 4 bytes)
                    edge_count_bytes = f.read(4)
                    if len(edge_count_bytes) == 4:
                        import struct
                        stats['edges'] = struct.unpack('I', edge_count_bytes)[0]
        except:
            # Fallback to old estimation if reading fails
            try:
                if stats['nodes'] == 0 and os.path.exists(self.nodes_file):
                    stats['nodes'] = os.path.getsize(self.nodes_file) // 100  # Very rough estimate
                if stats['edges'] == 0 and os.path.exists(self.edges_file):
                    stats['edges'] = os.path.getsize(self.edges_file) // 8
            except:
                pass
        
        return stats


class FactParser:
    """Parse facts into concept-relationship-concept triples"""
    
    @staticmethod
    def parse_fact(fact: str) -> List[Tuple[str, str, str]]:
        """
        Parse a fact into (subject, relationship, object) triples
        Example: "Neurons transmit electrical signals" 
                 → [("neurons", "transmit", "electrical signals")]
        """
        fact = fact.strip()
        if not fact:
            return []
        
        triples = []
        
        # Common relationship patterns
        patterns = [
            (r'(.+?)\s+(is|are)\s+(.+)', 'is'),
            (r'(.+?)\s+(has|have)\s+(.+)', 'has'),
            (r'(.+?)\s+(does|do)\s+(.+)', 'does'),
            (r'(.+?)\s+(contains?|contain)\s+(.+)', 'contains'),
            (r'(.+?)\s+(produces?|produce)\s+(.+)', 'produces'),
            (r'(.+?)\s+(creates?|create)\s+(.+)', 'creates'),
            (r'(.+?)\s+(enables?|enable)\s+(.+)', 'enables'),
            (r'(.+?)\s+(causes?|cause)\s+(.+)', 'causes'),
            (r'(.+?)\s+(affects?|affect)\s+(.+)', 'affects'),
            (r'(.+?)\s+(controls?|control)\s+(.+)', 'controls'),
            (r'(.+?)\s+(forms?|form)\s+(.+)', 'forms'),
        ]
        
        for pattern, relation in patterns:
            match = re.search(pattern, fact, re.IGNORECASE)
            if match:
                subject = match.group(1).strip().lower()
                obj = match.group(3).strip().lower()
                
                # Clean up
                subject = re.sub(r'^(the|a|an)\s+', '', subject)
                obj = re.sub(r'^(the|a|an)\s+', '', obj)
                
                if subject and obj and subject != obj:
                    triples.append((subject, relation, obj))
                    break
        
        return triples


def print_header():
    """Print system header"""
    print("\n╔═══════════════════════════════════════════════════════════════════════════╗")
    print("║                                                                           ║")
    print("║       🧠⚡ MELVIN CONTINUOUS LEARNING - OLLAMA MODE ⚡🧠                   ║")
    print("║                                                                           ║")
    print("║  Ollama → Knowledge Generation → Melvin Processing → Global Storage      ║")
    print("║                                                                           ║")
    print("╚═══════════════════════════════════════════════════════════════════════════╝\n")


def print_stats(stats: dict, facts_processed: int, elapsed: float):
    """Print current statistics"""
    print("\n┌───────────────────────────────────────────────────────────────────────────┐")
    print("│  📊 CURRENT BRAIN STATISTICS                                              │")
    print("├───────────────────────────────────────────────────────────────────────────┤")
    print(f"│  Nodes:              {stats['nodes']:,}".ljust(76) + "│")
    print(f"│  Edges:              {stats['edges']:,}".ljust(76) + "│")
    print(f"│  Storage (nodes):    {stats['nodes_size_mb']:.2f} MB".ljust(76) + "│")
    print(f"│  Storage (edges):    {stats['edges_size_mb']:.2f} MB".ljust(76) + "│")
    print(f"│  Facts processed:    {facts_processed:,}".ljust(76) + "│")
    print(f"│  Runtime:            {elapsed/60:.1f} minutes".ljust(76) + "│")
    print(f"│  Rate:               {facts_processed/elapsed:.1f} facts/sec".ljust(76) + "│")
    print("└───────────────────────────────────────────────────────────────────────────┘")


def main():
    print_header()
    
    # Configuration
    BATCH_SIZE = 20  # Facts per batch
    CYCLE_DELAY = 5  # Seconds between batches
    STATS_INTERVAL = 60  # Show stats every 60 seconds
    
    print("Initializing components...\n")
    
    # Initialize components
    ollama = OllamaFeeder()
    brain = MelvinBrain()
    parser = FactParser()
    
    # Check Ollama
    print("Checking Ollama status...", end=' ')
    if not ollama.check_ollama():
        print("❌\n")
        print("ERROR: Ollama is not running!\n")
        print("To start:")
        print("  1. Install: https://ollama.ai")
        print("  2. Run: ollama serve")
        print("  3. Pull model: ollama pull llama3.2\n")
        return 1
    print("✅\n")
    
    # Get initial stats
    initial_stats = brain.get_stats()
    print(f"Initial state: {initial_stats['nodes']:,} nodes, {initial_stats['edges']:,} edges\n")
    
    print("═" * 79)
    print("🚀 STARTING CONTINUOUS LEARNING MODE")
    print("═" * 79)
    print(f"\nConfiguration:")
    print(f"  • Batch size:      {BATCH_SIZE} facts")
    print(f"  • Cycle delay:     {CYCLE_DELAY} seconds")
    print(f"  • Stats interval:  {STATS_INTERVAL} seconds")
    print(f"  • Model:           {ollama.model}")
    print(f"\n⚠️  Press Ctrl+C to stop gracefully\n")
    
    # Main learning loop
    cycle = 0
    total_facts = 0
    total_batches = 0
    successful_batches = 0
    start_time = time.time()
    last_stats_time = start_time
    
    try:
        while True:
            cycle += 1
            cycle_start = time.time()
            
            print(f"[{datetime.now().strftime('%H:%M:%S')}] Cycle {cycle} - ", end='', flush=True)
            
            # Generate facts from Ollama
            print("Generating...", end=' ', flush=True)
            facts = ollama.generate_batch(BATCH_SIZE)
            
            if not facts:
                print("❌ No facts generated")
                time.sleep(CYCLE_DELAY)
                continue
            
            print(f"✓ {len(facts)} facts - ", end='', flush=True)
            
            # Feed to Melvin's brain
            print("Feeding...", end=' ', flush=True)
            success = brain.feed_facts(facts)
            
            if success:
                print("✓ Processed - ", end='', flush=True)
                total_facts += len(facts)
                total_batches += 1
                successful_batches += 1
            else:
                print("⚠️ Feed failed - ", end='', flush=True)
                total_batches += 1
            
            cycle_time = time.time() - cycle_start
            print(f"({cycle_time:.1f}s)")
            
            # Show sample facts occasionally
            if cycle % 5 == 0 and facts:
                print(f"  📝 Sample: \"{facts[0][:80]}...\"")
            
            # Show detailed stats periodically
            current_time = time.time()
            if current_time - last_stats_time >= STATS_INTERVAL:
                elapsed = current_time - start_time
                stats = brain.get_stats()
                print_stats(stats, total_facts, elapsed)
                
                # Show growth
                nodes_growth = stats['nodes'] - initial_stats['nodes']
                edges_growth = stats['edges'] - initial_stats['edges']
                print(f"\n  📈 Growth: +{nodes_growth:,} nodes, +{edges_growth:,} edges")
                print(f"  ✅ Success rate: {successful_batches/total_batches*100:.1f}% ({successful_batches}/{total_batches})\n")
                
                last_stats_time = current_time
            
            # Brief pause before next cycle
            time.sleep(CYCLE_DELAY)
            
    except KeyboardInterrupt:
        print("\n\n🛑 Stopping gracefully...\n")
    
    # Final statistics
    elapsed = time.time() - start_time
    final_stats = brain.get_stats()
    
    print("═" * 79)
    print("📊 FINAL STATISTICS")
    print("═" * 79)
    print(f"\nSession Summary:")
    print(f"  • Total cycles:        {cycle}")
    print(f"  • Facts processed:     {total_facts:,}")
    print(f"  • Successful batches:  {successful_batches}/{total_batches}")
    print(f"  • Runtime:             {elapsed/60:.1f} minutes")
    print(f"  • Average rate:        {total_facts/elapsed:.1f} facts/sec")
    
    print(f"\nBrain Growth:")
    print(f"  • Initial nodes:       {initial_stats['nodes']:,}")
    print(f"  • Final nodes:         {final_stats['nodes']:,}")
    print(f"  • Growth:              +{final_stats['nodes'] - initial_stats['nodes']:,} nodes")
    print(f"\n  • Initial edges:       {initial_stats['edges']:,}")
    print(f"  • Final edges:         {final_stats['edges']:,}")
    print(f"  • Growth:              +{final_stats['edges'] - initial_stats['edges']:,} edges")
    
    print(f"\nStorage:")
    print(f"  • nodes.bin:           {final_stats['nodes_size_mb']:.2f} MB")
    print(f"  • edges.bin:           {final_stats['edges_size_mb']:.2f} MB")
    print(f"  • Total:               {final_stats['nodes_size_mb'] + final_stats['edges_size_mb']:.2f} MB")
    
    print("\n✅ Learning session complete!")
    print("\nMelvin's brain has been updated and saved to global storage:")
    print("  • nodes.bin (concepts)")
    print("  • edges.bin (connections)")
    print("\n🧠 Melvin is smarter than before! 🚀\n")
    
    return 0


if __name__ == "__main__":
    exit(main())

