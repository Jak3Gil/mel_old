# MELVIN v2: Evolution-First, Humanlike Cognitive Architecture

## Overview

MELVIN v2 is a complete reimagining of the cognitive architecture with **evolution as the primary design principle**. Instead of hand-tuning parameters, v2 uses a **genome** that encodes learning rules, neuromodulator schedules, and architectural choices. Natural selection sculpts these genomes through multi-objective fitness evaluation.

### Key Innovations

1. **Genome-Driven Brain**: Every parameter comes from evolved genes
2. **Four Core Loops**: Perception → Attention → Reasoning → Action (all feedback-connected)
3. **Three Memory Systems**: Episodic (hippocampus), Semantic (cortex/graph), Procedural (skills)
4. **Neuromodulators**: DA/NE/ACh/5-HT control plasticity, exploration, attention, stability
5. **Global Workspace**: Central "blackboard" for conscious content
6. **Hopfield Attractors**: Pattern completion for noisy/partial inputs
7. **Safety First**: Constraints and monitoring baked into the architecture

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    GLOBAL WORKSPACE (GWT)                        │
│  Conscious "blackboard": goals, hypotheses, percepts, actions   │
└────────┬────────────────────────────────────────┬───────────────┘
         ↓                                        ↑
    ┌────────────┐                          ┌─────────────┐
    │ PERCEPTION │                          │  REASONING  │
    │  (Vision,  │──────────────────────────│ (Prediction,│
    │   Audio)   │   Thoughts posted to GW  │  Planning)  │
    └──────┬─────┘                          └──────┬──────┘
           ↓                                       ↑
    ┌────────────┐         ┌──────────┐    ┌──────────────┐
    │ ATTENTION  │         │ WORKING  │    │ NEUROMOD     │
    │ (Bottom-up │←────────│  MEMORY  │───→│ (DA/NE/ACh/  │
    │  + Top-    │  Bias   │ (PFC-    │    │  5-HT State) │
    │   down)    │         │  like)   │    └──────────────┘
    └──────┬─────┘         └────┬─────┘           ↑
           ↓                    ↓                  │
    ┌────────────────────────────────────┐        │
    │      MEMORY SYSTEMS                │        │
    │  • Episodic (Hippocampus)          │────────┘
    │  • Semantic (V1 AtomicGraph Bridge)│  Prediction
    │  • Procedural (Skills/Policies)    │  Errors →
    │  • Hopfield (Pattern Completion)   │  Modulators
    └────────────────────────────────────┘
```

## Directory Structure

```
v2/
├── evolution/          # Genome, mutation, selection, expression
│   ├── genome.h/cpp
│   ├── mutation.h/cpp
│   ├── selection.h/cpp
│   ├── population.h/cpp
│   ├── expression.h/cpp
│   └── fitness_env.h/cpp
├── core/               # Central cognitive components
│   ├── types_v2.h
│   ├── global_workspace.h/cpp
│   ├── neuromodulators.h/cpp
│   └── telemetry.h/cpp
├── memory/             # Three memory systems + consolidation
│   ├── working_memory.h/cpp
│   ├── episodic.h/cpp
│   ├── semantic_bridge.h/cpp      # V1 AtomicGraph adapter
│   ├── procedural.h/cpp
│   ├── hopfield_bridge.h/cpp      # V1 Hopfield adapter
│   └── consolidation.h/cpp
├── attention/          # Attention system
│   ├── saliency.h/cpp
│   ├── topdown.h/cpp
│   ├── arbitration.h/cpp
│   └── pulvinar_router.h/cpp
├── perception/         # Sensory processing
│   ├── vision_v2.h/cpp
│   ├── features.h/cpp
│   └── perception_bridge.h/cpp    # V1 vision adapter
├── reasoning/          # Reasoning & planning
│   ├── predictive_coding.h/cpp
│   ├── reasoning_bridge.h/cpp     # V1 ReasoningEngine adapter
│   └── causal_graph.h/cpp
├── action/             # Action selection
│   ├── affordances.h/cpp
│   └── policy.h/cpp
├── safety/             # Safety constraints
│   ├── constraints.h/cpp
│   └── monitors.h/cpp
├── unified_loop_v2.h/cpp           # Main cognitive tick
└── demos/              # Example applications
    ├── run_evolution.cpp
    └── minimal_loop_demo.cpp
```

## Core Concepts

### 1. Genome

A **Genome** encodes all architectural parameters:

```cpp
struct Genome {
  GenomeID id;
  std::vector<GeneModule> modules;  // attention, predictive, wm, neuromod, etc.
  
