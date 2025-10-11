# 📚 Master Index - Research-Grade Training Loop

## 🎯 Quick Navigation

**First time here?** → Read **`START_HERE.md`**  
**Ready to train?** → Read **`TRAINING_WORKFLOW.md`**  
**Need a command?** → Check **`QUICK_REFERENCE.txt`**  
**Want best practices?** → Read **`OPERATIONS_GUIDE.md`**

---

## 📦 Complete File Index (24 Files)

### 🟢 Essential Files (Read These First)

1. **`START_HERE.md`** - Your entry point
   - Quick overview of the system
   - First steps to take
   - Tool summary
   - 3-command quick start

2. **`TRAINING_WORKFLOW.md`** - Complete training guide
   - Step-by-step: 0.0 → 0.5 similarity
   - Week-by-week checklist
   - Expected progression timeline
   - Decision trees for each stage

3. **`QUICK_REFERENCE.txt`** - Command cheatsheet
   - All build commands
   - All run commands  
   - Tool purposes
   - Metric targets
   - Keep this handy!

### 🟡 Core Documentation

4. **`OPERATIONS_GUIDE.md`** - Best practices ⭐ NEW
   - The 4 best practices implemented
   - Complete operational cadence
   - Data versioning strategy
   - Git archival workflow
   - Drift detection procedures
   - Scientific protocol

5. **`TRAINING_RHYTHM_GUIDE.md`** - Long-term strategy
   - Progressive 4-layer data strategy
   - Weekly/monthly/quarterly rhythms
   - Milestone definitions
   - Tracking templates
   - Visualization guidance
   - Internal confidence governor concept

6. **`LEAP_DIAGNOSTIC_README.md`** - Complete user manual
   - Detailed command reference
   - Metric interpretation
   - Troubleshooting guide
   - Advanced usage
   - Theory and rationale

7. **`DIAGNOSTIC_SYSTEM_SUMMARY.md`** - Technical architecture
   - Component descriptions
   - Integration points
   - Advanced customization
   - Code walkthrough

### 🔵 Reference Documents

8. **`OPTIMIZED_CONFIG_EXAMPLE.h`** - Example configurations
   - Tuned parameter examples
   - Performance observations
   - Use-case recommendations
   - Tuning history

9. **`DELIVERABLES.md`** - Complete project inventory
   - All files listed
   - Features summary
   - Success criteria
   - Build status

10. **`IMPLEMENTATION_COMPLETE.md`** - Implementation summary
    - What was delivered
    - Build status
    - Quick start
    - File statistics

11. **`COMPLETE_SYSTEM_OVERVIEW.txt`** - Full system overview
    - Complete capabilities
    - Tool reference
    - Expected timelines
    - Status summary

12. **`README.md`** - Original Melvin README
    - Background on Melvin
    - Original architecture

---

## 🛠️ Executable Tools (9 Scripts)

### Core Diagnostic Tools

13. **`run_diagnostic.sh`** - Convenience launcher
    ```bash
    ./run_diagnostic.sh [basic|quick|tune]
    ```
    - Builds if needed
    - Runs diagnostic in chosen mode
    - Shows output files

### Monitoring & Analysis

14. **`weekly_summary.sh`** ⭐ RUN EVERY MONDAY
    ```bash
    ./weekly_summary.sh [weeks]
    ```
    - Rolling averages
    - Trend analysis
    - Milestone detection
    - Recommendations

15. **`plot_progress.py`** ⭐ RUN EVERY FRIDAY
    ```bash
    python3 plot_progress.py [--save]
    ```
    - 4 progress subplots
    - Entropy vs similarity (THE KEY PLOT)
    - Phase transition detection
    - Saves as melvin_progress.png

### Qualitative Verification

16. **`qualitative_check.sh`** ⭐ RUN AT 0.4+ SIMILARITY
    ```bash
    ./qualitative_check.sh
    ```
    - Checks conceptual link strength
    - Tests 10 basic concept pairs
    - Activates automatically at threshold

17. **`test_abstract_prompts.sh`** ⭐ RUN AT 0.5+ SIMILARITY
    ```bash
    ./test_abstract_prompts.sh
    ```
    - Tests higher-order reasoning
    - 10 abstract concept pairs
    - Reveals true conceptual understanding

18. **`log_nearest_neighbors.sh`** ⭐ RUN EVERY 2-3 WEEKS
    ```bash
    ./log_nearest_neighbors.sh [output_file]
    ```
    - Logs nearest-neighbor tables
    - 12 anchor concepts
    - Drift detection
    - Quality assessment

### Data Management

