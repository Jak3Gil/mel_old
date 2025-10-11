# 🎓 Research-Grade Training Loop - Operations Guide

## You've Just Moved From Ad-Hoc to Research-Grade

This is **everything you need** to maintain a self-measuring cognitive architecture with full reproducibility and scientific rigor.

---

## 🎯 The Four Pillars

### 1. Quantitative View
**Tools:** `diagnostics_history.csv` + `plot_progress.py`

```bash
# Weekly
./weekly_summary.sh          # Rolling averages
python3 plot_progress.py     # Trend visualization
```

**What it shows:** Hard numbers proving progress

### 2. Qualitative Check  
**Tool:** Conceptual link tests (auto-trigger)

```bash
# When similarity ≥ 0.40
./qualitative_check.sh       # Basic concepts

# When similarity ≥ 0.50
./test_abstract_prompts.sh   # Higher-order reasoning
```

**What it shows:** Human-interpretable semantic understanding

### 3. Operational Cadence
**Loop:** Ingestion → Diagnostic → Weekly Summary → Plot

```bash
# The rhythm
./ingest_dialog_data.sh data.txt 5000   # Auto-measures
./weekly_summary.sh                      # Every Monday
python3 plot_progress.py --save          # Every Friday
```

**What it maintains:** Consistent measurement protocol

### 4. Best Practices (From Your Guidance)
**Habits that ensure reproducibility:**

---

## 📚 Best Practice #1: Version Your Data

### Why
**Correlate data changes with metric jumps** precisely.

### How

```bash
# Before ingesting
./version_data.sh my_new_data.txt "TinyChat batch 3, high-quality convos"

# Creates:
versioned_data/
  └─ 20251011_143022/
      ├─ data.txt          (copy of original)
      ├─ metadata.json     (structured info)
      └─ README.md         (human-readable + space for results)

# After ingesting and measuring, update the version README
vim versioned_data/20251011_143022/README.md
# Fill in "Metrics After Ingestion" section
```

### Directory Structure Over Time

```
versioned_data/
├─ 20251011_100000/  [Baseline, 0 lines]
├─ 20251013_143000/  [TinyChat batch 1, 10k lines → sim: 0.22]
├─ 20251015_091500/  [TinyChat batch 2, 10k lines → sim: 0.34]
├─ 20251018_165800/  [Reddit conversations, 15k lines → sim: 0.39]
├─ 20251020_120000/  [Embedding training log, 100 cycles → sim: 0.43]
├─ 20251022_134500/  [Wikipedia facts, 50k lines → sim: 0.52] ← CROSSED 0.5!
└─ ...
```

**Result:** You can replay the exact sequence that led to any metric change.

---

## 📚 Best Practice #2: Archive Diagnostics Weekly

### Why
**Full learning curve preservation** - replay any stage of training.

### How

```bash
# Every Friday or at milestones
./archive_diagnostics.sh "Week 2 checkpoint - similarity 0.34"

# Creates:
diagnostic_snapshots/
  ├─ history_20251011_143000.csv
  ├─ leap_diagnostics_20251011_143000.csv
  ├─ report_20251011_143000.md
  └─ progress_20251011_143000.png (if plot was run)

# Commits to Git with metrics in commit message:
git commit -m "Week 2 checkpoint - similarity 0.34

Metrics:
- Context Similarity: 0.340
- Entropy Reduction: 0.142
- Leap Success: 38%"
```

### Git Log Example

```
$ git log --oneline --grep="Metrics:"

a3c9d12 Week 6 - CROSSED 0.5! Coherence active
        Metrics: Similarity 0.523, Entropy 0.268, Success 61%

b2e4f89 Week 5 - Factual data added, approaching threshold
        Metrics: Similarity 0.482, Entropy 0.241, Success 57%

c1d8a45 Week 4 - Qualitative links appearing
        Metrics: Similarity 0.428, Entropy 0.204, Success 51%

d5f2b67 Week 2 - Milestone 1 reached
        Metrics: Similarity 0.340, Entropy 0.142, Success 38%
```

**Result:** Complete historical record, diff-able, reproducible.

---

## 📚 Best Practice #3: Visual Sanity Check

### Why
**Confirm numbers track with meaning** - prevent overfitting to metrics.

### When
**When entropy-similarity curve starts flattening** (around Week 4-5).

