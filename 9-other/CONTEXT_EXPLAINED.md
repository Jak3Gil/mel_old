# 🧠 Context in Melvin's Brain - Complete Explanation

## What Is Context?

**Context is the living field of meaning** that flows through Melvin's brain. It's not a single variable, but a **distributed pattern of activation** across all the concepts in the knowledge graph.

Think of it like this:
- Your brain isn't thinking about ONE thing at a time
- You have a **cloud of related concepts** that are all partially active
- This cloud shapes what you notice, what you remember, and what you think next

**That's context!**

---

## 🔥 Example: Understanding Context

### Scenario: You See Fire

**In a traditional system (OLD WAY - RIGID):**
```
Attention weights: ALWAYS
  α = 0.45 (saliency)
  β = 0.35 (goal)
  γ = 0.20 (curiosity)

Every frame, same weights. Boring!
```

**In Melvin with Context (NEW WAY - DYNAMIC):**
```
Frame 1: See something red
  → "fire" concept activates (0.8)
  → Spreads to "danger" (0.6), "heat" (0.5), "run" (0.4)
  
  Context field now contains:
    fire:    0.80  ████████
    danger:  0.60  ██████
    heat:    0.50  █████
    run:     0.40  ████
  
  This context changes attention weights:
    α = 0.55 (increase! - look for bright/hot things)
    β = 0.30 (decrease - less goal focus)
    γ = 0.15 (decrease - danger is not time for curiosity)
    
  New formula: F = 0.55·S + 0.30·G + 0.15·C
  
Frame 2: With this context
  → Red patches get HIGHER saliency weight
  → Fire-related objects prioritized
  → Attention biased toward danger!
  
Frame 3: Fire concept decays
  → activation drops: fire (0.60)
  → Weights shift back toward normal
  → α = 0.48, β = 0.33, γ = 0.19
```

**The context CHANGES THE FORMULA each frame!**

---

## 📊 How Context Works - Step by Step

### Step 1: Activation Injection

```cpp
// Melvin sees "fire"
context.inject(fire_node, 0.8, SENSORY_INPUT);

// Activation stored:
fire_node.activation = 0.8
```

---

### Step 2: Diffusion (Spreading Activation)

```cpp
context.diffuse(0.05);  // Spread to connected concepts

// Before diffusion:
fire:    0.80
heat:    0.00
danger:  0.00

// After diffusion (fire→heat has edge weight 0.9):
fire:    0.80
heat:    0.04  (= 0.80 × 0.9 × 0.05)
danger:  0.03  (= 0.80 × 0.7 × 0.05)

// Related concepts "light up"!
```

---

### Step 3: Decay (Forgetting)

```cpp
context.decay();  // Fade inactive nodes

// Each cycle:
activation_new = activation_old × 0.97

// After 10 cycles with no reinforcement:
fire: 0.80 → 0.74 → 0.68 → ... → 0.59

// After 50 cycles:
fire: 0.80 → 0.20 (mostly forgotten)
```

---

### Step 4: Reinforcement (From Thought)

```cpp
// Melvin thinks: "fire causes heat"
context.update_from_thought(fire, causes, heat);

// Activations boosted:
fire:  0.59 → 0.95  (reinforced!)
heat:  0.04 → 0.50  (reinforced!)
causes: 0.00 → 0.30 (activated!)

// Thinking ABOUT something keeps it active!
```

---

### Step 5: Dynamic Weight Computation

```cpp
// Context field examines itself:
auto weights = context.compute_dynamic_weights();

// High "fire" activation (0.95):
weights.alpha = 0.55  // Boost saliency (look for bright/hot)
weights.beta = 0.28   // Reduce goal (emergency mode)
weights.gamma = 0.17  // Reduce curiosity (danger, not time to explore)

// Attention formula CHANGED by context!
```

---

## 🎯 Four Cognitive Modes (Change the Weights!)

### 1. EXPLORING Mode
```
Context: Melvin is exploring, curious, open

Attention weights:
  α = 0.35 (saliency)    ← Lower (everything is new)
  β = 0.25 (goal)        ← Lower (no specific goal)
  γ = 0.40 (curiosity)   ← HIGHER! (novelty is king)

Decay: Fast (0.95) - don't dwell on anything
Diffusion: Broad (0.08) - spread widely

Result: Attention jumps to novel, unexpected things
        Less focus on bright/salient objects
        More focus on surprising objects
```

---

