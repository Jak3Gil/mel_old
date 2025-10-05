# 🤗 Melvin HuggingFace Integration & Analytics Framework

## Overview

This framework extends Melvin with comprehensive external validation using real-world datasets from HuggingFace. It provides multi-domain reasoning validation, advanced analytics, and regression monitoring capabilities.

## 🎯 Supported Datasets

| Dataset | Domain | Purpose | Reasoning Type |
|---------|--------|---------|----------------|
| `commonsense_qa` | Commonsense | Everyday logic and analogy | Multi-choice reasoning |
| `piqa` | Physical Commonsense | Cause-effect and physical reasoning | Binary choice reasoning |
| `gsm8k` | Mathematical | Multi-hop arithmetic reasoning | Numerical problem solving |
| `boolq` | Boolean QA | Yes/No question answering | Binary classification |
| `openbookqa` | Open-book QA | Knowledge-based reasoning | Multi-choice with context |

## 🚀 Quick Start

### 1. Install Dependencies
```bash
make install-hf-deps
# or
pip install -r analysis/requirements.txt
```

### 2. Run Single Dataset
```bash
make run-hf-single
```

### 3. Run Comprehensive Suite
```bash
make run-hf-comprehensive
```

### 4. Run Full Pipeline
```bash
make run-hf-pipeline
```

## 📊 Analytics & Visualization

### Command Line Analytics
```bash
# Basic analysis
python3 analysis/verify_results.py

# Domain-specific analysis
python3 analysis/verify_results.py --domain-analysis --plot-all

# Filter by profile
python3 analysis/verify_results.py --profile Conservative

# Filter by domain
python3 analysis/verify_results.py --domain commonsense_qa
```

### Interactive Dashboard
```bash
make run-dashboard
# or
streamlit run analysis/dashboard.py
```

## 🎛️ Makefile Targets

| Target | Description |
|--------|-------------|
| `run-hf-single` | Download and process single dataset |
| `run-hf-comprehensive` | Run comprehensive multi-dataset suite |
| `run-hf-multi` | Run multi-dataset validation |
| `run-hf-tests` | Run HuggingFace test suite through Melvin |
| `analyze-hf-results` | Analyze results with visualizations |
| `run-hf-pipeline` | Run complete validation pipeline |
| `run-dashboard` | Launch Streamlit regression dashboard |
| `install-hf-deps` | Install Python dependencies |

## 📈 Key Metrics & Guardrails

### Health Score Components
- **Attention Entropy** (25%): Measures attention distribution quality
- **Output Diversity** (25%): Measures response variety and creativity
- **Top2 Margin** (20%): Measures decision confidence
- **Latency** (15%): Measures response time performance
- **Fanout** (15%): Measures computational complexity

### Guardrail Thresholds
```python
guardrails = {
    'attn_entropy_min': 0.08,    # Minimum attention entropy
    'attn_entropy_max': 0.35,    # Maximum attention entropy
    'diversity_min': 0.45,       # Minimum output diversity
    'diversity_max': 0.85,       # Maximum output diversity
    'top2_margin_min': 0.2,      # Minimum decision margin
    'max_fanout': 16,            # Maximum computational fanout
    'latency_p95_max_ms': 50,    # Maximum 95th percentile latency
    'embedding_coherence_min': 0.55  # Minimum embedding coherence
}
```

### Domain-Specific Weights
Different domains emphasize different aspects of performance:

- **GSM8K** (Math): 60% reasoning, 40% accuracy, 0% diversity
- **PIQA** (Physical): 50% reasoning, 30% accuracy, 20% diversity  
- **CommonsenseQA**: 40% reasoning, 30% accuracy, 30% diversity
- **BoolQ**: 30% reasoning, 50% accuracy, 20% diversity
- **OpenBookQA**: 40% reasoning, 40% accuracy, 20% diversity

## 📁 File Structure

```
huggingface_data/
├── commonsense_qa_processed.json      # Processed dataset
├── commonsense_qa_melvin_input.txt    # Melvin-friendly format
├── commonsense_qa_melvin_input.csv    # Analytics format
├── test_commonsense_qa_with_melvin.sh # Test script
└── melvin_tests/                      # Test results
    ├── conservative_test.log
    ├── balanced_test.log
    └── creative_test.log

analysis/
├── verify_results.py                  # Main analytics script
├── dashboard.py                       # Streamlit dashboard
├── requirements.txt                   # Python dependencies
└── plots/                            # Generated visualizations
    ├── accuracy_vs_attention_entropy.png
    ├── domain_performance_comparison.png
    ├── profile_domain_heatmap.png
    └── correlation_heatmap.png
```