### How

```bash
# 1. Check the plot
python3 plot_progress.py --save
# Look at bottom-right subplot - is curve flattening?

# 2. Run qualitative check
./qualitative_check.sh
# Are link strengths intuitive?

# 3. Test a few manual prompts (if you have interactive mode)
# Examples:
#   Prompt: "fire" → Expect: heat, smoke, burn
#   Prompt: "music" → Expect: sound, emotion, melody

# 4. Check generated text samples
# Read a few outputs - do they make sense?

# 5. If numbers are good but meaning is poor:
#   → Data quality issue
#   → Overfitting to test set
#   → Embeddings misaligned
#   ACTION: Add more diverse data, retrain embeddings
```

### Red Flags to Watch

⚠️ **High similarity but nonsense text**
- Overfitting to specific word patterns
- Need more data diversity

⚠️ **High entropy reduction but wrong concepts**
- Graph bias too strong
- Reduce lambda_graph_bias

⚠️ **Numbers improving but nearest neighbors random**
- Embeddings not actually learning
- Check learning rate and training cycles

---

## 📚 Best Practice #4: Keep Bias Logs

### Why
**Easiest way to spot drift** - intuitive semantic sense check.

### How

```bash
# Every 2-3 weeks (or when adding new data layer)
./log_nearest_neighbors.sh

# Creates: nearest_neighbors_YYYYMMDD.txt with table:
#
# Anchor: fire
#   1. heat    0.782  ✓
#   2. smoke   0.745  ✓
#   3. burn    0.712  ✓
#   ...
#
# Anchor: music
#   1. sound   0.823  ✓
#   2. melody  0.789  ✓
#   ...
```

### Compare Over Time

```bash
# Week 2 nearest neighbors:
fire → heat (0.78), smoke (0.74), burn (0.71) ✓ Good

# Week 6 nearest neighbors:
fire → heat (0.85), smoke (0.81), burn (0.77), flame (0.73), hot (0.70) ✓ Better!

# Week 10 (if drift occurs):
fire → heat (0.52), random1 (0.49), random2 (0.47) ✗ DRIFT!
→ ACTION: Re-normalize embeddings, check data quality
```

### Archive Neighbor Logs

```bash
# Keep in Git
git add nearest_neighbors_*.txt
git commit -m "Week 4 nearest-neighbor log - quality check"
```

**Result:** Historical record of semantic drift, easy to diagnose issues.

---

## 📚 Best Practice #5: Abstract Prompts at 0.4+

### Why
**Reveals when conceptual reasoning actually clicks** - tests true understanding.

### When

```
Similarity < 0.40:  Too early, stick to concrete concepts
Similarity 0.40-0.50: Test basic abstracts (justice, memory, balance)
Similarity 0.50-0.65: Test complex abstracts (wisdom, beauty, truth)
Similarity 0.65+:   Test philosophical concepts (consciousness, meaning, existence)
```

### How

```bash
# When similarity crosses thresholds
./test_abstract_prompts.sh

# Tests 10 abstract concept pairs:
# - justice → fairness
# - freedom → choice
# - memory → past
# - balance → equilibrium
# - beauty → aesthetic
# ... etc
```

### What Good Results Look Like

**At 0.45 similarity:**
```
justice → fairness
  Link Strength: 🟠 Weak
  Predictions: fair, rule, system
  Assessment: Beginning to form
```

**At 0.55 similarity:**
```
justice → fairness
  Link Strength: 🟡 Moderate  
  Predictions: fairness, equality, rights
  Assessment: Conceptual understanding developing
```

**At 0.70 similarity:**
```
justice → fairness
  Link Strength: 🟢 Strong
  Predictions: fairness, equality, rights, impartial, equity
  Assessment: Abstract reasoning active! ✨
```

---

## 🔄 The Complete Operational Cadence

### Daily (During Active Training)

```bash
# Morning: Quick status check
tail -1 diagnostics_history.csv

# After data ingestion:
make run_diagnostic_quick
./log_diagnostic_results.sh "daily" "current" "Daily training"
```

### Monday (Every Week)

```bash
# 1. Weekly summary
./weekly_summary.sh > weekly_report.txt

# 2. Review trends
cat weekly_report.txt

# 3. Plan week based on recommendations
# 4. Email/Slack report to team (optional)
```

