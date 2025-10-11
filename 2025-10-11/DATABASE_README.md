# Melvin Research Database System

**Durable, Queryable, Long-Term Data Storage for Melvin's Cognitive Research**

---

## 🎯 **Overview**

The Melvin Research Database provides persistent storage for all diagnostic runs, metrics, qualitative samples, and dataset metadata using SQLite. This system is designed for **decades of longevity**, **complete reproducibility**, and **powerful querying**.

### Why SQLite?

- ✅ **Longevity:** Stable since 2000, backward compatible, expected lifespan 50+ years
- ✅ **Portability:** Single file, cross-platform, no server needed
- ✅ **Power:** Full SQL query capabilities, joins, indexes
- ✅ **Simplicity:** Zero configuration, works out of the box
- ✅ **Durability:** ACID compliant, crash-safe
- ✅ **Versionable:** Git-friendly, can be tracked with LFS

---

## 📦 **Components**

### Core Files

1. **`melvin_research.db`** - The main database file (SQLite)
2. **`src/database_manager.py`** - Python API for database operations
3. **`plot_from_db.py`** - Generate visualizations from database
4. **`export_run_to_md.py`** - Export runs to Markdown reports
5. **`archive_database.sh`** - Create timestamped snapshots

### Integration

- **`log_diagnostic_results.sh`** - Logs to both CSV (redundancy) and database (primary)
- **`collect_qualitative_samples.sh`** - Can log samples to database
- All scripts backward compatible with CSV-only mode

---

## 🗄️ **Database Schema**

### Table: `runs`

Main experiment tracking table.

```sql
CREATE TABLE runs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    entry_number INTEGER,
    date TEXT NOT NULL,
    timestamp TEXT NOT NULL,
    git_hash TEXT,
    tag TEXT,
    data_version TEXT,
    data_layer TEXT,
    data_size TEXT,
    similarity_before REAL,
    similarity_after REAL,
    entropy_before REAL,
    entropy_after REAL,
    leap_success_before REAL,
    leap_success_after REAL,
    lambda_graph_bias REAL,
    leap_entropy_threshold REAL,
    learning_rate_embeddings REAL,
    notes TEXT,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP
);
```

### Table: `samples`

Qualitative outputs from diagnostic runs.

```sql
CREATE TABLE samples (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    run_id INTEGER NOT NULL,
    prompt TEXT NOT NULL,
    output TEXT,
    quality TEXT,           -- strong, moderate, weak, none
    entropy REAL,
    similarity REAL,
    comment TEXT,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (run_id) REFERENCES runs(id) ON DELETE CASCADE
);
```

### Table: `datasets`

Data lineage and provenance tracking.

```sql
CREATE TABLE datasets (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    version_path TEXT UNIQUE NOT NULL,
    description TEXT,
    size_lines INTEGER,
    size_bytes INTEGER,
    source TEXT,
    quality TEXT,
    md5_hash TEXT,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP
);
```

---

## 🚀 **Quick Start**

### Initialize Database

```bash
cd /path/to/Melvin/2025-10-11
python3 src/database_manager.py init
```

This creates `melvin_research.db` with the schema.

### Log a Run (Automatically)

The integration is automatic - when you run diagnostics:

```bash
make run_diagnostic_quick
./log_diagnostic_results.sh "layer1" "10k" "First batch"
```

This logs to:
- ✅ `diagnostics_history.csv` (redundant backup)
- ✅ `melvin_research.db` (primary source of truth)

### Query Database

```bash
# Show statistics
python3 src/database_manager.py stats

# Query all runs
python3 src/database_manager.py query

# Query specific runs
python3 src/database_manager.py query "similarity_after >= 0.3"

# Export to JSON
python3 src/database_manager.py export melvin_backup.json
```

### Plot from Database

```bash
# Generate progress plot
python3 plot_from_db.py --save

# Plot specific range
python3 plot_from_db.py --since 2025-10-01 --output recent.png

# Plot parameter evolution
python3 plot_from_db.py --parameters --save

# Plot only Entry 5-10
python3 plot_from_db.py --entry-min 5 --entry-max 10 --save
```

### Export Run to Markdown

```bash
# Export latest run
python3 export_run_to_md.py --latest

# Export specific run
python3 export_run_to_md.py 7

# Export by entry number
python3 export_run_to_md.py --entry 5 -o reports/entry5.md

# Export by tag
python3 export_run_to_md.py --tag milestone1 -o milestone1_report.md
```

### Archive Database

```bash
# Create weekly snapshot
./archive_database.sh "Weekly snapshot before Entry 10"

# View archives
ls -lh archives/

# Query an archive
python3 src/database_manager.py --db archives/melvin_research_20251011_120000/melvin_research.db stats
```

---

## 📊 **Example SQL Queries**

### Direct SQL Access

You can use the `sqlite3` command-line tool:

```bash
sqlite3 melvin_research.db
```

### Query Examples

**1. Top Performing Runs**

```sql
SELECT entry_number, date, similarity_after, leap_success_after
FROM runs
WHERE similarity_after >= 0.5
ORDER BY similarity_after DESC;
```

