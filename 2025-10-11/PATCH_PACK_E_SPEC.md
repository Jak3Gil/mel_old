# Patch Pack E — Retention & Multi-Hop Reasoning

**Enhancement Package for Melvin Teaching System**

**Goal:** Transform teaching from shallow 1-hop → deep multi-hop reasoning with spaced repetition, path verification, and retention optimization.

---

## Problem Statement

### Current Limitations (70% Retention, Shallow Reasoning)

**Identified Issues:**
1. **Shallow hops:** Fails "cats → milk" unless direct edge exists
   - Current: 1-hop graph traversal
   - Needed: k-hop beam search (k=3 default)
   
2. **Memory leaks:** 70% retention = 30% forgotten
   - No decay mechanism
   - No spaced repetition
   - Weak edges never pruned

3. **Wrong reasoning paths:** Answer correct but reasoning wrong
   - No path verification
   - Doesn't check if path matches expected reasoning class
   - Can't explain WHY an answer is correct

4. **Weight drift:** Re-teaching can over-inflate weights
   - Uncapped growth (weight → ∞)
   - No utility tracking
   - Unused edges accumulate

5. **No generalization testing:** Teaches "cats→milk", doesn't test "dogs→milk"

---

## Deliverables Overview

### Core Components (8 new files + 6 modifications)

**New Files:**
1. `src/reasoning/multihop.h/.cpp` - k-hop beam search with relation priors
2. `src/learning/srs_scheduler.h/.cpp` - Spaced repetition queue
3. `src/learning/edge_decay.h/.cpp` - Ebbinghaus forgetting curve
4. `src/teaching/teaching_format_v2.h/.cpp` - Extended parser (ALIAS, DERIVE, NEG)
5. `src/teaching/path_verifier.h/.cpp` - Path-level correctness checking
6. `src/tools/admin_decay.cpp` - CLI tool for applying decay
7. `src/tools/teach_srs.cpp` - CLI tool for SRS-driven teaching
8. `lessons/03_multihop.tch` - Multi-hop test lesson

**Modified Files:**
1. `src/teaching/teaching_verify.cpp` - Use multi-hop reasoning
2. `src/teaching/teaching_ingest.cpp` - Handle ALIAS/DERIVE/NEG
3. `src/teaching/teaching_metrics.cpp` - Add hop_count, path_class
4. `learning_hooks.cpp` - Add utility tracking, weight capping
5. `Makefile` - Add teach_srs, decay, eval_generalize targets
6. `docs/TEACHING_FORMAT.md` - Document v2 tags

---

## Part 1: Multi-Hop Reasoning

### 1.1 File: `src/reasoning/multihop.h`

```cpp
namespace melvin {
namespace reasoning {

// Relation type enum (for priors)
enum class RelationType {
    ISA,        // Category (ARE, IS_A)
    PRODUCES,   // Creation/output
    HAS,        // Property/possession
    REQUIRES,   // Dependency
    CAUSES,     // Causation
    PART_OF,    // Composition
    ASSOC,      // Generic association
    UNKNOWN
};

// Relation priors for different query types
struct RelationPriors {
    std::unordered_map<std::string, float> weights;
    
    static RelationPriors for_query_type(const std::string& question);
};

// Path candidate for beam search
struct PathCandidate {
    std::vector<uint64_t> nodes;
    std::vector<std::string> relations;
    float score;
    int hop_count;
};

// Multi-hop reasoning options
struct MultiHopOptions {
    int max_hops = 3;
    int beam_width = 5;
    float min_edge_weight = 0.1f;
    bool use_relation_priors = true;
    bool use_rules = false;  // Virtual rule edges (future)
};

// Multi-hop result
struct MultiHopResult {
    std::string answer;
    std::vector<uint64_t> path;
    std::vector<std::string> relations_used;
    float confidence;
    int hop_count;
    bool success;
};

// Multi-hop reasoning engine
class MultiHopReasoner {
public:
    // Answer query with k-hop search
    static MultiHopResult answer_query(
        const std::string& question,
        std::unordered_map<uint64_t, ::Node>& nodes,
        std::vector<::Edge>& edges,
        const MultiHopOptions& opts = MultiHopOptions()
    );
    
private:
    // Beam search over graph
    static std::vector<PathCandidate> beam_search(
        const std::vector<uint64_t>& start_nodes,
        const std::vector<uint64_t>& target_nodes,
        std::unordered_map<uint64_t, ::Node>& nodes,
        std::vector<::Edge>& edges,
        const MultiHopOptions& opts
    );
    
    // Score a path
    static float score_path(
        const PathCandidate& path,
        const RelationPriors& priors,
        const MultiHopOptions& opts
    );
    
    // Extract query keywords → node IDs
    static std::vector<uint64_t> extract_query_nodes(
        const std::string& question,
        std::unordered_map<uint64_t, ::Node>& nodes
    );
    
    // Determine relation type from edge
    static RelationType classify_relation(const std::string& rel);
};

} // namespace reasoning
} // namespace melvin
```

