# 🚀 Next Session: Patch Pack E - Multi-Hop + SRS + Decay

**Start Here When Beginning Fresh Session**

---

## 🎯 Mission for Next Session

**Implement Patch Pack E** - Transform 70% retention → 85%+ with multi-hop reasoning, spaced repetition, and edge decay.

---

## 📋 Current State (After This Commit)

### What's Complete ✅

- ✅ Research Infrastructure (33 files)
- ✅ Long-Term Database (SQLite, 8 files)
- ✅ In-Memory Learning (11 files, 1100% growth verified)
- ✅ Teaching System (15 files, 100% curriculum success)

**Brain State:**
- Nodes: 49
- Edges: 48
- Snapshot: melvin_brain.bin (15 KB)
- Retention: 70% average

### What's Partial (40% Done)

Already implemented (keep these):
- ✅ `src/reasoning/multihop.h` (complete)
- ✅ `src/reasoning/multihop.cpp` (complete)
- ✅ `src/learning/edge_decay.h` (complete)
- ✅ `src/learning/edge_decay.cpp` (complete)
- ✅ `src/learning/srs_scheduler.h` (complete)
- ✅ `PATCH_PACK_E_SPEC.md` (complete specification)

### What Remains (60% To Do)

**Need to implement:**
1. ⏳ `src/learning/srs_scheduler.cpp` (~400 lines)
2. ⏳ `src/teaching/path_verifier.h/cpp` (~300 lines)
3. ⏳ Teaching format v2 parser extensions (~200 lines)
4. ⏳ Integration with teaching_verify.cpp (~150 lines)
5. ⏳ CLI tools: `admin_decay.cpp`, `teach_srs.cpp` (~200 lines)
6. ⏳ `lessons/03_multihop.tch` (test lesson)
7. ⏳ Makefile updates (decay, srs, eval_generalize targets)
8. ⏳ Documentation updates
9. ⏳ Complete integration testing
10. ⏳ Verify ≥85% retention target

**Estimated:** ~1,500 lines, 12-18 hours focused work

---

## 🎯 Patch Pack E Goals

### Problems to Solve

1. **Shallow reasoning** - Currently 1-hop only
   - Target: 3-hop beam search
   - Fix: "cats → milk" should work (cats→mammals→milk)

2. **Memory leaks** - 70% retention = 30% forgotten
   - Target: Realistic forgetting curve
   - Fix: Ebbinghaus decay + spaced repetition

3. **No path verification** - Answer correct but reasoning wrong
   - Target: Verify reasoning path matches expected
   - Fix: DERIVE specs + path class checking

4. **Weight drift** - Re-teaching over-inflates weights
   - Target: Capped, stable weights
   - Fix: EdgeKey + utility tracking + w_max cap

5. **No generalization** - No transfer testing
   - Target: Auto-generate sibling tests
   - Fix: After "cats→milk", test "dogs→milk"

### Expected Improvements

| Metric | Before | After | Change |
|--------|--------|-------|--------|
| Pass rate | 70% | ≥85% | +15% |
| Hop capability | 1-hop | 3-hop | Multi-hop |
| "cats→milk" | ❌ FAILS | ✅ PASSES | 2-hop works |
| Weight stability | Drifts | Capped | Controlled |
| Path verification | N/A | ≥80% | New metric |

---

## 📚 Implementation Plan

### Read First

1. **PATCH_PACK_E_SPEC.md** - Complete specification (10,500 words)
   - All features detailed
   - API specifications
   - Code examples
   - Success criteria

2. **TEACHING_SYSTEM_COMPLETE.md** - Current teaching system
   - Understand what works
   - See limitations (70% retention)
   - Know integration points

3. **LEARNING_SYSTEM_README.md** - Learning substrate
   - Edge structure
   - Growth tracking
   - Binary format

### Implementation Order

**Priority 1: Multi-Hop Reasoning** (3-4 hours)
- Complete implementation is ready: multihop.h/cpp
- Just need to integrate with teaching_verify.cpp
- Test on 03_multihop.tch
- Verify "cats→milk" passes

