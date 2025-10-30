# 🔧 Git Large Files Fix

## Problem
Large files (venv_tts, data/*.bin) are in git history, causing push failures.

## ✅ Solution: Create Fresh Branch (Easiest)

Since old commits have large files, create a clean branch:

```bash
# Create orphan branch (no history)
git checkout --orphan clean-main

# Add all current files (already excludes large files via .gitignore)
git add -A

# Commit
git commit -m "Clean repo - Jetson ready production code"

# Push new branch
git push -u origin clean-main

# Make it the default branch on GitHub, then delete old main locally:
git branch -D main
git checkout -b main
git branch --set-upstream-to=origin/clean-main main
```

## Alternative: Keep Current Branch, Use Deploy Script

**For Jetson deployment**, the large files are handled separately via rsync in the deploy script - so you don't need them in git!

- ✅ Code (source files) → Git
- ✅ Large data files → Transfer via rsync in deploy script
- ✅ venv_tts → Recreate on Jetson (not needed in git)

**To update Jetson after code changes:**
```bash
git pull  # On Jetson - gets code only
make clean && make  # Rebuild
sudo systemctl restart melvin
```

The deployment script already transfers data files separately, so this workflow works fine!

---

**Recommendation:** Use the fresh branch approach for clean git history.