### 1.2 Implementation: `multihop.cpp`

**Key Algorithm:**

```cpp
MultiHopResult MultiHopReasoner::answer_query(...) {
    // 1. Extract start nodes from question keywords
    auto start_nodes = extract_query_nodes(question, nodes);
    
    // 2. Determine query type and get relation priors
    RelationPriors priors = RelationPriors::for_query_type(question);
    
    // 3. Beam search with k hops
    auto beam = beam_search(start_nodes, {}, nodes, edges, opts);
    
    // 4. Select best path
    PathCandidate best;
    for (const auto& candidate : beam) {
        float score = score_path(candidate, priors, opts);
        if (score > best.score) best = candidate;
    }
    
    // 5. Return answer from final node
    MultiHopResult result;
    if (!best.nodes.empty()) {
        result.answer = nodes[best.nodes.back()].text;
        result.path = best.nodes;
        result.relations_used = best.relations;
        result.hop_count = best.hop_count;
        result.confidence = best.score;
        result.success = true;
    }
    
    return result;
}
```

**Path Scoring:**

```cpp
float score_path(const PathCandidate& path, const RelationPriors& priors, ...) {
    float score = 1.0f;
    
    // Edge weight product
    for (const auto& rel : path.relations) {
        // Look up edge weight between consecutive nodes
        score *= edge_weight;
    }
    
    // Relation prior bonus
    for (const auto& rel : path.relations) {
        float prior = priors.weights.get(rel, 0.5f);
        score *= prior;
    }
    
    // Hop bonus (prefer shorter paths when tied)
    float hop_penalty = 1.0f / (1.0f + path.hop_count * 0.2f);
    score *= hop_penalty;
    
    return score;
}
```

---

## Part 2: Spaced Repetition System

### 2.1 File: `src/learning/srs_scheduler.h`

```cpp
namespace melvin {
namespace learning {

// SRS card states
enum class CardState {
    NEW,       // Never tested
    LEARNING,  // Failed at least once
    MATURE,    // Passed multiple times
    LAPSED     // Was mature, now failed
};

// SRS card for a test
struct SRSCard {
    std::string test_id;       // "lesson::test_name"
    std::string question;
    std::vector<std::string> expects;
    CardState state = CardState::NEW;
    int reviews = 0;
    int successes = 0;
    int failures = 0;
    double ease_factor = 2.5;  // SM-2 algorithm
    int interval_days = 0;
    std::string next_review_date;  // ISO 8601
    std::string last_review_date;
};

// SRS queue
class SRSScheduler {
public:
    // Load SRS state
    bool load(const std::string& filepath = "melvin_srs.json");
    
    // Save SRS state
    bool save(const std::string& filepath = "melvin_srs.json");
    
    // Add cards from teaching doc
    void add_cards(const TchDoc& doc, const std::string& lesson_id);
    
    // Get due cards (for today)
    std::vector<SRSCard> get_due_cards(const std::string& today_iso);
    
    // Record review result
    void record_review(
        const std::string& test_id,
        bool passed,
        int quality  // 0-5, SM-2 scale
    );
    
    // Compute next review date (SM-2 algorithm)
    static std::string compute_next_review(
        const SRSCard& card,
        bool passed,
        int quality
    );
    
private:
    std::unordered_map<std::string, SRSCard> cards_;
};

} // namespace learning
} // namespace melvin
```

### 2.2 File: `src/learning/edge_decay.h`

