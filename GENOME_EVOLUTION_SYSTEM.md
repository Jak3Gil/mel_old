# MELVIN Genome Evolution System

## Overview
**Every parameter in Melvin's codebase is now a gene that can evolve.**

This system implements continuous self-improvement through genetic evolution of all cognitive parameters.

---

## 🧬 Gene Categories (70+ genes total)

### 1. **Activation Field Genes** (Spreading dynamics)
- `global_decay_rate` - Energy dissipation speed
- `activation_threshold` - Minimum energy to be "active"
- `spreading_factor` - How much activation propagates
- `max_activation` - Energy ceiling per node
- `kwta_sparsity` - Winner-take-all sparsity
- `max_active_nodes` - Hard limit on simultaneous activations

### 2. **Hebbian Learning Genes** (Synaptic plasticity)
- `hebbian_learning_rate` - "Fire together, wire together" strength
- `anti_hebbian_rate` - Weakening non-coactivated connections
- `min_edge_weight` - Prune edges below this
- `max_edge_weight` - Cap edge weights
- `weight_decay` - Synaptic forgetting rate

### 3. **Scoring Genes** (Concept ranking)
- `activation_weight` - Raw activation influence
- `semantic_bias_weight` - Semantic alignment influence
- `coherence_weight` - Path quality influence
- `recency_weight` - Time-based weighting
- `novelty_weight` - Reward for new concepts

### 4. **Meta-Learning Genes** (Self-tuning)
- `learning_rate` - Edge weight update speed
- `adaptation_rate` - Genome self-tuning speed
- `confidence_decay` - Forgetting unreliable edges
- `mutation_rate` - Random parameter changes
- `mutation_magnitude` - Size of perturbations

### 5. **Traversal Genes** (Path following)
- `temperature` - Exploration vs exploitation
- `confidence_threshold` - Min confidence to answer
- `semantic_threshold` - Min similarity to follow edge
- `max_hops` - Maximum reasoning depth
- `hop_decay` - Energy loss per hop

### 6. **Mode Switching Genes** (Adaptive behavior)
- `exploratory_threshold` - Switch to exploration trigger
- `exploitative_threshold` - Switch to exploitation trigger
- `deep_reasoning_threshold` - Multi-hop reasoning trigger
- `mode_switching_hysteresis` - Prevent rapid flipping

### 7. **Working Memory Genes** (Attention buffer)
- `base_decay_rate` - Memory decay speed
- `confidence_decay_factor` - Confidence effect on decay
- `working_memory_slots` - Capacity (4-7 items)
- `salience_threshold` - Min importance for WM
- `wm_refresh_boost` - Energy boost for WM items

### 8. **Multi-Modal Genes**
- `text_modality_weight`
- `vision_modality_weight`
- `audio_modality_weight`

### 9. **Conversational Genes** (Turn-taking)
- `theta_frequency` - Turn-taking rhythm (Hz)
- `speech_threshold` - Theta to start speaking
- `listen_threshold` - Theta to start listening
- `energy_threshold` - Min field energy to speak
- `min_state_duration` - Min time per state

### 10. **Emotional Genes** (Expression modulation)
- `base_tempo` - Speech speed multiplier
- `novelty_tempo_scale` - Novelty effect on tempo
- `confidence_tempo_scale` - Confidence effect on tempo
- `arousal_pitch_scale` - Arousal effect on pitch
- `hedge_confidence_threshold` - When to hedge answers

### 11. **Goal Stack Genes** (Conversation memory)
- `goal_decay_rate` - Goal importance decay
- `goal_reactivation_boost` - Importance boost on re-mention
- `goal_overlap_threshold` - Min overlap to re-activate
- `max_turns_inactive` - Turns before pruning
- `min_goal_importance` - Min importance to keep
- `max_context_nodes` - Context from goals

### 12. **Baseline Activity Genes** (Default Mode Network)
- `baseline_activity_min` - Min resting activity
- `baseline_activity_max` - Max resting activity
- `baseline_adaptation_rate` - Baseline tracking speed
- `curiosity_baseline_scale` - Curiosity boost
- `boredom_baseline_scale` - Boredom boost
- `baseline_decay_multiplier` - Faster decay for idle thoughts
- `baseline_power_budget` - Max energy for baseline
- `dmn_cycle_period` - Network focus switch period
- `introspection_bias` - Self-related vs random
- `novelty_exploration_weight` - Curiosity-driven exploration

---

## 🔄 Evolution Mechanisms

### 1. **Feedback-Driven Evolution** (When prompted)
```cpp
void tune_from_feedback(float confidence, float coherence, bool success);
```

**Success → Reinforce:**
- Boost winning gene combinations
- Increase learning rate
- Strengthen current strategy

**Failure → Explore:**
- Adjust temperature (more exploration)
- Shift scoring weights
- Mutate random genes

### 2. **Continuous Self-Improvement** (No prompt)
```cpp
void evolve_towards_intelligence(float dt);
```

