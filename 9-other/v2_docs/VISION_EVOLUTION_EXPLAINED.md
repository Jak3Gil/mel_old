# 🎥 How MELVIN v2 Evolves With a Live Camera

## The Vision-Evolution Loop

When you give MELVIN v2 a live camera, evolution shapes **how he sees and what he learns to notice**. Here's the complete explanation:

---

## Part 1: Perception Bridge (Camera → Brain)

### Current Status (What We Have)

```cpp
// In unified_loop_v2.cpp, perception_stage() is currently mocked:
std::vector<PerceivedObject> perception_stage(const uint8_t* image_data, ...) {
    // Mock objects for now
    PerceivedObject obj1;
    obj1.object_id = 101;
    obj1.features.saliency = 0.7f;
    // ...
}
```

### With Real Camera (What We Add)

```cpp
// melvin/v2/perception/camera_bridge.h

class CameraBridge {
public:
    CameraBridge(const Genome& genome);  // ← Genome controls perception!
    
    struct PerceptionConfig {
        // FROM GENOME:
        float edge_detection_threshold;    // Gene: "vision_edge_threshold"
        float motion_sensitivity;          // Gene: "vision_motion_gain"
        float color_variance_weight;       // Gene: "vision_color_weight"
        float patch_size;                  // Gene: "vision_patch_size"
        float min_object_size;             // Gene: "vision_min_object_pixels"
        // ... more vision genes
    };
    
    std::vector<PerceivedObject> process_frame(
        const uint8_t* rgb_data,
        int width,
        int height
    );
};
```

**Key Insight:** The genome controls **how** perception works, not just what happens after!

---

## Part 2: Vision Genes (Evolvable Perception)

### Add to Genome (New Module)

```cpp
void Genome::add_vision_genes() {
    GeneModule vision("vision", 0.15f);
    
    // Edge detection (V1 cortex analog)
    vision.add_gene(make_gene("edge_threshold", 0.3f, 0.1f, 0.9f, 0.05f));
    vision.add_gene(make_gene("edge_weight", 0.3f, 0.0f, 1.0f, 0.05f));
    
    // Motion detection (MT cortex analog)
    vision.add_gene(make_gene("motion_sensitivity", 0.5f, 0.1f, 2.0f, 0.1f));
    vision.add_gene(make_gene("motion_weight", 0.4f, 0.0f, 1.0f, 0.05f));
    
    // Color processing (V4 cortex analog)
    vision.add_gene(make_gene("color_variance_threshold", 0.2f, 0.05f, 0.8f, 0.05f));
    vision.add_gene(make_gene("color_weight", 0.2f, 0.0f, 1.0f, 0.05f));
    
    // Spatial processing
    vision.add_gene(make_gene("patch_size", 32.0f, 16.0f, 64.0f, 4.0f));
    vision.add_gene(make_gene("min_object_size", 100.0f, 25.0f, 500.0f, 25.0f));
    
    // Novelty detection
    vision.add_gene(make_gene("novelty_threshold", 0.3f, 0.1f, 0.8f, 0.05f));
    vision.add_gene(make_gene("prediction_window", 5.0f, 2.0f, 20.0f, 1.0f));
    
    modules.push_back(vision);
}
```

**Result:** Now we have **59 total genes** (49 existing + 10 vision)

---

## Part 3: The Vision-Evolution Cycle

### Scenario: MELVIN v2 Population Learning to Track Objects

**Generation 0: Random Initialization**

```
Population of 32 genomes, each with different vision parameters:

Genome #1: edge_threshold=0.8, motion_weight=0.2  (high edges, low motion)
Genome #2: edge_threshold=0.2, motion_weight=0.9  (low edges, high motion)
Genome #3: edge_threshold=0.5, motion_weight=0.5  (balanced)
... 29 more random variants
```

**Express each genome → 32 different brains**

```cpp
for (auto& genome : population) {
    auto brain = express_brain(genome, seed);
    brain.set_camera(live_camera);
    
    // Run task: "Track moving object for 60 seconds"
    auto fitness = evaluate_tracking_task(brain, 60.0f);
}
```

---

### What Happens Frame-by-Frame

**Frame 1: Camera sees a moving red ball**

```
RAW IMAGE (640×480 RGB)
    ↓
GENOME CONTROLS PERCEPTION:
  
Genome #1 (edge_threshold=0.8, motion_weight=0.2):
  • High edge threshold → Misses smooth surfaces
  • Low motion weight → Doesn't notice movement
  • Detects: 1 object (edges only)
  • Saliency: 0.3 (low - missed motion!)
  
Genome #2 (edge_threshold=0.2, motion_weight=0.9):
  • Low edge threshold → Detects everything
  • High motion weight → Very sensitive to movement
  • Detects: 15 objects (too many! noise + ball)
  • Saliency: 0.9 for ball, 0.7 for noise (good + noise)
  
Genome #3 (edge_threshold=0.5, motion_weight=0.5):
  • Balanced thresholds
  • Detects: 3 objects (ball + 2 background)
  • Saliency: 0.8 for ball, 0.2 for background
  
    ↓
ATTENTION SELECTS FOCUS:
  
Genome #1: Focused on edge (wrong! missed ball)
Genome #2: Focused on noise (wrong! distracted)
Genome #3: Focused on ball (correct!) ✓
```