```cpp
namespace melvin {
namespace learning {

// Decay options
struct DecayOptions {
    float lambda = 0.015f;  // Decay rate (per day)
    float w_min = 0.1f;     // Minimum weight (don't decay to zero)
    bool prune_low = false; // Remove edges below w_min
    bool verbose = true;
};

// Decay result
struct DecayResult {
    uint32_t edges_decayed;
    uint32_t edges_pruned;
    float avg_weight_before;
    float avg_weight_after;
};

// Edge decay manager
class EdgeDecay {
public:
    // Apply Ebbinghaus decay to all edges
    static DecayResult apply_decay(
        std::vector<::Edge>& edges,
        const std::string& current_date_iso,
        const DecayOptions& opts = DecayOptions()
    );
    
    // Rehearse edges (anti-decay on success)
    static void rehearse_edges(
        std::vector<::Edge>& edges,
        const std::vector<uint64_t>& path,
        bool success,
        float alpha = 0.05f,  // Boost on success
        float beta = 0.8f     // Penalty on failure
    );
    
private:
    // Compute decay factor
    static float compute_decay(
        float days_since_use,
        float lambda
    );
};

} // namespace learning
} // namespace melvin
```

### 2.3 Implementation

**Decay formula:**
```cpp
float compute_decay(float days, float lambda) {
    return std::exp(-lambda * days);
}

// Apply to edge:
edge.weight = std::max(
    edge.weight * compute_decay(days_since_last_use, lambda),
    w_min
);
```

**Rehearsal:**
```cpp
void rehearse_edges(..., bool success, ...) {
    for (size_t i = 0; i + 1 < path.size(); i++) {
        // Find edge
        auto* edge = find_edge(path[i], path[i+1]);
        if (edge) {
            if (success) {
                edge->weight = std::min(edge->weight + alpha, 1.0f);
                edge->last_use_date = today();
            } else {
                edge->weight = std::max(edge->weight * beta, w_min);
            }
        }
    }
}
```

---

## Part 3: Teaching Format v2

### 3.1 New Tags

**#ALIAS - Synonym Declaration:**
```
#ALIAS
cats SYNONYM felines

#ALIAS
H2O MEANS water
```

Creates bidirectional SYNONYM edges with high weight (0.95).

**#DERIVE - Expected Reasoning Path:**
```
#DERIVE
cats → ISA mammals → PRODUCES milk

#DERIVE
fire → REQUIRES oxygen → IS_A element
```

Specifies the expected path class for verification.
Format: `node1 → REL1 node2 → REL2 node3 ...`

**#NEG - Negative Example:**
```
#NEG
stones PRODUCE milk

#NEG
water IS_A solid
```

Creates anti-edge (negative weight, -0.5) to penalize incorrect paths.

### 3.2 Parser Updates

Extend `teaching_format.h`:

```cpp
enum class BlockType {
    // ... existing ...
    ALIAS,
    DERIVE,
    NEG
};

struct TchAlias {
    std::string left;
    std::string rel;  // SYNONYM, MEANS, EQUIVALENT_TO
    std::string right;
    Meta meta;
};

struct TchDerive {
    std::vector<std::string> path_nodes;
    std::vector<std::string> path_relations;
    Meta meta;
};

struct TchNeg {
    std::string subj;
    std::string rel;
    std::string obj;
    Meta meta;
};

struct TchBlock {
    // ... existing ...
    std::optional<TchAlias> alias;
    std::optional<TchDerive> derive;
    std::optional<TchNeg> neg;
};
```

---

## Part 4: Path Verification

### 4.1 File: `src/teaching/path_verifier.h`

```cpp
namespace melvin {
namespace teaching {

// Path class (expected reasoning pattern)
enum class PathClass {
    DIRECT,         // 1-hop
    ISA_PROPERTY,   // X→ISA→Y→HAS/PRODUCES→Z
    CAUSAL_CHAIN,   // X→CAUSES→Y→CAUSES→Z
    COMPOSITIONAL,  // X→PART_OF→Y→HAS→Z
    ANY             // No specific requirement
};

// Path verification result
struct PathVerifyResult {
    bool path_correct;
    bool answer_correct;
    bool fully_correct;  // Both path and answer
    std::string path_class_found;
    std::string path_class_expected;
    int hop_count;
    float top2_margin;
};

// Path verifier
class PathVerifier {
public:
    // Verify that reasoning path matches expected class
    static PathVerifyResult verify_path(
        const MultiHopResult& reasoning,
        const TchQuery& expected,
        const TchDerive* derive_spec  // May be null
    );
    
    // Classify a path's reasoning pattern
    static PathClass classify_path(
        const std::vector<std::string>& relations
    );
    
    // Check if path matches DERIVE specification
    static bool matches_derive_spec(
        const std::vector<uint64_t>& actual_path,
        const std::vector<std::string>& actual_relations,
        const TchDerive& spec,
        std::unordered_map<uint64_t, ::Node>& nodes
    );
};

} // namespace teaching
} // namespace melvin
```

