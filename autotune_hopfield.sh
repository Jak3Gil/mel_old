#!/usr/bin/env bash
# Auto-tune Hopfield-Diffusion hyperparameters
# [Hopfield-Diffusion Upgrade - Auto-Tuning]

set -euo pipefail

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  🔧 HOPFIELD-DIFFUSION AUTO-TUNING                            ║"
echo "║     Finding optimal β, λ (leak), and steps                   ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Check if demo exists
if [ ! -f "./demo_hopfield_diffusion" ]; then
    echo "Building demo first..."
    make -f Makefile.hopfield all || { echo "Build failed"; exit 1; }
fi

OUTPUT="hopfield_autotune.csv"

echo "Running parameter sweep..."
echo "This will take ~2-3 minutes..."
echo ""

# CSV header
echo "beta,leak,steps,final_energy,convergence,time_ms" > $OUTPUT

total=0
completed=0

# Count total runs
for b in 3 5 7 9; do
  for l in 0.96 0.98 0.99; do
    for s in 20 30 40; do
      total=$((total + 1))
    done
  done
done

echo "Running $total configurations..."
echo ""

# Grid search
for b in 3 5 7 9; do
  for l in 0.96 0.98 0.99; do
    for s in 20 30 40; do
      completed=$((completed + 1))
      echo -n "  [$completed/$total] β=$b, λ=$l, steps=$s ... "
      
      # Time the run
      t0=$(python3 -c "import time; print(int(time.time()*1000))")
      
      # Run demo and capture output
      timeout 30s ./demo_hopfield_diffusion > .autotune_tmp.txt 2>&1 || {
        echo "timeout"
        echo "$b,$l,$s,NaN,0,30000" >> $OUTPUT
        continue
      }
      
      t1=$(python3 -c "import time; print(int(time.time()*1000))")
      ms=$((t1 - t0))
      
      # Extract final energy
      E=$(grep "Final energy:" .autotune_tmp.txt | tail -1 | awk '{print $NF}' || echo "NaN")
      
      # Check if converged
      converged=$(grep -q "Converged: Yes" .autotune_tmp.txt && echo "1" || echo "0")
      
      echo "$b,$l,$s,$E,$converged,$ms" >> $OUTPUT
      echo "E=$E, conv=$converged, ${ms}ms"
    done
  done
done

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "  RESULTS SAVED TO: $OUTPUT"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Analyze results
python3 - "$OUTPUT" <<'PY'
import sys, csv

results = []
with open(sys.argv[1]) as f:
    reader = csv.DictReader(f)
    for row in reader:
        try:
            row['final_energy'] = float(row['final_energy'])
            row['time_ms'] = int(row['time_ms'])
            row['beta'] = float(row['beta'])
            row['leak'] = float(row['leak'])
            row['steps'] = int(row['steps'])
            row['convergence'] = int(row['convergence'])
            results.append(row)
        except:
            pass

if not results:
    print("No valid results found")
    sys.exit(1)

# Find best configurations
print("TOP 5 CONFIGURATIONS (by lowest energy):")
print("")
print("  Rank | β    | λ     | Steps | Energy    | Conv | Time")
print("  ─────┼──────┼───────┼───────┼───────────┼──────┼──────")

sorted_by_energy = sorted([r for r in results if r['final_energy'] == r['final_energy']], 
                           key=lambda x: x['final_energy'])

for i, r in enumerate(sorted_by_energy[:5], 1):
    conv_mark = "✓" if r['convergence'] == 1 else "✗"
    print(f"  {i:4d} | {r['beta']:4.1f} | {r['leak']:5.2f} | {r['steps']:5d} | {r['final_energy']:9.4f} | {conv_mark:^4s} | {r['time_ms']:4d}ms")

print("")
print("RECOMMENDED CONFIG:")
best = sorted_by_energy[0]
print(f"  beta = {best['beta']}")
print(f"  leak = {best['leak']}")
print(f"  steps = {best['steps']}")
print(f"  (Achieves energy = {best['final_energy']:.4f} in {best['time_ms']}ms)")
print("")
PY

ok "Auto-tuning complete!"

echo ""
echo "View full results: cat $OUTPUT"
echo ""

# Cleanup
rm -f .autotune_tmp.txt

exit 0




