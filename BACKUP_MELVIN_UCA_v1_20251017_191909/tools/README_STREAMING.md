# Continuous Web Learning - Stream & Ingest

**Feed Melvin live data from the web continuously!**

---

## What It Does

Two-process system for continuous learning:

1. **Streamer** (Python) - Fetches web data, extracts facts, writes to queue
2. **Watcher** (Bash) - Monitors queue, ingests facts into Melvin's brain

**Result:** Melvin learns from the real web 24/7! 🌍

---

## Quick Start

### Terminal 1: Start the watcher
```bash
./scripts/watch_and_ingest.sh
```

### Terminal 2: Start the streamer
```bash
python3 scripts/stream_commoncrawl.py
```

**That's it!** Melvin will continuously learn from incoming data.

---

## How It Works

### Data Flow

```
Web/CommonCrawl
       ↓
  [Python Streamer]
    • Fetches WARC segments
    • Extracts clean text
    • Extracts fact triples
    • Writes to queue file
       ↓
  data/stream_queue.csv
       ↓
  [Bash Watcher]
    • Monitors queue file
    • Runs melvin_ingest when enough facts
    • Clears queue
       ↓
  Melvin's Brain
    • New nodes created
    • New edges created
    • Auto-saved to disk
```

---

## Components

### 1. Stream Script (`stream_commoncrawl.py`)

**What it does:**
- Fetches web data (Common Crawl or simulated)
- Cleans HTML and extracts text
- Uses regex to extract fact triples
- Writes to CSV queue

**Fact patterns:**
- "X is Y" → (X, is, Y)
- "X has Y" → (X, has, Y)
- "X needs Y" → (X, needs, Y)
- "X produces Y" → (X, produces, Y)

**Output:**
```csv
subject,predicate,object
dog,is,animal
plant,produces,oxygen
```

### 2. Watch Script (`watch_and_ingest.sh`)

**What it does:**
- Checks queue file every N seconds
- When >= MIN_FACTS accumulate
- Runs `melvin_ingest` on queue
- Clears queue
- Shows brain state

**Output:**
```
[Ingestion #1]
📥 Found 10 facts in queue
✓ Ingested 10 facts
📊 Total: 10 facts in 1 batches

🧠 Current brain state:
  Total Nodes: 215
  Total Connections: 945
```

---

## Configuration

### Streamer Settings

Edit `scripts/stream_commoncrawl.py`:

```python
STREAM_RATE = 0.5           # Seconds between samples
MAX_DOC_LENGTH = 2000       # Skip huge pages
MIN_DOC_LENGTH = 50         # Skip tiny fragments
BATCH_SIZE = 10             # Facts per batch
```

### Watcher Settings

Edit `scripts/watch_and_ingest.sh`:

```bash
INGEST_INTERVAL=10     # Seconds between checks
MIN_FACTS=5            # Minimum facts before ingesting
```

---

## Modes

### 1. Simulated Mode (Current/Demo)

**Uses:** Built-in sample texts  
**No network:** Works offline  
**Good for:** Testing, development  

```bash
python3 scripts/stream_commoncrawl.py
```

### 2. Real Common Crawl (Production)

**Requirements:**
```bash
pip3 install warcio requests
```

**Edit streamer:**
```python
# Use real Common Crawl URLs
CC_SEGMENTS = [
    "https://data.commoncrawl.org/crawl-data/CC-MAIN-2024-10/...",
]

# Use actual WARC iterator
from warcio.archiveiterator import ArchiveIterator

def stream_real_commoncrawl(segment_url):
    resp = requests.get(segment_url, stream=True)
    for record in ArchiveIterator(gzip.GzipFile(fileobj=resp.raw)):
        if record.rec_type == "response":
            payload = record.content_stream().read()
            text = payload.decode("utf-8", errors="ignore")
            yield clean_html(text)
```

### 3. Custom Data Source

**Replace stream source:**
```python
def stream_from_api():
    while True:
        response = requests.get("https://api.example.com/facts")
        yield response.text
```

---

## Usage Examples

### Basic Streaming
```bash
# Terminal 1
./scripts/watch_and_ingest.sh

# Terminal 2
python3 scripts/stream_commoncrawl.py
```

### Monitor Growth
```bash
# Terminal 3 (while streaming)
watch -n 5 'make stats'
```

### Stop Streaming
```bash
# In streamer terminal
Press Ctrl+C

# Watcher will ingest remaining facts and continue monitoring
```

### Resume Later
```bash
# Streamer adds to queue
python3 scripts/stream_commoncrawl.py

# Watcher picks up where it left off
./scripts/watch_and_ingest.sh
```

---

## Output Files

**Queue file:**
```
data/stream_queue.csv
- Temporary holding area
- Cleared after each ingestion
- Can grow to 100s of facts before ingestion
```

**Brain files:**
```
data/nodes.melvin
data/edges.melvin
- Continuously updated
- Auto-saved after each ingestion
- Persistent across restarts
```

---

## Performance

### Streaming Speed

**Simulated mode:**
- 0.5 sec per document
- ~2 facts per document
- ~4 facts/second
- ~14,400 facts/hour

**Real Common Crawl:**
- Network dependent
- Typically 1-5 sec per document
- 1-10 facts per document
- ~1,000-10,000 facts/hour

### Ingestion Speed

- 75,000 facts/second (melvin_ingest)
- Batch of 100 facts: ~1.3 ms
- Essentially instant!

### Brain Growth

