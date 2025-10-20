#!/bin/bash

# ╔═══════════════════════════════════════════════════════════════════╗
# ║  MELVIN STREAMLINING SCRIPT                                       ║
# ║  Reduces 237 files → 35 essential files (85% reduction)          ║
# ╚═══════════════════════════════════════════════════════════════════╝

set -e  # Exit on error

echo ""
echo "╔═══════════════════════════════════════════════════════════════════╗"
echo "║  🧹 MELVIN STREAMLINING                                           ║"
echo "╚═══════════════════════════════════════════════════════════════════╝"
echo ""
echo "This will archive non-essential files to preserve them while"
echo "reducing clutter. All files will be MOVED (not deleted)."
echo ""
echo "Target structure:"
echo "  ✅ Keep 20 core files (essential cognition)"
echo "  ✅ Keep 15 essential tools"
echo "  📦 Archive ~200 experimental/demo/test files"
echo ""
read -p "Continue? (y/n) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Cancelled."
    exit 1
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 1: Creating archive directories..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

mkdir -p archive/core
mkdir -p archive/tools
mkdir -p archive/demos
mkdir -p archive/tests
mkdir -p archive/v2
mkdir -p archive/scripts
mkdir -p archive/bin
mkdir -p archive/docs

echo "✅ Archive directories created"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 2: Archiving non-essential core files..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Archive experimental/incomplete core components
for file in binary_ingestor generator hybrid_predictor \
            dataset_loader fast_learning autonomous_learner \
            input_manager melvin_focus melvin_graph melvin_output \
            melvin_reasoning melvin_reflect melvin_vision \
            metrics optimized_storage pattern_detector \
            sequence_predictor tokenizer; do
    if [ -f "melvin/core/${file}.h" ] || [ -f "melvin/core/${file}.cpp" ]; then
        mv melvin/core/${file}.* archive/core/ 2>/dev/null || true
        echo "  📦 Archived ${file}"
    fi
done

echo "✅ Core files archived"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 3: Archiving redundant tools..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Archive experimental learning tools
for file in continuous_internet_cpp continuous_scraper_learning \
            direct_scrape_to_brain feed_dictionary_verbose \
            feed_internet_knowledge learn_ultra_verbose \
            start_autonomous_learning ultra_fast_continuous_learning \
            melvin_conversation_real melvin_intelligent_conversation \
            check_uniqueness large_knowledge_base; do
    if [ -f "tools/src/${file}.cpp" ]; then
        mv "tools/src/${file}.cpp" archive/tools/
        echo "  📦 Archived ${file}.cpp"
    fi
done

echo "✅ Tools archived"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 4: Archiving demos..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if [ -d "melvin/demos" ]; then
    mv melvin/demos/* archive/demos/ 2>/dev/null || true
    echo "✅ Demos archived (~35 files)"
else
    echo "  ℹ️  No demos directory found"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 5: Archiving tests..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if [ -d "melvin/tests" ]; then
    mv melvin/tests/* archive/tests/ 2>/dev/null || true
    echo "✅ Tests archived (~11 files)"
else
    echo "  ℹ️  No tests directory found"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 6: Archiving V2 (optional)..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

echo ""
echo "V2 contains evolution system (genome, neuromodulators, global workspace)"
read -p "Archive V2? This is the future evolution system (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    if [ -d "melvin/v2" ]; then
        mv melvin/v2/* archive/v2/ 2>/dev/null || true
        echo "✅ V2 archived (~47 files)"
    fi
else
    echo "  ℹ️  Keeping V2 (recommended)"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 7: Archiving Python scripts..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if [ -d "scripts" ]; then
    find scripts/ -name "*.py" -exec mv {} archive/scripts/ \; 2>/dev/null || true
    echo "✅ Python scripts archived"
fi

if [ -d "melvin/v2/tools" ]; then
    find melvin/v2/tools/ -name "*.py" -exec mv {} archive/scripts/ \; 2>/dev/null || true
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 8: Archiving non-essential binaries..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

cd bin/

# Keep only essentials
KEEP_BINS=(
    "analyze_database"
    "check_real_stats"
    "inspect_kb"
    "diagnose_graph"
    "watch_learning"
    "melvin_integrated"
    "build_knowledge"
)

for bin_file in *; do
    if [[ ! " ${KEEP_BINS[@]} " =~ " ${bin_file} " ]]; then
        mv "$bin_file" ../archive/bin/ 2>/dev/null || true
    fi
done

cd ..
echo "✅ Non-essential binaries archived"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 9: Archiving documentation duplicates..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Keep only essential docs
if [ -d "melvin" ]; then
    for doc in ANTI_STARING_EXPLAINED.md CAMERA_FIX.md \
               RICH_CONTEXT_SOLUTION.md SEE_YOURSELF_NOW.txt \
               VISION_QUICKSTART.txt VISUALIZATION_GUIDE.md; do
        if [ -f "melvin/$doc" ]; then
            mv "melvin/$doc" archive/docs/
        fi
    done
fi

echo "✅ Documentation archived"

echo ""
echo "╔═══════════════════════════════════════════════════════════════════╗"
echo "║  ✅ STREAMLINING COMPLETE!                                        ║"
echo "╚═══════════════════════════════════════════════════════════════════╝"
echo ""
echo "📊 RESULTS:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Count files
CORE_FILES=$(find melvin/core -type f \( -name "*.h" -o -name "*.cpp" \) 2>/dev/null | wc -l)
TOOL_FILES=$(find tools/src -type f -name "*.cpp" 2>/dev/null | wc -l)
ARCHIVE_FILES=$(find archive -type f 2>/dev/null | wc -l)

echo "  Active Core Files:   $CORE_FILES"
echo "  Active Tool Files:   $TOOL_FILES"
echo "  Total Active:        $(($CORE_FILES + $TOOL_FILES))"
echo "  Archived Files:      $ARCHIVE_FILES"
echo ""
echo "  All archived files preserved in: ./archive/"
echo ""
echo "🎯 Next Steps:"
echo "  1. Test core functionality: ./bin/analyze_database"
echo "  2. Rebuild binaries: cd melvin && make clean && make"
echo "  3. If you need archived files: cp archive/{file} back"
echo ""
echo "✨ You now have a clean, focused Melvin codebase!"
echo ""

