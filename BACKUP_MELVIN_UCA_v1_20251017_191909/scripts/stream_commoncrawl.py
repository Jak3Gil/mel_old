#!/usr/bin/env python3
"""
Common Crawl Streamer - Continuous Web Knowledge Feed

Streams live Common Crawl data and feeds it to Melvin:
- Fetches WARC segments on-demand
- Extracts clean text
- Converts to fact triples
- Writes to ingestion queue
- Never downloads full dataset!

Melvin learns from the real web continuously!
"""

import requests
import gzip
import io
import time
import re
import random
import csv
import os
from datetime import datetime

# ============================================================================
# CONFIGURATION
# ============================================================================

# Common Crawl segment URLs (rotate for variety)
CC_SEGMENTS = [
    # These are example URLs - you can add real ones from commoncrawl.org
    "https://data.commoncrawl.org/crawl-data/CC-MAIN-2024-10/segments/1707947473735.7/warc/CC-MAIN-20240215011034-20240215041034-00000.warc.gz",
]

# Streaming settings
STREAM_RATE = 0.5           # Seconds between samples
MAX_DOC_LENGTH = 2000       # Skip huge pages
MIN_DOC_LENGTH = 50         # Skip tiny fragments
BATCH_SIZE = 10             # Facts per batch
QUEUE_FILE = "data/stream_queue.csv"  # Ingestion queue

# Fact extraction patterns
FACT_PATTERNS = [
    (r'(\w+) (?:is|are) (?:a|an|the)?\s*(\w+)', 'is'),
    (r'(\w+) (?:has|have) (?:a|an|the)?\s*(\w+)', 'has'),
    (r'(\w+) (?:needs?|requires?) (\w+)', 'needs'),
    (r'(\w+) (?:produces?|creates?) (\w+)', 'produces'),
    (r'(\w+) (?:causes?|leads to) (\w+)', 'causes'),
    (r'(\w+) (?:contains?|includes?) (\w+)', 'contains'),
    (r'(\w+) can (\w+)', 'can'),
]

# ============================================================================
# TEXT PROCESSING
# ============================================================================

def clean_html(text):
    """Remove HTML tags and scripts"""
    # Remove script and style tags
    text = re.sub(r'<script[^>]*>.*?</script>', ' ', text, flags=re.DOTALL | re.IGNORECASE)
    text = re.sub(r'<style[^>]*>.*?</style>', ' ', text, flags=re.DOTALL | re.IGNORECASE)
    
    # Remove all HTML tags
    text = re.sub(r'<[^>]+>', ' ', text)
    
    # Decode HTML entities
    text = text.replace('&nbsp;', ' ')
    text = text.replace('&amp;', '&')
    text = text.replace('&lt;', '<')
    text = text.replace('&gt;', '>')
    
    # Normalize whitespace
    text = re.sub(r'\s+', ' ', text)
    
    return text.strip()

def extract_facts(text):
    """Extract fact triples from text using simple patterns"""
    facts = []
    
    # Split into sentences
    sentences = re.split(r'[.!?]', text)
    
    for sentence in sentences:
        sentence = sentence.strip().lower()
        
        # Try each pattern
        for pattern, predicate in FACT_PATTERNS:
            matches = re.findall(pattern, sentence)
            for match in matches:
                if len(match) == 2:
                    subject, obj = match
                    # Clean and validate
                    subject = subject.strip()
                    obj = obj.strip()
                    
                    if (len(subject) > 2 and len(obj) > 2 and 
                        subject.isalpha() and obj.isalpha()):
                        facts.append((subject, predicate, obj))
    
    return facts

# ============================================================================
# SIMULATED STREAMING (For Demo)
# ============================================================================