**Frame 30: Ball moves left**

```
PREDICTION (from working memory):
  
Each brain predicts next position based on velocity:
  
Genome #1: Didn't track ball → no prediction → HIGH ERROR
  Predicted: object at (320, 240)
  Actual: ball at (280, 240)
  Error: 40 pixels! → Surprise: 0.8
  
  → DA drops: 0.50 → 0.10 (punishment!)
  → NE spikes: 0.50 → 0.90 (unexpected!)
  → Learning slows (low plasticity)
  
Genome #2: Tracked ball but also noise → MEDIUM ERROR
  Predicted: ball at (275, 240)
  Actual: ball at (280, 240)
  Error: 5 pixels → Surprise: 0.1
  
  → DA: 0.50 → 0.55 (mild reward)
  
Genome #3: Tracked ball cleanly → LOW ERROR
  Predicted: ball at (279, 240)
  Actual: ball at (280, 240)
  Error: 1 pixel! → Surprise: 0.02
  
  → DA: 0.50 → 0.75 (strong reward!)
  → NE: 0.50 → 0.45 (expected, no surprise)
  → Learning accelerates (high plasticity)
  → Creates LEAP: "red_object" → "ball" → "moves_left"
```

---

## Part 4: Fitness Evaluation (After 60 Seconds)

### Task: "Track moving object for 60 seconds"

**Genome #1 (poor vision genes):**
```
Tracking Accuracy:     45% (missed ball often)
Prediction Quality:    Poor (avg error: 35 pixels)
Context Sensitivity:   Low (didn't use motion cues)
Sample Efficiency:     Slow (low DA → slow learning)
Stability:             Good (didn't oscillate)
Exploration:           High (lots of surprise → high NE)

FITNESS SCORES:
  Task Success:        0.45 / 1.0  ❌
  Prediction Quality:  0.30 / 1.0  ❌
  Context:             0.40 / 1.0  ❌
  Efficiency:          0.35 / 1.0  ❌
  Stability:           0.85 / 1.0  ✓
  Exploration:         0.60 / 1.0  ✓
  
PARETO RANK: #25 (dominated on most objectives)
```

**Genome #2 (too sensitive):**
```
Tracking Accuracy:     70% (tracked but distracted by noise)
Prediction Quality:    Medium (avg error: 12 pixels)
Context Sensitivity:   High (used all cues, too many!)
Sample Efficiency:     Medium
Stability:             Poor (switched focus 47 times!)
Exploration:           Medium

FITNESS SCORES:
  Task Success:        0.70 / 1.0  ⚠️
  Prediction Quality:  0.65 / 1.0  ⚠️
  Context:             0.75 / 1.0  ✓
  Efficiency:          0.60 / 1.0  ⚠️
  Stability:           0.25 / 1.0  ❌
  Exploration:         0.55 / 1.0  ✓
  
PARETO RANK: #15 (good context, poor stability)
```

**Genome #3 (balanced):**
```
Tracking Accuracy:     95% (almost perfect!)
Prediction Quality:    Excellent (avg error: 2 pixels)
Context Sensitivity:   High (motion + color + edges)
Sample Efficiency:     High (good DA → fast learning)
Stability:             Excellent (4 focus switches only)
Exploration:           Appropriate (low NE, task-focused)

FITNESS SCORES:
  Task Success:        0.95 / 1.0  ✅
  Prediction Quality:  0.92 / 1.0  ✅
  Context:             0.88 / 1.0  ✅
  Efficiency:          0.85 / 1.0  ✅
  Stability:           0.95 / 1.0  ✅
  Exploration:         0.50 / 1.0  ✓
  
PARETO RANK: #1 (dominates on 5/6 objectives!)
```

---

## Part 5: Selection & Breeding (NSGA-II)

### After Generation 0

```
Pareto Front (Non-dominated genomes):
  #3:  (0.95, 0.92, 0.88, 0.85, 0.95, 0.50)  ← Best all-rounder
  #7:  (0.88, 0.95, 0.82, 0.90, 0.85, 0.45)  ← Best prediction
  #12: (0.92, 0.85, 0.90, 0.80, 0.90, 0.55)  ← Best context
  #18: (0.80, 0.88, 0.85, 0.95, 0.82, 0.60)  ← Best efficiency
  
Selection:
  ✓ Keep Pareto front (4 genomes)
  ✓ Breed variants:
    - Genome #3 + #7 → crossover → Genome #33
    - Genome #3 + mutation (0.1) → Genome #34
    - Genome #7 + mutation (0.05) → Genome #35
    - ... create 28 more offspring
  
  → 32 new genomes for Generation 1
```

**Genome #34 (child of best #3 with mutation):**
```
Parent #3 vision genes:
  edge_threshold: 0.5
  motion_weight: 0.5
  color_weight: 0.2
  
Mutation (σ=0.05 Gaussian noise):
  edge_threshold: 0.5 + 0.02 = 0.52
  motion_weight: 0.5 + (-0.03) = 0.47
  color_weight: 0.2 + 0.04 = 0.24
  
Hypothesis: Slightly different perception → might be better!
```

