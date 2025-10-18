#!/usr/bin/env python3
"""
Real Web Data Streamer - Fast & Reliable

Streams REAL web data from multiple sources:
- Wikipedia Recent Changes (live edits)
- HackerNews API (tech articles)  
- Reddit RSS (diverse topics)
- ArXiv (scientific papers)

Much faster than Common Crawl, genuine web content!
"""

import requests
import time
import re
import json
import random
from datetime import datetime
from xml.etree import ElementTree

# ============================================================================
# CONFIGURATION
# ============================================================================

BATCH_SIZE = 100
INGEST_ENDPOINT = "http://localhost:5050/ingest_batch"
STATS_INTERVAL = 10
FETCH_DELAY = 0.1  # Be nice to servers

# ============================================================================
# TEXT EXTRACTION
# ============================================================================

def clean_text(text):
    """Clean and normalize text"""
    # Remove HTML tags
    text = re.sub(r'<[^>]+>', ' ', text)
    # Remove URLs
    text = re.sub(r'http[s]?://\S+', ' ', text)
    # Remove special chars except spaces and basic punctuation
    text = re.sub(r'[^A-Za-z\s\.\,\!\?]', ' ', text)
    # Normalize whitespace
    text = re.sub(r'\s+', ' ', text)
    return text.strip()

def extract_facts(text):
    """Extract fact triples from text"""
    facts = []
    
    # Comprehensive patterns
    patterns = [
        (r'(\w+) (?:is|are|was|were) (?:a|an|the)?\s*(\w+)', 'is'),
        (r'(\w+) (?:has|have|had) (?:a|an|the)?\s*(\w+)', 'has'),
        (r'(\w+) (?:need|needs|needed|require|requires|required) (\w+)', 'needs'),
        (r'(\w+) (?:produce|produces|produced|create|creates|created) (\w+)', 'produces'),
        (r'(\w+) (?:cause|causes|caused|lead|leads|led to) (\w+)', 'causes'),
        (r'(\w+) (?:contain|contains|contained|include|includes|included) (\w+)', 'contains'),
        (r'(\w+) can (\w+)', 'can'),
        (r'(\w+) (?:enable|enables|enabled|allow|allows|allowed) (\w+)', 'enables'),
        (r'(\w+) (?:use|uses|used|utilize|utilizes|utilized) (\w+)', 'uses'),
        (r'(\w+) (?:describe|describes|described|explain|explains|explained) (\w+)', 'describes'),
        (r'(\w+) (?:analyze|analyzes|analyzed|examine|examines|examined) (\w+)', 'analyzes'),
        (r'(\w+) (?:study|studies|studied|investigate|investigates|investigated) (\w+)', 'studies'),
        (r'(\w+) (?:connect|connects|connected|link|links|linked) (\w+)', 'connects'),
        (r'(\w+) (?:support|supports|supported|maintain|maintains|maintained) (\w+)', 'supports'),
        (r'(\w+) (?:form|forms|formed) (\w+)', 'forms'),
        (r'(\w+) (?:affect|affects|affected|influence|influences|influenced) (\w+)', 'affects'),
        (r'(\w+) (?:provide|provides|provided|supply|supplies|supplied) (\w+)', 'provides'),
        (r'(\w+) (?:result|results|resulted) in (\w+)', 'results_in'),
        (r'(\w+) (?:become|becomes|became) (\w+)', 'becomes'),
        (r'(\w+) (?:develop|develops|developed) (\w+)', 'develops'),
    ]
    
    for pattern, predicate in patterns:
        matches = re.findall(pattern, text, re.IGNORECASE)
        for match in matches:
            if len(match) == 2:
                subject, obj = match
                # Filter out single/double letter words and numbers
                if len(subject) > 2 and len(obj) > 2 and subject.isalpha() and obj.isalpha():
                    facts.append((subject.lower(), predicate, obj.lower()))
    
    return list(set(facts))  # Deduplicate

def send_batch(batch, endpoint):
    """Send batch to ingestion server"""
    try:
        facts = []
        for doc in batch:
            facts.extend(extract_facts(doc['text']))
        
        if facts:
            payload = {
                'facts': [{'subject': s, 'predicate': p, 'object': o} for s, p, o in facts]
            }
            
            resp = requests.post(endpoint, json=payload, timeout=5)
            return len(facts) if resp.status_code == 200 else 0
    except Exception as e:
        print(f"⚠️  Batch send error: {e}")
    return 0

# ============================================================================
# WIKIPEDIA RECENT CHANGES (Real-time edits)
# ============================================================================

