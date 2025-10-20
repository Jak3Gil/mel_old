# MELVIN v1 → v2 Migration Guide

## Overview

This guide describes the planned migration from MELVIN v1 (hand-tuned, component-based) to MELVIN v2 (evolution-first, genome-driven). The migration is designed to be **gradual and non-disruptive**, running v2 in parallel with v1 during the transition.

### Timeline

| Phase | Duration | Status | Description |
|-------|----------|--------|-------------|
| **Phase 0** | Months 0-3 | 🚧 Current | v2 development, v1 operational |
| **Phase 1** | Months 3-6 | ⏳ Planned | Gradual module migration |
| **Phase 2** | Months 6-9 | ⏳ Planned | v1 deprecation warnings |
| **Phase 3** | Months 9-12 | ⏳ Planned | v1 code removal |

---

## Phase 0: Parallel Development (Months 0-3)

**Goal:** Build v2 alongside v1 without breaking existing functionality.

### Strategy

- v2 code lives in `melvin/v2/` namespace
- v1 code remains untouched in `melvin/core/`
- Bridge adapters allow v2 to use v1 components
- Separate build system (`Makefile.v2`)
- Independent testing

### Bridge Adapters (V1 ← V2 Interface)

v2 accesses v1 components through adapter classes:

```cpp
// v2 → v1 bridges
melvin::v2::SemanticBridge    → melvin::AtomicGraph (4.29M edges preserved!)
melvin::v2::HopfieldBridge    → melvin::hopfield::HopfieldDiffusion
melvin::v2::PerceptionBridge  → melvin::vision::VisionBridge
melvin::v2::ReasoningBridge   → melvin::ReasoningEngine
```

**Example: SemanticBridge**

```cpp
// V2 code
auto semantic_bridge = std::make_unique<SemanticBridge>(v1_atomic_graph);
auto related = semantic_bridge->find_related(concept_id, 0.5f);

// V1 graph is accessed but not modified (read-only mode available)
semantic_bridge->set_readonly_mode(true);
```

### What Gets Preserved

✅ **All V1 Knowledge**
- 65,536 nodes
- 4,290,229 edges (183K EXACT + 4.1M LEAP)
- 34.6 MB storage
- Zero migration cost - v2 reads v1 format directly

✅ **All V1 Capabilities**
- Vision processing (camera, YOLO)
- Audio pipeline
- Reasoning engine
- Hopfield pattern completion
- Episodic memory

✅ **All V1 Tools**
- Python scripts
- Visualization
- Analysis tools

### Current Progress (Phase 0)

- [x] v2 directory structure
- [x] Core types defined
- [x] Genome system complete
- [x] Neuromodulators complete
- [x] Global Workspace complete
- [x] Build system working
- [x] Demo running
- [ ] Working Memory (in progress)
- [ ] Bridge adapters (in progress)
- [ ] Attention system (in progress)
- [ ] Unified loop v2 (in progress)

---

## Phase 1: Gradual Migration (Months 3-6)

**Goal:** Migrate one module at a time, v1 and v2 coexist.

### Migration Order

1. **Perception** (Month 3)
   - v2 PerceptionBridge wraps v1 vision
   - v2 attention uses v2 formula with genome weights
   - v1 vision still accessible

2. **Memory Systems** (Month 4)
   - v2 Working Memory replaces ad-hoc WM
   - v2 SemanticBridge reads v1 AtomicGraph
   - v2 Episodic uses v1 episode storage format
   - No data migration needed

3. **Reasoning** (Month 5)
   - v2 Predictive Coding added
   - v2 ReasoningBridge wraps v1 ReasoningEngine
   - Both engines available

4. **Output** (Month 6)
   - v2 Affordances & Policy
   - v2 UnifiedLoopV2 runs full cognitive cycle
   - v1 loop still operational

### Compatibility Layer

During Phase 1, both v1 and v2 APIs are available:

```cpp
// V1 API (still works)
#include "melvin/core/atomic_graph.h"
AtomicGraph* graph = new AtomicGraph();

// V2 API (new)
#include "melvin/v2/memory/semantic_bridge.h"
auto bridge = std::make_unique<SemanticBridge>(graph);  // Wraps v1

// Both can coexist in same binary
```