**2. Progress Over Time**

```sql
SELECT date, 
       similarity_after, 
       entropy_after,
       leap_success_after
FROM runs
ORDER BY date ASC;
```

**3. Strong Quality Samples**

```sql
SELECT prompt, output, quality, comment
FROM samples
WHERE quality = 'strong'
ORDER BY created_at DESC;
```

**4. Run with Best Similarity**

```sql
SELECT *
FROM runs
WHERE similarity_after = (SELECT MAX(similarity_after) FROM runs);
```

**5. Samples from Specific Run**

```sql
SELECT prompt, output, quality
FROM samples
WHERE run_id = 7;
```

**6. Average Metrics by Data Layer**

```sql
SELECT data_layer,
       AVG(similarity_after) as avg_similarity,
       AVG(entropy_after) as avg_entropy,
       COUNT(*) as num_runs
FROM runs
GROUP BY data_layer;
```

**7. Parameter Evolution**

```sql
SELECT date, 
       lambda_graph_bias,
       leap_entropy_threshold,
       learning_rate_embeddings
FROM runs
ORDER BY date ASC;
```

**8. Recent Runs with Tags**

```sql
SELECT entry_number, date, tag, similarity_after
FROM runs
WHERE tag IS NOT NULL AND tag != ''
ORDER BY date DESC
LIMIT 10;
```

**9. Dataset Correlation**

```sql
SELECT r.entry_number, 
       r.date,
       r.similarity_after,
       d.description,
       d.size_lines
FROM runs r
LEFT JOIN datasets d ON r.data_version = d.version_path
WHERE r.similarity_after >= 0.3;
```

**10. Quality Distribution**

```sql
SELECT quality, COUNT(*) as count
FROM samples
GROUP BY quality
ORDER BY 
    CASE quality
        WHEN 'strong' THEN 1
        WHEN 'moderate' THEN 2
        WHEN 'weak' THEN 3
        ELSE 4
    END;
```

---

## 🔍 **Python API Usage**

### Basic Usage

```python
from src.database_manager import MelvinResearchDB

# Connect
db = MelvinResearchDB()

# Log a run
run_id = db.log_run({
    'entry_number': 2,
    'date': '2025-10-11',
    'git_hash': 'abc123',
    'tag': 'entry2',
    'data_version': 'versioned_data/20251011_120000',
    'data_layer': 'layer1',
    'data_size': '10k',
    'similarity_before': 0.0,
    'similarity_after': 0.218,
    'entropy_before': 0.0,
    'entropy_after': 0.082,
    'leap_success_before': 0.0,
    'leap_success_after': 26.0,
    'lambda_graph_bias': 0.20,
    'leap_entropy_threshold': 0.40,
    'learning_rate_embeddings': 0.010,
    'notes': 'First real data batch - TinyChat conversations'
})

# Log samples
db.log_sample(run_id, 'fire→water', 'heat, steam, cool', 'weak', 
              entropy=0.08, similarity=0.22, 
              comment='First conceptual link emerging')

# Query runs
runs = db.fetch_runs("similarity_after >= 0.3", limit=10)
for run in runs:
    print(f"Entry {run['entry_number']}: {run['similarity_after']:.3f}")

# Get statistics
stats = db.get_stats()
print(f"Total runs: {stats['total_runs']}")
print(f"Best similarity: {stats['best_similarity']:.3f}")

# Close connection
db.close()
```

### Context Manager (Recommended)

```python
with MelvinResearchDB() as db:
    runs = db.fetch_runs()
    # Connection automatically closed
```

---

## 📈 **Visualization Examples**

### Standard Progress Plot

```bash
python3 plot_from_db.py --save
```

Generates `melvin_progress_db.png` with 4 panels:
- Entropy Reduction over time
- Context Similarity over time
- Leap Success Rate over time
- **Phase Diagram** (Entropy vs Similarity) - THE KEY PLOT

### Custom Queries

```bash
# Only high-performing runs
python3 plot_from_db.py --query "similarity_after >= 0.4" --save

# Recent runs only
python3 plot_from_db.py --since 2025-10-15 --save

# Specific entry range
python3 plot_from_db.py --entry-min 5 --entry-max 15 --save
```

---

## 🗄️ **Archival Strategy**

### Weekly Snapshots

Create archives regularly:

```bash
./archive_database.sh "End of Week 2"
```

This creates:
```
archives/melvin_research_20251011_151500/
├── melvin_research.db    # Database snapshot
├── metadata.json         # Machine-readable info
└── README.md            # Human-readable info
```

And compresses to:
```
archives/melvin_research_20251011_151500.tar.gz
```

### Offsite Backup

```bash
# Option 1: Git LFS
git lfs track 'archives/*.tar.gz'
git add archives/*.tar.gz
git commit -m "Weekly database archive"
git push

# Option 2: Cloud storage
rclone copy archives/ remote:melvin-archives/

# Option 3: Simple copy
cp -r archives/ /backup/location/
```

### Restoring from Archive

