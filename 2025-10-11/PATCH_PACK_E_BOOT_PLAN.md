# 🧬 Patch Pack E - Evolution Mode: Boot Plan

**Base Version:** v0.9.1-continuous-learning (d4bb113)  
**Target Version:** v0.10.0-evolution-mode  
**Branch:** `feature/patch-pack-e-evolution`  
**Status:** READY TO BEGIN

---

## 🎯 Mission

Transform Melvin from **continuous learning** → **adaptive evolution**:

- Multi-hop reasoning (trace chains, not just answers)
- Ebbinghaus decay (forget unused knowledge)
- Spaced repetition (reinforce important edges)
- Path verification (correct reasoning, not just correct answers)
- Enhanced teaching format (ALIAS, DERIVE, NEG tags)

---

## 📊 Current State Analysis

### ✅ What We Have (v0.9.1)

```
✓ Continuous learning watcher (data/inbox/ → auto-ingest)
✓ Teaching System (.tch parser, ingest, verify)
✓ Learning hooks (edge weight updates, node creation)
✓ Binary snapshots (melvin_brain.bin persistence)
✓ Metrics logging (continuous_learning_metrics.csv)
✓ Storage layer (save/load brain state)
```

### 🎯 What We Need (v0.10.0)

```
→ Multi-hop reasoning engine (k-hop beam search)
→ Edge decay system (time-based forgetting)
→ SRS scheduler (rehearsal queue)
→ Path verification (track reasoning chains)
→ Teaching Format v2 (new tags)
→ Integration hooks in continuous learning mode
```

---

## 🗂️ Component Breakdown

### **Component 1: Multi-Hop Reasoning Engine**

**Files to Create:**
- `src/reasoning/multihop_engine.h` (interface)
- `src/reasoning/multihop_engine.cpp` (k-hop beam search)

**Files to Modify:**
- `src/teaching/teaching_verify.cpp` (use multi-hop for queries)
- `predictive_sampler.cpp` (optional integration)