### 2. SEARCHING Mode
```
Context: Melvin is looking for something specific

Attention weights:
  α = 0.30 (saliency)    ← Lower (don't get distracted)
  β = 0.55 (goal)        ← HIGHER! (relevance is key)
  γ = 0.15 (curiosity)   ← Lower (no time for novelty)

Decay: Slow (0.98) - maintain focus
Diffusion: Narrow (0.03) - stay on task

Result: Attention locked on goal-relevant items
        Ignores bright but irrelevant things
        Ignores distractions
```

---

### 3. MONITORING Mode (Default/Balanced)
```
Context: Melvin is just observing, balanced attention

Attention weights:
  α = 0.45 (saliency)    ← Balanced
  β = 0.35 (goal)        ← Balanced
  γ = 0.20 (curiosity)   ← Balanced

Decay: Medium (0.97)
Diffusion: Medium (0.05)

Result: Standard attention
        Responds to salient, relevant, and novel equally
```

---

### 4. LEARNING Mode
```
Context: Melvin is consolidating knowledge

Attention weights:
  α = 0.40 (saliency)    ← Moderate
  β = 0.30 (goal)        ← Moderate
  γ = 0.30 (curiosity)   ← Higher (learning from novelty)

Decay: Very slow (0.99) - remember everything
Diffusion: Medium (0.06) - integrate knowledge

Result: Attention balanced but retentive
        Strong memory formation
        Concepts stay active longer
```

---

## 🎬 Real Example: Watching the Weights Change

### Scenario: Melvin Exploring, Then Spots Fire

**Frame 1-10: Exploring mode, nothing interesting**
```
Context: Empty
Weights: α=0.35, β=0.25, γ=0.40 (exploring)
Formula: F = 0.35·S + 0.25·G + 0.40·C

Attention: Jumpy, goes to novel things
```

**Frame 11: Sees red object (fire!)**
```
Perception injects: "fire" concept
Context after injection:
  fire: 0.80 ████████

Weights STILL: α=0.35, β=0.25, γ=0.40
(Context hasn't settled yet)
```

**Frame 12: Diffusion spreads**
```
Context after diffusion:
  fire:    0.78 ████████
  danger:  0.35 ████
  heat:    0.30 ███
  red:     0.25 ███

Weights start changing:
  α=0.42, β=0.28, γ=0.30
  (Starting to shift toward saliency)
```

**Frame 13: Melvin thinks "fire is dangerous"**
```
Reasoning reinforces:
  fire: 0.78 → 0.95 ██████████
  danger: 0.35 → 0.70 ███████

Context now STRONG:
  fire:    0.95 ██████████
  danger:  0.70 ███████
  heat:    0.40 ████
  
Weights SHIFT significantly:
  α=0.55 (HIGH! - look for bright/hot)
  β=0.28 (lower - not searching)
  γ=0.17 (LOW - danger, not exploring)

Formula: F = 0.55·S + 0.28·G + 0.17·C
```

**Frame 14-20: Fire context maintained**
```
Every frame:
  - Context reinforced by focus on fire
  - Weights stay danger-biased
  - α stays high (0.50-0.55)
  - Attention LOCKED on salient (bright) objects
  
Behavior: Melvin keeps tracking fire!
```

**Frame 30+: Fire out of view**
```
No more fire perception
Context decays:
  fire: 0.95 → 0.85 → 0.75 → ... → 0.30

Weights shift back:
  α=0.40, β=0.32, γ=0.28

Gradually returns to normal!
```

---

## 🔄 The Feedback Loop

```
Current Context
      ↓
Computes α, β, γ (dynamic weights)
      ↓
Attention uses these weights
      ↓
Selects focus based on F = α·S + β·G + γ·C
      ↓
Reasoning thinks about focus
      ↓
Thought reinforces concepts
      ↓
Context updated
      ↓
(Loop continues with NEW weights!)
```

**Each thought changes the weights for next frame!**

---

## 💡 Why This Is Better Than Rigid Weights

### Old Way (Rigid):
```
Frame 1: α=0.45, β=0.35, γ=0.20
Frame 2: α=0.45, β=0.35, γ=0.20
Frame 3: α=0.45, β=0.35, γ=0.20
...

Same formula forever. Context doesn't matter.
```

### New Way (Context-Driven):
```
Frame 1 (exploring):  α=0.35, β=0.25, γ=0.40
Frame 2 (saw fire):   α=0.42, β=0.28, γ=0.30
Frame 3 (thinking):   α=0.55, β=0.28, γ=0.17
Frame 4 (focused):    α=0.53, β=0.29, γ=0.18
Frame 10 (calming):   α=0.48, β=0.32, γ=0.20
...

Weights adapt to what Melvin is thinking about!
```

