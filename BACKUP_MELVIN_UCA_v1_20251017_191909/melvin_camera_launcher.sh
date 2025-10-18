#!/bin/bash
# Ultra-simple camera launcher - just run this!

cd "$(dirname "$0")/melvin" 2>/dev/null || cd melvin 2>/dev/null || {
    echo "❌ Cannot find melvin directory"
    exit 1
}

# Auto-detect and run
exec python3 melvin_sees.py