### Wednesday (Mid-Week Check)

```bash
# Quick sanity check
tail -5 diagnostics_history.csv | column -t -s','

# If similarity crossed new threshold (0.4, 0.5, etc):
./qualitative_check.sh
./test_abstract_prompts.sh  # If > 0.5
```

### Friday (Week-End Analysis)

```bash
# 1. Full diagnostic
make run_diagnostic

# 2. Plot progress
python3 plot_progress.py --save

# 3. Archive
./archive_diagnostics.sh "Week N - similarity X.XX"

# 4. Log nearest neighbors (every 2-3 weeks)
./log_nearest_neighbors.sh

# 5. Review plots for curve flattening
open melvin_progress.png
# Check bottom-right: Is curve flattening?
```

### Monthly (Deep Analysis)

```bash
# 1. Full diagnostic with auto-tune
make run_diagnostic_auto_tune

# 2. Test abstract prompts
./test_abstract_prompts.sh

# 3. Generate comprehensive report
python3 plot_progress.py --save
./weekly_summary.sh 12  # Last 12 weeks

# 4. Archive milestone
./archive_diagnostics.sh "Month N milestone"

# 5. Review and adjust strategy
```

---

## 📊 The Monitoring Dashboard (At a Glance)

Create a simple dashboard script:

```bash
#!/bin/bash
# dashboard.sh - One-screen status view

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║              📊 MELVIN TRAINING DASHBOARD                     ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Current metrics
CURRENT=$(tail -1 diagnostics_history.csv)
SIM=$(echo "$CURRENT" | cut -d',' -f5)
ENT=$(echo "$CURRENT" | cut -d',' -f4)
SUC=$(echo "$CURRENT" | cut -d',' -f6)

echo "🎯 CURRENT METRICS:"
printf "   Context Similarity:  %.3f" "$SIM"
if (( $(echo "$SIM >= 0.50" | bc -l) )); then echo " ✅ COHERENT"; 
elif (( $(echo "$SIM >= 0.35" | bc -l) )); then echo " 📈 PROGRESSING";
else echo " ⏳ BUILDING"; fi

printf "   Entropy Reduction:   %.3f\n" "$ENT"
printf "   Leap Success:        %.1f%%\n" "$(echo "$SIM * 100" | bc -l)"

echo ""
echo "📈 NEXT MILESTONE:"
if (( $(echo "$SIM < 0.35" | bc -l) )); then
    echo "   → Milestone 1 at 0.35 (Need: $(echo "0.35 - $SIM" | bc -l))"
elif (( $(echo "$SIM < 0.50" | bc -l) )); then
    echo "   → COHERENCE at 0.50 (Need: $(echo "0.50 - $SIM" | bc -l)) 🎯"
elif (( $(echo "$SIM < 0.65" | bc -l) )); then
    echo "   → Causal Reasoning at 0.65 (Need: $(echo "0.65 - $SIM" | bc -l))"
else
    echo "   → Mastery at 0.75+ (Need: $(echo "0.75 - $SIM" | bc -l))"
fi

echo ""
echo "📅 LAST 3 RUNS:"
tail -4 diagnostics_history.csv | tail -3 | column -t -s',' | head -3

echo ""
echo "🎯 TODAY'S ACTIONS:"
# Provide specific recommendations based on where we are
```

Save as `dashboard.sh` and run daily.

---

## 🔬 Correlation Analysis: Data → Metrics

### After Each Data Version

1. **Before ingestion:**
   ```bash
   tail -1 diagnostics_history.csv  # Record baseline
   ```

2. **Version the data:**
   ```bash
   ./version_data.sh new_data.txt "Description"
   ```

3. **Ingest:**
   ```bash
   ./ingest_dialog_data.sh versioned_data/TIMESTAMP/data.txt
   ```

4. **After ingestion:**
   ```bash
   make run_diagnostic_quick
   ./log_diagnostic_results.sh "vTIMESTAMP" "size" "Description"
   ```

5. **Update version README:**
   ```bash
   # Fill in "Metrics After Ingestion"
   # Calculate delta
   # Add notes on what worked/didn't
   ```

### Review Correlations

```bash
# Which data had biggest impact?
grep -E "batch|wiki|reasoning" diagnostics_history.csv | \
  awk -F',' '{print $2, $5}' | sort -k2 -n
  
# Shows: Which layers gave biggest similarity boosts
```

