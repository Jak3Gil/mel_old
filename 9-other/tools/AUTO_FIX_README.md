# MELVIN Autonomous Build & Repair Tool

## Overview

The **Auto Build & Fix** tool automatically detects compilation errors, uses AI to generate fixes, applies patches, and retries until all tests pass. This dramatically accelerates development by handling routine build errors automatically.

### Features

- 🤖 **AI-Powered Fix Generation** - Uses GPT-4 to analyze and fix errors
- 🔄 **Automatic Retry Loop** - Keeps trying until success or max iterations
- 📦 **Safe Backups** - Backs up all files before modification
- 📝 **Git Integration** - Commits each fix attempt automatically
- 🛡️ **Runaway Detection** - Stops if same errors repeat
- 📊 **Detailed Telemetry** - Logs all prompts, responses, and attempts
- 🎨 **Pretty Output** - Uses `rich` library for beautiful CLI output

---

## Quick Start

### 1. Install Dependencies

```bash
pip install openai pyyaml rich
```

### 2. Set API Key

```bash
export OPENAI_API_KEY="your_openai_api_key_here"
```

### 3. Run Auto-Fix

```bash
# Use default config
python scripts/auto_build_and_fix.py

# Use custom config
python scripts/auto_build_and_fix.py --config auto_fix_config.yaml

# Dry run (simulate without changes)
python scripts/auto_build_and_fix.py --dry-run
```

---

## How It Works

### The Auto-Fix Loop

```
1. Run build command (e.g., "make v2")
   ↓
2. Parse compiler errors from build.log
   ↓
3. Extract source context (±50 lines around errors)
   ↓
4. Generate AI prompt with errors + context
   ↓
5. Call OpenAI API to get fixes
   ↓
6. Parse code blocks from AI response
   ↓
7. Apply fixes to source files
   ↓
8. Commit to git (optional)
   ↓
9. Go to step 1 (retry build)
   ↓
10. STOP when build succeeds or max iterations reached
```

### Error Parsing

The tool detects common compiler error formats:

```
file.cpp:123:45: error: use of undeclared identifier 'foo'
file.h:67: error: expected ';' after class definition
```

Extracts:
- File path
- Line number  
- Column (if available)
- Error type (error vs warning)
- Error message

### AI Fix Generation

**Prompt sent to GPT-4:**

```
You are an expert C++ systems engineer fixing build errors.

Below are compilation errors from MELVIN v2 codebase...

Error #1:
File: melvin/v2/core/types_v2.h
Line: 40
Message: use of undeclared identifier 'get_timestamp_ns'

Context:
    35|     Thought(uint64_t id_, const std::string& type_, float sal)
    36|         : id(id_), type(type_), salience(sal), 
    37|             created_at(get_timestamp_ns()), confidence(1.0f) {}
                              ^^^^^^^^^^^^^^^
Provide fixes now:
```

**AI Response:**

```cpp
```cpp:melvin/v2/core/types_v2.h
// Move get_timestamp_ns() before Thought constructor

inline Timestamp get_timestamp_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

struct Thought {
    // ... rest of struct
};
```
```

### Patch Application

The tool:
1. Extracts code blocks from AI response
2. Identifies target files
3. Backs up originals
4. Applies fixes
5. Validates changes

---

## Configuration

### config.yaml

```yaml
# Build & test commands
build_command: "make v2"
test_command: "make v2-test"

# Iteration limits
max_iters: 20

# AI model settings
model: "gpt-4"
temperature: 0.3
max_tokens: 4096

# Context extraction
context_lines: 50

# Safety
enable_git_commits: true
backup_before_fix: true
detect_runaway: true

# Dry run (simulate)
dry_run: false
```

### Command-Line Overrides

```bash
# Use different model
python scripts/auto_build_and_fix.py --model gpt-3.5-turbo

# More iterations
python scripts/auto_build_and_fix.py --max-iters 50

# Dry run
python scripts/auto_build_and_fix.py --dry-run
```

---

## Examples

### Example 1: Fixing Undeclared Identifier

**Build Error:**
```
melvin/v2/core/types_v2.h:40:22: error: use of undeclared identifier 'get_timestamp_ns'
          created_at(get_timestamp_ns()), confidence(1.0f) {}
                     ^
```

