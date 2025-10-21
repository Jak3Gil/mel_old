# MELVIN v2 - Quick Start Guide

Get up and running with MELVIN v2 in 5 minutes.

---

## 🚀 Build & Run

```bash
# Build v2
make v2

# Run demo
make v2-demo

# Check status
make status
```

**Expected output:** Genome demo shows all 7 gene modules, creates variants, and expresses neuromodulators.

---

## 📁 Key Files

| Component | Files | Purpose |
|-----------|-------|---------|
| **Genome** | `v2/evolution/genome.h/cpp` | 49 genes, 7 modules |
| **Global Workspace** | `v2/core/global_workspace.h/cpp` | Conscious blackboard |
| **Neuromodulators** | `v2/core/neuromodulators.h/cpp` | DA/NE/ACh/5-HT systems |
| **Types** | `v2/core/types_v2.h` | All v2 type definitions |
| **Demo** | `v2/demos/demo_genome.cpp` | Complete example |

---

## 🧬 Create a Genome

```cpp
#include "melvin/v2/evolution/genome.h"
using namespace melvin::v2::evolution;

// Base genome (default parameters)
Genome genome = GenomeFactory::create_base();

// Random genome
Genome random = GenomeFactory::create_random(seed);

// Variant (base + noise)
Genome variant = GenomeFactory::create_from_template(genome, 0.1f, seed);
```

---

## 🧠 Express Brain Components

```cpp
#include "melvin/v2/core/neuromodulators.h"
#include "melvin/v2/core/global_workspace.h"
using namespace melvin::v2;

// Neuromodulators from genome
Neuromodulators neuromod(genome);

// Global workspace (default config)
GlobalWorkspace gw;

// Working memory (coming soon)
// WorkingMemory wm = expression::express_wm(genome);
```

---

## 💭 Post Thoughts to Global Workspace

```cpp
// Create a percept thought
Thought percept = make_percept_thought(object_id, salience=0.8f);
gw.post(percept);

// Create a goal
gw.set_goal("Find food", priority=1.0f);

// Query thoughts
auto percepts = gw.query("percept", min_salience=0.5f);

// Get snapshot (lock-free)
auto snapshot = gw.get_snapshot();
```

---

## 🧪 Use Neuromodulators

```cpp
// Get current state
auto state = neuromod.get_state();
// state.dopamine, state.norepinephrine, etc.

// React to events
neuromod.on_prediction_error(0.5f);    // Positive surprise
neuromod.on_unexpected_event(1.0f);    // High surprise
neuromod.on_goal_progress(0.2f);       // Making progress

// Get computed effects
float plasticity = neuromod.get_plasticity_rate();    // Learning rate mult
float exploration = neuromod.get_exploration_bias();  // Explore vs exploit
float attention = neuromod.get_attention_gain();      // Sensory boost
float stability = neuromod.get_stability_bias();      // Inertia mult

// Homeostatic regulation
neuromod.tick(dt=0.05f);  // Decay toward baseline
```

---

## 🔧 Access Genes

```cpp
// Quick accessors
float alpha = genome.get_alpha_saliency();
float beta = genome.get_beta_goal();
float da_gain = genome.get_da_gain();

// Generic access
float value = genome.get_gene_value("attention", "alpha_saliency");

// Modify
genome.set_gene_value("attention", "alpha_saliency", 0.6f);
```

---

## 💾 Save & Load

```cpp
// Save genome to JSON
genome.save("/tmp/my_genome.json");

// Serialize to string
std::string json = genome.to_json();

// Load (TODO: implement JSON parsing)
// genome.load("/tmp/my_genome.json");
```

---

## 📊 Get Statistics

```cpp
// Global workspace stats
auto gw_stats = gw.get_stats();
// gw_stats.total_posts
// gw_stats.current_thought_count
// gw_stats.avg_salience

// Neuromodulator stats
auto nm_stats = neuromod.get_stats();
// nm_stats.avg_dopamine
// nm_stats.total_prediction_errors
```

---

## 🎯 Common Patterns

### Pattern 1: Genome → Brain Expression

```cpp
Genome genome = GenomeFactory::create_base();
Neuromodulators neuromod(genome);
// Attention attention(genome);  // Coming soon
// WorkingMemory wm(genome);      // Coming soon
```

### Pattern 2: Event-Driven Neuromod Update

```cpp
// Prediction phase
float predicted = predict_next_state();
float actual = observe_next_state();
float error = actual - predicted;

// Update neuromodulators
neuromod.on_prediction_error(error);

// Use updated plasticity
float learning_rate = neuromod.get_effective_learning_rate(0.01f);
apply_learning(learning_rate);
```

### Pattern 3: Global Workspace Cycle

```cpp
// Each cognitive cycle
gw.clear();  // Optional: clear old thoughts

// Perception posts
gw.post(percept_thought_1);
gw.post(percept_thought_2);

// Reasoning posts
gw.post(hypothesis_thought);

// Get most salient
Thought focus = gw.get_most_salient();

// Decay & prune
gw.tick(dt);
```

---

## 📖 Documentation

| Document | Purpose |
|----------|---------|
| **README_V2.md** | Complete architecture overview |
| **STATUS_V2.md** | Current implementation status |
| **MIGRATION_GUIDE.md** | v1→v2 transition plan |
| **QUICK_START_V2.md** | This file (quick reference) |

---

## 🧪 Run the Demo

```bash
./build/v2/bin/demo_genome
```

**Demo shows:**
1. Base genome creation (7 modules, 49 genes)
2. Gene access & modification
3. JSON serialization
4. Genome variants (random, template-based)
5. Genome → neuromodulator expression
6. Event simulation (prediction error)

---

## 🔍 Inspect Build

```bash
# Library
ls -lh build/v2/libmelvinv2.a

# Demo binary
ls -lh build/v2/bin/demo_genome

# Object files
find build/v2/obj -name "*.o"
```

---

## 🎓 Learn More

### Key Concepts

- **Genome**: Encodes all 49 brain parameters
- **Evolution**: Mutation + selection shapes genomes
- **Neuromodulators**: DA/NE/ACh/5-HT control learning/attention/exploration
- **Global Workspace**: Central "conscious" blackboard (GWT)
- **Bridge Adapters**: v2 accesses v1 components

### Examples

See `melvin/v2/demos/demo_genome.cpp` for complete working example.

### Status

Current: Foundation complete (genome, GW, neuromod)  
Next: Working memory, attention, bridges, unified loop

---

## ❓ Troubleshooting

### Build fails

```bash
# Clean and rebuild
make v2-clean
make v2
```

### Demo won't run

```bash
# Check library built
ls build/v2/libmelvinv2.a

# Rebuild demo
make v2-demo
```

### Missing files

```bash
# Verify directory structure
ls melvin/v2/core/
ls melvin/v2/evolution/
```

---

## 🚀 Next Steps

1. Read `README_V2.md` for architecture details
2. Check `STATUS_V2.md` for what's implemented
3. Review `MIGRATION_GUIDE.md` for v1→v2 plan
4. Explore `demo_genome.cpp` source code
5. Start building with v2 components!

---

**MELVIN v2 - Evolution-first cognitive architecture. Ready to grow.** 🧠⚡

