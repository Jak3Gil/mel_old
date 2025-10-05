# 🚀 Enhanced Melvin Monitoring Integration Guide

## Overview

The enhanced monitoring system transforms your basic growth monitor into a **deep reasoning intelligence** that tells you not just "more" but "better" growth. It tracks composition, detects stalls, provides instant alerts, and can even auto-tune Melvin's parameters.

## 🎯 What's New

### 1. **Composition Tracking** 
- **Taught vs Thought nodes**: See learning vs reasoning phases
- **Exact/Temporal/Leap edges**: Monitor consolidation vs creativity
- **Weight statistics**: Track connection strength patterns

### 2. **Advanced Metrics**
- **Path length tracking**: Monitor reasoning depth
- **Edge rate & E2N ratio**: Detect healthy vs runaway growth
- **Health banding**: Color-coded performance (GREEN/YELLOW/RED)

### 3. **Stall Detection**
- **Stalled**: No growth in any dimension
- **Runaway**: Excessive edge growth with high entropy
- **Locked**: Very low entropy and diversity (mode lock)
- **Spiky leap**: Excessive leap edges vs temporal

### 4. **Auto-Tuning**
- **Automatic recommendations** logged to `out/autotune.log`
- **Parameter suggestions** based on detected patterns
- **Real-time feedback** for optimal Melvin configuration

## 🔧 Integration Steps

### Step 1: Add Composition Tracking to Your Code

**In your memory graph creation points:**

```cpp
#include "src/utils/MetricsLogger.h"

// After creating a node
void onNodeCreated(melvin::NodeKind kind) {
    Melvin::Utils::onNodeCreated(kind == TAUGHT ? Melvin::Utils::TAUGHT : Melvin::Utils::THOUGHT);
}

// After creating an edge
void onEdgeCreated(melvin::Rel rel, uint32_t weight_bits) {
    Melvin::Utils::Rel2Bit rel2bit;
    switch (rel) {
        case melvin::Rel::EXACT: rel2bit = Melvin::Utils::REL_EXACT; break;
        case melvin::Rel::TEMPORAL: rel2bit = Melvin::Utils::REL_TEMPORAL; break;
        case melvin::Rel::LEAP: rel2bit = Melvin::Utils::REL_LEAP; break;
        default: rel2bit = Melvin::Utils::REL_EXACT; break;
    }
    Melvin::Utils::onEdgeCreated(rel2bit, weight_bits);
}

// After creating a reasoning path
void onPathCreated(const std::vector<NodeID>& path) {
    Melvin::Utils::onPathCreated(path.size());
}
```

### Step 2: Enhanced Logging in Your Evaluation Loop

```cpp
// In your main evaluation loop:
static uint64_t step = 0;
auto& logger = Melvin::Utils::getMetricsLogger();

// Calculate deltas and rates
static uint64_t last_nodes = 0, last_edges = 0;
uint64_t delta_nodes = current_nodes - last_nodes;
uint64_t delta_edges = current_edges - last_edges;
double edge_rate = delta_edges / delta_time_ms;
double e2n_ratio = (double)current_edges / current_nodes;

// Get composition counters
auto& counters = Melvin::Utils::g_counters; // Access global counters

// Enhanced logging
logger.logComposition(
    ++step,
    current_nodes, current_edges, current_paths,
    counters.taught_nodes, counters.thought_nodes,
    counters.rel_exact, counters.rel_temporal, counters.rel_leap,
    counters.avg_weight_bits, counters.max_weight_bits,
    counters.avg_path_len, counters.max_path_len,
    edge_rate, e2n_ratio,
    attention_entropy, output_diversity, top2_margin,
    health_score, latency_ms, fanout,
    profile, dataset
);

// Stall detection and auto-tuning
auto detectors = Melvin::Utils::detectStalls(
    delta_nodes, delta_edges, delta_paths,
    delta_rel_temporal, delta_rel_leap,
    attention_entropy, output_diversity, health_score
);

if (!detectors.alert_message.empty()) {
    std::cout << "🚨 " << detectors.alert_message << std::endl;
    Melvin::Utils::autoTuneSettings(detectors.runaway, detectors.locked, detectors.spiky_leap);
}

last_nodes = current_nodes;
last_edges = current_edges;
```

### Step 3: Environment Setup

```bash
# Enable enhanced metrics logging
export DUMP_METRICS="out/metrics_live.csv"

# Or with log rotation
export DUMP_METRICS="out/metrics_$(date +%Y%m%d_%H%M%S).csv"

# Or zero-disk monitoring with named pipe
mkfifo out/metrics.pipe
export DUMP_METRICS="out/metrics.pipe"
```

## 🎮 Usage Examples

### Basic Enhanced Monitoring
```bash
# Terminal 1: Run evaluation with enhanced metrics
make run-verification-profile profile=Balanced DUMP_METRICS=out/metrics_live.csv

# Terminal 2: Enhanced dashboard
make watch-growth
```

### Micro HUD for Single Tests
```bash
# Terminal 1: Run single test
./your_evaluation_program DUMP_METRICS=out/metrics_live.csv

# Terminal 2: One-line progress bar
make watch-micro
```