**Auto-Fix Actions:**
1. Detected error in `types_v2.h` line 40
2. Extracted 50 lines of context
3. AI suggested moving function declaration
4. Applied fix
5. Rebuild succeeded ✅

**Result:** Fixed in 1 iteration (~10 seconds)

### Example 2: Missing Include

**Build Error:**
```
melvin/v2/core/global_workspace.cpp:5:22: error: no member named 'lock_guard' in namespace 'std'
```

**Auto-Fix Actions:**
1. Detected missing `<mutex>` include
2. AI added `#include <mutex>`
3. Applied fix
4. Rebuild succeeded ✅

**Result:** Fixed in 1 iteration

### Example 3: Type Mismatch

**Build Error:**
```
genome.cpp:150:21: error: no viable conversion from 'int' to 'float'
```

**Auto-Fix Actions:**
1. AI changed `int` to `float` cast
2. Applied fix
3. Rebuild succeeded ✅

**Result:** Fixed in 1 iteration

---

## Output Example

```
╔═══════════════════════════════════════════════════════╗
║  🤖 MELVIN Autonomous Build & Repair Tool            ║
║  Model: gpt-4                                         ║
║  Max Iterations: 20                                   ║
║  Build: make v2                                       ║
╚═══════════════════════════════════════════════════════╝

============================================================
Iteration 1/20
============================================================

[⚙️  Building] Running: make v2
❌ Build failed (exit code 2)

Found 2 errors to fix:
  • melvin/v2/core/types_v2.h:40: error: use of undeclared identifier 'get_timestamp_ns'
  • melvin/v2/core/types_v2.h:61: error: use of undeclared identifier 'get_timestamp_ns'

📦 Backed up 1 files to backups/autofix_20251019_143022_iter1
🤖 Calling OpenAI (gpt-4)...
✅ Received 1234 chars of fixes
📝 Found 1 code blocks to apply
✏️  Replaced melvin/v2/core/types_v2.h
📝 Git commit: Auto-fix iteration #1: 2 errors

============================================================
Iteration 2/20
============================================================

[⚙️  Building] Running: make v2
✅ Build succeeded!
🎉 Build succeeded! Checking tests...
[🧪 Testing] Running: make v2-test
✅ All tests passed!
🎉 All tests passed!

============================================================
FINAL SUMMARY
============================================================

┌─────────────────────┬──────────────┐
│ Metric              │ Value        │
├─────────────────────┼──────────────┤
│ Status              │ ✅ SUCCESS   │
│ Total Iterations    │ 1            │
│ Total Time          │ 15.3s        │
│ Errors Fixed        │ 2            │
│ Files Modified      │ 1            │
└─────────────────────┴──────────────┘

Iteration History:
  ✅ Iter 1: 2 errors, 1 files modified
```

---

## Safety Features

### 1. Automatic Backups

Before any modification:
```
backups/autofix_20251019_143022_iter1/
└── melvin/v2/core/types_v2.h  (original copy)
```

### 2. Git Commits

Each iteration commits:
```bash
git log --oneline
abc1234 Auto-fix iteration #3: 1 errors
def5678 Auto-fix iteration #2: 2 errors
```

Rollback if needed:
```bash
git reset --hard HEAD~3  # Undo last 3 auto-fixes
```

### 3. Runaway Detection

If same errors appear twice:
```
🛑 Runaway detected (same errors repeating)
```

Tool stops to prevent infinite loops.

### 4. Dry Run Mode

Test without making changes:
```bash
python scripts/auto_build_and_fix.py --dry-run
```

Shows what would happen without actually modifying files.

---

## Telemetry & Logs

All activity logged to `logs/auto_fix/`:

```
logs/auto_fix/
├── iteration_1_prompt.txt     # Prompt sent to AI
├── iteration_1_response.txt   # AI's response
├── iteration_2_prompt.txt
├── iteration_2_response.txt
└── ...
```

Review to understand:
- What errors occurred
- What fixes were tried
- Why certain fixes failed

---

## Advanced Usage

### Custom Build Command

```yaml
# For specific component
build_command: "cd melvin && make"
test_command: "./melvin --self-test"
```

### Multiple Configurations

