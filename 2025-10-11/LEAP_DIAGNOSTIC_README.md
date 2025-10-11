# 🧭 Graph-Guided Predictive System Diagnostic & Auto-Tuning

## Overview

This diagnostic and auto-tuning system measures and optimizes Melvin's **graph-guided predictive reasoning system**, specifically testing:

1. **Graph Bias Effectiveness** - Whether graph structure influences token predictions
2. **Node-Token Embedding Alignment** - Semantic similarity between graph nodes and language tokens
3. **Leap Node Performance** - Effectiveness of conceptual leap abstractions

## 🎯 Primary Metrics

The system tracks three key health metrics:

| Metric | Target | Description |
|--------|--------|-------------|
| Mean Entropy Reduction | ≥ 0.20 | How much uncertainty is reduced by graph guidance |
| Mean Context Similarity | ≥ 0.50 | Cosine similarity between activations and predictions |
| Leap Success Rate | ≥ 60% | Percentage of leaps that improve predictions |

## 📋 Test Suite

The diagnostic runs 10 conceptual leap tests:

```
fire → water
music → emotion
robot → person
sun → night
anger → calm
bird → flight
tree → air
food → energy
thought → memory
rain → growth
```

Each test measures:
- Entropy before/after bias application
- Graph bias strength
- Active cluster count
- Top biased tokens
- Top similar nodes
- Conceptual leap success

## 🚀 Quick Start

### Build the Diagnostic Tool

```bash
cd /Users/jakegilbert/Desktop/Melvin/Melvin/2025-10-11
make diagnostic
```

### Run Basic Diagnostics

```bash
make run_diagnostic
```

This will:
- Run 10 standard test prompts
- Generate `leap_diagnostics.csv`
- Generate `leap_tuning_report.md`
- Print summary to console

### Run Full Auto-Tuning

```bash
make run_diagnostic_auto_tune
```

This will:
1. Sweep `lambda_graph_bias` from 0.2 → 1.2
2. Sweep `leap_entropy_threshold` from 0.4 → 0.8
3. Check embedding quality
4. Verify activation normalization
5. Tune learning rate from 0.01 → 0.05
6. Generate comprehensive report

### Quick Diagnostic (5 tests)

```bash
make run_diagnostic_quick
```

## 📊 Output Files

### 1. `leap_diagnostics.csv`

Raw diagnostic data with columns:
- Prompt
- EntropyBefore, EntropyAfter, EntropyReduction
- MeanBias, Lambda, ClusterCount
- ContextSimilarity
- LeapTriggered, LeapSuccess
- TopToken1, TopToken2, TopToken3
- Comments

### 2. `leap_tuning_report.md`

Markdown report with:
- Summary statistics table
- System health assessment
- Detailed results per test
- Recommendations
- Current configuration snapshot

### 3. `leap_tuning_results.txt`

Auto-tuning results with:
- Initial vs. final performance
- Improvements
- Optimized parameters
- Specific recommendations

## 🔧 Command Line Options

```bash
./diagnostic_main [OPTIONS]

Options:
  --auto-tune      Run comprehensive auto-tuning procedure
  --full-report    Generate detailed markdown report
  --quick          Run quick diagnostic (5 tests only)
  --csv-only       Only generate CSV output
  --help           Show help message
```

## 🧪 Understanding the Results

### Entropy Reduction

```
entropy_before=0.79
entropy_after=0.48
entropy_reduction=0.31  ✅ GOOD (>0.20)
```

**Interpretation:**
- **High (>0.30)**: Graph is strongly influencing predictions
- **Moderate (0.15-0.30)**: Some influence, may need tuning
- **Low (<0.15)**: Bias too weak, increase `lambda_graph_bias`

### Context Similarity

```
context_similarity=0.58  ✅ GOOD (>0.50)
```

**Interpretation:**
- **High (>0.50)**: Embeddings well-aligned
- **Moderate (0.30-0.50)**: Partial alignment, increase learning rate
- **Low (<0.30)**: Embeddings misaligned, retrain needed

### Leap Success Rate

```
leap_success_rate=68%  ✅ GOOD (>60%)
```

**Interpretation:**
- **High (>60%)**: Leaps consistently improve predictions
- **Moderate (40-60%)**: Some leaps work, tune thresholds
- **Low (<40%)**: Leaps ineffective, check clustering

## ⚙️ Tuning Parameters

### 1. Lambda Graph Bias (`lambda_graph_bias`)

**Range:** 0.2 - 1.2  
**Default:** 0.5

Controls how strongly graph structure biases token predictions.

- **Too low (<0.3)**: Graph has little influence, random predictions
- **Optimal (0.5-0.8)**: Balanced between graph and language model
- **Too high (>1.0)**: May produce nonsense but semantically related

### 2. Leap Entropy Threshold (`leap_entropy_threshold`)

**Range:** 0.4 - 0.8  
**Default:** 0.6

Entropy level that triggers a conceptual leap.

- **Too low (<0.4)**: Leaps trigger too often, unnecessary
- **Optimal (0.5-0.7)**: ~30-50% trigger rate
- **Too high (>0.8)**: Leaps rarely trigger, missed opportunities

