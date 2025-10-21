#!/bin/bash
# Monitor Melvin in production

LOG_FILE="${1:-production.log}"

echo "╔═══════════════════════════════════════════════════════╗"
echo "║  MELVIN PRODUCTION MONITOR                            ║"
echo "╚═══════════════════════════════════════════════════════╝"
echo ""
echo "Watching: $LOG_FILE"
echo "Press Ctrl+C to stop"
echo ""

# Create log file if it doesn't exist
touch "$LOG_FILE"

# Monitor log with colors
tail -f "$LOG_FILE" | while read -r line; do
    if [[ $line == *"ERROR"* ]]; then
        echo -e "\033[0;31m$line\033[0m"  # Red
    elif [[ $line == *"WARNING"* ]]; then
        echo -e "\033[0;33m$line\033[0m"  # Yellow
    elif [[ $line == *"✓"* ]] || [[ $line == *"SUCCESS"* ]]; then
        echo -e "\033[0;32m$line\033[0m"  # Green
    else
        echo "$line"
    fi
done

