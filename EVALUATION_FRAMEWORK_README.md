# 🧠 Melvin C++ Evaluation Framework

## Overview

A comprehensive C++ evaluation framework for Melvin that implements production-grade validation with data leakage prevention, confidence calibration, error taxonomy, and ablation testing. Built for reliability, reproducibility, and CI/CD integration.

## 🎯 Key Features

### ✅ Data Leakage Prevention (Non-negotiable)
- **Official test splits only**: Uses HuggingFace's official validation/test splits
- **RUN_META.json persistence**: Tracks dataset, split, commit hash, seed, and profile
- **Few-shot exemplar pinning**: Separate file to prevent overlap with test items
- **Content overlap detection**: Validates no train/test contamination

### ✅ Per-Dataset Evaluation Adapters (Exact Metrics)

#### MCQ/Bool Datasets (CommonsenseQA, PIQA, OpenBookQA, BoolQ)
- **Accuracy@1**: Standard multiple choice accuracy
- **Calibrated Accuracy**: Temperature-scaled confidence accuracy
- **ECE**: Expected Calibration Error
- **Brier Score**: Confidence calibration quality
- **Error Taxonomy**: Retrieval miss, attention scatter, leap hallucination

#### Math Datasets (GSM8K, AQUA-RAT)
- **Exact Match**: Numerical answer precision
- **Digit-wise F1**: Partial credit for correct digits
- **Consistency**: Repeat same question 3×, % identical answers
- **Error Taxonomy**: Arithmetic slip, reasoning failure, magnitude error

### ✅ Confidence Calibration
- **Temperature Scaling**: Per-domain calibration parameters
- **ECE Calculation**: Expected Calibration Error with 10 bins
- **Brier Score**: Mean squared error of confidence
- **Cross-validation**: 5-fold CV for robust temperature fitting
- **Calibration Curves**: Visual validation of calibration quality

### ✅ Error Taxonomy (Fast Root-Cause)
Auto-buckets failures into:
- **Retrieval miss**: No relevant nodes found (context_overlap < 0.2)
- **Attention scatter**: High entropy at decision step (> 0.35)
- **Leap hallucination**: Low cosine similarity on chosen leap (< 0.5)
- **Arithmetic slip**: Working looks right, final extraction wrong
- **Context misuse**: Low context overlap despite multi-turn reference
- **Reasoning failure**: Complete reasoning breakdown
- **Magnitude error**: Order of magnitude mistakes

### ✅ Ablation Testing (Prove Each Upgrade Helps)
A/B testing with component flags:
```cpp
ComponentFlags flags;
flags.soft_traversal = true/false;
flags.prob_output = true/false;
flags.embeddings = true/false;
flags.dual_state = true/false;
flags.context_buffer = true/false;
```
Reports: Δaccuracy, Δlatency, Δentropy with statistical significance

### ✅ Robustness Tests (Cheap and Revealing)
- **Paraphrase**: Synonym swap & passive/active flip (≤3% drop)
- **Noise**: Inject 1-2 typos per query (≤5% drop)
- **Adversarial distractor**: Extra irrelevant sentence (≤5% drop)
- **Negation**: Add negation to questions
- **Context shift**: Slight context modifications

### ✅ Efficiency & Scaling Guardrails
- **Latency monitoring**: p50/p95 tracking per dataset+profile
- **RAM monitoring**: Peak memory usage tracking
- **Fanout limits**: Computational complexity bounds
- **Automatic failure**: Fails run if p95 latency > 50ms or RAM > cap

### ✅ Long-Context Sanity
- **Threaded conversations**: 8-16 turn sequences
- **Continuity gain**: Accuracy with context - accuracy without context ≥ +15%
- **Drift control**: Persona anchors unchanged (string match over fixed probe)
- **Context anchor matching**: Tracks relevant context usage

### ✅ Reproducibility Harness
Creates replay JSON for exact reproduction:
```json
{
  "dataset": "gsm8k",
  "item_id": "gsm8k_test_00412",
  "seed": 42,
  "profile": "Balanced",
  "flags": {"soft_traversal":1,"prob_output":1,"embeddings":1},
  "context_packet_ids": [...],
  "graph_snapshot": "snapshots/2025-10-05T15-22-11.bin"
}
```

### ✅ CI Gate System (Green = Ship)
Produces `results_summary.json` with:
- Domain accuracies
- ECE scores
- p95 latency
- Guardrail violations count

**CI Blocking Thresholds**:
- CommonsenseQA acc ≥ 70%
- PIQA acc ≥ 70%
- GSM8K EM ≥ 20%
- ECE ≤ 0.08
- p95 latency ≤ 1.8× classic
- Violations = 0

