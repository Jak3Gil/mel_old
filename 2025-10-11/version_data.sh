#!/bin/bash
# Data Versioning Helper
# Keep each ingestion under a dated subfolder for correlation with metric jumps
# Usage: ./version_data.sh <data_file> <description>

set -e

DATA_FILE="$1"
DESCRIPTION="${2:-Data ingestion}"

if [ -z "$DATA_FILE" ]; then
    echo "Usage: $0 <data_file> [description]"
    echo ""
    echo "Example:"
    echo "  $0 tinychat_10k.txt 'TinyChat conversations batch 1'"
    echo ""
    echo "This will:"
    echo "  • Create versioned_data/YYYYMMDD_HHMMSS/"
    echo "  • Copy data file there"
    echo "  • Create metadata.json"
    echo "  • Log ingestion details"
    exit 1
fi

if [ ! -f "$DATA_FILE" ]; then
    echo "❌ Error: File not found: $DATA_FILE"
    exit 1
fi

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║              📦 DATA VERSIONING SYSTEM                        ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Create versioned data directory structure
BASE_DIR="versioned_data"
mkdir -p "$BASE_DIR"

# Create dated subdirectory
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
VERSION_DIR="$BASE_DIR/$TIMESTAMP"
mkdir -p "$VERSION_DIR"

echo "📅 Version: $TIMESTAMP"
echo "📁 Directory: $VERSION_DIR"
echo ""

# Get file info
FILE_SIZE=$(wc -c < "$DATA_FILE" | tr -d ' ')
LINE_COUNT=$(wc -l < "$DATA_FILE" | tr -d ' ')
FILE_HASH=$(md5sum "$DATA_FILE" 2>/dev/null | cut -d' ' -f1 || md5 "$DATA_FILE" 2>/dev/null | cut -d'=' -f2 | tr -d ' ')

echo "📊 File Statistics:"
echo "   Filename:   $(basename "$DATA_FILE")"
echo "   Size:       $FILE_SIZE bytes"
echo "   Lines:      $LINE_COUNT"
echo "   MD5:        $FILE_HASH"
echo ""

# Copy data file
cp "$DATA_FILE" "$VERSION_DIR/data.txt"
echo "✅ Data file copied"

# Get current diagnostic state (before ingestion)
BEFORE_SIM="0.000"
BEFORE_ENTROPY="0.000"
BEFORE_SUCCESS="0.0"

if [ -f diagnostics_history.csv ]; then
    BEFORE_SIM=$(tail -1 diagnostics_history.csv | cut -d',' -f5)
    BEFORE_ENTROPY=$(tail -1 diagnostics_history.csv | cut -d',' -f4)
    BEFORE_SUCCESS=$(tail -1 diagnostics_history.csv | cut -d',' -f6)
fi

# Create metadata file
cat > "$VERSION_DIR/metadata.json" << EOF
{
  "timestamp": "$TIMESTAMP",
  "description": "$DESCRIPTION",
  "data_file": "$(basename "$DATA_FILE")",
  "file_size_bytes": $FILE_SIZE,
  "line_count": $LINE_COUNT,
  "md5_hash": "$FILE_HASH",
  "metrics_before_ingestion": {
    "context_similarity": $BEFORE_SIM,
    "entropy_reduction": $BEFORE_ENTROPY,
    "leap_success_rate": $BEFORE_SUCCESS
  },
  "date": "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
}
EOF

echo "✅ Metadata created: metadata.json"

# Create README for this version
cat > "$VERSION_DIR/README.md" << EOF
# Data Version: $TIMESTAMP

## Description
$DESCRIPTION

## File Information
- **Original File:** $(basename "$DATA_FILE")
- **Size:** $FILE_SIZE bytes ($LINE_COUNT lines)
- **MD5 Hash:** $FILE_HASH
- **Ingested:** $(date)

## Metrics Before Ingestion
- Context Similarity: $BEFORE_SIM
- Entropy Reduction: $BEFORE_ENTROPY
- Leap Success Rate: $(echo "$BEFORE_SUCCESS * 100" | bc -l 2>/dev/null || echo "0")%

## Metrics After Ingestion
Run diagnostic after ingestion and update this section:
\`\`\`bash
make run_diagnostic_quick
./log_diagnostic_results.sh "v$TIMESTAMP" "$LINE_COUNT" "$DESCRIPTION"
\`\`\`

Then record:
- Context Similarity: [To be filled]
- Entropy Reduction: [To be filled]
- Leap Success Rate: [To be filled]
- Delta: [To be calculated]

## Expected Impact
Based on data type and volume, expected similarity change:
- Small dialog batch (<5k): +0.02 - 0.05
- Large dialog batch (10k+): +0.05 - 0.10
- Factual knowledge: +0.03 - 0.08
- Reasoning chains: +0.05 - 0.12

## Notes
Add observations here after reviewing diagnostic results.
EOF

echo "✅ README created"
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📝 VERSION CREATED:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "   Directory: $VERSION_DIR"
echo "   Contents:"
echo "     • data.txt (copy of original)"
echo "     • metadata.json (structured info)"
echo "     • README.md (human-readable)"
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🎯 NEXT STEPS:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "1. Ingest the data:"
echo "   $ ./ingest_dialog_data.sh $VERSION_DIR/data.txt 5000"
echo "   OR"
echo "   $ ./melvin_ingest_text $VERSION_DIR/data.txt"
echo ""
echo "2. Run diagnostic:"
echo "   $ make run_diagnostic_quick"
echo ""
echo "3. Log results:"
echo "   $ ./log_diagnostic_results.sh 'v$TIMESTAMP' '$LINE_COUNT' '$DESCRIPTION'"
echo ""
echo "4. Update version README with results:"
echo "   $ vim $VERSION_DIR/README.md"
echo ""
echo "5. Archive with Git:"
echo "   $ git add $VERSION_DIR/"
echo "   $ git commit -m 'Data version $TIMESTAMP: $DESCRIPTION'"
echo ""

# Show all versions
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📚 ALL DATA VERSIONS:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if [ -d "$BASE_DIR" ]; then
    ls -lt "$BASE_DIR" | tail -n +2 | head -10 || echo "   (This is the first version)"
else
    echo "   (This is the first version)"
fi

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  Data versioned. Correlation with metrics now possible.      ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

