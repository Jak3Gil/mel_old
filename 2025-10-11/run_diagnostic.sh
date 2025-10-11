#!/bin/bash

# Quick diagnostic and tuning script for Melvin's graph-guided predictive system
# Usage: ./run_diagnostic.sh [basic|quick|tune]

set -e

MODE="${1:-basic}"

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  MELVIN GRAPH-GUIDED PREDICTIVE SYSTEM DIAGNOSTIC            ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Build if needed
if [ ! -f diagnostic_main ]; then
    echo "🔨 Building diagnostic tool..."
    make diagnostic
    echo "✅ Build complete"
    echo ""
fi

case "$MODE" in
    basic)
        echo "🔬 Running basic diagnostic (10 tests)..."
        ./diagnostic_main
        ;;
    quick)
        echo "⚡ Running quick diagnostic (5 tests)..."
        ./diagnostic_main --quick
        ;;
    tune)
        echo "⚙️  Running comprehensive auto-tuning..."
        echo "⏱️  This may take 5-10 minutes..."
        echo ""
        ./diagnostic_main --auto-tune --full-report
        ;;
    *)
        echo "❌ Unknown mode: $MODE"
        echo ""
        echo "Usage: $0 [basic|quick|tune]"
        echo ""
        echo "Modes:"
        echo "  basic  - Run 10 diagnostic tests (default)"
        echo "  quick  - Run 5 quick diagnostic tests"
        echo "  tune   - Run full auto-tuning procedure"
        exit 1
        ;;
esac

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Show output files
echo "📁 Output files:"
if [ -f leap_diagnostics.csv ]; then
    echo "   ✓ leap_diagnostics.csv"
fi
if [ -f leap_tuning_report.md ]; then
    echo "   ✓ leap_tuning_report.md"
fi
if [ -f leap_tuning_results.txt ]; then
    echo "   ✓ leap_tuning_results.txt"
fi

echo ""
echo "📖 Next steps:"
if [ "$MODE" = "tune" ]; then
    echo "   1. Review leap_tuning_results.txt"
    echo "   2. Apply recommended parameter updates"
    echo "   3. Re-run basic diagnostic to verify"
else
    echo "   1. Review leap_tuning_report.md"
    if [ -f leap_tuning_report.md ]; then
        echo "   2. If system needs tuning, run: $0 tune"
    fi
fi

echo ""
echo "✅ Diagnostic complete!"
echo ""

