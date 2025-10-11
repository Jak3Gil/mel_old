# 🔬 Reproducibility Protocol - Verifiable Experiments

## Overview

Each diagnostic run = **One verifiable experiment**

**Formula:** Diagnostic Report + Git Commit Hash + Data Version = Fully Reproducible Result

---

## 🎯 The Five Elements of Reproducibility

### 1. Control Baseline (Protected)

```bash
./protect_baseline.sh
```

**Creates:** `baseline_control/`
- Original 0.0 / 0.0 / 0% metrics
- Never deleted or modified
- Reference point for all improvements

**Why:** Every experiment needs a control group

### 2. Data Versioning (Correlate Changes)

```bash
./version_data.sh data.txt "Description"
```

**Creates:** `versioned_data/TIMESTAMP/`
- Copy of data file
- Metadata (size, hash, date)
- README with before/after metrics

**Why:** Correlate data changes with metric jumps

### 3. Diagnostic Archival (Full State)

```bash
./archive_diagnostics.sh "Milestone description"
```

**Creates:**
- `diagnostic_snapshots/` with CSV, reports, plots
- Git commit with metrics in message
- Snapshot timestamp

**Why:** Replay full learning curve later

### 4. Milestone Tagging (Key Moments)

```bash
./tag_milestone.sh
# Auto-detects milestone (0.25, 0.35, 0.50, 0.65, 0.75)
```

**Creates:**
- Git annotated tag
- `milestone_snapshots/MILESTONE/`
- MILESTONE.md documentation

**Why:** Mark important transitions, enable checkout

### 5. Qualitative Samples (Human Evidence)

```bash
./collect_qualitative_samples.sh
```

**Creates:** `qualitative_samples/samples_DATE.txt`
- Test prompt results
- Link quality assessment
- Human-readable evidence

**Why:** Numbers alone aren't enough - semantic sense matters

---

## 🔬 Experimental Protocol

### Running One Experiment

**Hypothesis:** Adding 10k TinyChat conversations will improve context similarity by 0.05-0.10

**Procedure:**

```bash
# 1. Record current state (before)
tail -1 diagnostics_history.csv

# 2. Version the data
./version_data.sh tinychat_10k.txt "TinyChat conversations batch 1"

# 3. Ingest data
./ingest_dialog_data.sh versioned_data/TIMESTAMP/data.txt 5000

# 4. Measure results (after)
make run_diagnostic_quick
./log_diagnostic_results.sh "exp_tinychat1" "10k" "TinyChat batch 1"

# 5. Calculate delta
BEFORE=$(tail -2 diagnostics_history.csv | head -1 | cut -d',' -f5)
AFTER=$(tail -1 diagnostics_history.csv | cut -d',' -f5)
DELTA=$(echo "$AFTER - $BEFORE" | bc -l)
echo "Result: Similarity improved by +$DELTA"

# 6. Collect qualitative evidence
./collect_qualitative_samples.sh

# 7. Archive experiment
./archive_diagnostics.sh "Experiment: TinyChat 10k - Δsim +$DELTA"

# 8. Document
git commit -m "Experiment: TinyChat 10k conversations

Hypothesis: Similarity will improve 0.05-0.10
Result: Similarity improved +$DELTA
Conclusion: $([ ... ] && echo 'Confirmed' || echo 'Partial')

Data: versioned_data/TIMESTAMP
Metrics: diagnostics_history.csv entry N"
```

**Result:** Fully documented, reproducible experiment

---

## 📊 Reproducibility Checklist

For each experiment to be reproducible, ensure:

- [ ] Control baseline exists (`baseline_control/`)
- [ ] Data version created (`versioned_data/TIMESTAMP/`)
- [ ] Before metrics recorded
- [ ] Experiment executed (ingestion + diagnostic)
- [ ] After metrics recorded
- [ ] Delta calculated and documented
- [ ] Qualitative samples collected
- [ ] Git commit with full context
- [ ] If milestone, Git tag created

**If all boxes checked:** ✅ Experiment is fully reproducible

---

## 🔄 Reproducing a Past Experiment

### Find Experiment in Git History

```bash
# List all experimental commits
git log --oneline --grep="Experiment:" --grep="Milestone:"

# Find specific similarity level
git log --all --grep="Similarity: 0.4"

# Find by data type
git log --all --grep="TinyChat"
```

### Checkout and Replay