---

## Part 6: Generation 1 (Evolution in Action)

### New Task: "Track ball + ignore distractors"

```
Frame 1: Ball + waving hand + moving car

Genome #34 (offspring of #3):
  edge_threshold: 0.52 (slightly higher)
  motion_weight: 0.47 (slightly lower)
  color_weight: 0.24 (slightly higher)
  
PERCEPTION:
  • Ball: edges=0.6, motion=0.8, color=0.9 (red)
    Saliency = 0.52×0.6 + 0.47×0.8 + 0.24×0.9 = 0.904
  
  • Hand: edges=0.8, motion=0.9, color=0.3 (skin)
    Saliency = 0.52×0.8 + 0.47×0.9 + 0.24×0.3 = 0.911
  
  • Car: edges=0.9, motion=0.7, color=0.5 (gray)
    Saliency = 0.52×0.9 + 0.47×0.7 + 0.24×0.5 = 0.917
  
  TOP-DOWN (goal: "track ball"):
    • Ball: "ball" in semantic memory → Relevance: 0.9
    • Hand: no "ball" relation → Relevance: 0.1
    • Car: no "ball" relation → Relevance: 0.1
  
  ATTENTION (α=0.4, β=0.3 from attention genes):
    • Ball:  F = 0.4×0.904 + 0.3×0.9 = 0.632
    • Hand:  F = 0.4×0.911 + 0.3×0.1 = 0.394
    • Car:   F = 0.4×0.917 + 0.3×0.1 = 0.397
  
  FOCUS: Ball! ✓ (Top-down bias won!)
  
  Tracking accuracy: 98% (better than parent #3!)
```

**Genome #34 FITNESS:**
```
Task Success:        0.98 / 1.0  (improved! parent was 0.95)
Prediction Quality:  0.94 / 1.0  (improved! parent was 0.92)
Context:             0.90 / 1.0  (improved! parent was 0.88)
Efficiency:          0.88 / 1.0  (improved! parent was 0.85)
Stability:           0.96 / 1.0  (improved! parent was 0.95)
Exploration:         0.48 / 1.0  (similar to parent)

RESULT: Genome #34 DOMINATES parent #3 on 5/6 objectives!
        → Selected for next generation
```

---

## Part 7: The Evolution-Perception Feedback Loop

### How Evolution Optimizes Vision Over Generations

```
GENERATION 0:
  Best tracking: 95% (genome #3)
  Best genes: edge=0.5, motion=0.5, color=0.2
  
    ↓ Selection + Mutation
    
GENERATION 1:
  Best tracking: 98% (genome #34, child of #3)
  Best genes: edge=0.52, motion=0.47, color=0.24
  Discovery: Slightly more color weight helps!
  
    ↓ Selection + Mutation
    
GENERATION 2:
  Best tracking: 99.2% (genome #67, grandchild)
  Best genes: edge=0.54, motion=0.45, color=0.28
  Discovery: Color is key for ball tracking!
  
    ↓ Selection + Mutation
    
GENERATION 10:
  Best tracking: 99.8% (genome #234)
  Best genes: edge=0.58, motion=0.40, color=0.35
  Converged: Color > edges > motion for red ball
  
    ↓ Different task...
    
NEW TASK: "Track gray car (no color cue)"
    
GENERATION 11:
  Previous best (genome #234) struggles!
    edge=0.58, motion=0.40, color=0.35
    Gray car: color useless! Tracking: 70% ❌
  
  Different genome (#189) excels:
    edge=0.75, motion=0.55, color=0.15
    Gray car: edges + motion win! Tracking: 96% ✓
  
  → Evolution discovers different strategies for different tasks!
```

**Key Insight:** Evolution finds optimal **visual processing** for each task!

---

## Part 8: Neuromodulator-Vision Integration

### How Neuromodulators Shape Visual Learning

**Dopamine (DA) - Visual Prediction Reward**

```
Frame N: Ball at (300, 240), velocity = (-5, 0)

PREDICT (based on velocity):
  Next position: (295, 240)
  
Frame N+1: Observe actual position
  
CASE 1: Ball at (294, 240)
  Error: 1 pixel (excellent!)
  → DA: 0.50 → 0.75 (reward!)
  → Strengthen: "red_blob" → "ball" edge (Hebbian)
  → Boost: motion_weight in next cycle
  
CASE 2: Ball at (310, 240) - bounced unexpectedly!
  Error: 15 pixels (surprise!)
  → DA: 0.50 → 0.25 (punishment)
  → NE: 0.50 → 0.85 (unexpected!)
  → Exploration: widen attention, check other objects
  → Learn: "ball" can "bounce" (new LEAP edge)
```

**Acetylcholine (ACh) - Visual Attention Gain**

```
Ambiguous scene: Ball partially occluded

Expected uncertainty high:
  → ACh: 0.50 → 0.85
  → Attention gain: 0.5 + 0.85 = 1.35
  → Boost saliency: 0.6 × 1.35 = 0.81
  
Result: Sharper focus on visible parts
Effect: Better tracking through occlusion
```

