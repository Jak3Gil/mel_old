# 📈 Melvin Live Growth Monitoring

## Overview

Real-time monitoring system that lets you **watch Melvin's nodes/edges grow in real time** during evaluation runs. Zero Python dependencies, minimal code changes, and plug-and-play integration.

## 🚀 Quick Start (Zero Code Changes)

If your verification/tuning runs already emit metrics CSV, you can just "tail" it:

```bash
# Terminal 1 — run any test that emits metrics
make run-verification-profile profile=Balanced DUMP_METRICS=out/metrics_live.csv

# Terminal 2 — live table of the last 20 samples
watch -n 0.5 'tail -n 20 out/metrics_live.csv | column -t -s,'
```

## 📊 Live ASCII Dashboard

For a beautiful real-time dashboard with sparklines and deltas:

```bash
# Terminal 1: Run evaluation with metrics
make run-verification-profile profile=Balanced DUMP_METRICS=out/metrics_live.csv

# Terminal 2: Live ASCII dashboard
make watch-growth
```

**Output:**
```
📈 Melvin Growth Monitor  (last 30 samples)
File: out/metrics_live.csv | Update: 1234
========================================================================================================================
Step   Nodes    Δ    Graph Growth                      Edges    Δ    Connections                    Paths    Δ    Reasoning                     Health      Entropy
------------------------------------------------------------------------------------------------------------------------
1      1.2K   +0    ██████████████████████████████████  2.1K   +0    ██████████████████████████████████  156   +0    ██████████████████████████████████  85.2       0.156
2      1.2K   +0    ██████████████████████████████████  2.1K   +0    ██████████████████████████████████  156   +0    ██████████████████████████████████  85.2       0.156
3      1.3K   +45   ██████████████████████████████████  2.2K   +67   ██████████████████████████████████  167   +11   ██████████████████████████████████  87.1       0.142
4      1.4K   +89   ██████████████████████████████████  2.4K   +123  ██████████████████████████████████  189   +22   ██████████████████████████████████  89.3       0.138
...
========================================================================================================================
📊 Latest: Nodes=1.4K Edges=2.4K Paths=189 Health=89.3% Entropy=0.138
💡 Legend: █ = relative scale; Δ = change vs previous; Colors: Green≥80% Yellow≥60% Red<60%
🔄 Auto-refresh every 350ms | Press Ctrl+C to exit
```

## 🔧 Integration (1 Minute Setup)

### Step 1: Add MetricsLogger Header

Add to your evaluation loop (e.g., in your C++ evaluation framework):

```cpp
#include "src/utils/MetricsLogger.h"

// In your main evaluation loop:
static MetricsLogger METRICS("out/metrics_live.csv");

// After each learn/reason cycle:
METRICS.logMelvinMetrics(
    step++, 
    G_nodes.size(),           // Current node count
    G_edges.size(),           // Current edge count  
    G_paths_count(),          // Current path count
    metrics.attention_entropy,
    metrics.output_diversity,
    metrics.top2_margin,
    metrics.health_score,
    metrics.latency_ms,
    metrics.fanout,
    profile,                  // e.g., "Conservative", "Balanced", "Creative"
    dataset_name              // e.g., "commonsense_qa"
);
```

### Step 2: Environment Variable Support

Your evaluation framework can read the metrics log path from environment:

```cpp
#include <cstdlib>

// In your main():
const char* metrics_path = std::getenv("DUMP_METRICS");
if (metrics_path) {
    Melvin::Utils::initMetricsLogger(metrics_path);
    std::cout << "📊 Metrics logging to: " << metrics_path << std::endl;
}
```

## 🎯 Usage Examples

### Basic Monitoring
```bash
# Build the growth monitor
make tools/growth_monitor

# Run evaluation with metrics
make run-verification-profile profile=Balanced DUMP_METRICS=out/metrics_live.csv

# Watch live growth (in another terminal)
make watch-growth
```

### Custom Tail Size
```bash
# Watch last 50 samples instead of 30
make watch-growth-tail tail=50
```

### Simple CSV Viewer
```bash
# Alternative: simple table view
make watch-csv
```

### HuggingFace Evaluation with Monitoring
```bash
# Terminal 1: Run HuggingFace evaluation
make run-hf-pipeline DUMP_METRICS=out/metrics_live.csv profile=Balanced

# Terminal 2: Watch growth
make watch-growth
```

### Dataset-Specific Monitoring
```bash
# Commonsense reasoning (steady growth, low entropy)
make run-verification-profile profile=Conservative DUMP_METRICS=out/metrics_live.csv

# PIQA (temporal/causal, watch leap edges form)  
make run-verification-profile profile=Balanced DUMP_METRICS=out/metrics_live.csv

# GSM8K (paths jump during solution steps)
make run-verification-profile profile=Balanced DUMP_METRICS=out/metrics_live.csv
```