```bash
# 1. Find commit hash
git log --oneline | grep "TinyChat batch 1"
# Example output: a3c9d12 Experiment: TinyChat 10k - Δsim +0.08

# 2. Checkout that state
git checkout a3c9d12

# 3. Find data version from commit message
git show a3c9d12 | grep "Data:"
# Example: Data: versioned_data/20251013_143000

# 4. Re-run diagnostic on that data
make run_diagnostic

# 5. Compare results to archived
diff leap_diagnostics.csv diagnostic_snapshots/leap_diagnostics_20251013_143000.csv

# 6. Return to current
git checkout main
```

**Result:** Exact replication of past experiment

---

## 📈 Curve Shape Anomaly Detection

### What Normal Looks Like

**Early Stage (Weeks 1-3):**
```
Similarity: 0.00 → 0.10 → 0.22 → 0.32  (steady rise)
Entropy:    0.00 → 0.05 → 0.12 → 0.18  (steady rise)
```
Both increasing together = healthy learning

**Transition (Weeks 4-5):**
```
Similarity: 0.32 → 0.41 → 0.48 → 0.52  (still rising)
Entropy:    0.18 → 0.23 → 0.26 → 0.27  (slowing/plateauing)
```
Curve flattening = phase transition = healthy

**Post-Coherence (Weeks 6+):**
```
Similarity: 0.52 → 0.58 → 0.64 → 0.70  (rising)
Entropy:    0.27 → 0.28 → 0.29 → 0.30  (stable)
```
Horizontal movement = reasoning active = healthy

### What Anomalies Look Like

**Sudden Drop:**
```
Similarity: 0.40 → 0.42 → 0.31 ← ⚠️ ANOMALY
```
Causes: Bad data, config reset, normalization issue

**Unexpected Spike:**
```
Entropy: 0.25 → 0.26 → 0.42 ← ⚠️ ANOMALY (when similarity > 0.4)
```
Causes: Lambda decreased, graph degraded, embeddings broken

**Plateau:**
```
Similarity: 0.28 → 0.29 → 0.29 → 0.30 ← ⚠️ ANOMALY (stuck for 4+ runs)
```
Causes: Need new data, learning rate too low

### Detect Automatically

```bash
./detect_anomalies.sh
# Runs every check, reports issues
```

---

## 📝 Experiment Documentation Template

For each significant experiment, create:

```markdown
# Experiment: [NAME]

**Date:** YYYY-MM-DD  
**Git Commit:** `abc123def`  
**Data Version:** `versioned_data/TIMESTAMP`  

## Hypothesis

[What you expect to happen]

## Method

1. Baseline: [Current metrics]
2. Data: [What you're adding]
3. Expected: [Predicted improvement]

## Execution

\`\`\`bash
./version_data.sh data.txt "Description"
./ingest_dialog_data.sh ...
make run_diagnostic_quick
\`\`\`

## Results

| Metric | Before | After | Delta | Target Met? |
|--------|--------|-------|-------|-------------|
| Similarity | X.XXX | X.XXX | +X.XXX | [✓/✗] |
| Entropy | X.XXX | X.XXX | +X.XXX | [✓/✗] |
| Success | XX% | XX% | +XX% | [✓/✗] |

## Qualitative Evidence

[Paste from qualitative_samples/]

## Conclusion

[Hypothesis confirmed/rejected, why, next steps]

## Reproducibility

\`\`\`bash
git checkout abc123def
# Use data from versioned_data/TIMESTAMP
make run_diagnostic
# Should produce same results
\`\`\`
```

---

## 🏷️ Git Tagging Strategy

### Automatic Milestone Tags

```bash
# Auto-tags at thresholds
./tag_milestone.sh

# Creates tags like:
#   milestone_1_associations_0.35
#   milestone_2_coherence_0.50
#   milestone_3_causal_0.65
#   milestone_4_mastery_0.75
```

### Manual Experiment Tags

```bash
# For significant experiments
git tag -a "exp_tinychat_10k" -m "Experiment: TinyChat 10k

Hypothesis: Similarity +0.08
Result: Similarity +0.09 (confirmed)

Data: versioned_data/20251013_143000
Metrics: 0.22 → 0.31 similarity"
```

### List All Tags

```bash
# Show all milestones
git tag -l "milestone_*"

# Show all experiments
git tag -l "exp_*"

# Show tag details
git show milestone_2_coherence_0.50
```