**The formula itself becomes context-aware!**

---

## 🧬 Biological Parallels

| Melvin Component | Brain System | Function |
|------------------|--------------|----------|
| NodeState.activation | Neural firing rate | How "active" a concept is |
| Diffusion | Synaptic spreading | Excitation flows through connections |
| Decay | Synaptic depression | Unused pathways fade |
| Injection | Sensory input / Thought | New activation sources |
| compute_dynamic_weights() | Attentional control networks | Prefrontal → parietal modulation |
| Context field | Working memory | What you're currently thinking about |

**Your brain does this too!**
- When you think about danger → You become more sensitive to movement
- When you're searching → You ignore distracting bright colors
- When you're relaxed → You notice novel, curious things

**Melvin now does the same!**

---

## 📐 The Math Behind Dynamic Weights

### Base Weights (from cognitive mode):
```cpp
if (mode == EXPLORING):
    α₀ = 0.35, β₀ = 0.25, γ₀ = 0.40
else if (mode == SEARCHING):
    α₀ = 0.30, β₀ = 0.55, γ₀ = 0.15
...
```

### Modulation (from active concepts):
```cpp
for each highly_active_concept:
    if (activation > 0.7):
        β += 0.1 × activation  // Increase goal focus
        α -= 0.05 × activation // Decrease saliency
```

### Normalization:
```cpp
total = α + β + γ
α = α / total
β = β / total  
γ = γ / total

// Ensures α + β + γ = 1.0
```

---

## 🎮 How to Control Context

### Set Mode Explicitly:
```cpp
mind.set_mode(ContextField::EXPLORING);
// Now γ (curiosity) is highest
// Melvin explores, notices novel things

mind.set_mode(ContextField::SEARCHING);
// Now β (goal) is highest
// Melvin focuses on relevant objects
```

### Set Goal:
```cpp
context.update_from_goal("find fire");
// Injects strong activation into "fire" node
// Spreads to related concepts (heat, danger, red)
// Biases attention toward fire-like things
```

### Let It Emerge Naturally:
```cpp
// Just run tick() normally
// Context builds from what Melvin sees and thinks
// Weights adapt automatically!
```

---

## 🔍 Visualizing Context

### Console Visualization:
```cpp
context.visualize();

Output:
╔════════════════════════════════════════════════════════════╗
║  🧠 CONTEXT FIELD - Active Concepts (Working Memory)       ║
╚════════════════════════════════════════════════════════════╝

Cycle: 42 | Mode: MONITORING

Node ID          | Activation | Bar
───────────────────────────────────────────────
12283556408445   | 0.89       | ████████████████████████
4328795604395    | 0.64       | ██████████████
9384756203847    | 0.33       | ████████
```

Shows what Melvin is "thinking about" in real-time!

---

## 🎯 Key Insights

### 1. Context = Distributed Activation
```
Not a single "current concept"
But a FIELD of partial activations across many concepts
```

### 2. Activation Spreads (Like Neural Networks)
```
If "fire" is active → "heat" activates → "danger" activates
Connected concepts co-activate
Meaningful relationships emerge
```

### 3. Decay = Forgetting
```
Unreinforced concepts fade away
Working memory has limited capacity
Old thoughts make room for new ones
```

### 4. Feedback Loop
```
Perception → Activates concepts
Concepts → Change attention weights
Attention → Selects different things
Selection → Generates new thoughts
Thoughts → Reinforce concepts
(Loop continues, self-modifying!)
```

### 5. Emergent Behavior
```
You don't program "pay more attention when scared"
Instead:
  - "danger" concept activates
  - Spreads to "threat", "escape", "vigilance"
  - These concepts have high activation
  - compute_dynamic_weights() sees this
  - Automatically increases α (saliency)
  - EMERGENT: Melvin becomes more sensitive to movement!
```

---

## 🧪 Test It Yourself

### Experiment 1: Watch Weights Change
```bash
# Run the test
cd melvin
make run

# Watch output - you'll see:
Context: 3 active nodes | Weights: α=0.45 β=0.35 γ=0.20 (Monitoring)
...
Context: 5 active nodes | Weights: α=0.48 β=0.32 γ=0.20 (Balanced)
...
Context: 8 active nodes | Weights: α=0.42 β=0.38 γ=0.20 (Goal-driven)

// Weights CHANGING each frame based on what's active!
```

