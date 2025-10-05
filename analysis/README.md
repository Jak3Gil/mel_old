# MELVIN LLM-STYLE ANALYTICS COMPANION

Complete verification, tuning, and analysis system for Melvin's LLM-style upgrades.

## 🎯 Overview

This analytics companion provides:
- **Automated verification** with A/B testing (Classic vs Hybrid)
- **Parameter tuning** across Conservative/Balanced/Creative profiles
- **Visualization** of key metrics and relationships
- **Health scoring** with guardrail monitoring
- **Performance optimization** recommendations

## 🚀 Quick Start

### 1. Run Verification Framework
```bash
# Baseline snapshot
make run-verification-profile profile=Conservative seed=42 > baseline_run.log

# Balanced tuning sweep
for T in 0.3 0.4 0.5; do
    make run-verification-profile profile=Balanced temp=$T | tee -a tuning_T${T}.log
done

# Creative stress test
make run-llm-smoke profile=Creative seed=123
```

### 2. Generate Analytics
```bash
# Install Python dependencies
pip install -r analysis/requirements.txt

# Run analytics with all plots
python3 analysis/verify_results.py --plot-all

# Filter by specific profile
python3 analysis/verify_results.py --profile Conservative --plot-all
```

### 3. Automated Tuning Sweep
```bash
# Run complete tuning sweep
./tuning_sweep.sh
```

## 📊 Key Metrics & Guardrails

### Target Ranges
- **Attention Entropy**: 0.08–0.35 (too low → mode lock; too high → scatterbrain)
- **Output Diversity**: 0.45–0.85 (too low → repetition; too high → ramble)
- **Top2 Margin**: >0.2 (decision confidence threshold)
- **Fanout**: ≤16 (prevent attention blow-up)
- **Latency p95**: <50ms (performance threshold)
- **Health Score**: >80% (overall system health)

### Health Score Calculation
- Attention entropy compliance: 25%
- Output diversity compliance: 25%
- Top2 margin compliance: 20%
- Latency compliance: 15%
- Fanout compliance: 15%

## 🎛️ Tuning Profiles

### Conservative Profile
- **Fanout**: 8 (limited exploration)
- **Temperature**: 0.2 (low creativity)
- **Leap threshold**: 0.67 (high similarity required)
- **Context decay**: 6 (fast forgetting)
- **Use case**: Production stability, deterministic responses

### Balanced Profile (Default)
- **Fanout**: 12 (moderate exploration)
- **Temperature**: 0.4 (balanced creativity)
- **Leap threshold**: 0.62 (moderate similarity)
- **Context decay**: 8 (balanced memory)
- **Use case**: General purpose, good performance

### Creative Profile
- **Fanout**: 16 (maximum exploration)
- **Temperature**: 0.7 (high creativity)
- **Leap threshold**: 0.57 (low similarity threshold)
- **Context decay**: 12 (persistent memory)
- **Use case**: Creative tasks, exploration, research

## 📈 Visualization Plots

### Core Plots
1. **accuracy_vs_attention_entropy.png**
   - Shows the "elbow" where reasoning quality flattens
   - Optimal range: 0.15-0.25 entropy

2. **diversity_vs_temperature.png**
   - Linear relationship between temperature and diversity
   - Target diversity: 0.6-0.8

3. **latency_histogram.png**
   - Performance distribution across runs
   - Target: <50ms p95

4. **health_score_distribution.png**
   - Overall system health across profiles
   - Target: >80% health score

### Advanced Plots (--plot-all)
5. **correlation_heatmap.png**
   - Metric relationships and dependencies
   - Identifies key optimization targets

6. **multi_metric_scatter.png**
   - Comprehensive view of all metrics
   - Helps identify optimal parameter combinations

## 🔧 Tuning Recommendations

### Common Patterns & Fixes

#### Low Accuracy + Low Entropy (<0.15)
- **Symptoms**: Repetitive, mode-locked responses
- **Fixes**: 
  - Increase temperature by +0.1
  - Increase fanout by +2
  - Lower leap threshold by -0.05