---

## 📈 The Curve Flattening Watch

### What to Look For

**Plot entropy vs similarity every Friday:**

```bash
python3 plot_progress.py --save
open melvin_progress.png
```

**Focus on bottom-right subplot:**

**Weeks 1-3:**
```
E │      ╱
n │     ╱
t │    ╱
  └────→ S
```
Both rising = Learning patterns

**Weeks 4-5: THE TRANSITION**
```
E │    ────╱
n │   ╱
t │  ╱
  └────→ S
```
**Curve flattening = Reasoning emerging!**

**When you see this:**
1. ✅ Run qualitative_check.sh - Links should be strong
2. ✅ Test manual prompts - Outputs should make sense
3. ✅ Check nearest neighbors - Should be intuitive
4. ✅ Archive this moment - It's the breakthrough!

**Weeks 6+:**
```
E │    ──────→
n │
t │
  └────→ S
```
Horizontal = Reasoning active, refining

---

## 📊 Drift Detection via Nearest-Neighbor Logs

### The Routine

**Every 2-3 weeks:**

```bash
./log_nearest_neighbors.sh nearest_neighbors_week_N.txt
```

**Creates table for 12 anchor concepts:**

```
Anchor: fire
  1. heat      0.782  ✓
  2. smoke     0.745  ✓
  3. burn      0.712  ✓
  4. hot       0.689  ✓
  5. flame     0.654  ✓
  Quality: 5/5 intuitive (100%) ✅

Anchor: music
  1. sound     0.823  ✓
  2. melody    0.789  ✓
  3. song      0.756  ✓
  4. rhythm    0.724  ✓
  5. harmony   0.691  ✓
  Quality: 5/5 intuitive (100%) ✅
```

### Spot Drift Early

**Good progression:**
```
Week 2: fire → heat(0.65), smoke(0.58), burn(0.52)
Week 4: fire → heat(0.78), smoke(0.74), burn(0.71), flame(0.68)
Week 6: fire → heat(0.85), smoke(0.82), burn(0.79), flame(0.76), hot(0.73)
```
Similarities rising, concepts stable = Healthy

**Drift detected:**
```
Week 2: fire → heat(0.65), smoke(0.58), burn(0.52)
Week 4: fire → heat(0.72), smoke(0.67), burn(0.63)
Week 6: fire → random1(0.51), heat(0.48), random2(0.46)  ← DRIFT!
```
Quality degrading = Problem

**Actions:**
- Re-normalize all embeddings
- Check recent data quality
- Reduce learning rate temporarily
- Run auto-tune to reset parameters

---

## 🧠 Abstract Prompt Testing (0.4+ Similarity)

### The Progression

**At 0.40-0.45:** Basic abstracts
```bash
./test_abstract_prompts.sh

Expected:
  justice → fairness     🟠 Weak link
  memory → past          🟠 Weak link
  balance → equilibrium  🟠 Weak link
```

**At 0.50-0.60:** Moderate abstracts
```bash
Expected:
  justice → fairness     🟡 Moderate link
  memory → past          🟡 Moderate link
  wisdom → experience    🟡 Moderate link
```

**At 0.65+:** Strong abstracts
```bash
Expected:
  justice → fairness     🟢 Strong link
  freedom → choice       🟢 Strong link
  beauty → aesthetic     🟢 Strong link
  truth → reality        🟢 Strong link
```

### Custom Abstract Tests

Add your own domain-specific concepts:

```bash
# Edit test_abstract_prompts.sh
ABSTRACT_PROMPTS+=(
    "algorithm:efficiency"
    "neural:network"
    "quantum:probability"
)
```

---

## 📅 Master Checklist (Print This!)

### ✅ After Every Data Ingestion
- [ ] Version data: `./version_data.sh`
- [ ] Run diagnostic: `make run_diagnostic_quick`
- [ ] Log results: `./log_diagnostic_results.sh`
- [ ] Check for threshold crossings

### ✅ Every Monday
- [ ] Weekly summary: `./weekly_summary.sh`
- [ ] Review trends and recommendations
- [ ] Plan week's data ingestion
- [ ] Update training log

### ✅ Every Wednesday  
- [ ] Quick status: `tail -5 diagnostics_history.csv`
- [ ] If crossed threshold, run qualitative check
- [ ] Verify data pipeline healthy