### 4.2 Path Classification

```cpp
PathClass classify_path(const std::vector<std::string>& rels) {
    if (rels.size() == 1) {
        return PathClass::DIRECT;
    }
    
    // ISA → PRODUCES/HAS pattern
    if (rels.size() == 2) {
        if (is_isa(rels[0]) && is_property(rels[1])) {
            return PathClass::ISA_PROPERTY;
        }
    }
    
    // CAUSES chain
    bool all_causal = std::all_of(rels.begin(), rels.end(), is_causal);
    if (all_causal) {
        return PathClass::CAUSAL_CHAIN;
    }
    
    // PART_OF → HAS
    if (rels.size() == 2 && is_compositional(rels[0]) && is_property(rels[1])) {
        return PathClass::COMPOSITIONAL;
    }
    
    return PathClass::ANY;
}
```

---

## Part 5: Enhanced Verification

### 5.1 Generalization Testing

Add to `teaching_verify.cpp`:

```cpp
// Auto-generate generalization tests
std::vector<TchQuery> generate_generalization_tests(
    const TchDoc& doc,
    std::unordered_map<uint64_t, ::Node>& nodes
) {
    std::vector<TchQuery> gen_tests;
    
    for (const auto& block : doc.blocks) {
        if (block.type == BlockType::QUERY && block.query) {
            // Example: "What do cats produce?" → milk
            // Generate: "What do dogs produce?" (should NOT be milk)
            
            std::string question = block.query->question;
            
            // Find subject in question
            // Replace with sibling node (same ISA parent)
            // Expect different answer or "(no answer)"
            
            // Implementation: Find "cats" in question, find siblings via ISA edges,
            // substitute, mark as generalization test
        }
    }
    
    return gen_tests;
}
```

### 5.2 Verification Report Enhancement

Extend `VerifyResult`:

```cpp
struct VerifyResult {
    // ... existing ...
    
    // New fields
    float avg_hop_count;
    float path_correctness_rate;  // Path matches expected class
    int generalization_tests_run;
    int generalization_tests_passed;
    std::vector<PathVerifyResult> detailed_results;
};
```

---

## Part 6: Drift Control & Utility Tracking

### 6.1 Edge Utility

Add to `Edge` struct (in learning_hooks.cpp):

```cpp
struct Edge {
    // ... existing fields ...
    
    // NEW: Utility tracking
    uint32_t queries_routed = 0;  // # successful queries using this edge
    uint32_t queries_attempted = 0;  // # queries that tried this edge
    std::string last_use_date;  // ISO 8601 for decay computation
    
    float utility() const {
        if (queries_attempted == 0) return 0.0f;
        return static_cast<float>(queries_routed) / queries_attempted;
    }
};
```

### 6.2 Weight Capping

In `teaching_ingest.cpp`:

```cpp
bool create_or_update_edge(...) {
    // ... existing logic ...
    
    if (found) {
        // Cap weight to prevent infinite inflation
        const float W_CAP = 0.95f;
        edge->weight = std::min(edge->weight + delta, W_CAP);
        edge->teaches_count++;  // Track how many times taught
    } else {
        // Initialize with confidence-based weight
        edge->weight = std::min(initial_weight, W_CAP);
    }
}
```

---

## Part 7: CLI Tools

### 7.1 `src/tools/admin_decay.cpp`

```cpp
// Apply decay to all edges based on time since last use
int main(int argc, char* argv[]) {
    float lambda = 0.015f;  // Default decay rate
    
    // Parse --lambda flag
    
    // Load brain
    melvin::load_brain_snapshot("melvin_brain.bin", G_nodes, G_edges);
    
    // Apply decay
    DecayResult result = EdgeDecay::apply_decay(
        G_edges, current_date_iso(), DecayOptions{.lambda = lambda}
    );
    
    // Save
    melvin::save_brain_snapshot("melvin_brain.bin", G_nodes, G_edges);
    
    // Report
    std::cout << "Decayed " << result.edges_decayed << " edges\n";
    std::cout << "Pruned " << result.edges_pruned << " edges\n";
    std::cout << "Avg weight: " << result.avg_weight_before 
              << " → " << result.avg_weight_after << "\n";
}
```