**Priority 2: Path Verification** (2-3 hours)
- Implement path_verifier.h/cpp
- Extend teaching format for DERIVE tag
- Check path class matches expected
- Report hop_count, path_class

**Priority 3: Edge Decay** (2-3 hours)
- Complete implementation ready: edge_decay.h/cpp
- Create admin_decay CLI tool
- Test decay application
- Verify weights decrease over time

**Priority 4: SRS Scheduler** (3-4 hours)
- Complete srs_scheduler.cpp
- Create teach_srs CLI tool
- Test spaced repetition queue
- Verify only due cards reviewed

**Priority 5: Format v2** (2 hours)
- Extend parser for ALIAS, DERIVE, NEG
- Update ingestion for new blocks
- Test all new tag types

**Priority 6: Integration & Testing** (2-3 hours)
- Wire everything together
- Create 03_multihop.tch
- Run full curriculum
- Verify ≥85% retention

**Total: 14-19 hours**

---

## 🧪 Test Plan for Patch Pack E

### Test 1: Multi-Hop Verification

```bash
make teach_reset
make teach FILE=lessons/03_multihop.tch

# Expected:
# "What do cats produce?" → "milk" ✅ (was ❌)
# Pass rate: ≥80% (was ~30% without multi-hop)
```

### Test 2: SRS + Decay

```bash
# Teach curriculum
make teach_dir DIR=lessons/

# Apply decay
make decay

# Run SRS (due cards only)
make srs

# Expected:
# Some edges weakened (realistic forgetting)
# Only due cards reviewed
# Successful reviews boost weights
```

### Test 3: Path Verification

```bash
make teach FILE=lessons/03_multihop.tch --require-path

# Expected:
# Reports show path_class for each test
# DERIVE specs checked
# Path correctness ≥80%
```

### Test 4: Complete System

```bash
make teach_reset
make teach_dir DIR=lessons/  # All 4 lessons (including 03_multihop)

# Expected:
# Pass rate: ≥85% (was 70%)
# Multi-hop working
# All DERIVE specs checked
# Growth: 60-70 nodes, 60-70 edges
```

---

## 🔧 Starting Point for Fresh Session

### Files Already Ready (Use These)

```
2025-10-11/src/reasoning/multihop.h          ✅ Complete
2025-10-11/src/reasoning/multihop.cpp        ✅ Complete
2025-10-11/src/learning/edge_decay.h         ✅ Complete
2025-10-11/src/learning/edge_decay.cpp       ✅ Complete
2025-10-11/src/learning/srs_scheduler.h      ✅ Complete
2025-10-11/PATCH_PACK_E_SPEC.md              ✅ Complete (10,500 words)
```

### Files to Create (Fresh Implementation)

```
2025-10-11/src/learning/srs_scheduler.cpp    (~400 lines)
2025-10-11/src/teaching/path_verifier.h      (~150 lines)
2025-10-11/src/teaching/path_verifier.cpp    (~200 lines)
2025-10-11/src/tools/admin_decay.cpp         (~100 lines)
2025-10-11/src/tools/teach_srs.cpp           (~150 lines)
2025-10-11/lessons/03_multihop.tch           (~80 lines)
```

### Files to Modify (Integration)

```
src/teaching/teaching_format.h/cpp           (add ALIAS/DERIVE/NEG)
src/teaching/teaching_verify.cpp             (use multi-hop)
src/teaching/teaching_ingest.cpp             (handle new blocks)
src/teaching/teaching_metrics.cpp            (add hop_count, path_class)
learning_hooks.cpp                           (add utility, weight cap)
Makefile                                     (add decay, srs targets)
docs/TEACHING_FORMAT.md                      (document v2 tags)
```

---

## 💡 Quick Start for Next Session

### Option A: Cursor Prompt (Recommended)

Copy this into Cursor:

```
Implement Patch Pack E for Melvin based on PATCH_PACK_E_SPEC.md

Current state:
- Four systems complete (see READY_TO_COMMIT.md)
- Brain: 49 nodes, 48 edges, 70% retention
- Partial work: multihop, edge_decay headers complete

Goal:
- Multi-hop reasoning (k=3)
- SRS scheduler (SM-2 algorithm)
- Edge decay (Ebbinghaus)
- Path verification (DERIVE specs)
- Target: 85%+ retention

Implementation priority:
1. Multi-hop integration (use existing multihop.h/cpp)
2. Path verifier
3. SRS scheduler completion
4. Edge decay tool
5. Format v2 (ALIAS/DERIVE/NEG)
6. Full testing

Start with multi-hop integration into teaching_verify.cpp.
All specifications in PATCH_PACK_E_SPEC.md.
```

### Option B: Manual Implementation

1. Read `PATCH_PACK_E_SPEC.md` (complete design)
2. Start with multi-hop integration (easiest win)
3. Follow implementation order in spec
4. Test incrementally
5. Commit when all tests pass

---

## 📊 Success Criteria for Patch Pack E

### Must Achieve

1. ✅ "What do cats produce?" → "milk" (2-hop working)
2. ✅ Pass rate ≥85% on full curriculum
3. ✅ Decay reduces avg weight by 10-20%
4. ✅ SRS only reviews due cards
5. ✅ Path verification reports hop_count
6. ✅ DERIVE specs checked
7. ✅ All builds passing
8. ✅ No regressions in existing tests

### Should Achieve

- 3-hop reasoning working
- Generalization tests auto-generated
- Weight capping prevents drift
- ALIAS creates bidirectional synonyms
- NEG creates anti-edges

---

## 🎓 Context for Fresh Session

### What Melvin Can Do Now

**After this commit:**
- Measure reasoning (diagnostics)
- Store experiments (database)
- Learn from interactions (edge growth)
- Be taught knowledge (lessons)
- Persist across sessions (snapshots)

**Limitations:**
- Only 1-hop reasoning (shallow)
- No forgetting (memory bloat)
- No path verification (can't explain WHY)
- 70% retention (leaks knowledge)

### What Patch Pack E Adds

**After Patch Pack E:**
- 3-hop reasoning (deep)
- Realistic forgetting (SRS + decay)
- Path verification (explainable)
- 85%+ retention (tighter learning)

**Impact:**
- "cats→milk" will pass
- Knowledge consolidates
- Retention improves
- Reasoning is explainable

---

## 📁 Key Files to Reference

**For understanding current system:**
- `TEACHING_SYSTEM_COMPLETE.md` - What works now
- `LEARNING_SYSTEM_README.md` - How learning works
- `SESSION_COMPLETE_SUMMARY.md` - Full session summary

**For implementing Patch Pack E:**
- `PATCH_PACK_E_SPEC.md` - Complete specification
- `src/reasoning/multihop.h` - Already implemented
- `src/learning/edge_decay.h` - Already implemented
- `GIT_COMMIT_PACKAGE.md` - What was just committed

---

## 🎯 Recommended Fresh Session Start

### Prompt to Use

```
Continue Melvin development with Patch Pack E implementation.

Context:
- Previous commit: v0.9.0-cognitive-substrate
- Four systems complete (research, database, learning, teaching)
- Current retention: 70% average
- Goal: Implement multi-hop + SRS + decay to reach 85%+ retention

Start by:
1. Reading PATCH_PACK_E_SPEC.md (complete specification)
2. Reviewing existing partial work (multihop.*, edge_decay.*)
3. Implementing remaining components
4. Testing on lessons/03_multihop.tch
5. Verifying 85%+ retention target

Begin with multi-hop integration (highest priority).
```

---

## ✅ Clean Handoff Checklist

- [x] Four systems complete and tested
- [x] Commit package prepared (GIT_COMMIT_PACKAGE.md)
- [x] Quick commit guide (READY_TO_COMMIT.md)
- [x] Patch Pack E specification complete (PATCH_PACK_E_SPEC.md)
- [x] Partial work documented (40% done)
- [x] Fresh session prompt prepared (this file)
- [x] All verification passing
- [x] Clean separation between commit and next work

---

**Everything ready for:**
1. **Now:** Commit four systems (v0.9.0)
2. **Next session:** Implement Patch Pack E fresh

**See you in the next context window for multi-hop reasoning!** 🧠