**Features:**
- k-hop beam search (configurable k=1,2,3)
- Relation priors (weight edges by relation type)
- Virtual edges from rules (#RULE becomes temporary edge)
- Path scoring (length penalty, weight product)
- Example: "cats drink water" → [cats -ARE→ mammals -DRINK→ water]

**Config:**
```yaml
multihop:
  max_hops: 3
  beam_width: 5
  relation_priors:
    ARE: 1.0
    DRINK: 0.9
    PART_OF: 0.8
```

---

### **Component 2: Edge Decay (Ebbinghaus Forgetting)**

**Files to Create:**
- `src/learning/edge_decay.h` (decay interface)
- `src/learning/edge_decay.cpp` (Ebbinghaus curve)

**Files to Modify:**
- `src/continuous_learning/continuous_learning.cpp` (call decay hook)
- `config/continuous_learning.yaml` (enable_decay: true)

**Features:**
- Exponential decay: `weight *= exp(-λ * Δt)`
- Configurable half-life (e.g., 7 days)
- Min weight threshold (don't decay below 0.1)
- Rehearsal resets decay timer
- Metrics: edges_decayed, avg_weight_loss

**Algorithm:**
```cpp
void apply_decay(Edge& edge, uint64_t current_time) {
    float days_since_access = (current_time - edge.last_access_time) / 86400.0;
    float lambda = 0.693 / half_life_days;  // half-life = 7 days
    edge.weight *= exp(-lambda * days_since_access);
    edge.weight = max(edge.weight, min_weight_threshold);
}
```

---

### **Component 3: SRS (Spaced Repetition System)**

**Files to Create:**
- `src/learning/srs_scheduler.h` (SRS interface)
- `src/learning/srs_scheduler.cpp` (rehearsal queue)

**Files to Modify:**
- `src/continuous_learning/continuous_learning.cpp` (call SRS hook)
- `src/tools/teach_curriculum.cpp` (SRS mode flag)

**Features:**
- Priority queue of edges needing rehearsal
- SM-2 algorithm intervals (1 day, 3 days, 7 days, 14 days, ...)
- Auto-generate rehearsal lessons
- Write to `data/inbox/rehearsal_YYYYMMDD.tch`
- Metrics: edges_in_queue, rehearsals_triggered

**SRS Queue Entry:**
```cpp
struct SRSEntry {
    uint64_t edge_id;
    uint64_t next_rehearsal_time;
    uint32_t repetition_number;  // 0, 1, 2, ...
    float ease_factor;           // 1.3-2.5
};
```

**Intervals (SM-2):**
- Rep 0: 1 day
- Rep 1: 6 days
- Rep 2+: interval * ease_factor

---

### **Component 4: Path Verification**

**Files to Modify:**
- `src/teaching/teaching_verify.h` (add path tracking)
- `src/teaching/teaching_verify.cpp` (verify path correctness)

**Features:**
- Capture reasoning path, not just answer
- Verify intermediate steps
- Report: `path_used`, `hop_count`, `path_correctness`
- Example:
  ```
  Query: "What do cats drink?"
  Expected Path: cats -ARE→ mammals -DRINK→ water
  Actual Path: cats -ARE→ mammals -DRINK→ water
  Result: ✓ CORRECT (path matches)
  ```

**Verification Levels:**
1. **Answer-only** (current): "water" = correct
2. **Path-required** (new): Must trace cats→mammals→water
3. **Partial credit**: Correct answer, wrong path = 50%

---

### **Component 5: Teaching Format V2**

**New Tags:**

**`#ALIAS`** - Define synonyms
```
#ALIAS
cats -> felines
dogs -> canines
```

**`#DERIVE`** - Teach inference rules
```
#DERIVE
IF X ARE mammals THEN X HAVE bones
IF X HAVE bones THEN X ARE vertebrates
```

**`#NEG`** - Teach negative examples (what NOT to infer)
```
#NEG
cats DO_NOT fly
fish DO_NOT breathe_air
```

**Files to Modify:**
- `src/teaching/teaching_format.h` (add new block types)
- `src/teaching/teaching_format.cpp` (parse ALIAS, DERIVE, NEG)
- `src/teaching/teaching_ingest.cpp` (ingest new block types)

**Impact:**
- ALIAS → merge node embeddings
- DERIVE → create virtual rule edges
- NEG → mark edges with negative weight or flag

---

## 🔗 Integration Points

### **Hook 1: Continuous Learning Watcher**

In `src/continuous_learning/continuous_learning.cpp`:

```cpp
void ContinuousLearner::run() {
    while (true) {
        process_one_batch();
        maybe_snapshot();
        maybe_metrics();
        
        // NEW HOOKS
        if (cfg_.enable_decay && decay_) {
            decay_();  // Apply Ebbinghaus decay
        }
        if (cfg_.enable_srs && srs_) {
            srs_();    // Check SRS queue, generate rehearsals
        }
        
        sleep(cfg_.poll_seconds);
    }
}
```

### **Hook 2: Teaching Verification**

In `src/teaching/teaching_verify.cpp`:

```cpp
TestResult verify_single_query(const TchQuery& query, ...) {
    // OLD: answer = simple_query(question)
    // NEW: Use multi-hop reasoning
    MultihopResult result = multihop_engine.search(
        question, max_hops=3, return_path=true
    );
    
    bool answer_correct = matches_any(result.answer, query.expects);
    bool path_correct = verify_path(result.path, query.expected_path);
    
    return TestResult{
        .passed = answer_correct && path_correct,
        .path_used = result.path,
        .hop_count = result.path.size() - 1
    };
}
```

### **Hook 3: SRS Auto-Rehearsal**

New function in `src/learning/srs_scheduler.cpp`:

```cpp
void check_and_generate_rehearsals(
    const std::string& inbox_dir,
    uint64_t current_time
) {
    auto due_edges = srs_queue.get_due(current_time);
    
    if (due_edges.empty()) return;
    
    // Generate rehearsal lesson
    std::ofstream f(inbox_dir + "/rehearsal_" + timestamp() + ".tch");
    f << "#FACT rehearsal session\n";
    
    for (auto& entry : due_edges) {
        Edge& e = edges[entry.edge_id];
        f << nodes[e.u].text << " " << e.rel << " " << nodes[e.v].text << "\n";
    }
    
    f << "#TEST rehearsal_check\n";
    // ... add verification queries
}
```

---

## 📁 File Structure

### **New Files to Create:**

```
src/reasoning/
  ├── multihop_engine.h          (150 lines)
  └── multihop_engine.cpp        (400 lines)

src/learning/
  ├── edge_decay.h               (80 lines)
  ├── edge_decay.cpp             (200 lines)
  ├── srs_scheduler.h            (120 lines)
  └── srs_scheduler.cpp          (350 lines)

tests/
  ├── test_multihop.cpp          (200 lines)
  ├── test_decay.cpp             (150 lines)
  └── test_srs.cpp               (180 lines)

docs/
  ├── MULTIHOP_GUIDE.md          (300 lines)
  ├── DECAY_TUNING.md            (200 lines)
  └── SRS_EXPLAINED.md           (250 lines)

config/
  └── evolution_config.yaml      (40 lines)

lessons/
  └── 03_multihop_test.tch       (100 lines - test multi-hop)
```

### **Files to Modify:**

```
src/continuous_learning/continuous_learning.cpp  (+50 lines - hooks)
src/teaching/teaching_format.h                   (+30 lines - new tags)
src/teaching/teaching_format.cpp                 (+100 lines - parser)
src/teaching/teaching_ingest.cpp                 (+150 lines - ALIAS/DERIVE/NEG)
src/teaching/teaching_verify.h                   (+20 lines - path tracking)
src/teaching/teaching_verify.cpp                 (+200 lines - path verification)
config/continuous_learning.yaml                  (+10 lines - decay/SRS config)
Makefile                                         (+50 lines - new targets)
```

**Total New Code:** ~2,550 lines  
**Total Modified Code:** ~610 lines  
**Grand Total:** ~3,160 lines

---

## 🧪 Test Plan

### **Test 1: Multi-Hop Reasoning**

**Lesson:** `lessons/03_multihop_test.tch`

```
#FACT
cats ARE mammals

#FACT
mammals DRINK water

#TEST CatsDrink
#QUERY What do cats drink?
#EXPECT water
#EXPECT_PATH cats -> mammals -> water
```

**Expected Output:**
```
[MULTIHOP] Query: "What do cats drink?"
[MULTIHOP] Searching with k=3...
[MULTIHOP] Path found: cats -ARE→ mammals -DRINK→ water
[MULTIHOP] Answer: water
[VERIFY] ✓ Answer correct: water
[VERIFY] ✓ Path correct: cats -> mammals -> water
[VERIFY] Result: PASSED (100%)
```

---

### **Test 2: Edge Decay**

**Setup:**
1. Teach `cats DRINK water`
2. Wait 24 hours (or simulate time)
3. Don't use edge during that time
4. Run decay hook
5. Check edge weight

**Expected:**
```
Initial weight: 1.0
After 1 day (no use): 0.91  (9% decay)
After 7 days (no use): 0.50  (50% decay - half-life)
After 14 days (no use): 0.25  (75% decay)
After use (rehearsal): Reset to 1.0
```

---

### **Test 3: SRS Scheduler**

**Setup:**
1. Teach 10 facts
2. Verify all pass
3. Run SRS scheduler after 1 day
4. Check `data/inbox/` for `rehearsal_YYYYMMDD.tch`
5. Process rehearsal lesson
6. Verify weights restored

**Expected:**
```
Day 0: Teach 10 facts
Day 1: SRS queue: 10 edges due
       Generated: data/inbox/rehearsal_20251012.tch
       Processed: 10/10 edges rehearsed
       Weights restored: 10/10
Day 6: SRS queue: 10 edges due again (second repetition)
```

---

### **Test 4: Path Verification**

**Lesson:**
```
#FACT
birds ARE animals

#FACT
animals NEED oxygen

#TEST BirdsNeedWhat
#QUERY What do birds need?
#EXPECT oxygen
#EXPECT_PATH birds -> animals -> oxygen
```

**Test Cases:**
1. Correct answer + correct path → PASS (100%)
2. Correct answer + wrong path → PARTIAL (50%)
3. Wrong answer + any path → FAIL (0%)

---

### **Test 5: Teaching Format V2**

**Lesson:** `lessons/04_format_v2_test.tch`

```
#ALIAS
felines -> cats
canines -> dogs

#DERIVE
IF X ARE mammals THEN X HAVE fur
IF X HAVE fur THEN X NEED grooming

#NEG
fish DO_NOT have_lungs
birds DO_NOT have_teeth

#TEST AliasTest
#QUERY What are felines?
#EXPECT mammals

#TEST DeriveTest
#QUERY Do cats need grooming?
#EXPECT yes

#TEST NegTest
#QUERY Do fish have lungs?
#EXPECT no
```

**Expected:**
- ALIAS: "felines" query returns same as "cats"
- DERIVE: Chain inference works (mammals → fur → grooming)
- NEG: Negative assertions respected

---

## 🔧 Implementation Order

### **Phase 1: Foundation (Days 1-2)**

1. Create branch `feature/patch-pack-e-evolution`
2. Implement `multihop_engine.h/cpp` (basic k-hop search)
3. Add unit test `test_multihop.cpp`
4. Verify multi-hop search works standalone

**Deliverable:** Multi-hop engine working in isolation

---

### **Phase 2: Decay System (Day 3)**

1. Implement `edge_decay.h/cpp` (Ebbinghaus curve)
2. Add time simulation for testing
3. Create `test_decay.cpp`
4. Hook into `continuous_learning.cpp` (behind flag)

**Deliverable:** Decay system working, configurable

---

### **Phase 3: SRS System (Days 4-5)**

1. Implement `srs_scheduler.h/cpp` (SM-2 algorithm)
2. Add rehearsal lesson generator
3. Create `test_srs.cpp`
4. Hook into `continuous_learning.cpp`

**Deliverable:** SRS queue working, auto-generates rehearsals

---

### **Phase 4: Teaching Format V2 (Day 6)**

1. Extend `teaching_format.cpp` parser (ALIAS, DERIVE, NEG)
2. Update `teaching_ingest.cpp` (handle new tags)
3. Add test lesson `04_format_v2_test.tch`
4. Verify new tags parse and ingest correctly

**Deliverable:** Teaching Format v2 complete

---

### **Phase 5: Path Verification (Day 7)**

1. Modify `teaching_verify.cpp` (track paths)
2. Add path comparison logic
3. Integrate multi-hop engine with verification
4. Update test metrics (add `hop_count`, `path_correctness`)

**Deliverable:** Path verification working, 3 levels of correctness

---

### **Phase 6: Integration & Testing (Days 8-9)**

1. Wire all components together
2. Run full test suite
3. Run continuous learning mode with all hooks enabled
4. Collect metrics for 100 lesson cycles
5. Verify:
   - Multi-hop reasoning working
   - Decay reducing unused edges
   - SRS generating rehearsals
   - Path verification accurate

**Deliverable:** All systems integrated and verified

---

### **Phase 7: Documentation & Finalization (Day 10)**

1. Write `MULTIHOP_GUIDE.md`
2. Write `DECAY_TUNING.md`
3. Write `SRS_EXPLAINED.md`
4. Update `CONTINUOUS_LEARNING.md` with new features
5. Create `PATCH_PACK_E_COMPLETE.md`
6. Prepare commit message and tag

**Deliverable:** Full documentation, ready to commit v0.10.0

---

## 📊 Success Metrics

### **Target Performance:**

| Metric                     | v0.9.1 (Current) | v0.10.0 (Target) |
| -------------------------- | ---------------- | ---------------- |
| Retention Rate             | 70%              | 85%              |
| Multi-hop Success          | N/A              | 80%              |
| Decay Active Edges         | 0                | Track in CSV     |
| SRS Rehearsals Generated   | 0                | Auto-generated   |
| Path Verification Accuracy | N/A              | 90%              |
| Max Reasoning Hops         | 1                | 3                |

### **Acceptance Criteria:**

✅ Multi-hop reasoning traces 3-hop chains correctly  
✅ Edge decay reduces weight by 50% after half-life period  
✅ SRS generates rehearsal lessons automatically  
✅ Path verification distinguishes correct reasoning from lucky guesses  
✅ Teaching Format v2 parses ALIAS, DERIVE, NEG correctly  
✅ Continuous learning mode runs with all hooks enabled  
✅ No regressions in v0.9.1 functionality  
✅ All unit tests passing  
✅ Documentation complete  
✅ Commit ready with detailed message  

---

## 🎛️ Configuration

### **evolution_config.yaml** (New)

```yaml
multihop:
  enabled: true
  max_hops: 3
  beam_width: 5
  length_penalty: 0.9  # Prefer shorter paths
  relation_priors:
    ARE: 1.0
    IS_A: 1.0
    DRINK: 0.9
    NEED: 0.9
    HAVE: 0.8
    PART_OF: 0.8
    PRODUCES: 0.7

decay:
  enabled: true
  half_life_days: 7.0
  min_weight: 0.1
  check_interval_hours: 24

srs:
  enabled: true
  base_interval_days: 1.0
  ease_factor_min: 1.3
  ease_factor_max: 2.5
  rehearsal_threshold: 0.5  # Trigger if weight < 0.5
  auto_generate_lessons: true

path_verification:
  enabled: true
  require_path: true      # FAIL if path doesn't match
  partial_credit: true    # 50% for right answer, wrong path
  max_path_length: 5
```

### **Update continuous_learning.yaml:**

```yaml
# Existing config...
enable_decay: true   # NEW: Was false
enable_srs: true     # NEW: Was false
```

---

## 🚀 Makefile Targets

### **New Targets:**

```makefile
# Evolution Mode Components
EVOLUTION_OBJS = src/reasoning/multihop_engine.o \
                 src/learning/edge_decay.o \
                 src/learning/srs_scheduler.o

src/reasoning/multihop_engine.o: src/reasoning/multihop_engine.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

src/learning/edge_decay.o: src/learning/edge_decay.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

src/learning/srs_scheduler.o: src/learning/srs_scheduler.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Test targets
test_multihop: tests/test_multihop.cpp $(EVOLUTION_OBJS) $(TEACHING_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

test_decay: tests/test_decay.cpp src/learning/edge_decay.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

test_srs: tests/test_srs.cpp src/learning/srs_scheduler.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

# Run all evolution tests
test_evolution: test_multihop test_decay test_srs
	./test_multihop && ./test_decay && ./test_srs

# Build continuous learning with evolution hooks
cl_evolution: melvin_learn_watch $(EVOLUTION_OBJS)
	@echo "✅ Continuous Learning with Evolution Mode enabled"

.PHONY: test_evolution test_multihop test_decay test_srs cl_evolution
```

---

## 🧬 Git Strategy

### **Branch Management:**

```bash
# Create feature branch from v0.9.1
git checkout -b feature/patch-pack-e-evolution v0.9.1-continuous-learning

# Work in phases, commit incrementally
git add src/reasoning/multihop*
git commit -m "Phase 1: Multi-hop reasoning engine"

git add src/learning/edge_decay*
git commit -m "Phase 2: Edge decay system"

# ... etc ...

# When complete, merge to main
git checkout main
git merge --no-ff feature/patch-pack-e-evolution
git tag -a v0.10.0-evolution-mode -m "Patch Pack E Complete"
```

### **Commit Message Template:**

```
Implement Patch Pack E - Evolution Mode (v0.10.0)

CAPABILITIES ADDED:
- Multi-hop reasoning (k=3 beam search)
- Edge decay (Ebbinghaus forgetting curve)
- SRS scheduler (auto-generate rehearsals)
- Path verification (track reasoning chains)
- Teaching Format v2 (ALIAS, DERIVE, NEG)

METRICS:
- Retention: 70% → 85%
- Multi-hop success: 80%
- Max reasoning depth: 1 → 3 hops
- Auto-rehearsals: Enabled

FILES:
- New: 9 (multihop, decay, SRS, tests)
- Modified: 8 (teaching system, continuous learning)
- Total code: ~3,160 lines

TESTING:
- All unit tests passing
- Integration tests verified
- 100 lesson cycle confirmed

See PATCH_PACK_E_COMPLETE.md for details.
```

---

## 📚 Documentation Deliverables

1. **MULTIHOP_GUIDE.md** - How multi-hop reasoning works, examples
2. **DECAY_TUNING.md** - Configuring decay rates, half-life tuning
3. **SRS_EXPLAINED.md** - How SRS works, interval scheduling
4. **PATCH_PACK_E_COMPLETE.md** - Full implementation report
5. **EVOLUTION_MODE_QUICKSTART.txt** - Quick reference

---

## 🎯 Final Deliverable

**v0.10.0-evolution-mode**

- ✅ All 5 components implemented
- ✅ All tests passing
- ✅ Documentation complete
- ✅ Integration verified
- ✅ Metrics tracked
- ✅ Ready to commit

**Transformation:**
```
v0.9.1 → Continuous learning (always learning)
v0.10.0 → Evolution mode (adaptive learning with memory)
```

**Impact:**
- Melvin now **forgets** unused knowledge (decay)
- Melvin now **remembers** important knowledge (SRS)
- Melvin now **reasons deeper** (multi-hop)
- Melvin now **verifies reasoning** (path tracking)
- Melvin now **learns better** (enhanced teaching format)

---

## 🔥 Ready to Begin

**Current Status:** Boot plan complete  
**Next Step:** Create feature branch and begin Phase 1 (Multi-hop Engine)  
**Estimated Time:** 10 days (~3,160 lines of code)  
**Confidence:** High (building on stable v0.9.1 foundation)

**Say:** `Begin Phase 1` to start multi-hop reasoning engine implementation.

---

**Melvin is about to evolve. 🧬**