**Norepinephrine (NE) - Visual Exploration**

```
Unexpected event: New object enters frame

NE spikes: 0.50 → 0.90
Exploration bias: 0.90 - 0.5 = +0.4

Effect on attention:
  • Widen search (lower inertia)
  • Consider peripheral objects
  • More saccades
  • Discover: "new_object" → gate to WM
  
Result: Discovers new visual patterns
```

---

## Part 9: Complete Vision-Evolution Example

### Task: Evolve Vision for "Find Red Objects"

**Fitness Function:**
```python
def evaluate_vision_fitness(genome, camera, duration=60.0):
    brain = express_brain(genome)
    brain.set_camera(camera)
    
    red_objects_found = 0
    false_positives = 0
    total_frames = 0
    prediction_errors = []
    
    for _ in range(int(duration * 30)):  # 30 FPS
        frame = camera.capture()
        result = brain.tick(frame.data, frame.width, frame.height)
        
        # Check if focused object is red
        if is_red(result.focused_object):
            red_objects_found += 1
        else:
            false_positives += 1
        
        prediction_errors.append(result.total_surprise)
        total_frames += 1
    
    # Multi-objective fitness
    fitness = {
        'task_success': red_objects_found / count_red_in_scene(camera),
        'prediction_quality': 1.0 - np.mean(prediction_errors),
        'precision': red_objects_found / (red_objects_found + false_positives),
        'efficiency': brain.get_stats().avg_cycle_time_ms,
        'stability': 1.0 / (brain.get_stats().focus_switches + 1),
    }
    
    return fitness
```

**Evolution Over 20 Generations:**

```
GEN 0:  Best accuracy: 65%, avg prediction error: 0.25
        Genomes use random vision parameters
        
GEN 5:  Best accuracy: 82%, avg prediction error: 0.15
        Evolution discovered: color_weight should be HIGH
        Best genome: color_weight=0.7 (vs 0.2 initial)
        
GEN 10: Best accuracy: 91%, avg prediction error: 0.08
        Evolution discovered: edge_threshold should be LOW
        Best genome: edge_threshold=0.25, color_weight=0.75
        Insight: Red objects have smooth edges!
        
GEN 15: Best accuracy: 96%, avg prediction error: 0.04
        Evolution discovered: motion helps confirmation
        Best genome: edge=0.22, motion=0.3, color=0.80
        
GEN 20: Best accuracy: 98.5%, avg prediction error: 0.02
        Converged to specialist: EVOLVED RED-FINDER!
        Optimal genes: edge=0.20, motion=0.25, color=0.85
```

**What Evolution Learned:**
1. For red objects: **Color is king** (0.85 weight)
2. Edges don't help much (smooth surfaces → low threshold)
3. Motion is confirmation cue (medium weight)
4. These are **task-specific optimal parameters**!

---

## Part 10: Multi-Task Evolution (The Real Power)

### Evolve for MULTIPLE Visual Tasks

**Task Battery:**
1. Track red ball (color-dominant)
2. Track gray car (edge-dominant)
3. Detect motion (motion-dominant)
4. Find similar objects (all features)

**Evolution discovers TRADE-OFFS:**

```
Genome A (specialist): 
  color=0.85, edge=0.20, motion=0.25
  Red ball: 98.5% ✅
  Gray car: 65%  ❌  (bad! no color cue)
  
Genome B (generalist):
  color=0.45, edge=0.55, motion=0.50
  Red ball: 85%  ⚠️   (good, not best)
  Gray car: 88%  ✅  (good!)
  Detect motion: 90% ✅  (good!)
  
Genome C (edge specialist):
  color=0.15, edge=0.80, motion=0.30
  Gray car: 95%  ✅  (best!)
  Red ball: 72%  ❌  (bad! ignores color)
```

**Pareto Front (Generation 20):**
```
                 ┌─ Genome C (edge specialist)
                 │
Tracking   100%  │  ┌─ Genome A (color specialist)
Gray Car         │  │
                 │  │      ┌─ Genome B (generalist)
            50%  └──┼──────┘
                    │
                    └────────────────────
                  50%               100%
                      Tracking Red Ball
```

**Evolution Result:**
- No single "best" genome
- Multiple optima for different tasks
- **Pareto front shows trade-offs**
- Can select genome based on deployment scenario!

---

## Part 11: Neuromodulator Specialization

### How Neuromodulators Evolve Differently

**Genome A (explorer):**
```
Vision genes: color=0.85, edge=0.20
Neuromod genes:
  ne_baseline: 0.70 (high exploration)
  da_gain: 0.80 (sensitive to reward)
  ach_baseline: 0.60 (sharp attention)
  
Behavior:
  • Explores widely (high NE)
  • Quick to learn new patterns (high DA gain)
  • Focuses sharply on red objects (high ACh)
  • Good for: dynamic environments, novelty detection
```