19. **`ingest_dialog_data.sh`** - Batch ingestion pipeline
    ```bash
    ./ingest_dialog_data.sh data.txt [batch_size]
    ```
    - Auto-runs diagnostic every N lines
    - Progress tracking
    - Metric monitoring during ingestion

20. **`version_data.sh`** ⭐ BEST PRACTICE #1
    ```bash
    ./version_data.sh data.txt "Description"
    ```
    - Creates versioned_data/TIMESTAMP/
    - Copies data + metadata
    - Enables correlation analysis

21. **`log_diagnostic_results.sh`** - Auto-logger
    ```bash
    ./log_diagnostic_results.sh layer size "notes"
    ```
    - Appends to diagnostics_history.csv
    - Extracts metrics from reports
    - Shows recent history

### Archival & Reproducibility

22. **`archive_diagnostics.sh`** ⭐ BEST PRACTICE #2
    ```bash
    ./archive_diagnostics.sh "Commit message"
    ```
    - Creates snapshot in diagnostic_snapshots/
    - Commits to Git with metrics
    - Full learning curve preservation

---

## 📊 Data & Tracking Files (4 Files)

23. **`sample_dialog_data.txt`** - Example conversations
    - 34 high-quality exchanges
    - Covers all 10 test concepts
    - Demonstrates proper format

24. **`diagnostics_history.csv`** ⭐ YOUR PRIMARY TRACKING
    - Time-series metrics database
    - 3 baseline entries logged
    - Append after every run
    - Plot with plot_progress.py

25. **`leap_diagnostics.csv`** - Latest test data (generated)
    - Raw diagnostic results
    - 10 rows (one per test pair)
    - Updated each diagnostic run

26. **`leap_tuning_report.md`** - Latest report (generated)
    - Markdown summary
    - Health assessment
    - Recommendations
    - Updated each diagnostic run

---

## 🎯 The Four Best Practices

Based on your excellent guidance:

### 1. Version Your Data ✅
**Tool:** `version_data.sh`  
**Why:** Correlate data changes with metric jumps  
**Result:** `versioned_data/TIMESTAMP/` with full metadata

### 2. Archive Diagnostics Weekly ✅
**Tool:** `archive_diagnostics.sh`  
**Why:** Replay full learning curve later  
**Result:** Git commits + `diagnostic_snapshots/`

### 3. Visual Sanity Check ✅
**Tools:** `plot_progress.py` + `qualitative_check.sh`  
**When:** Curve starts flattening  
**Why:** Ensure meaning tracks with numbers  
**Result:** Verified semantic understanding

### 4. Keep Bias Logs ✅
**Tool:** `log_nearest_neighbors.sh`  
**When:** Every 2-3 weeks  
**Why:** Easiest way to spot drift  
**Result:** `nearest_neighbors_DATE.txt` tables

---

## 📈 The Three Core Metrics

| Metric | Current | Target | Excellent |
|--------|---------|--------|-----------|
| **Entropy Reduction** | 0.000 | ≥0.20 | ≥0.30 |
| **Context Similarity** | 0.000 | ≥0.50 | ≥0.70 |
| **Leap Success Rate** | 0.0% | ≥60% | ≥80% |

**The Key:** Context Similarity = 0.5 is when coherence emerges

---

## 🔄 Your Operational Rhythm

### Daily (During Active Training)
```bash
tail -1 diagnostics_history.csv  # Quick check
make run_diagnostic_quick         # After ingestion
```

### Monday (Every Week)
```bash
./weekly_summary.sh              # Review trends
```

### Wednesday (Mid-Week)
```bash
./qualitative_check.sh           # If > 0.4
```

### Friday (Week-End)
```bash
python3 plot_progress.py --save  # Generate plots
./archive_diagnostics.sh "Week N" # Archive snapshot
./log_nearest_neighbors.sh       # Every 2-3 weeks
```

### Monthly
```bash
make run_diagnostic_auto_tune    # Deep analysis
./test_abstract_prompts.sh       # If > 0.5
```

---

## 📊 Quick Command Reference

```bash
# Build & Run
make diagnostic                  # Build tool
make run_diagnostic              # Full test (10 prompts)
make run_diagnostic_quick        # Quick test (5 prompts)
make run_diagnostic_auto_tune    # Auto-optimize parameters

# Monitor Progress
./weekly_summary.sh              # Weekly rolling averages
python3 plot_progress.py --save  # Visual trends + curve
./qualitative_check.sh           # Conceptual links (≥0.4)
./test_abstract_prompts.sh       # Abstract reasoning (≥0.5)
./log_nearest_neighbors.sh       # Drift detection

# Data Management
./version_data.sh data.txt "Desc" # Version before ingest
./ingest_dialog_data.sh data.txt  # Batch ingest + measure
./log_diagnostic_results.sh ...   # Log to history

# Archival
./archive_diagnostics.sh "Msg"   # Weekly snapshot + Git
```

