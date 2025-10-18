# 🧠 Melvin UCA v1 - Architecture Documentation

## Overview

Melvin UCA (Unified Cognitive Architecture) implements a complete brain-inspired perception-reasoning-action loop that mirrors biological cognitive processing.

## The Complete Loop

```
┌─────────────────────────────────────────────────────────────────┐
│  INPUT (Camera frames)                                          │
└──────────────────┬──────────────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────────────────────────┐
│  1. PERCEPTION (MelvinVision)                                   │
│     Retina/LGN → V1 → V2 → V4 → IT                             │
│     • Tokenize frames into patches                             │
│     • Extract low-level features (edges, motion)               │
│     • Gestalt grouping into objects                            │
│     • Link to concepts                                         │
└──────────────────┬──────────────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────────────────────────┐
│  2. ATTENTION (MelvinFocus)                                     │
│     FEF + Superior Colliculus                                  │
│     • Score regions: F = α·S + β·G + γ·C                       │
│     • Select ONE focus target                                  │
│     • Inhibition of return (IOR)                               │
│     • Maintain focus with inertia                              │
└──────────────────┬──────────────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────────────────────────┐
│  3. REASONING (MelvinReasoning)                                 │
│     Prefrontal Cortex (PFC)                                    │
│     • Multi-hop graph traversal                                │
│     • Generate thoughts (subject-predicate-object)             │
│     • Update active concepts (global workspace)                │
└──────────────────┬──────────────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────────────────────────┐
│  4. REFLECTION (MelvinReflection)                               │
│     Predictive Coding (Cerebellum + ACC)                       │
│     • Generate predictions                                     │
│     • Compute prediction errors                                │
│     • Learn from errors (reinforce/decay)                      │
│     • Form LEAPs (shortcuts) when useful                       │
└──────────────────┬──────────────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────────────────────────┐
│  5. OUTPUT (MelvinOutput)                                       │
│     Motor Cortex + Broca's Area                                │
│     • Apply focus (gaze/ROI shift)                             │
│     • Generate speech (optional)                               │
│     • Log events to graph                                      │
└──────────────────┬──────────────────────────────────────────────┘
                   ↓
┌─────────────────────────────────────────────────────────────────┐
│  6. FEEDBACK                                                    │
│     Active concepts → Next perception                          │
│     • Bias attention toward relevant objects                   │
│     • Reinforce used connections                               │
│     • Decay unused connections                                 │
└──────────────────┬──────────────────────────────────────────────┘
                   │
                   └────────────┐
                                │
                   ┌────────────┘
                   ↓
           (Loop continues ~20 Hz)
```

## Biological Mappings

| Brain Region | Melvin Component | Function |
|--------------|------------------|----------|
| Retina + LGN | Frame ingestion | Convert light → neural signals |
| V1 (Primary Visual) | extract_patches() | Edge, orientation, color detection |
| V2 | group_patches() | Gestalt grouping, boundaries |
| V4 | form_objects() | Complex shapes, surfaces |
| IT (Inferior Temporal) | link_concepts() | Object recognition, semantics |
| Parietal Cortex | compute_saliency() | Bottom-up attention map |
| FEF (Frontal Eye Fields) | FocusManager | Top-down attention control |
| Superior Colliculus | select() | Saccade generation |
| PFC (Prefrontal Cortex) | ReasoningEngine | Multi-hop reasoning, planning |
| Hippocampus | AtomicGraph (episodic) | Temporal sequences, events |
| Cortical Networks | AtomicGraph (semantic) | Concept relations |
| Cerebellum | Predictions | Forward models |
| ACC | Prediction errors | Error detection |
| Motor Cortex (M1) | apply_focus() | Motor commands |
| Broca's Area | say() | Speech production |

## Constants and Formulas

### Attention Formula
```
F = α·Saliency + β·Goal + γ·Curiosity
```
Where:
- α (ALPHA_SALIENCY) = 0.45
- β (BETA_GOAL) = 0.35
- γ (GAMMA_CURIOSITY) = 0.20

### Learning Parameters
- **Decay**: λ = 0.0025 per second
- **Reinforcement**: η = 0.10
- **Curiosity boost**: 0.15 ... 0.40 (capped at 1.0)
- **LEAP threshold**: 0.12 error reduction