### 7.2 `src/tools/teach_srs.cpp`

```cpp
// Run spaced repetition review
int main(int argc, char* argv[]) {
    // Load SRS state
    SRSScheduler srs;
    srs.load("melvin_srs.json");
    
    // Get due cards
    auto due = srs.get_due_cards(today_iso());
    
    std::cout << due.size() << " cards due for review\n";
    
    // Run each card
    for (auto& card : due) {
        // Create temporary teaching doc with just this query
        TchDoc doc;
        // ... populate with card.question, card.expects ...
        
        // Verify
        auto result = TeachingVerifier::verify(doc, G_nodes, G_edges);
        
        // Record result
        bool passed = result.tests_passed > 0;
        int quality = passed ? 4 : 1;  // SM-2 quality score
        
        srs.record_review(card.test_id, passed, quality);
        
        // Rehearse edges if needed
        if (passed) {
            EdgeDecay::rehearse_edges(G_edges, result.path_used, true);
        }
    }
    
    // Save updated SRS state
    srs.save("melvin_srs.json");
}
```

---

## Part 8: Makefile Targets

Add to `Makefile`:

```makefile
# Multi-hop reasoning
MULTIHOP_OBJS = src/reasoning/multihop.o

src/reasoning/multihop.o: src/reasoning/multihop.cpp src/reasoning/multihop.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# SRS and decay
SRS_OBJS = src/learning/srs_scheduler.o src/learning/edge_decay.o

admin_decay: src/tools/admin_decay.cpp $(SRS_OBJS) $(LEARNING_OBJS) vm.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

teach_srs: src/tools/teach_srs.cpp $(SRS_OBJS) $(TEACHING_OBJS) $(LEARNING_OBJS) vm.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

# Commands
decay:
	./admin_decay --lambda=0.015
	@echo "[DECAY] Applied Ebbinghaus decay"

srs:
	./teach_srs
	@echo "[SRS] Spaced repetition review complete"

eval_generalize: teach_file
	@echo "Testing generalization..."
	./teach_file --file lessons/00_basics.tch --generalize

.PHONY: decay srs eval_generalize
```

---

## Part 9: Sample Multi-Hop Lesson

### File: `lessons/03_multihop.tch`

```
# Lesson 03: Multi-Hop Reasoning Test
# Tests that Melvin can chain relationships

#FACT
cats ARE mammals

#FACT
mammals PRODUCE milk

#DERIVE
cats → ARE mammals → PRODUCE milk

#TEST CatsMilkMultiHop
#QUERY
What do cats produce?
#EXPECT
milk

#FACT
dogs ARE mammals

#DERIVE
dogs → ARE mammals → PRODUCE milk

#TEST DogsMilkMultiHop
#QUERY
What do dogs produce?
#EXPECT
milk

# Negative examples (should fail)

#NEG
stones PRODUCE milk

#TEST StonesNoMilk
#QUERY
Do stones produce milk?
#EXPECT
no, (no answer found)

# 3-hop chain

#FACT
lions ARE felines

#FACT
felines ARE mammals

#DERIVE
lions → ARE felines → ARE mammals → PRODUCE milk

#TEST LionsMilk3Hop
#QUERY
What do lions produce?
#EXPECT
milk

# Synonyms for robust matching

#ALIAS
felines SYNONYM cats

#ALIAS
H2O MEANS water

#TEST H2OAlias
#QUERY
What is H2O?
#EXPECT
water, liquid

# Summary: Tests 2-hop and 3-hop reasoning
# Expected pass rate with multi-hop: ≥80%
# Expected pass rate without multi-hop: ≤30%
```

---

## Part 10: Implementation Order

### Phase 1: Multi-Hop Foundation
1. Implement `multihop.h/.cpp` (beam search, relation priors)
2. Integrate with `teaching_verify.cpp`
3. Test on `03_multihop.tch`
4. Verify "cats→milk" now passes

### Phase 2: Format v2
1. Extend parser for ALIAS, DERIVE, NEG
2. Update ingestion for new block types
3. Test ALIAS matching, NEG penalties

### Phase 3: Path Verification
1. Implement `path_verifier.h/.cpp`
2. Add path class checking to verification
3. Report path correctness separately

### Phase 4: SRS & Decay
1. Implement `srs_scheduler.h/.cpp`
2. Implement `edge_decay.h/.cpp`
3. Create `admin_decay` and `teach_srs` tools
4. Add nightly decay cron job