def stream_wikipedia():
    """Stream from Wikipedia's recent changes feed"""
    print("  📚 Wikipedia Recent Changes")
    
    url = "https://en.wikipedia.org/w/api.php"
    headers = {'User-Agent': 'MelvinBot/1.0 (Educational Research; melvin@example.com)'}
    params = {
        'action': 'query',
        'list': 'recentchanges',
        'rcprop': 'title|ids|sizes|flags|user',
        'rclimit': '50',
        'format': 'json'
    }
    
    while True:
        try:
            resp = requests.get(url, params=params, headers=headers, timeout=10)
            data = resp.json()
            
            for change in data.get('query', {}).get('recentchanges', []):
                title = change.get('title', '')
                
                # Get article content
                article_params = {
                    'action': 'query',
                    'titles': title,
                    'prop': 'extracts',
                    'exintro': True,
                    'explaintext': True,
                    'format': 'json'
                }
                
                article_resp = requests.get(url, params=article_params, headers=headers, timeout=10)
                article_data = article_resp.json()
                
                pages = article_data.get('query', {}).get('pages', {})
                for page_id, page in pages.items():
                    extract = page.get('extract', '')
                    if extract and len(extract) > 100:
                        yield {
                            'text': extract[:2000],
                            'source': 'wikipedia',
                            'title': title
                        }
                
                time.sleep(FETCH_DELAY)
            
            time.sleep(5)  # Wait before next batch
            
        except Exception as e:
            print(f"⚠️  Wikipedia error: {e}")
            time.sleep(10)

# ============================================================================
# HACKERNEWS (Tech news)
# ============================================================================

def stream_hackernews():
    """Stream from HackerNews top stories"""
    print("  💻 HackerNews Top Stories")
    
    while True:
        try:
            # Get top story IDs
            resp = requests.get('https://hacker-news.firebaseio.com/v0/topstories.json', timeout=10)
            story_ids = resp.json()[:30]  # Top 30
            
            for story_id in story_ids:
                # Get story details
                story_resp = requests.get(
                    f'https://hacker-news.firebaseio.com/v0/item/{story_id}.json',
                    timeout=10
                )
                story = story_resp.json()
                
                title = story.get('title', '')
                text = story.get('text', '')
                url = story.get('url', '')
                
                if title:
                    content = f"{title}. {text}" if text else title
                    yield {
                        'text': content[:2000],
                        'source': 'hackernews',
                        'url': url
                    }
                
                time.sleep(FETCH_DELAY)
            
            time.sleep(300)  # Wait 5 minutes before refreshing
            
        except Exception as e:
            print(f"⚠️  HackerNews error: {e}")
            time.sleep(30)

# ============================================================================
# REDDIT RSS (Diverse topics)
# ============================================================================

def stream_reddit():
    """Stream from Reddit RSS feeds"""
    print("  🗣️  Reddit Top Posts")
    
    subreddits = ['science', 'technology', 'worldnews', 'todayilearned', 
                  'askscience', 'futurology', 'philosophy', 'history']
    
    while True:
        for sub in subreddits:
            try:
                url = f'https://www.reddit.com/r/{sub}/top.rss?t=hour'
                resp = requests.get(url, timeout=10, headers={'User-Agent': 'Melvin/1.0'})
                
                # Parse RSS XML
                root = ElementTree.fromstring(resp.content)
                
                for item in root.findall('.//item')[:5]:  # Top 5 per sub
                    title = item.find('title').text if item.find('title') is not None else ''
                    desc = item.find('description').text if item.find('description') is not None else ''
                    
                    if title:
                        content = f"{title}. {clean_text(desc)}"
                        yield {
                            'text': content[:2000],
                            'source': f'reddit_{sub}'
                        }
                
                time.sleep(FETCH_DELAY)
                
            except Exception as e:
                print(f"⚠️  Reddit error ({sub}): {e}")
                continue
        
        time.sleep(300)  # Wait 5 minutes

# ============================================================================
# ARXIV (Scientific papers)
# ============================================================================

def stream_arxiv():
    """Stream from arXiv papers"""
    print("  🔬 arXiv Scientific Papers")
    
    categories = ['cs.AI', 'cs.LG', 'physics', 'math', 'q-bio']
    
    while True:
        for cat in categories:
            try:
                url = f'http://export.arxiv.org/api/query?search_query=cat:{cat}&start=0&max_results=10&sortBy=submittedDate&sortOrder=descending'
                resp = requests.get(url, timeout=10)
                
                root = ElementTree.fromstring(resp.content)
                ns = {'atom': 'http://www.w3.org/2005/Atom'}
                
                for entry in root.findall('atom:entry', ns):
                    title = entry.find('atom:title', ns)
                    summary = entry.find('atom:summary', ns)
                    
                    if title is not None and summary is not None:
                        content = f"{title.text}. {summary.text}"
                        yield {
                            'text': clean_text(content)[:2000],
                            'source': f'arxiv_{cat}'
                        }
                
                time.sleep(FETCH_DELAY)
                
            except Exception as e:
                print(f"⚠️  arXiv error ({cat}): {e}")
                continue
        
        time.sleep(600)  # Wait 10 minutes