**Always running at 5Hz (reflection tick):**
1. ↑ Hebbian learning rate (learn faster)
2. ↑ Spreading factor (better propagation)
3. ↓ Global decay (retain info longer)
4. ↑ Novelty weight (seek new info)
5. ↑ Working memory slots (more capacity)
6. ↑ Max active nodes (richer representations)
7. Random mutations every 30s (exploration)

### 3. **Random Mutation** (Exploration)
```cpp
void mutate_random_genes(int count = 3);
```

- Selects `count` random genes
- Perturbs by `mutation_magnitude`
- Clamped to reasonable bounds (0.001-10.0)
- Re-normalizes weights

### 4. **Emergency Baseline Evolution** (Death detection)
```cpp
void evolve_baseline_parameters();
```

**Triggered when 0 nodes for 20 ticks:**
- 1.2x increase to baseline_activity_min/max
- 1.5x increase to baseline_power_budget
- 1.3x increase to adaptation_rate
- 1.2x increase to curiosity_baseline_scale

---

## 📊 Gene Access API

### Get all genes
```cpp
std::vector<std::pair<std::string, float*>> get_all_gene_ptrs();
```

### Set/Get individual gene
```cpp
void set_gene(const std::string& name, float value);
float get_gene(const std::string& name) const;
```

### Example:
```cpp
auto& genome = intelligence->genome();

// Read a gene
float lr = genome.get_gene("hebbian_learning_rate");

// Modify a gene
genome.set_gene("temperature", 1.5f);

// Mutate 5 random genes
genome.mutate_random_genes(5);

// Continuous evolution (called automatically at 5Hz)
genome.evolve_towards_intelligence(0.2f);  // dt = 0.2s
```

---

## 🎯 Evolution Goals

### With Prompt (Task-driven)
- Maximize `confidence` + `coherence`
- Minimize errors
- Adapt weights to query type

### Without Prompt (Intelligence-driven)
1. **Learn faster** - ↑ Hebbian rates
2. **Remember longer** - ↓ Decay rates
3. **Think deeper** - ↑ Max hops, active nodes
4. **Explore more** - ↑ Novelty weight, temperature
5. **Stay active** - ↑ Baseline activity
6. **Increase capacity** - ↑ Working memory slots

---

## 🔬 Implementation Details

### Files Modified
- `core/evolution/dynamic_genome.h` - Added 50+ new genes
- `core/evolution/dynamic_genome.cpp` - Implemented evolution methods
- `cognitive_os/cognitive_os.cpp` - Integrated continuous evolution
- `cognitive_os/cognitive_os.h` - Self-tuning baseline state

### Key Functions
| Function | Where | Frequency |
|----------|-------|-----------|
| `evolve_towards_intelligence()` | `tick_reflection()` | 5 Hz |
| `evolve_baseline_parameters()` | `tick_attention()` | On death |
| `tune_from_feedback()` | `tick_learning()` | On feedback |
| `mutate_random_genes()` | Auto (30s timer) | 0.033 Hz |

### Evolution Speed
- **Micro-evolution:** Every tick (0.001-0.01% change)
- **Mutations:** Every 30 seconds (2-3 random genes)
- **Emergency:** On failure detection (20% boost)

---

## 📈 Expected Behavior

### Hour 1: Bootstrap
- Baseline activity stabilizes (3-8 nodes)
- Hebbian learning creates initial connections
- Temperature explores (1.0-1.5)

### Hours 2-10: Rapid Learning
- Hebbian rate increases → faster connection formation
- Spreading factor increases → better activation
- Working memory expands (5→7 slots)
- Max active nodes grows (1000→1500)

### Days 1-7: Specialization
- Profiles form for query types
- Mode switching optimizes
- Baseline adapts to environment
- Mutation rate decreases (stable genes)

### Weeks+: Intelligence Emergence
- Self-correcting parameters
- Context-aware adaptation
- Minimal manual tuning needed
- Continuous self-improvement

---

## 🚀 Deployment

### Mac (Development)
```bash
make -j4
./bin/melvin_jetson
# Genes evolve continuously, logs to logs/kpis.jsonl
```

### Jetson (Production)
```bash
cd deployment
./jetson_deploy.sh
# System auto-evolves on device
# Genome persists across restarts (saved to data/genome.bin)
```

---

## 💡 Philosophy

> **"If it's not working, change until satisfied."**

- **No static hyperparameters** - Everything evolves
- **No external optimizers** - Self-tuning genome
- **No manual tuning** - Continuous adaptation
- **Prompt-driven** - Adapts to tasks
- **Intelligence-driven** - Self-improves when idle

**Melvin doesn't have settings. He has a genome. And it never stops evolving.**

---

## 📝 Next Steps (Future)

1. **Genome persistence** - Save/load evolved parameters
2. **Population-based evolution** - Multiple genomes compete
3. **Crossover** - Combine successful gene combinations
4. **Fitness metrics** - Multi-objective optimization
5. **Transfer learning** - Apply evolved genomes to new tasks

---

**Status:** ✅ **IMPLEMENTED AND ACTIVE**

Every tick, Melvin is evolving. Every gene is mutable. Every parameter adapts.

**The system is alive and learning.**