### Phase 5: Integration & Testing
1. Wire multi-hop into all verification
2. Enable SRS mode in teach_curriculum
3. Run full curriculum with multi-hop
4. Verify ≥85% retention

---

## Success Criteria

### Multi-Hop (AC1-3)

**AC1:** "What do cats produce?" returns "milk" (2-hop)
**AC2:** "What do lions produce?" returns "milk" (3-hop via felines→mammals)
**AC3:** Pass rate on 03_multihop.tch ≥80% (was ≤30% with 1-hop)

### SRS & Decay (AC4-6)

**AC4:** `make decay` reduces avg edge weight by ~10-20% for old edges
**AC5:** `make srs` only reviews due cards (not entire curriculum)
**AC6:** Successful review increases edge weight; failure decreases

### Path Verification (AC7-8)

**AC7:** DERIVE specs checked, path class reported
**AC8:** Generalization tests auto-generated and run

### Format v2 (AC9-10)

**AC9:** ALIAS creates bidirectional synonyms
**AC10:** NEG creates anti-edges, penalizes incorrect paths

---

## Expected Improvements

### Before Patch Pack E

- Pass rate: 70% average
- Hop capability: 1-hop only
- "cats→milk": ❌ FAILS
- Retention: No decay (inflates forever)
- Path verification: Answer only

### After Patch Pack E

- Pass rate: ≥85% average (target)
- Hop capability: 3-hop beam search
- "cats→milk": ✅ PASSES (2-hop)
- Retention: SRS + decay (realistic forgetting)
- Path verification: Answer + reasoning path

### Metrics Improvements

| Metric | Before | After | Change |
|--------|--------|-------|--------|
| Pass rate | 70% | ≥85% | +15% |
| Avg hop count | 1.0 | 1.5-2.0 | Multi-hop working |
| Path correctness | N/A | ≥80% | New metric |
| Memory bloat | Uncapped | Capped+pruned | Controlled |

---

## Code Estimates

### New Code

- Multi-hop reasoning: ~400 lines
- SRS scheduler: ~300 lines
- Edge decay: ~200 lines
- Path verifier: ~250 lines
- Format v2 parser: ~150 lines
- CLI tools (decay, srs): ~200 lines
- **Total:** ~1,500 lines

### Modified Code

- teaching_verify.cpp: ~100 lines changed
- teaching_ingest.cpp: ~80 lines changed
- teaching_metrics.cpp: ~50 lines changed
- learning_hooks.cpp: ~60 lines changed
- **Total:** ~290 lines modified

### Documentation

- Update TEACHING_FORMAT.md: +200 lines
- New MULTIHOP_REASONING.md: ~400 lines
- New SRS_GUIDE.md: ~300 lines
- **Total:** ~900 lines docs

### Grand Total

**~2,700 lines** for Patch Pack E

---

## Testing Plan

### Test 1: Multi-Hop Verification

```bash
make teach_reset
make teach FILE=lessons/03_multihop.tch

# Expected:
# "What do cats produce?" → "milk" ✅ (was ❌)
# "What do lions produce?" → "milk" ✅ (3-hop)
# Pass rate: ≥80% (was ~30%)
```

### Test 2: Decay Mechanism

```bash
# Teach curriculum
make teach_dir DIR=lessons/

# Apply decay (simulate 7 days)
make decay

# Re-verify
make teach_verify FILE=lessons/00_basics.tch

# Expected: Some tests fail (edges weakened)
# Pass rate drops ~5-10% (realistic forgetting)
```

### Test 3: SRS Rehearsal

```bash
# Initial teaching
make teach_dir DIR=lessons/

# Mark some cards as due
# (manually edit melvin_srs.json or wait 1 day)

# Run SRS
make srs

# Expected: Only due cards reviewed
# Successful reviews boost edge weights
# Failed reviews recorded for next session
```

### Test 4: Path Verification

```bash
make teach FILE=lessons/03_multihop.tch --require-path

# Expected:
# Report shows path_class for each test
# DERIVE specs checked
# Failures if answer correct but path wrong
```

---

## Integration Points

### With Learning System

Multi-hop reasoning → better paths → better learning signals

### With Teaching System

Enhanced format → richer specifications → better verification

### With Database