### Experiment 2: Set Different Modes
```cpp
// In test code:
mind.set_mode(ContextField::EXPLORING);
// Run 50 frames - notice high γ (curiosity)

mind.set_mode(ContextField::SEARCHING);
// Run 50 frames - notice high β (goal)
```

---

## 📖 Code Walkthrough

### Activation Injection:
```cpp
void ContextField::inject(uint64_t node_id, float amount, ActivationSource source) {
    // Get node state (or create if new)
    auto& state = get_or_create_state(node_id);
    
    // Different sources have different strengths
    float source_weight = (source == GOAL_INJECTION) ? 1.2 : 1.0;
    
    // Inject activation (saturating add)
    state.activation = min(1.0, state.activation + amount * source_weight);
}
```

### Diffusion (The Magic!):
```cpp
void ContextField::diffuse(float factor) {
    // For each active node
    for (auto& [node_id, state] : states_) {
        if (state.activation < 0.1) continue;  // Too weak
        
        // Get connected neighbors
        auto neighbors = graph.neighbors(node_id, {CAUSES, EXPECTS, CO_OCCURS_WITH});
        
        // Spread activation to each
        for (auto neighbor_id : neighbors) {
            float edge_weight = graph.get_edge_weight(node_id, neighbor_id);
            float spread = state.activation × edge_weight × factor;
            
            neighbor_state.activation += spread;  // Flows through edge!
        }
    }
}
```

### Dynamic Weights (The Key!):
```cpp
AttentionWeights ContextField::compute_dynamic_weights() const {
    // Start with mode defaults
    if (mode == EXPLORING):
        weights = {0.35, 0.25, 0.40};  // High curiosity
    else if (mode == SEARCHING):
        weights = {0.30, 0.55, 0.15};  // High goal
    
    // Modulate based on active concepts
    auto top_active = get_top_active(5);
    
    for (auto [node_id, activation] : top_active) {
        if (activation > 0.7) {  // Strong concept
            weights.beta += 0.1 * activation;  // Increase goal focus
            weights.alpha -= 0.05 * activation; // Decrease saliency
        }
    }
    
    // Normalize to sum to 1.0
    normalize(weights);
    
    return weights;
}
```

**This function is called EVERY FRAME!**
**Weights change based on what's in working memory!**

---

## 🎨 Visual Metaphor

### Static Weights (Old):
```
┌─────────┐
│ α = 0.45 │ ← Never changes
│ β = 0.35 │
│ γ = 0.20 │
└─────────┘
```

### Dynamic Context (New):
```
┌─────────────────────────────┐
│ Context Field (Working Memory)│
│                              │
│  fire: 0.80 ████████         │
│  danger: 0.60 ██████         │
│  heat: 0.40 ████             │
│                              │
│        ↓                     │
│ compute_dynamic_weights()    │
│        ↓                     │
│  α = 0.55 (boosted!)        │
│  β = 0.28                    │
│  γ = 0.17                    │
└─────────────────────────────┘

Weights CHANGE based on active concepts!
```

---

## 🏆 Why This Is Revolutionary

### Before (Rigid Weights):
- Same formula every frame
- No adaptation to situation
- Can't shift attention strategy
- Like a robot with fixed program

### After (Context-Driven):
- Formula changes every frame
- Adapts to what Melvin is thinking
- Shifts strategy based on situation
- Like a real mind that adapts!

---

## 🧩 Summary

**Context = What Melvin is Currently Thinking About**

It's a distributed activation field over the knowledge graph where:

1. **Concepts light up** when perceived, thought about, or recalled
2. **Activation spreads** to related concepts through edges
3. **Weak activations decay** over time (forgetting)
4. **Strong activations persist** and get reinforced
5. **The field shapes attention** by computing dynamic α, β, γ weights

**Result:** Melvin's attention formula changes based on mental state!

- Exploring → High γ (curiosity-driven)
- Searching → High β (goal-driven)
- Danger detected → High α (saliency-driven)
- Learning → Balanced but persistent

**The weights are no longer rigid constants - they're living variables shaped by context!**

---

## 🚀 Try It

```bash
cd melvin
make clean && make
make run

# Watch the output - you'll see:
# "Context: X active nodes | Weights: α=... β=... γ=..."
# The weights CHANGE each cycle!
```

---

**Context = The living meaning that flows through Melvin's mind** 🧠✨

*Now attention isn't just a formula - it's a context-sensitive adaptive process!*

