# 🚀 Execution Guide - First Research-Grade Run

## Your First Proper Research Run

This guide walks you through launching your first complete diagnostic cycle.

---

## 🎯 Pre-Flight Check

```bash
# 1. Confirm you're in the project root
cd ~/Desktop/Melvin/Melvin/2025-10-11
pwd
# Should show: /Users/jakegilbert/Desktop/Melvin/Melvin/2025-10-11

# 2. Check that all scripts are executable
ls -l *.sh *.py | grep -E "^-rwx"
# Should see 13 executable scripts

# 3. Verify baseline is protected
ls -l baseline_control/
# Should see read-only files (r--r--r--)
```

---

## 🚀 Run Sequence (First Time)

### Step 1: Clean Build

```bash
# Clean previous builds
make clean

# Build everything fresh
make all
```

**Expected output:**
```
g++ -std=c++20 ... (compilation messages)
✅ Build successful
```

**If errors:** Check that you have g++ with C++20 support

---

### Step 2: Run Full Diagnostic

```bash
make run_diagnostic
```

**Expected output:**
```
╔═══════════════════════════════════════════════════════════════╗
║  MELVIN GRAPH-GUIDED PREDICTIVE SYSTEM DIAGNOSTICS            ║
╚═══════════════════════════════════════════════════════════════╝

🔧 Initializing test graph...
✅ Test graph initialized: 51 nodes, 5 edges

🔬 Running full diagnostic suite (10 tests)...

🧪 Testing: fire→water
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📊 Entropy (before): 0.XXX
🧠 Leap triggered: YES/NO
📉 Entropy reduction: 0.XXX
✅ Leap success: YES/NO

[... 9 more tests ...]

╔═══════════════════════════════════════════════════════════════╗
║            DIAGNOSTIC SUMMARY                                 ║
╠═══════════════════════════════════════════════════════════════╣
║ Total Tests:                                               10 ║
║ Successful Leaps:                                           X ║
║ Success Rate:                                             X % ║
╠═══════════════════════════════════════════════════════════════╣
║ Mean Entropy Reduction:                               X.XXX ║
║ Mean Context Similarity:                              X.XXX ║
╚═══════════════════════════════════════════════════════════════╝

💾 Diagnostics saved to leap_diagnostics.csv
📄 Report saved to leap_tuning_report.md

✅ Diagnostic session complete!
```

---

### Step 3: Review Results

```bash
# View the markdown report
cat leap_tuning_report.md

# Check the CSV data
cat leap_diagnostics.csv

# See your tracking history
cat diagnostics_history.csv
```

**What to look for:**
- Entropy reduction values (baseline should be low ~0.0)
- Context similarity (baseline should be ~0.0)
- Which tests generated candidates (shows graph connectivity)

---

### Step 4: Run Auto-Tune (Optional - Takes 5-10 minutes)

```bash
make run_diagnostic_auto_tune
```

**This will:**
- Sweep λ from 0.2 → 1.2 (6 values)
- Test thresholds 0.4 → 0.8 (5 values)
- Tune learning rate 0.01 → 0.05 (5 values)
- Generate optimized parameters

**Expected output:**
```
╔═══════════════════════════════════════════════════════════════╗
║  Step 1: BIAS STRENGTH SWEEP (λ)                             ║
╚═══════════════════════════════════════════════════════════════╝

🔧 Testing λ = 0.20
  → Entropy reduction: X.XXX
  → Score: X.XXX

[... more values ...]

✅ Best λ = X.XX (score: X.XXX)

[... Steps 2-5 ...]

╔═══════════════════════════════════════════════════════════════╗
║           AUTO-TUNING SUMMARY                                 ║
╠═══════════════════════════════════════════════════════════════╣
║ OPTIMIZED PARAMETERS:                                         ║
║   lambda_graph_bias        = X.XX                             ║
║   leap_entropy_threshold   = X.XX                             ║
║   learning_rate_embeddings = X.XXX                            ║
╚═══════════════════════════════════════════════════════════════╝
```

**Files generated:**
- `leap_tuning_results.txt` - Full tuning report

---

### Step 5: Generate Plots

```bash
# Install dependencies (one-time)
pip3 install pandas matplotlib numpy

# Generate plots
python3 plot_progress.py --save
```

**Expected output:**
```
🎨 Generating diagnostic progress plots...

📊 CURRENT STATUS:
   Entropy Reduction:  0.000
   Context Similarity: 0.000
   Leap Success:       0.0%

✅ Plot saved to melvin_progress.png
```

**View the plot:**
```bash
open melvin_progress.png  # macOS
# or
xdg-open melvin_progress.png  # Linux
```

**What to look for:**
- Bottom-right plot: Entropy vs Similarity
- Should show single point at origin (0,0)
- This is your baseline - future runs will show trajectory

---

### Step 6: Tag This as Baseline

```bash
./tag_milestone.sh "baseline_run_complete" "First complete diagnostic run"
```

**Expected output:**
```
╔═══════════════════════════════════════════════════════════════╗
║              🏆 TAG MILESTONE COMMIT                          ║
╚═══════════════════════════════════════════════════════════════╝

📊 Current Metrics:
   Context Similarity:  0.000
   Entropy Reduction:   0.000

🏷️  Milestone: baseline_run_complete
📝 Notes: First complete diagnostic run

✅ Milestone documented
✅ Git tag created
```

---

## 🔍 What to Watch For

### ✅ Good Signs

- Diagnostic runs without crashes ✅
- CSV files generated ✅
- Markdown reports created ✅
- Plots generated successfully ✅
- Baseline protected ✅

### ⚠️ Potential Issues & Fixes