# ============================================================================
# MULTI-SOURCE AGGREGATOR
# ============================================================================

def stream_real_web():
    """Aggregate multiple real web sources"""
    
    # Create round-robin generator
    sources = [
        ('Wikipedia', stream_wikipedia()),
        ('HackerNews', stream_hackernews()),
        ('Reddit', stream_reddit()),
        ('arXiv', stream_arxiv()),
    ]
    
    source_idx = 0
    
    while True:
        name, stream = sources[source_idx]
        try:
            doc = next(stream)
            yield doc
        except StopIteration:
            # Shouldn't happen with infinite streams, but handle it
            pass
        except Exception as e:
            print(f"⚠️  {name} stream error: {e}")
        
        # Round-robin through sources
        source_idx = (source_idx + 1) % len(sources)
        time.sleep(FETCH_DELAY)

# ============================================================================
# MAIN STREAMING LOOP
# ============================================================================

def main():
    print("\n╔═══════════════════════════════════════════════════════╗")
    print("║  MELVIN REAL WEB DATA STREAMER                        ║")
    print("╚═══════════════════════════════════════════════════════╝\n")
    
    print("🌐 Real Data Sources:")
    print("  📚 Wikipedia - Live article edits")
    print("  💻 HackerNews - Tech news & discussions")
    print("  🗣️  Reddit - Diverse community content")
    print("  🔬 arXiv - Scientific papers\n")
    
    print("⚙️  Configuration:")
    print(f"  Batch size:         {BATCH_SIZE} docs")
    print(f"  Endpoint:           {INGEST_ENDPOINT}")
    print(f"  Fetch delay:        {FETCH_DELAY}s (rate limiting)\n")
    
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
    print("  STREAMING REAL WEB DATA")
    print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n")
    print("Press Ctrl+C to stop...\n")
    
    # Stats
    total_docs = 0
    total_facts = 0
    total_batches = 0
    start_time = time.time()
    last_stats = start_time
    
    doc_buffer = []
    stream = stream_real_web()
    
    try:
        while True:
            doc = next(stream)
            doc_buffer.append(doc)
            total_docs += 1
            
            # Process batch
            if len(doc_buffer) >= BATCH_SIZE:
                facts_sent = send_batch(doc_buffer, INGEST_ENDPOINT)
                total_facts += facts_sent
                total_batches += 1
                doc_buffer.clear()
                
                print(f"[{datetime.now().strftime('%H:%M:%S')}] "
                      f"Batch #{total_batches} | "
                      f"Docs: {total_docs} | "
                      f"Facts: {total_facts}")
            
            # Stats
            now = time.time()
            if now - last_stats > STATS_INTERVAL:
                elapsed = now - start_time
                doc_rate = total_docs / elapsed if elapsed > 0 else 0
                fact_rate = total_facts / elapsed if elapsed > 0 else 0
                
                print(f"\n📊 Performance:")
                print(f"   Docs/sec:    {doc_rate:.2f}")
                print(f"   Facts/sec:   {fact_rate:.2f}")
                print(f"   Facts/min:   {fact_rate * 60:.0f}")
                print(f"   Total facts: {total_facts}\n")
                
                last_stats = now
                
    except KeyboardInterrupt:
        print("\n\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━")
        print("  STREAMING STOPPED")
        print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n")
        
        # Process remaining
        if doc_buffer:
            facts_sent = send_batch(doc_buffer, INGEST_ENDPOINT)
            total_facts += facts_sent
            total_batches += 1
        
        # Final stats
        elapsed = time.time() - start_time
        print(f"  Runtime:            {elapsed:.1f} seconds")
        print(f"  Batches sent:       {total_batches}")
        print(f"  Documents:          {total_docs}")
        print(f"  Facts extracted:    {total_facts}")
        if elapsed > 0:
            print(f"  Avg rate:           {total_facts / elapsed:.1f} facts/sec")
            print(f"  Avg rate:           {(total_facts / elapsed) * 60:.0f} facts/min\n")

if __name__ == "__main__":
    main()