```bash
# Quick fixes (cheap model)
python scripts/auto_build_and_fix.py --model gpt-3.5-turbo --max-iters 5

# Deep fixes (powerful model)
python scripts/auto_build_and_fix.py --model gpt-4-turbo --max-iters 50
```

### Integration with CI/CD

```yaml
# .github/workflows/auto-fix.yml
- name: Auto-fix build errors
  run: |
    export OPENAI_API_KEY=${{ secrets.OPENAI_API_KEY }}
    python scripts/auto_build_and_fix.py --max-iters 10
```

---

## Limitations

### What It CAN Fix

✅ Syntax errors (missing semicolons, brackets)  
✅ Type mismatches (int vs float)  
✅ Missing includes  
✅ Undeclared identifiers  
✅ Template errors  
✅ Const correctness  
✅ Namespace issues  

### What It CAN'T Fix (Reliably)

❌ Logic errors (wrong algorithm)  
❌ Design flaws (architectural issues)  
❌ Complex refactoring  
❌ Performance optimization  
❌ Concurrency bugs  
❌ Memory leaks  

**Best for:** Routine compilation errors, especially after refactoring.

---

## Troubleshooting

### "OpenAI library not available"

```bash
pip install openai
```

### "OPENAI_API_KEY not set"

```bash
export OPENAI_API_KEY="sk-..."
```

Or add to your shell profile:
```bash
echo 'export OPENAI_API_KEY="sk-..."' >> ~/.zshrc
source ~/.zshrc
```

### "Build timeout"

Increase timeout in script (line ~200):
```python
timeout=600  # 10 minutes instead of 5
```

### "Same errors repeating"

AI might not understand the error. Check logs:
```bash
cat logs/auto_fix/iteration_1_prompt.txt
cat logs/auto_fix/iteration_1_response.txt
```

Manually fix the error or adjust prompt in script.

### "Too many files modified"

Tool might be overzealous. Use `--dry-run` first:
```bash
python scripts/auto_build_and_fix.py --dry-run
```

Review what would change before applying.

---

## Cost Estimation

### OpenAI API Costs

Typical fix iteration:
- Prompt: ~2,000 tokens (errors + context)
- Response: ~1,000 tokens (fixes)
- Total: ~3,000 tokens per iteration

**GPT-4 Pricing** (as of 2024):
- Input: $0.03 / 1K tokens
- Output: $0.06 / 1K tokens
- Cost per iteration: ~$0.09

**For 10 iterations: ~$0.90**

**GPT-3.5-Turbo** (cheaper):
- Cost per iteration: ~$0.003
- For 10 iterations: ~$0.03

---

## Best Practices

### 1. Start with Dry Run

```bash
python scripts/auto_build_and_fix.py --dry-run
```

Understand what will happen before applying changes.

### 2. Use Version Control

Always commit your work before running auto-fix:
```bash
git add -A
git commit -m "Before auto-fix"
python scripts/auto_build_and_fix.py
```

### 3. Review Generated Fixes

Check git diffs after auto-fix:
```bash
git diff HEAD~3  # Review last 3 auto-commits
```

### 4. Start Small

Fix one component at a time:
```yaml
build_command: "g++ -c melvin/v2/core/genome.cpp -o genome.o"
```

### 5. Monitor Costs

Track API usage in OpenAI dashboard.

---

## Example Session

### Scenario: After major refactoring, 5 build errors

```bash
$ python scripts/auto_build_and_fix.py

╔═══════════════════════════════════════════════════════╗
║  🤖 MELVIN Autonomous Build & Repair Tool            ║
╚═══════════════════════════════════════════════════════╝

Iteration 1/20
──────────────────────────────────────────────────────

[⚙️  Building] make v2
❌ Build failed

Found 5 errors:
  • types_v2.h:40: undeclared identifier 'get_timestamp_ns'
  • types_v2.h:61: undeclared identifier 'get_timestamp_ns'
  • genome.cpp:309: unused parameter 'path'
  • genome.cpp:315: unused parameter 'json_str'
  • global_workspace.cpp:299: unused parameter 'goal_desc'

📦 Backed up 3 files
🤖 Calling OpenAI (gpt-4)...
✅ Received 2,847 chars of fixes
✏️  Applied fixes to 3 files
📝 Git commit: Auto-fix iteration #1: 5 errors

Iteration 2/20
──────────────────────────────────────────────────────

[⚙️  Building] make v2
✅ Build succeeded!
🎉 All tests passed!

╔═══════════════════════════════════════════════════════╗
║                   FINAL SUMMARY                       ║
╠═══════════════════════════════════════════════════════╣
║  Status:            ✅ SUCCESS                         ║
║  Total Iterations:  1                                 ║
║  Total Time:        18.7s                             ║
║  Errors Fixed:      5                                 ║
║  Files Modified:    3                                 ║
║  API Cost:          ~$0.09                            ║
╚═══════════════════════════════════════════════════════╝

✅ Auto-fix complete! Build is clean.
```