### 3. Learning Rate Embeddings (`learning_rate_embeddings`)

**Range:** 0.01 - 0.05  
**Default:** 0.01

How quickly embeddings adapt to feedback.

- **Too low (<0.01)**: Slow convergence, 1000+ iterations needed
- **Optimal (0.02-0.03)**: Good convergence, 100-200 iterations
- **Too high (>0.05)**: Unstable, oscillations

## 🔍 Diagnostic Interpretation

### Scenario 1: Entropy drops + relevant words

```
✅ Graph influence working
```

**Action:** No tuning needed, continue monitoring

### Scenario 2: Entropy unchanged + random words

```
❌ Embeddings misaligned
```

**Action:**
- Increase `learning_rate_embeddings`
- Run 100+ training cycles
- Check embedding initialization

### Scenario 3: Entropy ↓ but nonsense text

```
⚠️  λ too high or activations unnormalized
```

**Action:**
- Reduce `lambda_graph_bias`
- Verify activation normalization

### Scenario 4: No leaps triggered

```
⚠️  Threshold too strict or detection off
```

**Action:**
- Lower `leap_entropy_threshold`
- Check repetition detection

## 🧠 Post-Tuning Verification

After tuning completes, re-run diagnostics:

```bash
# 1. Apply tuned parameters (shown at end of auto-tune)
# 2. Re-run diagnostic
make run_diagnostic

# 3. Check if ≥70% tests show improvement
```

**Success criteria:**
- ≥70% of tests show conceptual continuity
- Mean cosine similarity > 0.5 for correct pairs
- Entropy reduction ≥ 0.2 on average

## 📈 Integration into Production

### 1. Update Configuration Files

After successful tuning, update these files:

**`src/util/config.h`:**
```cpp
float lambda_graph_bias = 0.75f;  // Your tuned value
```

**`melvin_leap_nodes.h`:**
```cpp
float leap_entropy_threshold = 0.55f;  // Your tuned value
```

**Embedding bridge config:**
```cpp
float learning_rate_embeddings = 0.025f;  // Your tuned value
```

### 2. Periodic Monitoring

Run diagnostics weekly:

```bash
# Quick health check
make run_diagnostic_quick

# Monthly comprehensive check
make run_diagnostic_auto_tune
```

### 3. Embedding Training

Continue training embeddings during production:

```cpp
// After each successful prediction
embedding_bridge.learn_from_prediction(
    active_nodes, 
    activations, 
    predicted_token, 
    reward=1.0f
);
```

## 🛠️ Troubleshooting

### Problem: All tests fail with "No nodes found"

**Solution:**
- Initialize knowledge graph first
- Load brain data: `melvin_brain.bin`
- Or run on test graph (automatically created)

### Problem: Entropy reduction is negative

**Solution:**
- Check that `lambda_graph_bias` isn't negative
- Verify embeddings are initialized (not all zeros)
- Confirm softmax is applied after bias

### Problem: Context similarity always zero

**Solution:**
- Embeddings not initialized - run training
- Check embedding dimensions match (default: 64)
- Verify normalization in activation vector

### Problem: Auto-tune makes performance worse

**Solution:**
- Revert to default values
- Run more samples per config (increase `test_samples_per_config`)
- Check for overfitting to test set

## 📚 Advanced Usage

### Custom Test Prompts

Edit `leap_diagnostic.h`:

```cpp
inline std::vector<TestPromptPair> get_standard_test_prompts() {
    return {
        {"fire", "water"},
        {"music", "emotion"},
        // Add your own:
        {"custom_concept_a", "custom_concept_b"}
    };
}
```

### Adjust Tuning Ranges

In `leap_auto_tune.cpp`:

```cpp
TuningConfig tuning_config;
tuning_config.lambda_min = 0.3f;     // Adjust range
tuning_config.lambda_max = 1.0f;
tuning_config.lambda_step = 0.1f;    // Finer steps
```

### Enable Verbose Logging

Set environment variable:

```bash
LOG_PREDICTIONS=1 ./diagnostic_main --auto-tune
```

## 📞 Support

For issues or questions:

1. Check `leap_tuning_report.md` recommendations
2. Review this README's troubleshooting section
3. Examine raw data in `leap_diagnostics.csv`
4. Check that knowledge graph is properly loaded

## 🎓 Theory

### Why Entropy Reduction Matters

Entropy measures prediction uncertainty:
- **High entropy**: Model unsure, many equally likely options
- **Low entropy**: Model confident, clear prediction

Graph bias should **reduce entropy** by:
- Ruling out semantically unrelated tokens
- Boosting contextually relevant tokens
- Providing structural constraints

### Why Context Similarity Matters

Context similarity measures embedding alignment:
- Node embeddings ↔ Token embeddings
- If aligned: high similarity when contextually appropriate
- If misaligned: similarity random or inverted

### Why Leap Success Matters

Leap nodes represent:
- Emergent conceptual patterns
- Cross-domain analogies
- Higher-order abstractions

Success rate indicates:
- Cluster quality
- Embedding richness
- Generalization capability

---

**Last Updated:** 2025-10-11  
**Version:** 1.0.0  
**Maintainer:** Melvin Team