**After 1 hour of streaming:**
```
Input: ~14,000 facts
Output: ~28,000 nodes, ~56,000 edges
(Assuming 2× node:fact, 4× edge:fact with LEAP)
```

---

## Monitoring

### Watch Brain Grow
```bash
# Real-time stats (updates every 5 seconds)
watch -n 5 'make stats | head -20'
```

### Track Growth Rate
```bash
# Start growth tracking during stream
make growth CYCLES=1000 &

# Let it run while streaming
```

### Check Queue
```bash
# See current queue size
wc -l data/stream_queue.csv
```

---

## Troubleshooting

### Queue file grows too large
```
Cause: Ingestion interval too long
Fix: Reduce INGEST_INTERVAL in watcher
Fix: Lower MIN_FACTS threshold
```

### No facts extracted
```
Cause: Patterns don't match text
Check: Review FACT_PATTERNS
Fix: Add more patterns
Fix: Use NLP library (spaCy)
```

### Streamer crashes
```
Cause: Network issues with real CC
Fix: Add retry logic
Fix: Use simulated mode for testing
```

### Watcher misses facts
```
Cause: Race condition (writing while reading)
Fix: Use file locking
Fix: Increase check interval
```

---

## Production Deployment

### With Real Common Crawl

**1. Install dependencies:**
```bash
pip3 install warcio requests
```

**2. Get real segment URLs:**
```bash
# From https://index.commoncrawl.org/collinfo.json
curl https://data.commoncrawl.org/crawl-data/CC-MAIN-2024-10/warc.paths.gz | \
  gunzip | head -10
```

**3. Update CC_SEGMENTS in streamer**

**4. Run in background:**
```bash
# Start watcher
nohup ./scripts/watch_and_ingest.sh > watcher.log 2>&1 &

# Start streamer
nohup python3 scripts/stream_commoncrawl.py > streamer.log 2>&1 &

# Check progress
tail -f watcher.log
```

### With Custom Data Source

**Replace stream function:**
```python
def stream_from_rss():
    import feedparser
    feeds = ["https://news.example.com/rss", ...]
    
    while True:
        for feed_url in feeds:
            feed = feedparser.parse(feed_url)
            for entry in feed.entries:
                yield entry.summary
        time.sleep(3600)  # Check hourly
```

---

## Safety & Limits

### Rate Limiting
```python
# Respect source server
STREAM_RATE = 1.0  # Don't hammer servers
```

### Storage Limits
```bash
# Monitor disk usage
df -h data/

# Set max brain size
if [ $(du -k data/*.melvin | cut -f1) -gt 100000 ]; then
    echo "Brain too large, pausing..."
    pkill -f stream_commoncrawl
fi
```

### Quality Control
```python
# Filter low-quality facts
if len(subject) < 3 or len(object) < 3:
    continue  # Skip

if subject == object:
    continue  # Skip self-loops
```

---

## Advanced Features

### Multi-Source Streaming

**Run multiple streamers:**
```bash
# Terminal 1: Common Crawl
python3 scripts/stream_commoncrawl.py

# Terminal 2: Wikipedia
python3 scripts/stream_wikipedia.py

# Terminal 3: News RSS
python3 scripts/stream_news.py

# All feed to same queue
# Single watcher ingests all
```

### Prioritized Ingestion

**Filter by topic:**
```python
if "science" in text or "technology" in text:
    facts.append(...)  # Keep
else:
    continue  # Skip
```

### Fact Validation

**Check fact quality:**
```python
# Deduplicate
seen = set()
if (subject, predicate, object) in seen:
    continue
seen.add((subject, predicate, object))

# Validate structure
if predicate not in ["is", "has", "needs", ...]:
    continue
```

---

## Comparison to Batch Import

| Method | Speed | Scalability | Complexity |
|--------|-------|-------------|------------|
| Batch (`melvin_ingest`) | Fast | Limited by disk | Simple |
| Streaming (this) | Continuous | Unlimited | Moderate |

**Use batch for:**
- One-time imports
- Known datasets
- Fast bootstrapping

**Use streaming for:**
- Continuous learning
- Live data sources
- Long-running systems

---

## Example Session

```bash
$ ./scripts/watch_and_ingest.sh &
[1] 12345
🔄 Starting continuous ingestion loop...

$ python3 scripts/stream_commoncrawl.py
🌍 Streaming data...

[10:15:23] Doc 1 | Extracted 3 facts | Queue: 3
[10:15:24] Doc 2 | Extracted 2 facts | Queue: 5
[10:15:24] Doc 3 | Extracted 4 facts | Queue: 9
[10:15:25] Doc 4 | Extracted 2 facts | Queue: 11
  ✓ Queued 11 facts for ingestion
  📊 Total: 11 facts from 4 documents

[Watcher detects queue]
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  INGESTION #0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

📥 Found 11 facts in queue
  Records processed:  11
  Nodes created:      15
  Edges created:      22

🧠 Current brain state:
  Total Nodes: 217
  Total Connections: 939

[Streaming continues...]
```

---

## Summary

**Continuous web learning system:**

✅ Streams data on-demand (no full download)  
✅ Extracts facts automatically  
✅ Queues for batch ingestion  
✅ Continuous brain growth  
✅ Persistent across restarts  
✅ Scalable to unlimited data  

**Status:** Ready for demo (simulated)  
**Production:** Requires warcio + real CC URLs  

**Impact:** Melvin learns from the web continuously! 🌍

---

*Last updated: October 14, 2025*

