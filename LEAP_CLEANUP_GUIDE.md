# 🧹 LEAP Cleanup Tool - Usage Guide

## What It Does

The LEAP cleanup tool removes low-quality LEAP edges from Melvin's brain:

- ❌ **Self-loops** (A→A) - Useless
- ❌ **Duplicates** of EXACT edges - Redundant  
- ❌ **No backing path** - LEAP without supporting EXACT edges
- ❌ **Low confidence** - Optional threshold filtering

**Result:** Higher quality LEAPs, faster reasoning, less noise

---

## Quick Start

### 1. Dry Run (Preview Only)
```bash
./leap_cleanup --dry-run
```
Shows what would be removed WITHOUT making changes

### 2. Basic Cleanup
```bash
./leap_cleanup
```
Removes self-loops, duplicates, and invalid LEAPs

### 3. Fast Cleanup (Skip Path Validation)
```bash
./leap_cleanup --skip-path-validation
```
Only removes self-loops and duplicates (much faster for large databases)

### 4. Aggressive Cleanup
```bash
./leap_cleanup --min-confidence 0.8
```
Also removes LEAPs with confidence < 0.8

---

## Command Options

```
./leap_cleanup [options]

Options:
  --dry-run, -d              Show what would be removed (don't save)
  --verbose, -v              Show detailed progress
  --min-confidence <N>       Remove LEAPs with confidence < N (0.0-1.0)
  --skip-path-validation     Skip checking for backing EXACT paths (faster)
  --help, -h                 Show this help
```

---

## When To Run

### Recommended Schedule

**After LEAP Inference:**
```bash
# After running LEAP inference, clean up
./leap_cleanup
```

**Periodic Maintenance:**
```bash
# Every week or after major learning sessions
./leap_cleanup --dry-run    # Check first
./leap_cleanup              # Then clean
```

**Before Deployment:**
```bash
# Clean up before deploying to production
./leap_cleanup --min-confidence 0.7
```

### Don't Run If

- Database is clean (< 1 week old)
- You just ran it recently
- You're in the middle of continuous learning (wait for pause)

---

## Where It Fits In Pipeline

```
┌──────────────────────────────────────────────────────────────┐
│              MELVIN LEARNING PIPELINE                        │
└──────────────────────────────────────────────────────────────┘

1. TEACH FACTS
   ├─ direct_fact_feeder
   ├─ ollama_continuous_learning
   └─ Adds EXACT edges
          ↓
2. CREATE LEAPS (automatic)
   ├─ LEAP inference runs
   └─ Creates shortcuts
          ↓
3. CLEANUP (optional) ← YOU ARE HERE
   ├─ ./leap_cleanup
   └─ Removes junk LEAPs
          ↓
4. USE FOR REASONING
   ├─ Query Melvin
   └─ Fast, clean reasoning

Best practice: Run cleanup after major learning sessions
```

### Integration Points

**After Continuous Learning:**
```bash
# Run Ollama learning
./start_ollama_learning.sh
# Press Ctrl+C to stop

# Then cleanup
./leap_cleanup
```

**As Periodic Maintenance:**
```bash
# Add to cron job (weekly)
0 0 * * 0 cd /path/to/Melvin && ./leap_cleanup --skip-path-validation
```

**In Learning Scripts:**
```bash
#!/bin/bash
# my_learning_script.sh

echo "Learning phase..."
./direct_fact_feeder my_facts.txt

echo "Cleanup phase..."
./leap_cleanup --skip-path-validation

echo "Done!"
```

---

## Example Output