## 🚀 Quick Start

### Build the Framework
```bash
make evaluation_framework
```

### Run Single Dataset Evaluation
```bash
make run-eval-single
# or
./evaluation_framework --dataset commonsense_qa --max-samples 50
```

### Run Comprehensive Suite
```bash
make run-eval-comprehensive
# or
./evaluation_framework --comprehensive --max-samples 100
```

### Run CI Gate Validation
```bash
make run-ci-gate
# or
./evaluation_framework --ci-gate --datasets commonsense_qa,piqa,gsm8k
```

### Run Ablation Testing
```bash
make run-eval-ablation
# or
./evaluation_framework --ablation --dataset commonsense_qa
```

### Run Robustness Testing
```bash
make run-eval-robustness
# or
./evaluation_framework --robustness --dataset commonsense_qa
```

### Run Confidence Calibration
```bash
make run-eval-calibration
# or
./evaluation_framework --calibration --dataset commonsense_qa
```

### Replay Failed Prediction
```bash
make run-replay replay_file=path/to/replay.json
# or
./evaluation_framework --replay path/to/replay.json
```

## 📊 Architecture

### Core Components

```
evaluation/
├── EvaluationAdapter.hpp/cpp      # Base evaluation adapters
├── ConfidenceCalibration.hpp/cpp  # Temperature scaling & ECE
├── HuggingFaceIntegration.hpp/cpp # Dataset processing with data leakage prevention
├── AblationTesting.hpp/cpp        # A/B testing framework
├── EvaluationFramework.hpp/cpp    # Main orchestration
└── main.cpp                       # CLI interface
```

### Data Flow

```
HuggingFace Dataset → Data Leakage Prevention → Evaluation Adapter
                                                      ↓
Confidence Calibration ← Prediction Results ← Melvin Core
                                                      ↓
Error Taxonomy ← Ablation Testing ← Robustness Testing
                                                      ↓
CI Gate Validation → results_summary.json
```

## 📈 Usage Examples

### Basic Evaluation
```cpp
#include "evaluation/EvaluationFramework.hpp"

using namespace Melvin::Evaluation;

// Create evaluation framework
EvaluationFramework framework("evaluation_results");

// Configure evaluation
EvaluationFramework::EvaluationConfig config;
config.dataset_name = "commonsense_qa";
config.split = "validation";  // Official test split
config.max_samples = 100;
config.enable_calibration = true;
config.enable_ablation = true;

// Run evaluation
auto results = framework.runEvaluation(config);

// Check CI gate
EvaluationFramework::CIGateConfig ci_config;
bool passed = framework.validateCIGate(results, ci_config);
```

### Ablation Testing
```cpp
// Create ablation testing
AblationTesting ablation("reports");

// Define ablation configurations
std::vector<AblationTesting::AblationConfig> configs = {
    {"baseline", {true, true, true, true, true}, "All components"},
    {"no_soft_traversal", {false, true, true, true, true}, "Without soft traversal"},
    {"no_embeddings", {true, true, false, true, true}, "Without embeddings"}
};

// Run ablation
ablation.runAblation("commonsense_qa", ground_truth, configs);

// Generate report
ablation.generateAblationReport("reports/ablations.md");
```

### Confidence Calibration
```cpp
// Create calibrator
ConfidenceCalibrator calibrator("calibration");

// Fit calibration
calibrator.fitCalibration("commonsense_qa", predictions, ground_truth, "temperature");

// Apply calibration
auto calibrated_predictions = calibrator.calibratePredictions("commonsense_qa", predictions);

// Get metrics
auto metrics = calibrator.getMetrics("commonsense_qa");
std::cout << "ECE: " << metrics.ece << std::endl;
std::cout << "Brier Score: " << metrics.brier_score << std::endl;
```

### Error Taxonomy
```cpp
// Create evaluation manager
EvaluationManager manager;

// Evaluate dataset
auto metrics = manager.evaluateDataset("commonsense_qa", predictions, ground_truth);

// Generate error report
manager.generateErrorReport("artifacts/errors");

// Access error buckets
auto error_buckets = manager.getErrorBuckets("commonsense_qa");
for (const auto& [error_type, buckets] : error_buckets) {
    std::cout << error_type << ": " << buckets.size() << " examples" << std::endl;
}
```

## 🔧 Configuration

