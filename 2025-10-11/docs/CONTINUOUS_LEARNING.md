# Continuous Learning Mode

## Overview

**Continuous Learning Mode** transforms Melvin from batch-taught to always-on learning. It watches a directory for new `.tch` lesson files, automatically ingests them into the knowledge graph, and periodically snapshots the growing brain.

This is the **live demonstration** of all four v0.9.0 systems working together:
- **Research Infrastructure** (metrics logging)
- **Long-Term Database** (optional integration)
- **In-Memory Learning** (edge growth visible)
- **Teaching System** (automated ingestion)

---

## Quick Start

### 1. Build and Run

```bash
cd /path/to/Melvin/2025-10-11
./scripts/run_continuous_learning.sh
```

This will:
- Create necessary directories (`data/inbox`, `data/processed`, etc.)
- Build the `melvin_learn_watch` executable
- Start the continuous learning loop

### 2. Feed Lessons

In another terminal:

```bash
# Copy a sample lesson into the inbox
cp data/feeds/sample_00_animals_drink.tch data/inbox/

# Or feed multiple lessons
cp data/feeds/*.tch data/inbox/

# Or create your own
echo "#FACT
birds CAN fly

#TEST BirdsAbility
#QUERY What can birds do?
#EXPECT fly" > data/inbox/birds.tch
```

### 3. Watch It Learn

The continuous learner will:
- ✅ Detect the new `.tch` file(s)
- ✅ Parse and ingest them into the graph
- ✅ Move successful lessons to `data/processed/`
- ✅ Move failed lessons to `data/failed/`
- ✅ Print growth statistics
- ✅ Log metrics to `logs/continuous_learning_metrics.csv`
- ✅ Create periodic snapshots in `data/snapshots/`

---

## Expected Output

```
=== Melvin Continuous Learning Mode ===

Building continuous learning watcher...
[Build output...]

Starting watcher (Press Ctrl+C to stop)...
[CL] Continuous Learning Mode Started
[CL] Watching: data/inbox
[CL] Poll interval: 3s
[CL] Snapshot interval: 60s
[CL] Metrics interval: 10s
[CL] Press Ctrl+C to stop

[CL] Loading existing brain state...
[CL] Loaded: 49 nodes, 48 edges

[CL] Processing: sample_00_animals_drink.tch
[TEACHING] Parsing: data/inbox/sample_00_animals_drink.tch
[TEACHING] Ingesting: 5 facts, 2 tests
[TEACHING] Added 6 nodes, 8 edges
[TEACHING] Verification: 2/2 passed
[CL] ✓ Success: sample_00_animals_drink.tch
[CL] Batch complete: 1 files processed

[CL] Processing: sample_01_plants.tch
[TEACHING] Parsing: data/inbox/sample_01_plants.tch
[TEACHING] Ingesting: 4 facts, 2 tests
[TEACHING] Added 5 nodes, 6 edges
[TEACHING] Verification: 2/2 passed
[CL] ✓ Success: sample_01_plants.tch
[CL] Batch complete: 1 files processed

[CL] 📸 Snapshot saved: data/snapshots/brain_2025-10-11_16-42-03.bin
```

---

## Configuration

Edit `config/continuous_learning.yaml`:

```yaml
inbox_dir: "data/inbox"           # Where to watch for new lessons
processed_dir: "data/processed"   # Where successful lessons go
failed_dir: "data/failed"         # Where failed lessons go

poll_seconds: 3                   # How often to check inbox
snapshot_every_seconds: 60        # How often to save brain state
metrics_every_seconds: 10         # How often to log metrics
max_files_per_tick: 4             # Max lessons per poll cycle

# Future features (Patch Pack E)
enable_decay: false               # Edge weight decay over time
enable_srs: false                 # Spaced repetition scheduling

metrics_csv: "logs/continuous_learning_metrics.csv"
```

---

## Metrics

The system logs metrics every 10 seconds (configurable) to a CSV:

**`logs/continuous_learning_metrics.csv`**

```csv
timestamp,tick,files_seen,files_ok,files_failed,nodes,edges
1697052123,20,5,5,0,54,58
1697052133,22,8,8,0,65,74
1697052143,24,8,8,0,65,74
```

**Columns:**
- `timestamp`: Unix timestamp (seconds since epoch)
- `tick`: Internal loop counter
- `files_seen`: Total `.tch` files processed
- `files_ok`: Successfully ingested lessons
- `files_failed`: Failed lessons (parse/ingest errors)
- `nodes`: Current node count in graph
- `edges`: Current edge count in graph

### Plotting Growth

```bash
# Use existing plot_from_db.py or create a quick plot:
python3 << 'EOF'
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("logs/continuous_learning_metrics.csv")
df['time_min'] = (df['timestamp'] - df['timestamp'].min()) / 60

plt.figure(figsize=(10, 5))
plt.subplot(1, 2, 1)
plt.plot(df['time_min'], df['nodes'], label='Nodes')
plt.plot(df['time_min'], df['edges'], label='Edges')
plt.xlabel('Time (minutes)')
plt.ylabel('Count')
plt.title('Graph Growth Over Time')
plt.legend()

plt.subplot(1, 2, 2)
plt.plot(df['time_min'], df['files_ok'], label='Success')
plt.plot(df['time_min'], df['files_failed'], label='Failed')
plt.xlabel('Time (minutes)')
plt.ylabel('Files')
plt.title('Lesson Ingestion')
plt.legend()

plt.tight_layout()
plt.savefig('continuous_learning_growth.png')
print("Saved: continuous_learning_growth.png")
EOF
```