**Issue:** `g++: command not found`
```bash
# Install build tools
xcode-select --install  # macOS
# or
sudo apt install build-essential  # Linux
```

**Issue:** `Python module not found`
```bash
pip3 install pandas matplotlib numpy
```

**Issue:** `Permission denied on scripts`
```bash
chmod +x *.sh *.py
```

**Issue:** `make: *** No rule to make target 'diagnostic'`
```bash
# Verify Makefile was updated
grep "diagnostic:" Makefile
# Should see diagnostic build rules
```

---

## 📊 Interpreting Your First Results

### Baseline Metrics (Expected)

With the minimal test graph:

```
Entropy Reduction:   ~0.000 - 0.100
Context Similarity:  ~0.000 - 0.050
Leap Success:        ~0% - 20%
Tests with candidates: ~3-5 of 10
```

**This is CORRECT!** Low scores indicate:
- Graph is sparse (only 5 edges)
- Embeddings untrained
- System correctly identifies need for improvement
- Baseline established for comparison

### What Each Metric Means

**Entropy Reduction:**
- 0.00 = Graph not influencing predictions
- 0.20+ = Graph providing useful constraints
- Higher = Better (but watch for nonsense text if >0.40)

**Context Similarity:**
- 0.00 = Embeddings not aligned
- 0.40 = Faint conceptual links appearing
- **0.50 = COHERENCE THRESHOLD** 🎯
- 0.70+ = Strong semantic understanding

**Leap Success Rate:**
- 0% = Leaps not helping predictions
- 60%+ = Leaps consistently improving output
- Higher = Better

---

## 📈 After First Run - Next Steps

### 1. Review Weekly Summary

```bash
./weekly_summary.sh
```

**Shows:**
- Current metrics
- Recommendations (will say "Add Layer 1 data")
- Recent history

### 2. Check Baseline Protection

```bash
ls -l baseline_control/
cat baseline_control/README.md
```

**Verify:**
- Files are read-only (r--r--r--)
- README explains never to delete
- Baseline metrics documented

### 3. Prepare for Layer 1 Data

**You need:**
- 10,000 - 50,000 conversation exchanges
- Q&A format or dialog format
- High quality, natural language

**Sources:**
- TinyChat logs
- Reddit conversations
- Your chat logs
- Public dialog datasets

**Format example:** (see `sample_dialog_data.txt`)

---

## 🔬 When You're Ready to Add Real Data

### Version and Ingest

```bash
# 1. Version your data
./version_data.sh my_conversations.txt "TinyChat batch 1 - 10k exchanges"

# 2. Ingest in batches (auto-measures every 5k)
./ingest_dialog_data.sh versioned_data/TIMESTAMP/data.txt 5000

# 3. Check progress
./weekly_summary.sh

# 4. Plot trends
python3 plot_progress.py --save

# 5. Archive
./archive_diagnostics.sh "Week 1 - First data ingestion"
```

---

## 🎯 What Success Looks Like (After Real Data)

### After 10k Conversations

```
Entropy Reduction:   0.000 → 0.080 (+0.080)
Context Similarity:  0.000 → 0.220 (+0.220)
Leap Success:        0% → 28%
```

**Plot shows:** Diagonal upward trajectory

### After 30k Conversations + Training

```
Entropy Reduction:   0.080 → 0.180 (+0.100)
Context Similarity:  0.220 → 0.380 (+0.160)
Leap Success:        28% → 45%
```

**Plot shows:** Continued rise, approaching 0.4

### After Adding Factual Data (Week 5-6)

```
Entropy Reduction:   0.180 → 0.270 (+0.090)
Context Similarity:  0.380 → 0.520 (+0.140) ← CROSSED 0.5!
Leap Success:        45% → 61%
```

**Plot shows:** CURVE FLATTENING 🎉  
**Qualitative check:** Strong links appearing  
**Generated text:** First coherent sentences!

---

## 📞 Reporting Results

### When You Run It, Share:

**1. Build output:**
```bash
make diagnostic 2>&1 | tee build.log
```

**2. Diagnostic summary:**
```bash
make run_diagnostic 2>&1 | grep -A 20 "DIAGNOSTIC SUMMARY"
```

**3. Files generated:**
```bash
ls -lh leap_*.csv leap_*.md diagnostics_history.csv
```

**4. Any errors or unexpected behavior**

I can then help you:
- Interpret the metrics
- Troubleshoot issues
- Optimize parameters
- Plan next steps

---

## 🎓 System Requirements

**Minimum:**
- g++ with C++20 support
- Bash shell
- ~100MB disk space

**For full features:**
- Python 3.7+ (for plotting)
- pandas, matplotlib, numpy (for visualization)
- Git (for versioning/tagging)

**Check your system:**
```bash
g++ --version          # Should be 9.0+
python3 --version      # Should be 3.7+
git --version          # Any recent version
```

---

## ✅ Verification Checklist

After running `make run_diagnostic`, verify:

- [ ] `diagnostic_main` executable exists
- [ ] `leap_diagnostics.csv` generated (10 rows)
- [ ] `leap_tuning_report.md` generated
- [ ] `diagnostics_history.csv` has new entry
- [ ] `baseline_control/` directory exists and is read-only
- [ ] No crash or segfault
- [ ] Summary shows metrics (even if all zeros)

**If all checked:** ✅ System working correctly

---

## 🚀 Ready When You Are

**Your move:**

1. Run the commands above
2. Share any output or errors
3. I'll help interpret and debug

**Or if you want me to continue:**
- I can run these commands for you
- I can build and test further
- I can help with any issues

Just let me know what you'd like to do next! 

The system is complete and ready - whether you run it yourself or want me to continue testing, everything is in place for that first breakthrough moment when similarity crosses 0.5. 🎯