### Timing
- Target frequency: 20 Hz (50ms per cycle)
- Inhibition of return: 0.8 seconds
- Focus inertia: 15% boost for current target

## Data Structures

### Node (24 bytes)
```cpp
struct Node {
    uint64_t id;      // Unique identifier
    NodeType type;    // FRAME, VISUAL_PATCH, OBJECT, CONCEPT, etc.
    uint32_t a, b;    // Payload slots (coordinates, indices)
    float c, d;       // Payload slots (features, scores)
};
```

### Edge (24 bytes)
```cpp
struct Edge {
    uint64_t a, b;    // Source, target node IDs
    Rel r;            // Relation type
    float w;          // Weight/strength
};
```

## Key Algorithms

### 1. Focus Selection
```
For each candidate:
    1. Check if inhibited (IOR)
    2. Compute F = α·S + β·G + γ·C
    3. Apply inertia if current focus
    4. Select highest F score
    5. Mark as ACTIVE_FOCUS
    6. Add to inhibition list
```

### 2. Hebbian Learning
```
When nodes co-activate:
    1. Find/create edge between them
    2. Increment weight by η (0.10)
    3. Mark reinforcement time
```

### 3. Edge Decay
```
For each edge:
    w_new = w_old × exp(-λ · dt)
    If w_new < 0.01: remove edge
```

### 4. LEAP Formation
```
If A→B and B→C exist:
    error = prediction_error(A, C)
    If error > threshold (0.12):
        Create A→C edge (type: EXPECTS)
```

## Module API Reference

### AtomicGraph
- `get_or_create_concept(label)` - Get/create concept node
- `add_or_bump_edge(a, b, rel, inc)` - Hebbian edge update
- `decay_edges(seconds, lambda)` - Forgetting
- `maybe_form_leap(a, b, error, threshold)` - Transitive shortcut
- `neighbors(id, rels, k)` - Query with relation filter

### VisionSystem
- `ingest_frame(meta, rgb)` - Tokenize frame → patches
- `score_regions(frame_node)` - Compute S, G, C scores
- `group_objects(frame_node)` - Gestalt grouping
- `set_active_concepts(concepts)` - Top-down modulation

### FocusManager
- `select(candidates)` - Choose focus (FEF/SC)
- `inhibit(node_id, seconds)` - IOR
- `update(dt)` - Update timers

### ReasoningEngine
- `infer_from_focus(focus_node)` - Generate thought
- `active_concepts()` - Get global workspace
- `verbalize(thought)` - Thought → language

### ReflectionEngine
- `prediction_error(node_id)` - Compute surprise
- `learn_from_error(node_id, error)` - Update graph
- `maintenance(seconds)` - Decay + consolidation

### UnifiedMind
- `tick(frame_meta, rgb)` - One complete cycle
- `get_graph()` - Access knowledge graph
- `print_stats()` - System statistics

## Test Suite

6 demos verify complete functionality:

1. **Tokenize & Link**: Frame/patch creation, TEMPORAL_NEXT edges
2. **Gestalt Group**: Spatial grouping, PART_OF relations
3. **Saliency vs Goal**: Attention formula, top-down biasing
4. **Reasoning Hop**: Multi-hop graph inference
5. **Predictive Error**: Prediction → error → learning → LEAPs
6. **Closed Loop**: 100 cycles with all components active

## Building and Running

```bash
# Build
cd melvin
make

# Run tests
make run

# Or
./build/test_uca_system

# Run main application
make main
./build/melvin_uca [num_cycles]
```

## Performance Characteristics

- **Cycle time**: ~2-5ms per frame (synthetic data)
- **Target Hz**: 20 Hz (real-time capable)
- **Memory**: ~24 bytes/node, 24 bytes/edge
- **Scalability**: O(1) node lookup, O(k) neighbor queries

## Future Extensions

- Real camera integration
- Audio processing pipeline
- Motor control (RobStride integration)
- Transformer sequence predictor
- Multi-modal fusion
- Hierarchical planning

---

**Built with biological inspiration. Powered by graph reasoning. Ready for embodiment.** 🧠✨