```
╔════════════════════════════════════════════════════════════════╗
║  🧹 LEAP CLEANUP TOOL                                          ║
╚════════════════════════════════════════════════════════════════╝

╔════════════════════════════════════════════════════════════════╗
║  LOADING DATABASE                                              ║
╚════════════════════════════════════════════════════════════════╝

Loading from melvin/data/...
✅ Loaded successfully!

Initial state:
  Nodes:       65,536
  EXACT edges: 183,436
  LEAP edges:  4,106,793
  Total edges: 4,290,229

╔════════════════════════════════════════════════════════════════╗
║  ANALYZING LEAP EDGES                                          ║
╚════════════════════════════════════════════════════════════════╝

Cleanup rules:
  ✓ Remove self-loops (A→A)
  ✓ Remove duplicates of EXACT edges
  ✓ Remove LEAPs without backing EXACT path (≤5 hops)

Processing 4290229 edges...
  Progress: 10% (429022/4290229)
  Progress: 20% (858044/4290229)
  ...

╔════════════════════════════════════════════════════════════════╗
║  CLEANUP RESULTS                                               ║
╚════════════════════════════════════════════════════════════════╝

LEAPs analyzed:        4,106,793

Removed:
  Self-loops:          2,345
  Duplicates:          18,672
  No backing path:     412,089
  Low confidence:      0
  ──────────────────
  Total removed:       433,106

Kept:                  3,673,687 (89.5%)

Quality improvement:   VERY GOOD (89.5% valid)

╔════════════════════════════════════════════════════════════════╗
║  APPLYING CLEANUP                                              ║
╚════════════════════════════════════════════════════════════════╝

Removing 433106 edges...
✓ Removed 433106 edges

Saving cleaned database...
✅ Saved successfully!

Creating backup snapshot...
✓ Backup saved to melvin/data/backup_before_cleanup.melvin

Final state:
  EXACT edges: 183,436 (unchanged)
  LEAP edges:  3,673,687 (was 4,106,793)
  Total edges: 3,857,123
  Reduction:   -433,106 edges
```

---

## Expected Results

### Before Cleanup
```
LEAP edges:   4,106,793
Quality:      ~70-85%
Junk ratio:   ~15-30%
```

### After Cleanup
```
LEAP edges:   3,600,000-3,800,000
Quality:      ~90-95%
Junk ratio:   ~5-10%
```

### Benefits
- ✅ Faster reasoning (less noise)
- ✅ Higher confidence in LEAP shortcuts
- ✅ Cleaner graph visualization
- ✅ Better query results
- ✅ ~3-5MB storage savings

---

## Performance

### Speed

**With path validation:**
- ~1,000-2,000 LEAPs/sec
- 4.1M LEAPs ≈ 30-60 minutes

**Without path validation (--skip-path-validation):**
- ~100,000+ LEAPs/sec
- 4.1M LEAPs ≈ 1-2 minutes

### Memory

- Loads full database into memory
- ~100-200MB RAM for 65K nodes + 4M edges

---

## Safety

### Backups
- ✅ Creates automatic backup before saving
- ✅ Backup location: `melvin/data/backup_before_cleanup.melvin`
- ✅ Can restore if needed

### Dry Run
```bash
# Always safe to run
./leap_cleanup --dry-run
```
Shows what would happen WITHOUT making changes

### What's Protected
- ✅ EXACT edges are NEVER removed
- ✅ Nodes are NEVER removed
- ✅ Only LEAP edges are cleaned

---

## Troubleshooting

### "Too slow!"
```bash
# Skip the expensive path validation
./leap_cleanup --skip-path-validation
```

### "Removed too many!"
```bash
# Start with just self-loops and duplicates
./leap_cleanup --skip-path-validation --dry-run
```

### "Want to restore"
```bash
# Restore from backup
cp melvin/data/backup_before_cleanup.melvin melvin/data/edges.melvin
```

---

## FAQ

**Q: How often should I run this?**
A: Weekly or after major learning sessions

**Q: Will it break my reasoning?**
A: No! EXACT edges are preserved, only junk LEAPs are removed

**Q: Can I undo it?**
A: Yes, automatic backups are created

**Q: Should I use --min-confidence?**
A: Optional. Start without it, add if needed (0.7-0.8 is good)

**Q: Is --skip-path-validation safe?**
A: Yes! It's faster and still removes most junk (self-loops + duplicates)

---

## Summary

```bash
# Quick and safe
./leap_cleanup --skip-path-validation

# Thorough (slow)
./leap_cleanup

# Very thorough (very slow)
./leap_cleanup --min-confidence 0.8
```

**Recommendation:** Run `--skip-path-validation` weekly, full cleanup monthly

---

**Built to keep Melvin's brain clean and efficient!** 🧹⚡