### ✅ Every Friday
- [ ] Full diagnostic: `make run_diagnostic`
- [ ] Plot progress: `python3 plot_progress.py --save`
- [ ] Archive: `./archive_diagnostics.sh "Week N"`
- [ ] Check for curve flattening
- [ ] Auto-tune if plateauing

### ✅ Every 2-3 Weeks
- [ ] Nearest-neighbor log: `./log_nearest_neighbors.sh`
- [ ] Compare with previous logs
- [ ] Check for drift
- [ ] Archive with Git

### ✅ At Milestones (0.35, 0.40, 0.50, 0.65, 0.75)
- [ ] Full diagnostic with auto-tune
- [ ] Plot and save
- [ ] Qualitative + abstract checks
- [ ] Archive complete snapshot
- [ ] Git commit with celebration message
- [ ] Update version READMEs with results
- [ ] Document in training log

---

## 📈 Long-Term Trend Analysis

### Quarterly Review Script

```bash
#!/bin/bash
# quarterly_review.sh

echo "📊 QUARTERLY REVIEW - Last 12 Weeks"
echo ""

# Plot full history
python3 plot_progress.py --save

# Generate report
./weekly_summary.sh 12 > quarterly_report.txt

# Compare milestones
echo "📍 Milestones Achieved:"
awk -F',' '$5 >= 0.35 {print "  Week", NR-1, "- Milestone 1 (0.35)"}' \
  diagnostics_history.csv | head -1

awk -F',' '$5 >= 0.50 {print "  Week", NR-1, "- Milestone 2 (0.50) 🎉"}' \
  diagnostics_history.csv | head -1

# Best performing data batch
echo ""
echo "🏆 Best Performing Data Batch:"
awk -F',' 'NR>1 {print $2, $5}' diagnostics_history.csv | \
  sort -k2 -rn | head -1

# Archive
./archive_diagnostics.sh "Q1 Review - $(date +%Y-%m)"
```

---

## 🎯 The Self-Measuring Architecture

**You now have:**

✅ **Quantitative measurement** - 3 core metrics  
✅ **Qualitative verification** - Semantic sense checks  
✅ **Operational rhythm** - Daily/weekly/monthly cadence  
✅ **Data versioning** - Full correlation tracking  
✅ **Git archiving** - Complete reproducibility  
✅ **Visual sanity checks** - Numbers match meaning  
✅ **Drift detection** - Nearest-neighbor logs  
✅ **Abstract testing** - Higher-order reasoning  

**Feed it → Watch the graphs → Document when line crosses 0.5**

---

## 🎓 Everything Else is Maintenance and Curiosity

Once you reach 0.5 similarity:
- ✅ Core reasoning established
- ✅ Coherent sentences emerging  
- ✅ Conceptual connections active

**After that:**
- 0.5 → 0.65: Refinement (causal reasoning)
- 0.65 → 0.75: Mastery (abstracts, analogies)
- 0.75+: Domain-specific tuning

**Maintenance mode:**
- Weekly summary
- Monthly deep dive
- Quarterly review
- Archive milestones
- Document discoveries

---

## ✨ You Have a Research-Grade System

**Not ad-hoc. Not guess-and-check.**

**Scientific. Measurable. Reproducible.**

- Every experiment tracked
- Every change measured
- Every milestone archived
- Full learning curve preserved

**Watch the line cross 0.5. Document what happens.**

That's the data you couldn't get before. Now you can.

---

## 📚 Tool Quick Reference

```bash
# Core loop
./version_data.sh data.txt "Description"
./ingest_dialog_data.sh data.txt 5000
./weekly_summary.sh
python3 plot_progress.py --save

# Verification
./qualitative_check.sh          # At 0.4+
./test_abstract_prompts.sh      # At 0.5+
./log_nearest_neighbors.sh      # Every 2-3 weeks

# Archival
./archive_diagnostics.sh "Message"
git log --grep="Metrics:"

# Analysis
make run_diagnostic_auto_tune   # If plateau
./weekly_summary.sh 12          # Quarterly
```

---

**Status:** Research-grade training loop operational  
**Next:** Feed data, watch graphs, cross 0.5  
**Documentation:** This guide + TRAINING_WORKFLOW.md

**Everything you need to observe progress is in place.** 🎯