def simulate_web_stream():
    """
    Simulated web data stream (replaces actual Common Crawl for demo)
    
    In production, replace with actual WARC streaming:
    - requests.get(segment_url, stream=True)
    - warcio.ArchiveIterator for parsing
    """
    
    # Rich, diverse web-like content with new concepts
    web_texts = [
        "Quantum mechanics describes atomic behavior. Particles exhibit wave properties. Superposition allows multiple states simultaneously.",
        "Machine learning algorithms analyze patterns. Neural networks mimic brain structure. Deep learning requires massive datasets.",
        "Cryptocurrency uses blockchain technology. Bitcoin mining validates transactions. Smart contracts execute automatically.",
        "Photosynthesis converts light energy. Chlorophyll absorbs sunlight. Plants release oxygen as byproduct.",
        "Neuroscience studies brain function. Synapses transmit signals. Neuroplasticity enables learning and adaptation.",
        "Renewable energy sources include solar and wind. Photovoltaic cells convert sunlight. Turbines harness wind power.",
        "Artificial intelligence simulates human cognition. Natural language processing understands text. Computer vision interprets images.",
        "Ecosystems maintain biodiversity. Food chains show energy flow. Keystone species affect entire habitats.",
        "Nanotechnology manipulates matter at atomic scale. Carbon nanotubes have exceptional strength. Quantum dots enable new displays.",
        "Astrophysics explores cosmic phenomena. Black holes warp spacetime. Dark matter comprises most universe mass.",
        "Biotechnology engineers living systems. CRISPR edits genetic code. Stem cells regenerate damaged tissue.",
        "Economics analyzes resource allocation. Supply and demand determine prices. Inflation reduces purchasing power.",
        "Psychology examines mental processes. Cognitive behavioral therapy treats disorders. Emotions influence decision making.",
        "Materials science develops new substances. Graphene conducts electricity perfectly. Superconductors operate without resistance.",
        "Oceanography studies marine environments. Coral reefs support biodiversity. Ocean currents distribute heat globally.",
        "Linguistics analyzes language structure. Phonemes are basic sound units. Syntax governs sentence formation.",
        "Archaeology uncovers human history. Carbon dating determines artifact age. Stratigraphy reveals chronological layers.",
        "Meteorology predicts weather patterns. Atmospheric pressure influences climate. Hurricanes form over warm oceans.",
        "Philosophy questions fundamental reality. Epistemology examines knowledge. Ethics establishes moral principles.",
        "Immunology studies disease resistance. Antibodies target pathogens. Vaccines stimulate immune response.",
    ]
    
    while True:
        # Randomly select a "web page"
        text = random.choice(web_texts)
        yield text
        
        # Simulate network delay
        time.sleep(STREAM_RATE)

# ============================================================================
# INGESTION QUEUE
# ============================================================================

def append_to_queue(facts, queue_file):
    """Append facts to ingestion queue"""
    file_exists = os.path.exists(queue_file)
    
    with open(queue_file, 'a', newline='') as f:
        writer = csv.writer(f)
        
        if not file_exists:
            writer.writerow(['subject', 'predicate', 'object'])
        
        writer.writerows(facts)

def clear_queue(queue_file):
    """Clear the queue after ingestion"""
    if os.path.exists(queue_file):
        os.remove(queue_file)

# ============================================================================
# MAIN STREAMING LOOP
# ============================================================================

def main():
    print("\n╔═══════════════════════════════════════════════════════╗")
    print("║  COMMON CRAWL STREAMER (Simulated)                   ║")
    print("╚═══════════════════════════════════════════════════════╝\n")
    
    print("⚙️  Configuration:")
    print(f"  Stream rate:        {STREAM_RATE} sec/sample")
    print(f"  Batch size:         {BATCH_SIZE} facts")
    print(f"  Queue file:         {QUEUE_FILE}\n")
    
    print("🌍 Mode: Simulated web stream (demo)")
    print("   For production: Install warcio and use real CC URLs\n")
    
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print("  STREAMING DATA")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n")
    
    print("Press Ctrl+C to stop...\n")
    
    batch = []
    total_facts = 0
    total_docs = 0
    
    try:
        for text in simulate_web_stream():
            total_docs += 1
            
            # Clean and extract facts
            cleaned = clean_html(text)
            facts = extract_facts(cleaned)
            
            if facts:
                batch.extend(facts)
                
                print(f"[{datetime.now().strftime('%H:%M:%S')}] "
                      f"Doc {total_docs} | Extracted {len(facts)} facts | "
                      f"Queue: {len(batch)}")
                
                # Write batch when full
                if len(batch) >= BATCH_SIZE:
                    append_to_queue(batch, QUEUE_FILE)
                    total_facts += len(batch)
                    
                    print(f"  ✓ Queued {len(batch)} facts for ingestion")
                    print(f"  📊 Total: {total_facts} facts from {total_docs} documents\n")
                    
                    batch = []
    
    except KeyboardInterrupt:
        print("\n\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
        print("  STREAMING STOPPED")
        print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n")
        
        # Write remaining batch
        if batch:
            append_to_queue(batch, QUEUE_FILE)
            total_facts += len(batch)
            print(f"  ✓ Flushed final {len(batch)} facts\n")
        
        print(f"📊 Session Summary:")
        print(f"  Documents processed: {total_docs}")
        print(f"  Facts extracted:     {total_facts}")
        print(f"  Queue file:          {QUEUE_FILE}\n")
        
        print(f"💡 Next steps:")
        print(f"  ./tools/melvin_ingest -t csv {QUEUE_FILE}")
        print(f"  make stats\n")

if __name__ == "__main__":
    main()