---

## Directory Structure

```
data/
  inbox/            # DROP LESSONS HERE (*.tch files)
  processed/        # Successfully learned lessons
  failed/           # Failed lessons (errors)
  feeds/            # Sample lessons for testing
  snapshots/        # Periodic brain backups
  melvin_brain.bin  # Main brain file (updated every snapshot)

logs/
  continuous_learning_metrics.csv  # Rolling metrics

config/
  continuous_learning.yaml         # Configuration
```

---

## Use Cases

### 1. **Live Training Demo**

Feed lessons one at a time and watch the graph grow in real-time:

```bash
for f in data/feeds/*.tch; do
  cp "$f" data/inbox/
  sleep 5  # Watch it process
done
```

### 2. **Batch Overnight Training**

Drop a large curriculum into the inbox before bed:

```bash
cp curriculum/*.tch data/inbox/
# Let it run overnight
```

### 3. **Continuous Dataset Ingestion**

Use `cron` or a script to periodically add new lessons:

```bash
# crontab -e
0 * * * * /path/to/fetch_new_lessons.sh && cp /path/to/new/*.tch /path/to/Melvin/data/inbox/
```

### 4. **Integration with Data Pipelines**

Pipe web scraping, API responses, or user feedback into `.tch` format and drop into inbox:

```bash
curl https://example.com/knowledge.json | python3 convert_to_tch.py > data/inbox/web_$(date +%s).tch
```

---

## Future Enhancements (Patch Pack E)

When enabled via `enable_decay` and `enable_srs` in config:

### Edge Decay (Ebbinghaus Forgetting)

Edges lose weight over time unless reinforced:

```cpp
void do_decay() {
    // Apply exponential decay to edge weights
    for (auto& edge : G_edges) {
        edge.weight *= 0.99;  // 1% decay per tick
    }
}
```

### Spaced Repetition Scheduling (SRS)

Failed or weak lessons are automatically re-queued:

```cpp
void do_srs() {
    // Check verification results
    // Re-queue failed lessons with increasing intervals
}
```

---

## Troubleshooting

### No Lessons Being Processed

**Check:**
- Is the watcher running? (`ps aux | grep melvin_learn_watch`)
- Are `.tch` files in `data/inbox/`? (`ls data/inbox/`)
- Check permissions: `chmod +x scripts/run_continuous_learning.sh`

### Lessons Moved to `failed/`

**Check:**
- Parse errors: Look at stderr output
- Malformed `.tch` syntax: See `docs/TEACHING_FORMAT.md`
- Missing tags: Every `#QUERY` needs an `#EXPECT`

### Metrics Not Updating

**Check:**
- `logs/` directory exists and is writable
- `metrics_csv` path in config is correct
- Tail the CSV: `tail -f logs/continuous_learning_metrics.csv`

### Brain Snapshots Not Created

**Check:**
- `data/snapshots/` directory exists
- `snapshot_every_seconds` interval in config
- Storage system working: `make test_learning && ./test_learning`

---

## Integration with Research Database

Optional: Log continuous learning runs to the SQLite research database:

```python
# In scripts/log_cl_session.py
import sqlite3
import pandas as pd

df = pd.read_csv("logs/continuous_learning_metrics.csv")
final = df.iloc[-1]

conn = sqlite3.connect("melvin_research.db")
conn.execute("""
    INSERT INTO runs (entry_number, date, tag, nodes_count, edges_count, notes)
    VALUES (?, datetime('now'), 'continuous_learning', ?, ?, 'Automated CL session')
""", (None, int(final['nodes']), int(final['edges'])))
conn.commit()
```

Run after stopping the watcher:

```bash
python3 scripts/log_cl_session.py
```

---

## Safety & Stability

**Safe to run continuously:**
- ✅ Uses only stable v0.9.0 systems
- ✅ No external dependencies
- ✅ Graceful file handling (atomic moves)
- ✅ Error isolation (failed lessons don't crash the loop)
- ✅ Periodic snapshots (no data loss)

**Resource usage:**
- Low CPU (polls every 3s by default)
- Low memory (in-memory graph grows slowly)
- Low disk I/O (snapshots are small, ~15-50 KB)

**Stop anytime:**
- `Ctrl+C` stops gracefully
- All progress saved in last snapshot
- Restart continues from saved state

---

## Summary

**Continuous Learning Mode** is the **live heartbeat** of Melvin's cognitive substrate. It transforms teaching from manual batch operations into a **24/7 always-learning system**.

**To get started:**

```bash
./scripts/run_continuous_learning.sh &
cp data/feeds/*.tch data/inbox/
tail -f logs/continuous_learning_metrics.csv
```

**Watch nodes and edges grow. That's continuous learning. That's Melvin alive. 🧠**