## 📈 What "Good Growth" Looks Like

### Healthy Patterns:
- **Nodes**: Monotonic increase while learning; smaller increases during reasoning (thought nodes)
- **Edges**: Increase faster than nodes early (temporal + leap edges); slope moderates as redundancy rises  
- **Paths**: Step-like growth when successful multi-hop chains are found
- **Attention entropy**: Should settle in **0.08–0.35** band; spikes mean scatterbrain, near-zero means mode lock

### Warning Signs:
- **Nodes plateauing early**: May indicate insufficient learning
- **Edges growing too fast**: Potential over-connection
- **Paths not increasing**: Reasoning may be failing
- **High entropy spikes**: Attention scatter, model confusion
- **Low entropy consistently**: Mode locking, lack of exploration

## 🛠️ Advanced Usage

### Named Pipes (Zero Disk I/O)
```bash
# Create named pipe
mkfifo out/metrics.pipe

# Point MetricsLogger to out/metrics.pipe
# Then consume from another terminal:
column -t -s, < out/metrics.pipe
```

### File Rotation
```bash
# Rotate per run for clean separation
export METRICS_FILE="out/metrics_$(date +%Y%m%d_%H%M%S).csv"
make run-verification-profile DUMP_METRICS=$METRICS_FILE
```

### Custom Metrics
```cpp
// Add custom metrics to the logger
METRICS.log(
    step,
    nodes, edges, paths,
    attention_entropy, diversity, top2_margin, health,
    latency_ms,
    fanout,
    profile,
    dataset,
    // Custom fields can be added to the CSV
    "custom_metric_value"
);
```

## 🎨 Dashboard Features

### Real-time Updates
- **350ms refresh rate**: Smooth real-time updates
- **Auto-clear screen**: Clean dashboard on each update
- **Timestamp display**: Shows when data was last updated

### Visual Elements
- **Sparklines**: █ bars show relative scale within the window
- **Delta indicators**: +123/-45 show change vs previous sample
- **Color coding**: Green≥80% Yellow≥60% Red<60% for health scores
- **Number formatting**: 1.2K, 1.4M for large numbers

### Information Display
- **Latest summary**: Current state at bottom
- **Legend**: Explains symbols and colors
- **File path**: Shows which CSV is being monitored
- **Sample count**: Shows how many recent samples are displayed

## 🔧 Troubleshooting

### Common Issues

**"Waiting for data..."**
```bash
# Check if CSV file exists and has data
ls -la out/metrics_live.csv
head -5 out/metrics_live.csv
```

**"No data" message**
```bash
# Ensure your evaluation is actually logging
# Check environment variable is set
echo $DUMP_METRICS

# Verify metrics logger is initialized
# Look for "📊 Initialized metrics logger" in your output
```

**Dashboard not updating**
```bash
# Check file permissions
ls -la out/metrics_live.csv

# Check if file is being written to
tail -f out/metrics_live.csv
```

**Build errors**
```bash
# Clean and rebuild
make clean
make tools/growth_monitor
```

### Debug Mode
```bash
# Run with debug output
make run-verification-profile profile=Balanced DUMP_METRICS=out/metrics_live.csv DEBUG=1
```

## 📊 CSV Format

The metrics CSV has the following columns:
```
timestamp,step,nodes,edges,paths,attn_entropy,diversity,top2_margin,health,latency_ms,fanout,profile,dataset
```

Example:
```
1704067200.123,1,1200,2100,156,0.156,0.623,0.234,85.2,12.5,8,Balanced,commonsense_qa
1704067200.456,2,1245,2167,167,0.142,0.634,0.241,87.1,11.8,7,Balanced,commonsense_qa
```

## 🚀 Pro Tips

1. **Smooth updates**: The logger flushes immediately for real-time updates
2. **File rotation**: Use timestamps in filenames for clean separation per run
3. **Named pipes**: Use for zero-disk I/O if you don't need persistence
4. **Multiple terminals**: Run evaluation in one terminal, monitoring in another
5. **Custom tail size**: Adjust the number of samples shown based on your needs
6. **Health thresholds**: Watch for health scores below 60% (red) as warning signs

## 🔮 Future Enhancements

- **Web dashboard**: HTML/JavaScript version for browser viewing
- **Alert system**: Automatic alerts when health drops below thresholds
- **Historical analysis**: Track growth patterns across multiple runs
- **Export features**: Save monitoring data for later analysis
- **Multi-run comparison**: Side-by-side comparison of different profiles
- **Performance profiling**: Integration with CPU/memory monitoring

---

**Built with ❤️ for real-time Melvin monitoring and debugging.**