### Evaluation Configuration
```cpp
struct EvaluationConfig {
    std::string dataset_name;
    std::string split = "test";           // Official test splits only
    int max_samples = 100;
    std::string profile = "Balanced";
    int seed = 42;
    
    // Feature flags
    bool enable_calibration = true;
    bool enable_ablation = false;
    bool enable_robustness = false;
    bool enable_long_context = false;
    bool enable_error_taxonomy = true;
    
    // Calibration settings
    std::string calibration_method = "temperature";
    int cv_folds = 5;
    
    // Thresholds
    double min_accuracy_threshold = 0.7;
    double max_ece_threshold = 0.08;
    double max_latency_threshold = 50.0;
    int max_fanout_threshold = 16;
};
```

### CI Gate Configuration
```cpp
struct CIGateConfig {
    // Accuracy thresholds per dataset
    std::map<std::string, double> accuracy_thresholds = {
        {"commonsense_qa", 0.70},
        {"piqa", 0.70},
        {"gsm8k", 0.20},  // Exact match for math
        {"boolq", 0.75},
        {"openbookqa", 0.65}
    };
    
    // Calibration thresholds
    double max_ece_threshold = 0.08;
    double max_brier_threshold = 0.25;
    
    // Performance thresholds
    double max_latency_threshold = 50.0;  // ms
    int max_fanout_threshold = 16;
    int max_guardrail_violations = 0;
    
    // Robustness thresholds
    double max_paraphrase_drop = 0.03;    // 3%
    double max_noise_drop = 0.05;         // 5%
    double max_distractor_drop = 0.05;    // 5%
    
    // Long-context thresholds
    double min_continuity_gain = 0.15;    // 15%
    double min_persona_consistency = 0.85; // 85%
};
```

## 📊 Output Files

### Core Results
- `results_summary.json`: CI gate validation results
- `evaluation_report.md`: Comprehensive markdown report
- `RUN_META_{experiment_id}.json`: Experiment metadata

### Calibration
- `calibration/{dataset}_calibration.json`: Calibration parameters
- `calibration/calibration_report.json`: Calibration metrics
- `calibration/{dataset}_calibration_curve.png`: Calibration curves

### Error Taxonomy
- `artifacts/errors/{dataset}/{error_type}.json`: Error examples
- `artifacts/errors/error_report.md`: Error analysis

### Ablation Results
- `reports/ablations.md`: Ablation analysis
- `reports/ablations.json`: Structured ablation data

### Robustness Results
- `reports/robustness.md`: Robustness analysis
- `reports/long_context.md`: Long-context validation

### Reproducibility
- `snapshots/{timestamp}.bin`: System state snapshots
- `replay/{experiment_id}.json`: Replay configurations

## 🚨 Troubleshooting

### Common Issues

**Build Errors**
```bash
# Ensure all dependencies are installed
make clean
make evaluation_framework
```

**Dataset Download Fails**
```bash
# Check internet connection and HuggingFace access
./evaluation_framework --dataset commonsense_qa --max-samples 10
```

**Calibration Errors**
```bash
# Check prediction format
./evaluation_framework --calibration --dataset commonsense_qa --debug
```

**CI Gate Failures**
```bash
# Check thresholds and metrics
./evaluation_framework --ci-gate --verbose
```

### Debug Mode
```bash
# Enable debug logging
./evaluation_framework --debug --dataset commonsense_qa
```

### Validation
```bash
# Validate configuration
./evaluation_framework --validate-config --config evaluation_config.json
```

## 🔮 Future Enhancements

- **Multi-modal evaluation**: Image + text reasoning
- **Real-time monitoring**: WebSocket-based live metrics
- **Distributed evaluation**: Multi-GPU evaluation pipeline
- **Advanced calibration**: Platt scaling, isotonic regression
- **Adversarial evaluation**: Generated adversarial examples
- **Human evaluation**: Crowdsourced quality assessment

## 📚 API Reference

### Core Classes
- `EvaluationFramework`: Main orchestration class
- `EvaluationAdapter`: Base class for dataset-specific evaluation
- `ConfidenceCalibrator`: Temperature scaling and calibration
- `HuggingFaceIntegration`: Dataset processing with data leakage prevention
- `AblationTesting`: A/B testing framework
- `RobustnessTesting`: Perturbation-based testing
- `LongContextValidation`: Multi-turn conversation testing
- `ReproducibilityHarness`: Exact reproduction system

### Key Functions
- `runEvaluation()`: Main evaluation pipeline
- `fitCalibration()`: Fit confidence calibration
- `runAblation()`: Run component ablation tests
- `validateCIGate()`: CI gate validation
- `createReplayConfig()`: Create reproduction configuration

---

**Built with ❤️ for Melvin's production-grade evaluation and continuous improvement.**