```bash
# Backup current
mv melvin_research.db melvin_research.db.backup

# Restore from archive
cp archives/melvin_research_20251011_151500/melvin_research.db ./

# Verify
python3 src/database_manager.py stats
```

---

## 🔄 **Migration from CSV**

If you have existing `diagnostics_history.csv` data, you can migrate it:

```python
import csv
from src.database_manager import MelvinResearchDB

with MelvinResearchDB() as db:
    with open('diagnostics_history.csv', 'r') as f:
        reader = csv.DictReader(f)
        for i, row in enumerate(reader, start=1):
            db.log_run({
                'entry_number': i,
                'date': row['date'],
                'git_hash': 'migrated',
                'data_layer': row['data_layer'],
                'data_size': row['data_size'],
                'similarity_before': 0.0,
                'similarity_after': float(row['context_similarity']),
                'entropy_before': 0.0,
                'entropy_after': float(row['entropy_reduction']),
                'leap_success_before': 0.0,
                'leap_success_after': float(row['leap_success_rate']) * 100,
                'lambda_graph_bias': float(row['lambda']),
                'leap_entropy_threshold': float(row['threshold']),
                'learning_rate_embeddings': float(row['learning_rate']),
                'notes': row['notes']
            })

print("✅ Migration complete")
```

---

## 📊 **Database Size**

Expected sizes:
- Empty database: ~16 KB
- 10 runs: ~20 KB
- 100 runs: ~50 KB
- 1000 runs: ~200 KB
- 10000 runs: ~2 MB

Qualitative samples add minimal overhead (~500 bytes per sample).

**Conclusion:** Database stays lightweight even after years of use.

---

## 🛠️ **Maintenance**

### Vacuum Database (Reclaim Space)

```bash
sqlite3 melvin_research.db "VACUUM;"
```

### Check Integrity

```bash
sqlite3 melvin_research.db "PRAGMA integrity_check;"
```

### Backup (Hot Copy)

```bash
# While database is in use
sqlite3 melvin_research.db ".backup melvin_backup_$(date +%Y%m%d).db"
```

### Export Full Database

```bash
# To SQL
sqlite3 melvin_research.db .dump > melvin_research.sql

# To JSON (via Python)
python3 src/database_manager.py export melvin_research.json
```

---

## 🔐 **Best Practices**

### DO ✅

- Archive weekly (automated with cron)
- Query database for analysis instead of CSV
- Use database for long-term storage
- Export runs to Markdown for papers
- Tag important milestones in the `tag` field
- Keep Git hash in every run for reproducibility

### DON'T ❌

- Manually edit the database (use API)
- Delete old runs (archive instead)
- Commit raw database to Git (use LFS or archives)
- Ignore archival (data loss risk)
- Skip Git hashes (breaks reproducibility)

---

## 🎯 **Success Criteria Verification**

Check that the system meets all requirements:

```bash
# 1. Every diagnostic run inserts to runs table
make run_diagnostic_quick
./log_diagnostic_results.sh "test" "100" "Verification run"
sqlite3 melvin_research.db "SELECT COUNT(*) FROM runs;"

# 2. Qualitative samples logged
sqlite3 melvin_research.db "SELECT COUNT(*) FROM samples;"

# 3. Plots from database work
python3 plot_from_db.py --save

# 4. Database size reasonable
ls -lh melvin_research.db

# 5. Queries return correct results
python3 src/database_manager.py query "similarity_after >= 0.0"
```

---

## 📚 **Additional Resources**

- **SQLite Documentation:** https://www.sqlite.org/docs.html
- **SQL Tutorial:** https://www.sqltutorial.org/
- **Python sqlite3:** https://docs.python.org/3/library/sqlite3.html

---

## 🎓 **Example Workflow**

### Complete Research Cycle

```bash
# 1. Run diagnostic
make run_diagnostic_quick

# 2. Log results (auto-logs to database)
./log_diagnostic_results.sh "layer1" "10k" "Entry 2: TinyChat batch"

# 3. Query database
python3 src/database_manager.py stats

# 4. Generate plots
python3 plot_from_db.py --save

# 5. Export latest run to Markdown
python3 export_run_to_md.py --latest -o reports/entry2.md

# 6. Weekly archive
./archive_database.sh "End of Week 1"

# 7. Commit everything
git add melvin_research.db reports/entry2.md
git commit -m "Entry 2: Similarity +0.22, database updated"
```

---

## 🔬 **Longevity Guarantee**

This database system is designed for **decades of use**:

- ✅ SQLite format stable since 2000
- ✅ Backward compatible (can read 20-year-old databases)
- ✅ Forward compatible (databases readable by future versions)
- ✅ Self-contained (all data in one file)
- ✅ Well-documented (this README, code comments)
- ✅ Open standard (not proprietary)
- ✅ Widely supported (Python, C, Java, every language)

**Expected lifespan: 50+ years minimum**

---

**Database system created: October 11, 2025**  
**Author: AI Research Infrastructure**  
**Version: 1.0**

---

╔═══════════════════════════════════════════════════════════════╗
║  Database for decades. Queries in seconds. Science forever.  ║
╚═══════════════════════════════════════════════════════════════╝