---

## 🎯 Milestones to Watch

```
0.00 ──→ 0.35 ──→ 0.40 ──→ 0.50 ──→ 0.65 ──→ 0.75+
 ↑        ↑        ↑        ↑        ↑        ↑
NOW    Basic   Quali.  COHERENT Causal  Abstract
      Assoc.   Check   SENTENCES Reason. Mastery
      (2 wk)  (3-4 wk) (4-6 wk)  (8 wk)  (12 wk)
```

---

## 📁 Directory Structure (After Training Begins)

```
2025-10-11/
├─ Core System (6 .h/.cpp files)
├─ Scripts (9 .sh/.py executables)
├─ Documentation (9 .md/.txt files)
├─ Data (sample_dialog_data.txt)
├─ Tracking (diagnostics_history.csv)
│
├─ versioned_data/          (Created by version_data.sh)
│  ├─ 20251013_100000/      [Batch 1]
│  ├─ 20251015_143000/      [Batch 2]
│  └─ ...
│
├─ diagnostic_snapshots/    (Created by archive_diagnostics.sh)
│  ├─ history_20251013.csv
│  ├─ report_20251013.md
│  └─ progress_20251013.png
│
└─ nearest_neighbors_*.txt  (Created by log_nearest_neighbors.sh)
```

---

## 🎓 What Makes This Research-Grade

✅ **Reproducible**
- Every data version tracked
- Every diagnostic archived
- Every parameter logged
- Full Git history

✅ **Measurable**
- Three quantitative metrics
- Time-series tracking
- Statistical analysis
- Trend visualization

✅ **Verifiable**
- Quantitative + qualitative
- Numbers + semantic sense
- Automatic + manual checks
- Cross-validation

✅ **Scientific**
- Hypothesis: Data will improve metrics
- Experiment: Ingest and measure
- Results: Track in CSV + plots
- Iterate: Based on data

✅ **Self-Aware**
- System measures itself
- Detects own transitions
- Optimizes own parameters
- Archives own learning

---

## ✨ The Transformation Complete

### Before (Ad-Hoc)
"I changed something..."
- ❌ No metrics
- ❌ No tracking
- ❌ No proof
- ❌ Guess-and-check

### Now (Research-Grade)
"Version 20251013_143000: Added 10k TinyChat conversations.
 Similarity: 0.22 → 0.30 (+0.08, on track).
 Qualitative: Faint links forming for fire, music.
 Nearest neighbors improving (fire→heat 0.65→0.72).
 Curve shows diagonal rise (still learning phase).
 Estimate 0.5 crossing in 3 weeks."

- ✅ Quantitative metrics
- ✅ Full tracking
- ✅ Visual proof
- ✅ Scientific process

---

## 🚀 Your Next Actions

**Immediate (2 minutes):**
```bash
./weekly_summary.sh           # See current state
cat START_HERE.md             # Read entry guide
```

**This Week:**
1. Gather 10k-20k dialog exchanges
2. Version: `./version_data.sh data.txt "Layer 1"`
3. Ingest: `./ingest_dialog_data.sh ...`
4. Monitor: `./weekly_summary.sh`

**Every Week:**
- Monday: Weekly summary
- Friday: Plot + archive

**At Milestones:**
- 0.40: Run qualitative check
- 0.50: Document breakthrough
- 0.65+: Test abstract reasoning

---

## 🎯 The Goal

**Context Similarity = 0.5**

When you cross it:
- Coherent sentences emerge
- Curve flattens (reasoning begins)
- Numbers + meaning align
- You have proof it's working

---

## 📞 Need Help?

**Command help:** `QUICK_REFERENCE.txt`  
**Training help:** `TRAINING_WORKFLOW.md`  
**Best practices:** `OPERATIONS_GUIDE.md`  
**Technical details:** `LEAP_DIAGNOSTIC_README.md`  
**Architecture:** `DIAGNOSTIC_SYSTEM_SUMMARY.md`

---

## ✅ System Status

**Implementation:** ✅ Complete  
**Testing:** ✅ Verified  
**Documentation:** ✅ Comprehensive  
**Best Practices:** ✅ Implemented  
**Baseline:** ✅ Established  
**Ready:** ✅ Yes

---

**Feed data. Watch graphs. Cross 0.5. Document the moment.**

**Everything you need to observe progress is in place.** 🎯
