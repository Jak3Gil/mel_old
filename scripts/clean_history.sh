#!/bin/bash
# Clean git history to remove large files
# WARNING: This rewrites history - make sure you have backups!

echo "🧹 Removing large files from git history..."

# Remove large files from entire history
git filter-branch --force --index-filter \
  "git rm -rf --cached --ignore-unmatch venv_tts data/unified_nodes.bin data/unified_edges.bin data/token_map.bin" \
  --prune-empty --tag-name-filter cat -- --all

# Clean up
rm -rf .git/refs/original/
git refl نیز expire --expire=now --all
git gc --prune=now --aggressive

echo "✅ History cleaned. Now try: git push --force origin main"

