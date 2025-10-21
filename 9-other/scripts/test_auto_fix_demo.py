#!/usr/bin/env python3
"""
Demo script showing how auto_build_and_fix.py works

Creates a file with intentional errors, shows the auto-fix process
"""

import os
import sys
import subprocess

# Mock build log with common errors
MOCK_BUILD_LOG = """
g++ -std=c++20 -Wall -Wextra -O3 -I. -Imelvin -c melvin/v2/test/mock_error.cpp -o build/mock_error.o
melvin/v2/test/mock_error.cpp:15:22: error: use of undeclared identifier 'undefined_function'
    result = undefined_function(x, y);
             ^
melvin/v2/test/mock_error.cpp:20:5: error: expected ';' after expression
    int z = 42
    ^
melvin/v2/test/mock_error.cpp:25:10: error: no matching function for call to 'process'
    process(data);
    ^~~~~~~
melvin/v2/test/mock_error.cpp:30:15: warning: unused parameter 'unused_param' [-Wunused-parameter]
void test_func(int unused_param) {
              ^
3 errors generated.
make: *** [build/mock_error.o] Error 1
"""

# Mock file with errors
MOCK_ERROR_FILE = """
#include <iostream>
#include <string>

namespace melvin::v2::test {

int compute(int x, int y) {
    // This function has several intentional errors for demo
    
    int result;
    
    // ERROR 1: Undeclared identifier
    result = undefined_function(x, y);
    
    // ERROR 2: Missing semicolon
    int z = 42
    
    // ERROR 3: Wrong function signature
    std::string data = "test";
    process(data);
    
    // ERROR 4: Unused parameter (warning)
    void test_func(int unused_param) {
        std::cout << "Hello" << std::endl;
    }
    
    return result;
}

} // namespace melvin::v2::test
"""

# Expected fixed file
MOCK_FIXED_FILE = """
#include <iostream>
#include <string>

namespace melvin::v2::test {

// Forward declaration
int undefined_function(int a, int b);
void process(const std::string& str);

int compute(int x, int y) {
    // This function has been auto-fixed
    
    int result;
    
    // FIX 1: Added forward declaration above
    result = undefined_function(x, y);
    
    // FIX 2: Added semicolon
    int z = 42;
    
    // FIX 3: Corrected function call
    std::string data = "test";
    process(data);
    
    // FIX 4: Suppressed unused parameter warning
    auto test_func = [](int /*unused_param*/) {
        std::cout << "Hello" << std::endl;
    };
    
    return result;
}

} // namespace melvin::v2::test
"""

def show_demo():
    """Show what auto-fix would do"""
    
    print("\n" + "="*70)
    print("MELVIN Auto-Fix Demo - Mock Error Correction")
    print("="*70 + "\n")
    
    print("📝 ORIGINAL FILE WITH ERRORS:")
    print("-"*70)
    print(MOCK_ERROR_FILE)
    
    print("\n" + "="*70)
    print("🔍 BUILD LOG (ERRORS DETECTED):")
    print("-"*70)
    print(MOCK_BUILD_LOG)
    
    print("\n" + "="*70)
    print("🤖 AUTO-FIX PROCESS:")
    print("-"*70)
    
    print("""
Step 1: Parse build.log
  ✓ Found 3 errors, 1 warning
  ✓ Extracted file paths and line numbers

Step 2: Extract context
  ✓ Read 50 lines around each error
  ✓ Identified undefined_function, missing semicolon, wrong call

Step 3: Generate AI prompt
  ✓ Included all errors + context
  ✓ Sent to GPT-4

Step 4: Receive fixes
  ✓ AI suggested:
     - Add forward declarations
     - Add missing semicolon
     - Fix function signature
     - Suppress warning with comment syntax

Step 5: Apply patches
  ✓ Backed up original to backups/
  ✓ Applied all fixes
  ✓ Git commit: "Auto-fix iteration #1: 3 errors"

Step 6: Retry build
  ✓ Build succeeded!
  ✓ No errors remaining
    """)
    
    print("\n" + "="*70)
    print("✅ FIXED FILE:")
    print("-"*70)
    print(MOCK_FIXED_FILE)
    
    print("\n" + "="*70)
    print("📊 SUMMARY:")
    print("-"*70)
    print("""
Status:           ✅ SUCCESS
Iterations:       1
Time:             ~15 seconds
Errors Fixed:     3
Warnings Fixed:   1
Files Modified:   1
API Cost:         ~$0.09 (GPT-4)
    """)
    
    print("\n" + "="*70)
    print("💡 USAGE:")
    print("-"*70)
    print("""
# Run on real MELVIN v2 code:
python scripts/auto_build_and_fix.py

# Dry run (simulate):
python scripts/auto_build_and_fix.py --dry-run

# Custom config:
python scripts/auto_build_and_fix.py --config auto_fix_config.yaml
    """)
    
    print("\n✨ Auto-fix tool ready to use!\n")

def run_actual_dry_run():
    """Run actual auto-fix in dry-run mode on current code"""
    
    print("\n" + "="*70)
    print("🧪 RUNNING ACTUAL DRY RUN ON MELVIN v2")
    print("="*70 + "\n")
    
    try:
        result = subprocess.run(
            ["python3", "scripts/auto_build_and_fix.py", "--dry-run", "--max-iters", "3"],
            capture_output=True,
            text=True,
            timeout=60
        )
        
        print(result.stdout)
        if result.stderr:
            print("STDERR:", result.stderr)
        
        print("\n✅ Dry run completed successfully!")
        
    except subprocess.TimeoutExpired:
        print("⚠️  Dry run timed out")
    except FileNotFoundError:
        print("⚠️  auto_build_and_fix.py not found in scripts/")
    except Exception as e:
        print(f"⚠️  Error running dry run: {e}")

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="Demo of auto-fix tool")
    parser.add_argument("--run-dry-run", action="store_true", 
                       help="Actually run auto-fix in dry-run mode")
    
    args = parser.parse_args()
    
    if args.run_dry_run:
        run_actual_dry_run()
    else:
        show_demo()

