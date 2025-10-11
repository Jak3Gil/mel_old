# 🏆 Milestone: entry1_baseline_demo

**Date:** Sat Oct 11 15:17:49 EDT 2025  
**Notes:** Milestone achieved

## Metrics Achieved

- **Context Similarity:** 0.000
- **Entropy Reduction:** 0.000
- **Leap Success Rate:** 0%
- **Data Size:** test_graph_complete

## Improvement from Baseline

- **Similarity:** +0 (from 0.000)
- **Entropy:** +0 (from 0.000)
- **Success:** +0% (from 0%)

## Configuration

- lambda_graph_bias: 0.5
- leap_entropy_threshold: 0.6
- learning_rate_embeddings: 0.01

## Data Layers Ingested

Data versions:
- 20251011_151653: TinyChat Sample Batch 1
- 20251011_145746: Test versioning

## Reproducibility

**Git Commit:** `99a85bac82545a2b6cf5498c600c77f448682d5f`  
**Diagnostic Report:** See `leap_tuning_report.md` in this directory  
**Full History:** See `diagnostics_history.csv` in this directory

To reproduce:
1. Check out commit: `git checkout <commit_hash>`
2. Load data versions up to this point
3. Run diagnostic: `make run_diagnostic`
4. Compare results to this milestone

## Next Steps

- Continue Layer 1 data (conversations)
- Build toward 0.35 basic associations
