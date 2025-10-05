# UCA Mathematical Formula Testing Framework

This comprehensive testing framework validates and tunes the mathematical formulas in Melvin's Unified Cognitive Architecture (UCA). It provides systematic testing, ablation studies, and performance monitoring to ensure the math is working as expected.

## 🧪 Quick Start

```bash
# Build everything
make all

# Run quick 2-minute tests
make test-quick

# Run comprehensive formula tests
make test-uca

# Run 1-minute smoke tests
make test-smoke

# Run full experiment suite
make test-experiment
```

## 📊 What Gets Tested

### Core Mathematical Formulas (A1-A6)

1. **A1: Hub Bias Mitigation** - Degree normalization prevents high-degree nodes from dominating
2. **A2: Relation Priors** - Different relation types (TEMPORAL, GENERALIZATION, EXACT, LEAP) have different weights
3. **A3: Contradiction Penalty** - Contradictory paths get exponentially penalized
4. **A4: Temporal Continuity** - Smooth temporal sequences preferred over jagged ones
5. **A5: Multi-hop Discount** - Longer paths get geometrically discounted
6. **A6: Beam Diversity** - Similar paths in beam get penalized for diversity

### Learning & Confidence (B1-B2, C1-C3)

1. **B1: Log-odds Confidence** - Confidence calculated from path score, length, similarity, contradiction
2. **B2: Path Entropy** - Entropy-based uncertainty quantification
3. **C1-C3: Learning Curve** - Confidence increases, time decreases, counts smooth with EMA

## 🔬 Testing Components

### 1. CSV Metrics Logging (`melvin_scheduler.cpp`)

Every inference logs detailed metrics to `uca_metrics.csv`:
- `t`: timestamp
- `conf`: confidence score
- `entropy`: path entropy
- `path_score`: raw path score
- `len`: path length
- `div_pen`: diversity penalty
- `rec_gap`: temporal gap
- `deg_norm`: degree normalization
- `reward`: feedback reward
- `formula_flags`: which formulas are active

### 2. Environment Variable Ablation Switches

Toggle formulas on/off without rebuilding:

```bash
# Disable degree normalization
UCA_NO_DEGREE=1 ./melvin_scheduler

# Disable relation priors
UCA_NO_RELPR=1 ./melvin_scheduler

# Disable contradiction penalty
UCA_NO_CONTRA=1 ./melvin_scheduler

# Disable temporal continuity
UCA_NO_CONT=1 ./melvin_scheduler

# Disable diversity penalty
UCA_NO_DIV=1 ./melvin_scheduler

# Enable multi-hop discount
UCA_USE_TD=1 ./melvin_scheduler
```

### 3. Micro-Graph Test Harness (`test_uca_formulas.cpp`)

Deterministic tests that validate each formula in isolation:

```bash
# Run all formula tests
make test-uca

# Run ablation tests
make test-uca-ablation
```

Each test creates specific micro-graphs and validates expected behavior:
- **HubStar(n, correct_leaf_idx)** for A1
- **TemporalChain(k, jitter)** for A4  
- **TwinPathsSameWeight(len2,len5)** for A5
- **ContradictPair()** for A3

### 4. Smoke Tests (`run_smoke_tests.sh`)

1-minute scripted sessions with clear expectations:

```bash
make test-smoke
```

Tests:
- Hub bias: meaningful leaf outranks hub routes >90% of trials
- Relation priors: TEMPORAL > GEN > EXACT > LEAP in edge selection
- Contradiction: reptile path score decays ~e^(-β) vs mammal
- Temporal continuity: smooth series wins consistently
- Multi-hop discount: len=2 wins vs len=5 unless γ≈1
- Beam diversity: top-K are diverse (low Jaccard overlap)

### 5. Experiment Runner (`experiment_runner.py`)

Systematic testing with statistical analysis:

```bash
# Full experiment suite (2 hours)
make test-experiment

# Quick ablation tests (2 minutes)
make test-quick

# Learning curve only
python3 experiment_runner.py --learning-only

# Ablation tests only
python3 experiment_runner.py --ablation-only
```

Generates:
- Confidence comparisons across ablations
- Learning curve analysis
- Stress test with noise prompts
- Statistical significance testing
- Visualization plots
- JSON report with recommendations

## 🎯 Expected Results

### Good Starting Parameters