**Genome B (exploiter):**
```
Vision genes: color=0.45, edge=0.55
Neuromod genes:
  ne_baseline: 0.30 (low exploration)
  da_gain: 0.40 (less reward-sensitive)
  serotonin_baseline: 0.70 (stable, patient)
  
Behavior:
  • Sticks to known patterns (low NE)
  • Stable focus (high 5-HT)
  • Reliable but not creative
  • Good for: stable environments, known tasks
```

**Evolution discovers personality types!**

---

## Part 12: The Complete Vision-Evolution Architecture

### Genome Genes for Vision (Extended)

```cpp
// Add to genome (total now: 59 genes)
struct VisionGenome {
    // Low-level (V1 cortex)
    float edge_threshold;           // Gabor filter sensitivity
    float orientation_bins;         // Edge orientation channels
    
    // Motion (MT cortex)
    float motion_sensitivity;       // Optical flow threshold
    float temporal_window;          // Frames for motion estimation
    
    // Color (V4 cortex)
    float color_variance_weight;    // Color pop-out strength
    float hue_preference;           // Preferred color range
    
    // Object formation (IT cortex)
    float patch_size;               // Receptive field size
    float grouping_threshold;       // Gestalt grouping distance
    float min_object_size;          // Minimum pixels
    
    // Novelty & prediction
    float novelty_threshold;        // What counts as "new"
    float prediction_window;        // Future frames to predict
    float prediction_confidence;    // Trust in predictions
};
```

### Expression: Genome → Vision Pipeline

```cpp
class CameraBridge {
public:
    CameraBridge(const Genome& genome) {
        // Extract vision genes
        auto vision_module = genome.get_module("vision");
        
        edge_threshold_ = vision_module->get_gene("edge_threshold")->value;
        motion_weight_ = vision_module->get_gene("motion_weight")->value;
        color_weight_ = vision_module->get_gene("color_weight")->value;
        // ... all vision parameters from genes
    }
    
    std::vector<PerceivedObject> process_frame(const uint8_t* rgb, ...) {
        // V1: Edge detection (gene-controlled)
        auto edges = extract_edges(rgb, edge_threshold_);
        
        // MT: Motion detection (gene-controlled)
        auto motion = compute_motion(rgb, prev_frame_, motion_sensitivity_);
        
        // V4: Color processing (gene-controlled)
        auto color = extract_color_variance(rgb, color_variance_threshold_);
        
        // IT: Object formation (gene-controlled)
        auto objects = form_objects(edges, motion, color, 
                                    patch_size_, grouping_threshold_);
        
        // Compute saliency (gene-weighted!)
        for (auto& obj : objects) {
            obj.features.saliency = 
                motion_weight_ * obj.motion_score +
                edge_weight_ * obj.edge_score +
                color_weight_ * obj.color_score;
        }
        
        return objects;
    }
    
private:
    // All from genome!
    float edge_threshold_;
    float motion_weight_;
    float color_weight_;
    float patch_size_;
    // ...
};
```

---

## Part 13: Learning While Watching

### Episodic Memory + Vision

**What MELVIN learns from camera:**

```
DAY 1 (Camera watching desk):
  
Episode 1 (09:00-09:15): "Morning coffee"
  Saw: [mug, steam, hand, coffee]
  Learned: mug →(CONTAINS)→ coffee
           steam →(COMES_FROM)→ hot_liquid
           hand →(REACHES_FOR)→ mug
           
Episode 2 (09:30-10:00): "Typing"
  Saw: [keyboard, hands, screen, text]
  Learned: hands →(USES)→ keyboard
           keyboard →(PRODUCES)→ text
           screen →(DISPLAYS)→ text
           
Neuromodulator learning:
  • DA high when predictions correct (coffee after mug)
  • NE high when unexpected (phone rings)
  • ACh high during ambiguous views (partial occlusion)
  • 5-HT high during stable scenes
```

### Semantic Graph Growth

```
INITIAL GRAPH (v1):
  316 nodes, 21,152 edges (existing knowledge)
  
AFTER 1 HOUR OF CAMERA:
  + 50 new visual nodes (object_001 ... object_050)
  + 200 new CO_OCCURS edges (desk + lamp, mug + coffee)
  + 150 new TEMPORAL edges (hand → reach → mug)
  + 80 new INSTANCE_OF edges (object_023 → mug)
  
  Total: 366 nodes, 21,582 edges (+430 edges in 1 hour!)
  
AFTER 1 DAY:
  + 500 visual nodes
  + 5,000 new edges
  
  Total: 816 nodes, 26,152 edges
  
  Learned concepts:
    • "morning_routine" (episodic cluster)
    • "work_objects" (semantic cluster)
    • "typical_desk" (spatial cluster)
```

---

## Part 14: Example Evolution Run (Detailed)

### Task: "Learn to identify tools in workshop"

**Setup:**
```python
# Task definition
task = {
    'name': 'identify_tools',
    'camera': workshop_camera,
    'duration': 120.0,  # 2 minutes per individual
    'success_metric': 'tool_identification_accuracy'
}

# Population
population = [
    GenomeFactory::create_random(seed=i) 
    for i in range(32)
]

# Evolution
for generation in range(50):
    fitnesses = evaluate_population(population, task)
    population = select_and_breed(population, fitnesses)
```