## 🔍 Advanced Usage

### Custom Dataset Processing
```python
# Add new dataset processor
def process_custom_dataset(dataset):
    processed_data = []
    for i, item in enumerate(dataset):
        melvin_item = {
            'id': f"custom_{i:04d}",
            'type': 'custom_reasoning',
            'question': item['question'],
            'answer': item['answer'],
            'melvin_input': f"Custom: {item['question']}\nAnswer: {item['answer']}"
        }
        processed_data.append(melvin_item)
    return processed_data
```

### Custom Analytics
```python
# Extend analytics class
class CustomAnalytics(MelvinAnalytics):
    def custom_metric(self, row):
        # Calculate custom metric
        return custom_score
```

### Regression Monitoring
The dashboard automatically tracks:
- Health score trends over time
- Guardrail violation patterns
- Domain-specific performance drift
- Profile effectiveness across domains

## 🎯 Best Practices

### 1. Dataset Selection
- Use **commonsense_qa** for general reasoning validation
- Use **piqa** for physical commonsense testing
- Use **gsm8k** for mathematical reasoning validation
- Combine datasets for comprehensive evaluation

### 2. Profile Testing
- Test all profiles (Conservative, Balanced, Creative) across domains
- Conservative: Best for accuracy-critical tasks
- Balanced: Best for general-purpose reasoning
- Creative: Best for diversity and exploration

### 3. Monitoring
- Run analytics after each major change
- Monitor guardrail violations closely
- Track domain-specific performance trends
- Use dashboard for visual regression detection

### 4. Performance Optimization
- Start with small sample sizes (50-100) for development
- Scale up to full datasets for final validation
- Use parallel processing for large-scale runs
- Monitor latency and fanout metrics

## 🚨 Troubleshooting

### Common Issues

**Dependencies Missing**
```bash
make install-hf-deps
```

**Dataset Download Fails**
```bash
# Check internet connection
# Try smaller sample size
python3 huggingface_integration.py --dataset commonsense_qa --max-samples 10
```

**Analytics Script Errors**
```bash
# Check CSV file format
# Ensure required columns exist
python3 analysis/verify_results.py --input-dir verification_results --no-plots
```

**Dashboard Won't Start**
```bash
# Install streamlit
pip install streamlit
# Check port availability
streamlit run analysis/dashboard.py --server.port 8502
```

## 📊 Sample Output

### Analytics Report
```
🧠 MELVIN LLM-STYLE ANALYTICS COMPANION
=======================================

📊 ANALYSIS RESULTS
==================
📈 Overall Statistics:
   Total runs: 15
   Average health score: 78.3%
   Best health score: 92.1%
   Worst health score: 45.2%

🎯 Profile Comparison:
              health_score  accuracy  attention_entropy  output_diversity
mode                      
Conservative         82.1      0.847              0.156             0.623
Balanced             76.8      0.823              0.189             0.678
Creative             75.9      0.801              0.234             0.712

⚠️  Guardrail Violations:
   Run conservative_001: attn_entropy=0.045
   Run creative_003: diversity=0.923
   Total violations: 3/15 runs

🏆 Top 5 Runs by Health Score:
   conservative_002 (Conservative): 92.1% - acc=0.891, attn=0.156, div=0.623
   balanced_001 (Balanced): 88.7% - acc=0.867, attn=0.189, div=0.678
   ...
```

### Domain Analysis
```
🎯 DOMAIN-SPECIFIC ANALYSIS
==========================

📊 COMMONSENSE_QA Domain:
   Runs: 5
   Avg Health Score: 81.2%
   Avg Accuracy: 0.834
   Best Profile: Conservative (85.3%)

📊 PIQA Domain:
   Runs: 5
   Avg Health Score: 76.8%
   Avg Accuracy: 0.789
   Best Profile: Balanced (82.1%)

📊 GSM8K Domain:
   Runs: 5
   Avg Health Score: 72.4%
   Avg Accuracy: 0.756
   Best Profile: Conservative (78.9%)
```

## 🔮 Future Enhancements

- **Visual Reasoning**: Add VQA and Winoground datasets
- **Audio Processing**: Integrate audio embedding validation
- **Dialogue Systems**: Add conversational reasoning datasets
- **Multi-modal**: Combine text, image, and audio reasoning
- **Real-time Monitoring**: WebSocket-based live dashboard
- **A/B Testing**: Automated profile comparison framework

---

**Built with ❤️ for Melvin's evolution into a truly capable reasoning system.**