```cpp
// Mathematical constants (tunable defaults)
DEGREE_EPSILON = 1e-6f;
RECENCY_TAU = 300.0f;  // 5 minutes
MULTI_HOP_GAMMA = 0.93f;
DIVERSITY_LAMBDA = 0.3f;
TEMPORAL_CONTINUITY_T = 5.0f;  // 5 seconds
CONTRADICTION_BETA = 1.5f;

// Relation priors
REL_PRIOR_TEMPORAL = 1.20f;
REL_PRIOR_GENERALIZATION = 1.10f;
REL_PRIOR_EXACT = 1.00f;
REL_PRIOR_LEAP = 0.85f;

// Confidence coefficients
CONF_BETA_PATH = 3.0f;
CONF_BETA_LENGTH = 0.15f;
CONF_BETA_SIMILARITY = 0.5f;
CONF_BETA_CONTRADICTION = 1.0f;
```

### Success Indicators

- **Baseline confidence**: 0.4-0.8 for meaningful queries
- **Hub bias mitigation**: Degree norm < 1.0 for high-degree nodes
- **Relation priors**: TEMPORAL > GENERALIZATION > EXACT > LEAP
- **Contradiction penalty**: Confidence drops 50%+ for contradictory paths
- **Learning curve**: Confidence increases or stays stable with repetition
- **Diversity**: Top-K paths have Jaccard similarity < 0.7

## 🛡️ Numerical Safety

All formulas include safety guards:

```cpp
// Clamp scores to safe range
score = std::max(1e-9f, std::min(score, 1e+9f));

// Clamp inputs to reasonable ranges
degree = std::max(1.0f, std::min(degree, 1000.0f));
path_length = std::max(1UL, std::min(path_length, 20UL));

// Prevent division by zero
result = 1.0f / (denominator + EPSILON);
```

## 📈 Monitoring & Tuning

### Real-time Monitoring

```bash
# Watch metrics in real-time
tail -f uca_metrics.csv

# Monitor specific formula
grep "D" uca_metrics.csv | tail -20
```

### Quick Tuning

```bash
# If outputs too timid: raise path weight
export UCA_CONF_BETA_PATH=4.0

# If too confident: lower emission threshold  
export UCA_MIN_CONF_EMIT=0.12

# If hub bias: ensure degree norm is on
unset UCA_NO_DEGREE
```

### Performance Analysis

```python
import pandas as pd
import matplotlib.pyplot as plt

# Load metrics
df = pd.read_csv('uca_metrics.csv')

# Plot confidence over time
plt.plot(df['t'], df['conf'])
plt.title('Confidence Over Time')
plt.show()

# Compare formula effectiveness
formula_groups = df.groupby('formula_flags')['conf'].mean()
print(formula_groups)
```

## 🚀 Quick Wins

1. **Turn on degree normalization + diversity** - usually yields visibly better paths
2. **Lower min_conf_for_emit to 0.12** while system "warms up"
3. **Start with EMA counts** - improves stability immediately
4. **Use relation priors** - TEMPORAL > GENERALIZATION > EXACT > LEAP

## 🔧 Troubleshooting

### Low Confidence
- Check if formulas are enabled (look for flags in CSV)
- Verify numerical safety guards aren't clamping too aggressively
- Ensure mock data is properly initialized

### High Variance
- Enable EMA smoothing for counts
- Increase beam width for more stable paths
- Check for numerical instability in log-space calculations

### No Learning
- Verify feedback loop is connected
- Check if learning rates are too small
- Ensure reward signals are reaching the learning engine

## 📁 File Structure

```
├── melvin_scheduler.cpp          # Main UCA loop with metrics logging
├── src/uca/ReasoningEngine.cpp   # Mathematical formulas with ablation switches
├── src/uca/uca_types.h          # Enhanced metrics structure
├── test_uca_formulas.cpp        # Micro-graph test harness
├── run_smoke_tests.sh           # 1-minute smoke tests
├── experiment_runner.py          # Systematic experiment suite
├── uca_metrics.csv              # Real-time metrics (generated)
├── test_results.csv             # Test results (generated)
└── experiment_results/          # Experiment outputs (generated)
    ├── experiment_report.json
    └── confidence_comparison.png
```

## 🎯 Next Steps

1. **Run baseline**: `make test-quick` to establish current performance
2. **Identify bottlenecks**: Look for formulas with low impact in ablation tests
3. **Tune parameters**: Adjust constants based on experiment results
4. **Validate improvements**: Re-run tests to confirm changes help
5. **Monitor in production**: Use CSV logging to track real-world performance

This framework provides everything needed to prove the math is working and tune it quickly without destabilizing the build.