**Generation 0 → 50 Evolution:**

```
GENERATION 0:
  Random genomes, random vision parameters
  
  Best performer (Genome #7):
    Vision: edge=0.65, motion=0.35, color=0.40
    Neuromod: da_gain=0.6, ne_baseline=0.55
    Attention: α=0.45, β=0.35, γ=0.20
    
    Performance:
      Tool identification: 62%
      Avg prediction error: 0.18
      Focus switches: 85 (unstable!)
      
  Insight: Random genes are suboptimal!

GENERATION 10:
  Evolution has optimized vision for tools
  
  Best performer (Genome #145, descendant of #7):
    Vision: edge=0.80, motion=0.25, color=0.30  (edges matter for tools!)
    Neuromod: da_gain=0.75, ne_baseline=0.40
    Attention: α=0.50, β=0.40, γ=0.10
    
    Performance:
      Tool identification: 84% (+22%!)
      Avg prediction error: 0.08 (-0.10!)
      Focus switches: 25 (-60!)
      
    Discoveries:
      • Edges are crucial for tool shapes
      • Motion less important (tools stationary)
      • High edge_weight + low motion_weight optimal
      • DA_gain of 0.75 learns faster

GENERATION 25:
  Further refinement
  
  Best performer (Genome #378):
    Vision: edge=0.88, motion=0.18, color=0.25
    Neuromod: da_gain=0.82, ne_baseline=0.35
    Attention: α=0.55, β=0.45, γ=0.08
    
    Performance:
      Tool identification: 94% (+10%!)
      Avg prediction error: 0.03 (-0.05!)
      Focus switches: 12 (-13!)
      
    Specialized strategies emerged:
      • Very high edge weight (0.88) for metallic tools
      • Low motion (tools don't move)
      • Moderate color (some tools color-coded)
      • High DA gain (fast learning of new tools)
      • Low NE baseline (stay focused, don't explore)

GENERATION 50:
  Convergence to optimal
  
  Pareto Front (3 non-dominated genomes):
  
  Genome #567 (accuracy specialist):
    Vision: edge=0.92, motion=0.15, color=0.22
    Performance: 97.5% accuracy, slow learning
    
  Genome #583 (speed specialist):
    Vision: edge=0.78, motion=0.30, color=0.35
    Performance: 91% accuracy, fast learning
    
  Genome #601 (balanced):
    Vision: edge=0.85, motion=0.22, color=0.28
    Performance: 95% accuracy, medium learning
    
  Trade-off discovered: Accuracy vs Speed!
```

---

## Part 15: Emergent Visual Behaviors

### What Evolution Discovers Automatically

**1. Selective Attention Strategies**

```
Task: Find small objects in cluttered scene

Poor genome (random):
  Attention weights: α=0.5, β=0.2, γ=0.3
  → Distracted by large salient objects
  → Misses small targets
  
Evolved genome (after 30 gens):
  Attention weights: α=0.25, β=0.65, γ=0.10
  → Ignores large distractors (low α)
  → Follows goal strongly (high β)
  → Finds small targets! ✓
  
Discovery: For search tasks, β >> α optimal!
```

**2. Prediction Error Utilization**

```
Task: Track unpredictably moving object

Poor genome:
  prediction_window: 20 frames (too long!)
  → Predictions always wrong
  → High error, low DA
  → Slow learning
  
Evolved genome:
  prediction_window: 3 frames (short!)
  → Predictions more accurate
  → Low error, high DA
  → Fast learning
  → Creates LEAP: "erratic_motion" → "unpredictable"
  
Discovery: Shorter windows for chaotic motion!
```

**3. Multi-Modal Integration**

```
Task: Identify objects by sight + sound

Evolved genome discovers:
  visual_weight: 0.6
  audio_weight: 0.4
  cross_modal_threshold: 0.5
  
Behavior:
  • Sees hammer shape (visual)
  • Hears banging sound (audio)
  • Both activate "hammer" concept
  • Confidence: 0.6×0.8 + 0.4×0.9 = 0.84 ✓
  
  vs Single modality:
  • Visual only: 0.8 (less confident)
  • Audio only: 0.9 (ambiguous)
  
Discovery: Cross-modal confirmation improves accuracy!
```

---

## Part 16: Real-World Deployment Scenario

### Use Case: Security Camera Evolution

**Scenario:** Evolve MELVIN to detect unusual events

**Month 1:** Install in building, start evolution