---

## Configuration Reference

### Full config.yaml Options

```yaml
# Commands
build_command: "make v2"          # Build command to run
test_command: "make v2-test"      # Test command (optional)

# Limits
max_iters: 20                     # Maximum fix attempts

# AI Model
model: "gpt-4"                    # OpenAI model
                                  # Options: gpt-4, gpt-4-turbo, gpt-3.5-turbo
temperature: 0.3                  # Randomness (0.0-1.0, lower=deterministic)
max_tokens: 4096                  # Max response length

# Context
context_lines: 50                 # Lines around error to show AI

# Safety
enable_git_commits: true          # Auto-commit each fix
backup_before_fix: true           # Backup files before patch
detect_runaway: true              # Stop if errors repeat

# Dry run
dry_run: false                    # Simulate without changes
```

---

## Integration with MELVIN

### Use in Development

```bash
# After making changes
git add -A
git commit -m "WIP: New feature"

# Auto-fix any build errors
python scripts/auto_build_and_fix.py

# Review changes
git diff HEAD~1
```

### Use in CI/CD

```yaml
# .github/workflows/ci.yml
- name: Build with auto-fix
  env:
    OPENAI_API_KEY: ${{ secrets.OPENAI_API_KEY }}
  run: |
    python scripts/auto_build_and_fix.py --max-iters 5
```

### Use for Code Review

Before submitting PR:
```bash
# Ensure clean build
python scripts/auto_build_and_fix.py
git push
```

---

## Advanced Features

### Custom Error Patterns

Edit `parse_build_log()` to handle project-specific error formats.

### Alternative AI Models

Use Claude, Gemini, or local models:
```python
# Edit script to use different API
# Or use OpenAI-compatible endpoints
```

### Patch Strategy

Modify `apply_fix_to_file()` for smarter patching:
- Fuzzy matching
- Multi-line context matching
- Incremental fixes

---

## Maintenance

### Update Dependencies

```bash
pip install --upgrade openai pyyaml rich
```

### Monitor Logs

```bash
# View recent attempts
ls -lt logs/auto_fix/

# Review specific iteration
cat logs/auto_fix/iteration_3_prompt.txt
cat logs/auto_fix/iteration_3_response.txt
```

### Clean Old Backups

```bash
# Remove backups older than 7 days
find backups/autofix_* -mtime +7 -delete
```

---

## FAQ

**Q: Is this safe?**  
A: Yes, with backups + git commits. You can always rollback.

**Q: Will it break my code?**  
A: Unlikely. It only applies AI-suggested patches to files with errors. Always review changes.

**Q: How much does it cost?**  
A: ~$0.09 per iteration with GPT-4, ~$0.003 with GPT-3.5-turbo.

**Q: Can it fix logic errors?**  
A: No, only compilation errors. Logic requires human understanding.

**Q: Does it work with CMake?**  
A: Yes, set `build_command: "cmake --build build"` in config.

**Q: Can I use it without OpenAI?**  
A: Not currently, but you could modify to use local LLMs or other APIs.

---

## License & Credits

Part of the MELVIN project. Uses OpenAI API for fix generation.

**Author:** MELVIN Dev Team  
**Version:** 1.0  
**Status:** Production-ready ✅

---

## Summary

The **Auto Build & Repair Tool** is a powerful development accelerator that:

✅ Automatically fixes routine build errors  
✅ Saves hours of debugging time  
✅ Maintains safety with backups and git commits  
✅ Provides full transparency via logs  
✅ Integrates seamlessly with MELVIN workflow  

**Use it to keep your build clean while focusing on architecture and algorithms!** 🚀