### Testing Strategy

- **Dual testing**: Run same tests on v1 and v2
- **Benchmark comparison**: Performance, accuracy, memory
- **Regression detection**: v2 must not break v1 behavior
- **Feature parity**: v2 matches v1 capabilities before migration

---

## Phase 2: V1 Deprecation (Months 6-9)

**Goal:** Warn users, provide migration tools, freeze v1 development.

### Deprecation Warnings

Add compiler warnings to v1 headers:

```cpp
// melvin/core/atomic_graph.h
#pragma message("WARNING: AtomicGraph is deprecated. Use melvin::v2::SemanticBridge instead.")
```

### Migration Tools

Provide automated migration:

```bash
# Migrate v1 code to v2
./tools/migrate_v1_to_v2.sh my_project/

# Shows:
# - Replaced includes
# - Updated API calls
# - Bridge adapter insertions
```

### Documentation Updates

- Mark v1 docs as "Legacy"
- Update examples to use v2
- Provide side-by-side comparison

### V1 Feature Freeze

- No new v1 features
- Bug fixes only
- All new work in v2

---

## Phase 3: V1 Removal (Months 9-12)

**Goal:** Remove v1 code, complete migration.

### Removal Checklist

- [ ] All v2 bridges replaced with native v2 implementations
- [ ] All tests passing with v2 only
- [ ] Performance equal or better than v1
- [ ] Documentation complete
- [ ] Migration guide for external users

### Code Removal

```bash
# Archive v1 code
git mv melvin/core melvin/archive/v1_core
git mv melvin/audio melvin/archive/v1_audio
git mv melvin/vision melvin/archive/v1_vision

# Keep archive branch
git checkout -b v1-archive
git push origin v1-archive
```

### Final V2 Structure

```
melvin/
├── v2/                  → melvin/  (v2 becomes main)
├── archive/v1_core/     (read-only reference)
├── tests/               (v2 tests)
├── demos/               (v2 demos)
└── tools/               (v2 tools)
```

---

## API Comparison: V1 vs V2

### Creating a Brain

**V1 (Hard-coded parameters):**
```cpp
AtomicGraph graph;
AttentionManager attention;
attention.weights_.wA = 0.4f;  // Manual tuning
attention.weights_.wR = 0.3f;
// ... 100+ parameters to set
```

**V2 (Genome-driven):**
```cpp
auto genome = GenomeFactory::create_base();
auto brain = expression::express_brain(genome, seed);
// All 49 parameters come from genome
```

### Querying Knowledge

**V1:**
```cpp
AtomicGraph* graph;
auto neighbors = graph->neighbors(node_id);
// Direct access, no interface
```

**V2:**
```cpp
SemanticBridge* semantic;
auto related = semantic->find_related(node_id, threshold);
// Unified interface, v1 graph underneath
```

### Attention Selection

**V1:**
```cpp
AttentionManager attention;
auto focus = attention.select_focus_target(time);
// Fixed weights
```

**V2:**
```cpp
AttentionArbitration attention(genome);  // Weights from genes
auto focus = attention.select_focus(candidates, saliency, topdown, neuromod);
// Weights evolved, neuromodulator-influenced
```

### Learning Rate

**V1:**
```cpp
float learning_rate = 0.001f;  // Hard-coded constant
```

**V2:**
```cpp
Neuromodulators neuromod(genome);
float learning_rate = neuromod.get_effective_learning_rate(base_rate);
// DA-modulated, genome-driven, context-dependent
```

---

## Data Migration

### Knowledge Graph (AtomicGraph)

**No migration needed!** v2 SemanticBridge reads v1 format directly.

```cpp
// V1 storage
nodes.bin (262 KB)
edges.bin (34.3 MB)

// V2 reads same files
SemanticBridge bridge(v1_graph);  // Zero-copy access
```

### Episodic Memory

**Format compatible:** v2 episodic memory uses v1's episode ID scheme.

```cpp
// V1 episodes
Episode { id, start_time, end_time, node_sequence }

// V2 reads v1 episodes
EpisodicMemory episodic;
episodic.load_v1_episodes("data/episodes.bin");
```

### Hopfield Patterns