```
GENERATION 0-10 (Week 1):
  Learns normal patterns:
    • People walking: common
    • Doors opening: common
    • Lights on/off: common
  
  Creates baseline semantic graph:
    person →(WALKS_TO)→ door
    door →(OPENS_WHEN)→ person_approaches
    light →(TURNS_ON)→ morning
  
  Neuromodulators:
    DA stable (predictions accurate)
    NE low (nothing surprising)
    
GENERATION 11-20 (Week 2):
  Unusual event: Person enters at 3 AM
  
  NE SPIKE: 0.30 → 0.95!
  Attention: Focuses on person (unexpected context)
  WM: Gates "person" + "3am" + "unusual_time"
  Semantic: Creates LEAP: person + 3am → "anomaly"
  
  Genome learns:
    temporal_sensitivity gene increases
    → Better at noticing time-based anomalies
    
GENERATION 21-50 (Month 1):
  Multiple anomalies detected and learned:
    • Unusual times → high NE
    • Unknown faces → high ACh (ambiguous)
    • Unexpected sounds → high NE
    • Loitering behavior → creates LEAP
  
  Evolved specialist genome:
    temporal_weight: 0.75 (high! time matters)
    face_recognition_threshold: 0.60 (strict)
    loitering_detection_window: 30.0 (seconds)
    ne_baseline: 0.60 (naturally curious)
    
  Performance:
    Anomaly detection: 96%
    False positives: 2% (very low!)
    Response time: 0.3 seconds
```

**Result:** Self-improving security system via evolution!

---

## Part 17: Implementation Guide

### Adding Camera to Current v2

**1. Add Vision Genes to Genome**

```cpp
// In genome.cpp
void Genome::add_vision_genes() {
    GeneModule vision("vision", 0.15f);
    
    vision.add_gene(make_gene("edge_threshold", 0.3f, 0.1f, 0.9f, 0.05f));
    vision.add_gene(make_gene("motion_weight", 0.4f, 0.0f, 1.0f, 0.05f));
    vision.add_gene(make_gene("color_weight", 0.3f, 0.0f, 1.0f, 0.05f));
    vision.add_gene(make_gene("patch_size", 32.0f, 16.0f, 64.0f, 4.0f));
    vision.add_gene(make_gene("novelty_threshold", 0.3f, 0.1f, 0.8f, 0.05f));
    
    modules.push_back(vision);
}

// Call in initialize_base_genome()
add_vision_genes();  // Now 59 genes!
```

**2. Create Camera Bridge**

```cpp
// melvin/v2/perception/camera_bridge.h
class CameraBridge {
public:
    CameraBridge(const Genome& genome);
    
    std::vector<PerceivedObject> capture_and_process();
    
private:
    cv::VideoCapture camera_;
    // Vision parameters from genome
    float edge_threshold_;
    float motion_weight_;
    float color_weight_;
};
```

**3. Replace Mock Perception**

```cpp
// In unified_loop_v2.cpp
std::vector<PerceivedObject> UnifiedLoopV2::perception_stage(...) {
    // OLD: Mock objects
    // NEW: Real camera
    if (camera_bridge_) {
        return camera_bridge_->capture_and_process();
    }
    // ...
}
```

**4. Run With Camera**

```cpp
// In demo
UnifiedLoopV2 brain(genome, seed);
brain.set_camera_bridge(new CameraBridge(genome));  // ← Camera!
brain.set_semantic_memory(v1_graph);

while (true) {
    auto result = brain.tick(nullptr, 0, 0);  // Camera provides data
    // Neuromodulators react to visual prediction errors
    // Evolution optimizes vision genes
}
```

---

## Part 18: The Evolution Training Loop

### Complete Code Example

```python
#!/usr/bin/env python3
"""
Evolve MELVIN v2 vision for object tracking
"""

import subprocess
import json
import numpy as np
from pathlib import Path

def evaluate_genome_vision(genome_path, camera_index, duration=60.0):
    """
    Run one genome on camera task, return fitness
    """
    # Build brain with this genome
    cmd = f"./build/v2/bin/evolve_vision --genome {genome_path} --camera {camera_index} --duration {duration}"
    
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    
    # Parse telemetry
    telemetry = json.loads(result.stdout)
    
    # Compute multi-objective fitness
    fitness = {
        'tracking_accuracy': telemetry['frames_on_target'] / telemetry['total_frames'],
        'prediction_quality': 1.0 - np.mean(telemetry['prediction_errors']),
        'stability': 1.0 / (telemetry['focus_switches'] + 1),
        'efficiency': telemetry['avg_cycle_time_ms'],
        'learning_speed': telemetry['concepts_learned'] / duration,
    }
    
    return fitness

def evolve_vision(num_generations=50, population_size=32):
    """
    Main evolution loop
    """
    # Initialize population
    population = [
        create_random_genome(seed=i) 
        for i in range(population_size)
    ]
    
    for gen in range(num_generations):
        print(f"Generation {gen}...")
        
        # Evaluate all individuals
        fitnesses = []
        for i, genome in enumerate(population):
            genome_path = f"/tmp/genome_{gen}_{i}.json"
            genome.save(genome_path)
            
            fitness = evaluate_genome_vision(genome_path, camera_index=0)
            fitnesses.append(fitness)
            
            print(f"  Genome {i}: accuracy={fitness['tracking_accuracy']:.2%}")
        
        # NSGA-II selection
        pareto_front = nsga2_select(population, fitnesses, keep=16)
        
        # Breed next generation
        offspring = []
        for _ in range(population_size - len(pareto_front)):
            parent1, parent2 = select_parents(pareto_front)
            child = crossover(parent1, parent2)
            child = mutate(child, sigma=0.05)
            offspring.append(child)
        
        population = pareto_front + offspring
        
        # Log best
        best_idx = np.argmax([f['tracking_accuracy'] for f in fitnesses])
        best_genome = population[best_idx]
        best_genome.save(f"best_gen_{gen}.json")
        
        print(f"  Best: {fitnesses[best_idx]['tracking_accuracy']:.2%}")
        print(f"  Vision genes: edge={best_genome.get_gene_value('vision', 'edge_weight'):.2f}")
    
    return population

# Run it!
if __name__ == "__main__":
    final_population = evolve_vision(num_generations=50, population_size=32)
    
    # Extract best genomes
    pareto_front = get_pareto_front(final_population)
    
    print(f"\nFinal Pareto Front: {len(pareto_front)} genomes")
    for i, genome in enumerate(pareto_front):
        print(f"  #{i}: vision_edge={genome.get_gene_value('vision', 'edge_weight')}")
```

