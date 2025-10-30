#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT_DIR/bin/test_reasoning_rigorous"
CSV="$ROOT_DIR/logs/rigorous_report/results_master.csv"

mkdir -p "$ROOT_DIR/logs/rigorous_report"

SEEDS=(42 43 44 45 46)

echo "Running rigorous suite → $CSV"

for SEED in "${SEEDS[@]}"; do
  echo "Seed $SEED: full"
  "$BIN" --seed=$SEED --csv="$CSV" >/dev/null

  echo "Seed $SEED: ablate no-goal"
  "$BIN" --seed=$SEED --no-goal --csv="$CSV" >/dev/null

  echo "Seed $SEED: ablate no-wm"
  "$BIN" --seed=$SEED --no-wm --csv="$CSV" >/dev/null

  echo "Seed $SEED: ablate no-causal"
  "$BIN" --seed=$SEED --no-causal --csv="$CSV" >/dev/null
done

echo "Done. Results at $CSV"


