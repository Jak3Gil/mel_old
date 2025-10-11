#!/bin/bash
# Archive Melvin Research Database
# Creates timestamped snapshot for long-term preservation
#
# Usage: ./archive_database.sh [description]
#
# Example:
#   ./archive_database.sh "Weekly snapshot before Entry 10"
#   ./archive_database.sh  # Uses default description
#
# Archives are stored in archives/ with timestamp and size info
# Each archive includes:
#   - Database file (.db)
#   - Metadata (JSON)
#   - README (Markdown)

set -e

DB_FILE="melvin_research.db"
ARCHIVE_DIR="archives"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
DESCRIPTION="${1:-Weekly database snapshot}"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║          📦 ARCHIVE MELVIN RESEARCH DATABASE                  ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# Check if database exists
if [ ! -f "$DB_FILE" ]; then
    echo "❌ Error: Database not found: $DB_FILE"
    echo "   Initialize with: python3 src/database_manager.py init"
    exit 1
fi

# Create archive directory
mkdir -p "$ARCHIVE_DIR"

# Get database stats
echo "📊 Collecting database statistics..."
DB_SIZE=$(du -h "$DB_FILE" | cut -f1)
DB_LINES=$(python3 src/database_manager.py stats 2>/dev/null | grep "total_runs" | awk '{print $2}' || echo "unknown")

# Archive name
ARCHIVE_NAME="melvin_research_${TIMESTAMP}"
ARCHIVE_PATH="${ARCHIVE_DIR}/${ARCHIVE_NAME}"

# Create archive directory
mkdir -p "$ARCHIVE_PATH"

# Copy database
echo "📦 Archiving database..."
cp "$DB_FILE" "${ARCHIVE_PATH}/melvin_research.db"

# Create metadata JSON
cat > "${ARCHIVE_PATH}/metadata.json" << EOF
{
  "archive_name": "${ARCHIVE_NAME}",
  "timestamp": "${TIMESTAMP}",
  "date": "$(date -u +%Y-%m-%d)",
  "datetime_utc": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "description": "${DESCRIPTION}",
  "database_size": "${DB_SIZE}",
  "total_runs": "${DB_LINES}",
  "git_hash": "$(git rev-parse HEAD 2>/dev/null || echo 'unknown')",
  "git_branch": "$(git branch --show-current 2>/dev/null || echo 'unknown')"
}
EOF

# Create README
cat > "${ARCHIVE_PATH}/README.md" << EOF
# Melvin Research Database Archive

## Archive Information

- **Archive Name:** ${ARCHIVE_NAME}
- **Created:** $(date -u)
- **Description:** ${DESCRIPTION}

## Database Statistics

- **File Size:** ${DB_SIZE}
- **Total Runs:** ${DB_LINES}
- **Git Hash:** $(git rev-parse HEAD 2>/dev/null || echo 'unknown')
- **Git Branch:** $(git branch --show-current 2>/dev/null || echo 'unknown')

## Contents

\`\`\`
${ARCHIVE_NAME}/
├── melvin_research.db    # SQLite database snapshot
├── metadata.json         # Machine-readable metadata
└── README.md            # This file
\`\`\`

## Restoring from Archive

To restore this archive:

\`\`\`bash
# Backup current database (if any)
cp melvin_research.db melvin_research.db.backup

# Restore from archive
cp archives/${ARCHIVE_NAME}/melvin_research.db ./melvin_research.db

# Verify
python3 src/database_manager.py stats
\`\`\`

## Querying Archive

You can query the archive directly without restoring:

\`\`\`bash
# View stats
python3 src/database_manager.py --db archives/${ARCHIVE_NAME}/melvin_research.db stats

# Query runs
python3 src/database_manager.py --db archives/${ARCHIVE_NAME}/melvin_research.db query

# Plot from archive
python3 plot_from_db.py --db archives/${ARCHIVE_NAME}/melvin_research.db --save
\`\`\`

## Portability

This archive is a complete, portable snapshot:
- Single SQLite file (works for decades)
- No dependencies on external systems
- Can be copied to any machine
- Compatible with Python ≥3.9

## Longevity

SQLite format is:
- Stable since 2000
- Backward compatible
- Cross-platform
- Human-readable (with sqlite3 CLI)

Expected lifespan: 50+ years

---

*Archive created by Melvin Research Infrastructure*
*Timestamp: $(date -u +%Y-%m-%dT%H:%M:%SZ)*
EOF

# Compress archive (optional)
if command -v tar &> /dev/null && command -v gzip &> /dev/null; then
    echo "🗜️  Compressing archive..."
    tar -czf "${ARCHIVE_PATH}.tar.gz" -C "$ARCHIVE_DIR" "$ARCHIVE_NAME"
    COMPRESSED_SIZE=$(du -h "${ARCHIVE_PATH}.tar.gz" | cut -f1)
    echo "✅ Compressed archive: ${ARCHIVE_PATH}.tar.gz (${COMPRESSED_SIZE})"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ ARCHIVE CREATED SUCCESSFULLY"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📦 Archive Details:"
echo "   Name:        $ARCHIVE_NAME"
echo "   Location:    $ARCHIVE_PATH"
echo "   Size:        $DB_SIZE"
echo "   Runs:        $DB_LINES"
echo "   Description: $DESCRIPTION"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📚 ALL ARCHIVES:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
ls -lh "$ARCHIVE_DIR" | tail -n +2 | grep -v "total"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "💡 USAGE:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Query this archive:"
echo "  python3 src/database_manager.py --db $ARCHIVE_PATH/melvin_research.db stats"
echo ""
echo "Restore this archive:"
echo "  cp $ARCHIVE_PATH/melvin_research.db ./melvin_research.db"
echo ""
echo "Offsite backup:"
echo "  # Upload to cloud/Git LFS"
echo "  git lfs track 'archives/*.tar.gz'"
echo "  git add archives/${ARCHIVE_NAME}.tar.gz"
echo "  git commit -m 'Archive: ${ARCHIVE_NAME}'"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  Database archived. Safe for decades, portable everywhere.   ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