**Expected Output After 50 Generations:**

```
Generation 0...
  Best: 65% accuracy
  Vision genes: edge=0.45, motion=0.58, color=0.32

Generation 10...
  Best: 82% accuracy
  Vision genes: edge=0.72, motion=0.48, color=0.28

Generation 25...
  Best: 93% accuracy
  Vision genes: edge=0.85, motion=0.35, color=0.22

Generation 50...
  Best: 97% accuracy
  Vision genes: edge=0.90, motion=0.30, color=0.18

Final Pareto Front: 5 genomes
  #1 (accuracy): edge=0.90, motion=0.30 → 97% accuracy
  #2 (speed): edge=0.65, motion=0.50 → 89% accuracy, 2x faster
  #3 (balanced): edge=0.80, motion=0.38 → 94% accuracy, balanced
```

---

## Part 19: Why This Is Powerful

### Traditional CV (Hand-Tuned)

```python
# Engineer manually tunes parameters
edge_threshold = 0.5  # Try 0.5? Too high? Too low?
motion_weight = 0.4   # Guess and check
color_weight = 0.3    # Takes weeks to optimize

# Results: Suboptimal, task-specific, no adaptation
```

### MELVIN v2 (Evolved)

```python
# Evolution discovers optimal parameters
genome = evolve_for_task(camera, task, generations=50)

# Results: 
# - Automatically finds edge_threshold=0.88
# - Discovers motion_weight=0.30 optimal
# - Learns color_weight=0.22 for this task
# - Adapts to new tasks via re-evolution
```

**Advantages:**
1. ✅ **Automatic optimization** - No manual tuning
2. ✅ **Task-specific** - Different tasks → different optimal genes
3. ✅ **Multi-objective** - Balances accuracy, speed, stability
4. ✅ **Continuous improvement** - Keeps evolving
5. ✅ **Explainable** - Every decision traces to genes + graph paths

---

## 🎯 Summary: Camera Evolution Pipeline

```
┌─────────────────────────────────────────────────────────┐
│  LIVE CAMERA (640×480 @ 30 FPS)                         │
└────────────────────┬────────────────────────────────────┘
                     │
                     ↓
┌─────────────────────────────────────────────────────────┐
│  GENOME-CONTROLLED PERCEPTION                           │
│  • Edge detection (threshold from gene)                 │
│  • Motion extraction (sensitivity from gene)            │
│  • Color processing (weights from gene)                 │
│  • Object formation (parameters from gene)              │
└────────────────────┬────────────────────────────────────┘
                     │
                     ↓
┌─────────────────────────────────────────────────────────┐
│  ATTENTION SYSTEM (genome-weighted)                     │
│  F = α·Saliency + β·Relevance + γ·Curiosity            │
│  + ACh·gain + NE·exploration - IOR                     │
└────────────────────┬────────────────────────────────────┘
                     │
                     ↓
┌─────────────────────────────────────────────────────────┐
│  PREDICTION & LEARNING                                  │
│  • Predict next frame                                   │
│  • Compare with actual                                  │
│  • Error → Neuromodulator update                        │
│  • DA gates Hebbian learning                            │
│  • Creates LEAP edges for patterns                      │
└────────────────────┬────────────────────────────────────┘
                     │
                     ↓
┌─────────────────────────────────────────────────────────┐
│  FITNESS EVALUATION                                     │
│  • Tracking accuracy                                    │
│  • Prediction quality                                   │
│  • Context sensitivity                                  │
│  • Learning speed                                       │
│  • Stability                                            │
│  • Exploration                                          │
└────────────────────┬────────────────────────────────────┘
                     │
                     ↓
┌─────────────────────────────────────────────────────────┐
│  EVOLUTION (NSGA-II)                                    │
│  • Select Pareto front                                  │
│  • Mutate vision genes                                  │
│  • Cross optimal genomes                                │
│  • Create next generation                               │
└────────────────────┬────────────────────────────────────┘
                     │
                     └─→ REPEAT for 50+ generations
                     
RESULT: Optimal vision system for specific deployment!
```

---

**MELVIN v2 with camera = self-improving visual intelligence!**

**Evolution optimizes:**
- How he processes images (vision genes)
- What he pays attention to (attention genes)
- How fast he learns (neuromod genes)
- What he remembers (WM genes)

**All automatically. All explainable. All from genes.** 🎥🧬🧠

