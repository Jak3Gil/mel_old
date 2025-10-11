# Melvin Diagnostic Run 2 - Entry 2

**Date:** 2025-10-18
**Timestamp:** 2025-10-11T19:32:03.453817
**Tag:** `entry2_first_data`

## 📂 Git Context

- **Commit Hash:** `abc12345`
- **Tag:** `entry2_first_data`
- **Reproducible:** `git checkout abc12345`

## 📊 Dataset

- **Version:** `versioned_data/20251018_093000`
- **Layer:** layer1
- **Size:** 12450

## 📈 Metrics

| Metric | Before | After | Delta |
|--------|--------|-------|-------|
| **Context Similarity** | 0.000 | 0.218 | +0.218 |
| **Entropy Reduction** | 0.000 | 0.082 | +0.082 |
| **Leap Success Rate** | 0.0% | 26.0% | +26.0% |

## ⚙️ Parameters

- **lambda_graph_bias:** 0.2
- **leap_entropy_threshold:** 0.4
- **learning_rate_embeddings:** 0.01

## 📝 Notes

Entry 2 - First real data TinyChat 12k

---

*Exported from Melvin Research Database on 2025-10-11 19:32 UTC*

**Reproducibility:**
```bash
git checkout abc12345
# Use data from versioned_data/20251018_093000
make run_diagnostic_quick
# Should reproduce: similarity 0.000 → 0.218
```