---

## 📊 Complete Reproducibility Example

### Experiment: "First 10k Conversations"

**1. Setup**
```bash
# Version data
./version_data.sh tinychat_10k.txt "First conversation batch"
# Creates: versioned_data/20251013_100000/

# Record baseline
tail -1 diagnostics_history.csv
# Output: 2025-10-12,layer1,5k,0.000,0.000,0.0,...
```

**2. Execute**
```bash
./ingest_dialog_data.sh versioned_data/20251013_100000/data.txt 5000
make run_diagnostic_quick
./log_diagnostic_results.sh "exp1" "10k" "TinyChat batch 1"
```

**3. Results**
```
Before: 0.000 similarity
After:  0.085 similarity
Delta:  +0.085
```

**4. Collect Evidence**
```bash
./collect_qualitative_samples.sh
# Shows: 1 weak link appearing (fire→heat)
```

**5. Archive**
```bash
./archive_diagnostics.sh "Exp1: TinyChat 10k - +0.085 similarity"
git tag -a "exp1_tinychat_10k" -m "First conversation batch"
```

**6. Document**
```
Git Commit: a3c9d12
Data Version: 20251013_100000
Metrics Change: 0.000 → 0.085
Evidence: qualitative_samples/samples_20251013.txt
```

### Reproducing This Experiment (6 Months Later)

```bash
# 1. Checkout experiment state
git checkout exp1_tinychat_10k

# 2. Find data version
git show HEAD | grep "Data:"
# Output: Data: versioned_data/20251013_100000

# 3. Verify data hash
cd versioned_data/20251013_100000
md5sum data.txt
# Should match metadata.json hash

# 4. Re-run diagnostic
cd ../..
make run_diagnostic_quick

# 5. Compare
diff leap_diagnostics.csv diagnostic_snapshots/leap_diagnostics_20251013_100000.csv

# Result: Should be identical (within floating-point precision)
```

---

## 📚 Reproducibility Best Practices

### DO:

✅ **Always version data before ingesting**
```bash
./version_data.sh new_data.txt "Clear description"
```

✅ **Always record before/after metrics**
```bash
# Before
tail -1 diagnostics_history.csv > before.txt
# After  
tail -1 diagnostics_history.csv > after.txt
diff before.txt after.txt
```

✅ **Always collect qualitative samples**
```bash
./collect_qualitative_samples.sh
# Weekly evidence that meaning tracks numbers
```

✅ **Always Git commit with full context**
```bash
git commit -m "Clear description

Before: X.XXX similarity
After: X.XXX similarity
Delta: +X.XXX

Data: versioned_data/TIMESTAMP
Evidence: qualitative_samples/samples_DATE.txt"
```

✅ **Always tag milestones**
```bash
./tag_milestone.sh
# At 0.35, 0.50, 0.65, 0.75
```

### DON'T:

❌ **Never delete baseline_control/**
- Your yardstick for all progress

❌ **Never overwrite versioned_data/**
- Historical record of what data produced what results

❌ **Never commit without metrics in message**
- Future you needs context

❌ **Never skip qualitative samples**
- Numbers without meaning are meaningless

❌ **Never modify archived snapshots**
- Historical integrity matters

---

## 🎓 Research Paper Quality Documentation

### Paper Structure Template

```markdown
# Emergence of Semantic Reasoning in Graph-Guided Neural Architecture

## Abstract

We demonstrate the emergence of conceptual reasoning in Melvin, 
a graph-guided predictive system, by systematically measuring 
three core metrics during progressive data ingestion...

## Methods

### Metrics
- Context Similarity (node-token embedding alignment)
- Entropy Reduction (graph bias effectiveness)  
- Leap Success Rate (conceptual leap quality)

### Data Progression
Layer 1: 50k conversation exchanges (Weeks 1-2)
Layer 2: 100k factual statements (Weeks 3-5)
Layer 3: 30k reasoning chains (Weeks 6-8)

### Experimental Protocol
- Batch size: 5,000 lines
- Measurement: After each batch
- Versioning: All data timestamped and hashed
- Control: Baseline at 0.0/0.0/0%

## Results

[Insert plots from plot_progress.py]

### Phase Transition at Week 5

Context similarity crossed 0.50 threshold in Week 5 after 
ingesting 80k total exchanges. Entropy vs similarity plot 
showed clear curve flattening, indicating transition from 
pattern matching to structured reasoning.

[Include melvin_progress.png]

### Qualitative Validation

Nearest-neighbor tables showed intuitive semantic clustering:
- fire → [heat(0.85), smoke(0.82), burn(0.79)] ✓
- music → [sound(0.87), melody(0.83), emotion(0.79)] ✓

[Include samples from qualitative_samples/]

## Reproducibility

All experiments documented in Git:
- Data versions: versioned_data/
- Metrics: diagnostics_history.csv
- Snapshots: diagnostic_snapshots/
- Code: Git commit abc123def

To reproduce milestone_2_coherence_0.50:
\`\`\`bash
git checkout milestone_2_coherence_0.50
# Follow versioned_data/ sequence
make run_diagnostic
\`\`\`

## Conclusion

The phase transition from guessing to reasoning is measurable,
reproducible, and demonstrable through both quantitative metrics
and qualitative semantic assessment.
```

---

## 📊 Data Lineage Tracking

### Example Lineage

```
Experiment: "Reaching 0.50 Similarity"

├─ Baseline (2025-10-11)
│  ├─ Similarity: 0.000
│  └─ Git: baseline_control/

├─ Layer 1 Batch 1 (2025-10-13)
│  ├─ Data: versioned_data/20251013_100000/ (TinyChat 10k)
│  ├─ Similarity: 0.000 → 0.085 (+0.085)
│  ├─ Git: commit a3c9d12
│  └─ Tag: exp1_tinychat_10k

├─ Layer 1 Batch 2 (2025-10-15)
│  ├─ Data: versioned_data/20251015_143000/ (TinyChat 10k)
│  ├─ Similarity: 0.085 → 0.168 (+0.083)
│  ├─ Git: commit b4e8a23
│  └─ Tag: exp2_tinychat_20k_total

├─ Layer 1 Batch 3 (2025-10-17)
│  ├─ Data: versioned_data/20251017_091500/ (Reddit 15k)
│  ├─ Similarity: 0.168 → 0.298 (+0.130) ← Big jump!
│  ├─ Git: commit c5f9b34
│  └─ Notes: Reddit data very effective

├─ Embedding Training (2025-10-19)
│  ├─ Method: 100 training cycles
│  ├─ Similarity: 0.298 → 0.362 (+0.064)
│  ├─ Git: commit d6g0c45
│  └─ Tag: milestone_1_associations_0.35

├─ Layer 2 Batch 1 (2025-10-22)
│  ├─ Data: versioned_data/20251022_134500/ (Wikipedia 50k)
│  ├─ Similarity: 0.362 → 0.487 (+0.125) ← Big jump!
│  ├─ Git: commit e7h1d56
│  └─ Notes: Approaching coherence threshold

└─ Layer 2 Batch 2 (2025-10-24)
   ├─ Data: versioned_data/20251024_162000/ (Wikipedia 30k)
   ├─ Similarity: 0.487 → 0.523 (+0.036)
   ├─ Git: commit f8i2e67
   └─ Tag: milestone_2_coherence_0.50 🎉
```

**Every step is documented, versioned, and reproducible.**

---

## 🔍 Verification Protocol

### Verify an Old Experiment

```bash
#!/bin/bash
# verify_experiment.sh <commit_hash>

COMMIT="$1"

# Checkout experiment
git checkout "$COMMIT"

# Extract data version from commit
DATA_VERSION=$(git show HEAD | grep "Data:" | cut -d' ' -f2)

# Extract expected metrics
EXPECTED_SIM=$(git show HEAD | grep "Similarity:" | grep -oE '[0-9]+\.[0-9]+' | head -1)

# Re-run diagnostic
make run_diagnostic_quick

# Compare
ACTUAL_SIM=$(tail -1 leap_diagnostics.csv | cut -d',' -f8)

DIFF=$(echo "$ACTUAL_SIM - $EXPECTED_SIM" | bc -l | sed 's/^-//')

if (( $(echo "$DIFF < 0.05" | bc -l) )); then
    echo "✅ VERIFIED: Results within 0.05 (expected: $EXPECTED_SIM, got: $ACTUAL_SIM)"
else
    echo "⚠️  DRIFT: Results differ by $DIFF (expected: $EXPECTED_SIM, got: $ACTUAL_SIM)"
fi

git checkout -
```

---

## 📊 Experiment Log Template

Create `experiment_log.md`:

```markdown
# Melvin Training Experiment Log

## Experiment 1: TinyChat Baseline
- **Date:** 2025-10-13
- **Hypothesis:** 10k conversations → +0.08 similarity
- **Data:** versioned_data/20251013_100000
- **Result:** +0.085 similarity ✅ Confirmed
- **Git:** a3c9d12, tag: exp1_tinychat_10k
- **Notes:** Good baseline, continue

## Experiment 2: TinyChat Scale
- **Date:** 2025-10-15
- **Hypothesis:** +10k more → +0.08 similarity
- **Data:** versioned_data/20251015_143000
- **Result:** +0.083 similarity ✅ Confirmed
- **Git:** b4e8a23
- **Notes:** Consistent, repeatable

## Experiment 3: Reddit Diversity
- **Date:** 2025-10-17
- **Hypothesis:** Different source → +0.10 similarity
- **Data:** versioned_data/20251017_091500
- **Result:** +0.130 similarity ✅ Exceeded!
- **Git:** c5f9b34
- **Notes:** Diversity matters - Reddit more effective than expected

[... continue for each experiment ...]

## Summary

Most Effective Data:
1. Reddit (diverse conversations): +0.130 per 15k
2. TinyChat (focused dialogs): +0.085 per 10k
3. Wikipedia (facts): +0.125 per 50k

Phase Transition:
- Occurred at Week 5
- Similarity: 0.487 → 0.523
- Curve flattening observed
- Coherent sentences emerged

Reproducibility:
- All experiments tagged
- All data versioned
- All states archived
```

---

## ✅ Reproducibility Validation

### Self-Check

Can you answer these for any experiment?

1. **What was the exact data?**
   → Yes: `versioned_data/TIMESTAMP/`

2. **What were the metrics before/after?**
   → Yes: `diagnostics_history.csv` + Git commit

3. **What were the parameters?**
   → Yes: Logged in diagnostics_history.csv

4. **What did the output look like?**
   → Yes: `diagnostic_snapshots/` + `qualitative_samples/`

5. **Can you re-run it exactly?**
   → Yes: `git checkout TAG` + re-run diagnostic

6. **What was the system state?**
   → Yes: Git commit captures code state

**If all YES:** ✅ Fully reproducible research

---

## 🎯 The Complete Research Loop

```
┌─ EXPERIMENT ────────────────────────────────────────────────┐
│  1. Version data:        ./version_data.sh                  │
│  2. Record baseline:     tail -1 diagnostics_history.csv    │
│  3. Execute:             ./ingest_dialog_data.sh            │
│  4. Measure:             make run_diagnostic_quick          │
│  5. Log:                 ./log_diagnostic_results.sh        │
│  6. Collect qualitative: ./collect_qualitative_samples.sh   │
│  7. Archive:             ./archive_diagnostics.sh           │
│  8. Tag if milestone:    ./tag_milestone.sh                 │
│  9. Commit with context: git commit -m "..."                │
│ 10. Document in log:     Update experiment_log.md           │
└──────────────────────────────────────────────────────────────┘

Result: One fully documented, verifiable, reproducible experiment
```

---

## 🏆 Success Criteria for Reproducibility

A training run is **research-grade reproducible** if:

✅ Control baseline is protected and never modified  
✅ All data is versioned with metadata  
✅ All diagnostics archived with timestamps  
✅ All milestones tagged in Git  
✅ All qualitative samples collected  
✅ All commits include metrics in message  
✅ Experiment log documents hypotheses and results  
✅ Any past state can be checked out and re-run  
✅ Numbers + meaning both verified  

**When all criteria met:** You have scientific-grade documentation.

---

## 🎉 What This Achieves

**Each diagnostic report + commit hash + data version = One verifiable experiment.**

You can now:
- Reproduce any past result
- Trace any metric change to specific data
- Replay full learning curve
- Publish with confidence
- Collaborate with full transparency

**This is research-grade measurement.**

Data, patience, and documentation from here.

When you see the first real uptick in context similarity and the first coherent cross-concept sentences, you'll have **measurable, reproducible proof** that the architecture itself—not just the dataset—is producing understanding.

---

**Status:** Reproducibility protocol complete  
**Tools:** All archival and tagging scripts ready  
**Next:** Execute experiments, document everything

**Plan for reproducibility. Execute with rigor. Document the breakthrough.** 🔬