New metrics in JSONL:
- `hop_count_avg`
- `path_correctness_rate`
- `generalization_pass_rate`
- `decay_applied` (boolean)

### With Research Log

Document in Entry 3:
```markdown
## Entry 3 - Multi-Hop Reasoning Enabled

Implemented Patch Pack E:
  - Multi-hop: 1-hop → 3-hop beam search
  - Pass rate: 70% → 87% (+17%)
  - "cats→milk": Now passes via 2-hop
  - SRS enabled: 45 cards in queue
  - Decay applied: Avg weight 0.75 → 0.68

Evidence: 03_multihop.tch passes with 92% (was 28%)
```

---

## Files to Create

**New (8 files):**
1. src/reasoning/multihop.h
2. src/reasoning/multihop.cpp
3. src/learning/srs_scheduler.h
4. src/learning/srs_scheduler.cpp
5. src/learning/edge_decay.h
6. src/learning/edge_decay.cpp
7. src/teaching/path_verifier.h
8. src/teaching/path_verifier.cpp
9. src/tools/admin_decay.cpp
10. src/tools/teach_srs.cpp
11. lessons/03_multihop.tch
12. docs/MULTIHOP_REASONING.md
13. docs/SRS_GUIDE.md

**Modified (6 files):**
1. src/teaching/teaching_format.h (add ALIAS/DERIVE/NEG)
2. src/teaching/teaching_format.cpp (parse new tags)
3. src/teaching/teaching_verify.cpp (use multi-hop)
4. src/teaching/teaching_ingest.cpp (handle new blocks)
5. src/teaching/teaching_metrics.cpp (new metrics)
6. learning_hooks.cpp (utility tracking, weight cap)
7. Makefile (new targets)
8. docs/TEACHING_FORMAT.md (document v2)

---

## Quick Win Test

Create and run `lessons/03_multihop.tch`:

```bash
# Before Patch Pack E
make teach FILE=lessons/03_multihop.tch
# Expected: ~30% pass (1-hop fails most tests)

# After Patch Pack E
make teach FILE=lessons/03_multihop.tch
# Expected: ≥80% pass (multi-hop solves chains)

# Generalization
make eval_generalize
# Expected: Auto-generated sibling tests show transfer
```

---

## Research Questions Enabled

With Patch Pack E, you can investigate:

1. **Optimal k for multi-hop:** Test k=2,3,4,5 - which maximizes accuracy?
2. **Decay rate tuning:** Test λ=0.01,0.015,0.02 - which balances retention vs pruning?
3. **SRS effectiveness:** Compare SRS vs random review - does spacing help?
4. **Path class distribution:** Which reasoning patterns emerge most?
5. **Generalization transfer:** Teaching domain A improves domain B?

---

## Timeline Estimate

### Implementation

- Multi-hop reasoning: 3-4 hours
- SRS scheduler: 2-3 hours
- Edge decay: 1-2 hours
- Path verification: 2-3 hours
- Format v2: 1-2 hours
- CLI tools: 1 hour
- Testing & integration: 2 hours
- Documentation: 1-2 hours

**Total:** ~12-18 hours of focused implementation

### Testing & Tuning

- Parameter tuning (λ, k, priors): 2 hours
- Curriculum adjustment: 1 hour
- Edge case testing: 2 hours

**Total:** ~17-23 hours complete

---

## Recommended Approach

### Option A: Implement All at Once (Full Patch)

Deliver complete Patch Pack E in one session:
- All 13 new files
- All 8 modifications
- Full testing
- Complete documentation

**Time:** ~20 hours  
**Benefit:** Complete solution, tested together

### Option B: Incremental (3 Sub-Patches)

**B1:** Multi-hop reasoning only (~4 hours)
**B2:** SRS + decay (~5 hours)
**B3:** Path verification + format v2 (~6 hours)

**Total:** Same ~20 hours  
**Benefit:** Can test/commit incrementally

### Option C: Priority Features Only

Just multi-hop + weight capping (~6 hours)
- Solves "cats→milk" failure
- Prevents weight inflation
- Defers SRS for later

---

## Your Call

**Which approach?**

- **A) Full Patch Pack E** - All features, one implementation
- **B) Incremental** - Three sub-patches, commit between
- **C) Priority only** - Multi-hop + weight cap, defer SRS

**Just say "Implement A" (or B, C)** and I'll build it with the same thoroughness as the teaching system.

---

**Melvin has a teacher. Let's make him a deep thinker.** 🧠