**Embeddings preserved:** v2 Hopfield bridge uses v1 embeddings.

```cpp
// V1 embeddings in Node.embedding (32-dim float vector)
// V2 HopfieldBridge accesses same vectors
```

---

## Breaking Changes

### Minimal Breakage

v2 is designed for **maximum compatibility**. Breaking changes are limited to:

1. **Namespace change**
   - v1: `melvin::`
   - v2: `melvin::v2::`
   - Fix: Use bridge adapters during transition

2. **Config structure**
   - v1: Scattered config variables
   - v2: `Genome` object
   - Fix: Extract from genome with helper functions

3. **Thread safety**
   - v1: Some components not thread-safe
   - v2: All components thread-safe
   - Fix: Should "just work" better

### Non-Breaking Changes

These are **enhancements**, not breakages:

- **Evolution**: New capability, doesn't affect existing code
- **Neuromodulators**: New layer, transparent to existing code
- **Global Workspace**: New integration point, optional use
- **Telemetry**: New monitoring, doesn't change behavior

---

## Performance Comparison

Expected v2 performance relative to v1:

| Component | V1 | V2 | Notes |
|-----------|----|----|-------|
| **Knowledge Query** | 0.0001 ms | 0.0001 ms | Same (uses v1 graph) |
| **Attention** | ~1 ms | ~1 ms | Similar (genome adds negligible overhead) |
| **Reasoning** | 5-50 ms | 5-50 ms | Same (uses v1 reasoning) |
| **Learning** | 120 f/s | 120 f/s | Same |
| **Cognitive Cycle** | ~50 ms | ~55 ms | +10% (neuromod, GW overhead) |
| **Memory** | 35 MB | 38 MB | +8% (GW, WM buffers) |

**Conclusion:** v2 overhead is **minimal** (~10%) while adding significant capabilities.

---

## Migration Checklist

### For Developers

- [ ] Read this guide completely
- [ ] Set up v2 build system (`make -f Makefile.v2`)
- [ ] Run v2 demos to understand new APIs
- [ ] Identify which v1 components you use
- [ ] Plan migration order (perception → memory → reasoning)
- [ ] Write bridge adapters for your custom components
- [ ] Add v2 tests alongside v1 tests
- [ ] Benchmark before/after performance
- [ ] Update documentation

### For Users

- [ ] Understand v2 benefits (evolution, neuromodulators)
- [ ] Note timeline (3-12 months)
- [ ] Backup v1 knowledge graphs
- [ ] Test v2 demos
- [ ] Provide feedback on new APIs
- [ ] Report compatibility issues

---

## Support & Questions

### Resources

- **v2 README**: `melvin/v2/README_V2.md`
- **Status**: `melvin/v2/STATUS_V2.md`
- **API Reference**: `melvin/v2/API_REFERENCE.md` (coming soon)
- **Design Decisions**: `melvin/v2/DESIGN_DECISIONS.md` (coming soon)

### Getting Help

- Check `STATUS_V2.md` for current implementation status
- Review demos in `melvin/v2/demos/`
- Consult bridge adapter examples
- File issues with "[v2]" prefix

---

## Summary

**MELVIN v2** is a major architectural upgrade that:

✅ **Preserves all v1 knowledge** (4.29M edges, zero migration cost)  
✅ **Runs in parallel** during transition (no forced migration)  
✅ **Maintains compatibility** through bridge adapters  
✅ **Adds evolution** (genome-driven brain parameters)  
✅ **Adds neuromodulators** (DA/NE/ACh/5-HT control)  
✅ **Minimal performance overhead** (~10%)  
✅ **Gradual migration path** (3-12 months)  

**Timeline:**
- **Now - Month 3:** v2 development (current phase)
- **Month 3-6:** Module-by-module migration
- **Month 6-9:** v1 deprecation warnings
- **Month 9-12:** v1 code removal

**The transition is designed to be smooth, incremental, and non-disruptive.**

---

**Next Steps:**
1. Complete v2 core components (WM, attention, bridges)
2. Build minimal cognitive loop
3. Run parallel tests (v1 vs v2)
4. Begin Phase 1 migration

**Questions? See `STATUS_V2.md` for current progress.**