#### High Entropy (>0.30) + Low Accuracy
- **Symptoms**: Scatterbrain, incoherent responses
- **Fixes**:
  - Decrease temperature by -0.1
  - Decrease fanout by -2
  - Increase leap threshold by +0.05

#### High Latency (>50ms)
- **Symptoms**: Slow response times
- **Fixes**:
  - Decrease fanout by -4
  - Enable attention dropout
  - Reduce context window size

#### Low Diversity (<0.6)
- **Symptoms**: Repetitive phrasing
- **Fixes**:
  - Increase temperature by +0.1
  - Increase repetition penalty
  - Enable n-gram blocking

#### High Diversity (>0.85)
- **Symptoms**: Rambling, incoherent responses
- **Fixes**:
  - Decrease temperature by -0.1
  - Increase top-p threshold
  - Add coherence penalties

## 📋 Verification Test Suites

### 1. Smoke Checks (10 minutes)
- **Determinism**: seed=42, temp=0.0 → identical outputs
- **Stochasticity**: temp=0.8, top_p=0.95 → diverse outputs
- **Context carryover**: Q1→Q2 → context overlap >0.4

### 2. A/B Test Suites
- **Facts** (50 prompts): "X are mammals", dates, capitals
- **Compositional** (30 prompts): 2-3 hop chains
- **Analogy** (30 prompts): cosine-based leaps
- **Dialogue continuity** (10 threads): Context buffer effect

### 3. Performance Tests
- **Latency & RAM**: p50/p95 + peak MB
- **Output quality**: Human rating 1-5
- **Dual-state validation**: Parameter drift analysis

## 🏗️ Architecture

### Files Structure
```
analysis/
├── verify_results.py          # Main analytics script
├── requirements.txt           # Python dependencies
└── README.md                 # This file

verification_results/          # CSV output directory
├── facts_results.csv
├── compositional_results.csv
├── analogy_results.csv
├── dialogue_results.csv
└── plots/                     # Generated visualizations
    ├── accuracy_vs_attention_entropy.png
    ├── diversity_vs_temperature.png
    ├── latency_histogram.png
    ├── health_score_distribution.png
    ├── correlation_heatmap.png
    └── multi_metric_scatter.png

tuning_results/               # Tuning sweep outputs
├── baseline_run.log
├── tuning_T0.3.log
├── tuning_T0.4.log
└── tuning_T0.5.log
```

### CSV Schema
```csv
run_id,mode,seed,attention_entropy,top2_margin,output_diversity,accuracy,latency_ms,mem_mb,context_overlap,health_score,fanout
```

## 🚀 Production Deployment

### Rollout Checklist
- ✅ Backfill embeddings for existing nodes
- ✅ Cap fanout (top-k) and enable dropout
- ✅ Set default temps (0.4) & top-p (0.92)
- ✅ Enable dual-state consolidation timer
- ✅ Add n-gram block and repetition penalty
- ✅ Ship A/B harness + dashboards

### Monitoring
- **Real-time**: Health score, guardrail violations
- **Daily**: Parameter drift analysis
- **Weekly**: Performance trend analysis
- **Monthly**: Full verification suite re-run

## 🔍 Troubleshooting

### Common Issues
1. **No CSV files found**: Ensure verification framework ran successfully
2. **Empty plots**: Check data ranges and guardrail thresholds
3. **High violation rate**: Review tuning parameters and profiles
4. **Low health scores**: Run parameter optimization

### Debug Mode
```bash
# Verbose analytics with debug info
python3 analysis/verify_results.py --input-dir verification_results/ --plot-all --debug
```

## 📚 References

- **Original Cursor Prompt**: LLM-Style Reasoning & Output Upgrade
- **Verification Framework**: verification_framework.cpp
- **Tuning Configuration**: TuningConfig.h
- **LLM Reasoning Engine**: LLMReasoningEngine.h/.cpp

## 🤝 Contributing

To extend the analytics system:
1. Add new metrics to `verify_results.py`
2. Create new plot types in `create_plots()`
3. Update guardrails in `guardrails` dictionary
4. Add new tuning profiles in `TuningConfig.h`

---

**🎯 Goal**: Achieve >90% health score with optimal parameter tuning for production deployment of Melvin's LLM-style upgrades.