### Log Rotation for Long Runs
```bash
# Rotate log with timestamp
make rotate-log

# Run evaluation with rotated log
make run-verification-profile profile=Balanced
```

### Zero-Disk Monitoring
```bash
# Setup named pipe
make setup-pipe

# Terminal 1: Run with pipe
export DUMP_METRICS="out/metrics.pipe"
./your_evaluation

# Terminal 2: Consume from pipe
column -t -s, < out/metrics.pipe
```

### Enhanced Demo
```bash
# See the enhanced system in action
make demo-enhanced
```

## 📊 What You'll See

### Enhanced Dashboard
```
📈 Melvin Enhanced Growth Monitor  (last 30 samples)
File: out/metrics_live.csv | Update: 1234
================================================================================================================================================
Step  Nodes   Δ  Growth               Edges   Δ  Connections         Paths   Δ  Reasoning           Health  Band  Entropy
------------------------------------------------------------------------------------------------------------------------------------------------
1     1.2K    0  ###############     2.1K    0  ###############     156     0  ###############     85.2   GREEN  0.156
2     1.2K   +45 #################   2.2K  +67 #################   167   +11 #################   87.1   GREEN  0.142
3     1.3K   +89 ##################  2.4K +123 ##################  189   +22 ##################  89.3   GREEN  0.138
...
------------------------------------------------------------------------------------------------------------------------------------------------
📊 Latest: Nodes=1.3K Edges=2.4K Paths=189 Health=89.3% Entropy=0.138
🧠 Composition: Taught=45 Thought=78 | Exact=120 Temporal=89 Leap=34
📏 Paths: Avg=3.2 Max=7.1 | E2N=1.85 Rate=12.5/ms
```

### Micro HUD
```
Step:  15  Nodes: 1245 (+ 23)  Edges: 2189 (+ 45)  Paths:  167 (+  8)  H=87.3  Ent=0.142  k= 6  T:45/78  TL:89/34  PL:3.2
```

### Stall Alerts
```
⚠️ RUNAWAY: Excessive edge growth (entropy 0.43) — consider lowering fanout_k or raising leap_threshold
🔒 LOCKED: Mode lock detected — consider increasing temperature or top_p
🎯 SPIKY: Excessive leap edges — consider raising leap_threshold
⏸️ STALLED: No growth detected — check input or increase exploration
```

## 🎛️ Auto-Tuning Recommendations

The system automatically logs tuning recommendations to `out/autotune.log`:

```
[1704067200] RECOMMEND: fanout_k: -2, leap_threshold: +0.03 (runaway edges)
[1704067201] RECOMMEND: temperature: +0.1, top_p: +0.02 (mode lock)
[1704067202] RECOMMEND: leap_threshold: +0.05 (excessive leaps)
```

## 🔍 Understanding the Metrics

### Composition Indicators
- **Taught nodes spike**: Learning phase, processing new information
- **Thought nodes increase**: Reasoning phase, building connections
- **Temporal edges rise**: Healthy consolidation, temporal reasoning
- **Leap edges spike**: Creative reasoning, but watch for "guessiness"

### Health Patterns
- **GREEN (≥80%)**: Optimal performance, balanced growth
- **YELLOW (60-80%)**: Good performance, minor issues
- **RED (<60%)**: Performance issues, needs attention

### Rate Indicators
- **Edge rate**: Should moderate over time, spikes indicate issues
- **E2N ratio**: Should plateau, second rise = excessive leap formation
- **Path length**: Longer paths = deeper reasoning

## 🚀 Ready-to-Run Examples

### HuggingFace Evaluation with Enhanced Monitoring
```bash
# Terminal 1: Run HuggingFace evaluation
make run-hf-pipeline DUMP_METRICS=out/metrics_live.csv profile=Balanced dataset=commonsense_qa

# Terminal 2: Enhanced monitoring
make watch-growth
```

### Ablation Testing with Composition Tracking
```bash
# Terminal 1: Run ablation test
make run-eval-ablation DUMP_METRICS=out/metrics_live.csv

# Terminal 2: Watch composition changes
make watch-growth
```

### Long-Run Monitoring with Rotation
```bash
# Setup log rotation
make rotate-log

# Run long evaluation
make run-eval-pipeline

# Monitor in separate terminal
make watch-growth-tail tail=100
```

## 🎯 Pro Tips

1. **Watch for patterns**: Temporal edge spikes = good consolidation
2. **Monitor leap ratio**: Too many leaps = raise leap_threshold
3. **Health trends**: Declining health = check attention entropy
4. **Path length**: Increasing avg path length = deeper reasoning
5. **Auto-tune logs**: Check `out/autotune.log` for recommendations

## 🔧 Troubleshooting

### No Composition Data
- Ensure you're calling `onNodeCreated()` and `onEdgeCreated()`
- Check that composition counters are being updated

### Stall Alerts Not Showing
- Verify you have enough data points (≥2 samples)
- Check that deltas are being calculated correctly

### Performance Issues
- Use named pipes for zero-disk monitoring
- Rotate logs for long runs
- Use micro HUD for single tests

---

**🎉 You now have a comprehensive monitoring system that tells you not just "more" but "better" growth!**