  // Each module has gene fields with constraints
  GeneField alpha_saliency { 0.4, min:0.0, max:1.0, sigma:0.05 };
  GeneField DA_gain { 0.5, min:0.0, max:2.0, sigma:0.1 };
  // ... hundreds of genes ...
};
```

**Expression**: `Genome → Brain Parameters → Working Brain`

### 2. Global Workspace Theory (GWT)

The **GlobalWorkspace** is a central blackboard where all modules post and read **Thoughts**:

```cpp
struct Thought {
  uint64_t id;
  string type;               // "percept", "goal", "hypothesis", "action"
  float salience;            // How important/activated (0-1)
  vector<NodeID> concept_refs; // Related semantic concepts
};
```

Only high-salience thoughts remain in the workspace (capacity-limited, like consciousness).

### 3. Neuromodulators

Four neuromodulators control system-wide behavior:

- **Dopamine (DA)**: Reward prediction error → gates plasticity (learning rate)
- **Norepinephrine (NE)**: Unexpected uncertainty → exploration bias
- **Acetylcholine (ACh)**: Expected uncertainty → attention gain
- **Serotonin (5-HT)**: Stability → suppress impulsive switching

Genes encode baseline levels and sensitivities.

### 4. Three Memory Systems

1. **Episodic** (Hippocampus): Time/place/context-bound episodes with fast binding
2. **Semantic** (Cortex/Graph): Facts, relations, LEAP edges (bridges to V1 AtomicGraph)
3. **Procedural** (Basal Ganglia): Skills, policies, action sequences

### 5. Attention

**Bottom-up** (stimulus-driven) + **Top-down** (goal-driven):

```
AttentionScore = α·Saliency + β·Relevance + γ·Curiosity + δ·Need + ε·Persistence
```

Genome evolves the weights {α, β, γ, δ, ε}.

### 6. Predictive Coding

Every tick:
1. Generate predictions based on current context
2. Compare with observations (prediction error)
3. Update beliefs proportional to error
4. High errors → curiosity → exploration → neuromod changes

## Evolution Process

### Multi-Objective Fitness

Genomes are evaluated on:
- **Task Success**: Goal completion rate
- **Prediction Quality**: Low surprise over time horizon
- **Context Sensitivity**: Disambiguation under ambiguity
- **Sample Efficiency**: Learning speed
- **Stability**: No oscillations/divergence
- **Exploration**: Useful curiosity (not random noise)

### Selection (NSGA-II)

Pareto-optimal individuals are kept. No single "best" brain—trade-offs between objectives.

### Mutation

- Gaussian noise on continuous genes
- Log-scale mutations for learning rates
- Rare topological changes (+1 WM slot, +1 gene field)

## V1 → V2 Migration Strategy

v2 runs **in parallel** to v1, reusing:
- **AtomicGraph** via `SemanticBridge` (4.29M edges preserved)
- **HopfieldDiffusion** via `HopfieldBridge`
- **Vision** via `PerceptionBridge`
- **ReasoningEngine** via `ReasoningBridge`

### Timeline

- **Months 0-3**: v2 development, v1 operational
- **Months 3-6**: Gradual module migration
- **Months 6-9**: v1 deprecation
- **Months 9-12**: v1 code removal

## Quick Start

### Build

```bash
make -f Makefile.v2
```

### Run Demo

```bash
./melvin/v2/demos/minimal_loop_demo
```

### Run Evolution

```bash
./melvin/v2/demos/run_evolution --pop 32 --gens 50 --tasks data/tasks.yaml
```

### Visualize

```bash
python melvin/tools/python/v2/plot_neuromod.py telemetry.json
python melvin/tools/python/v2/visualize_graph.py telemetry.json
```

## Testing

```bash
make -f Makefile.v2 test
```

All tests must pass:
- Genome mutation determinism
- Global workspace thread-safety
- Working memory gating/decay
- Neuromodulator homeostasis
- Attention arbitration
- Bridge adapters (v1 compatibility)
- E2E cognitive loop stability

## Documentation

- `MIGRATION_GUIDE.md` - V1→V2 transition plan
- `GENOME_REFERENCE.md` - All gene fields documented
- `DESIGN_DECISIONS.md` - Why evolution-first, memory systems, etc.
- `API_REFERENCE.md` - Full API documentation

## Safety & Alignment

Built-in safety:
- **Hard constraints**: Bounds on all parameters
- **Monitors**: Detect oscillations, runaway exploration
- **Red-team tests**: Adversarial probes logged as fitness penalties
- **Transparent introspection**: Every decision traceable via semantic paths

## Performance Targets

- **Cognitive cycle**: 10-30 Hz (33-100ms per tick)
- **Evolution**: Hours for 50 generations on 32 genomes
- **Memory**: < 100 MB for full brain state
- **Compatibility**: V1 knowledge graph accessible with zero migration cost

## Status

**Phase 1 (Current)**: Foundation scaffold + Genome system + Minimal cognitive loop

See `../../../melvin-v2-integration.plan.md` for complete implementation roadmap.

---

**MELVIN v2 - Where evolution sculpts intelligence.**

