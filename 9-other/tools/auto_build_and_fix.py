#!/usr/bin/env python3
"""
MELVIN Autonomous Build & Repair Tool

Automatically detects build errors, uses AI to generate fixes, applies patches,
and retries until all tests pass or max iterations reached.

Usage:
    python scripts/auto_build_and_fix.py
    python scripts/auto_build_and_fix.py --config auto_fix_config.yaml
    python scripts/auto_build_and_fix.py --dry-run
"""

import argparse
import subprocess
import os
import sys
import re
import time
import yaml
import json
import shutil
from pathlib import Path
from datetime import datetime
from typing import List, Dict, Optional, Tuple
from dataclasses import dataclass
from difflib import unified_diff

# Try to import optional dependencies
try:
    from rich.console import Console
    from rich.progress import Progress, SpinnerColumn, TextColumn
    from rich.table import Table
    from rich.panel import Panel
    RICH_AVAILABLE = True
except ImportError:
    RICH_AVAILABLE = False
    print("⚠️  'rich' not available. Install with: pip install rich")

try:
    import openai
    OPENAI_AVAILABLE = True
except ImportError:
    OPENAI_AVAILABLE = False
    print("⚠️  'openai' not available. Install with: pip install openai")

# ============================================================================
# DATA STRUCTURES
# ============================================================================

@dataclass
class BuildError:
    """Represents a single build error"""
    file_path: str
    line_number: int
    column: Optional[int]
    error_type: str  # "error" or "warning"
    message: str
    full_line: str
    
    def __str__(self):
        return f"{self.file_path}:{self.line_number}: {self.error_type}: {self.message}"

@dataclass
class FixAttempt:
    """Represents one fix attempt"""
    iteration: int
    errors: List[BuildError]
    files_modified: List[str]
    fix_applied: bool
    build_success: bool
    timestamp: str
    
@dataclass
class Config:
    """Configuration for auto-fix tool"""
    build_command: str = "make v2"
    test_command: str = "make v2-test"
    max_iters: int = 20
    model: str = "gpt-4"
    temperature: float = 0.3
    max_tokens: int = 4096
    context_lines: int = 50
    enable_git_commits: bool = True
    backup_before_fix: bool = True
    detect_runaway: bool = True
    dry_run: bool = False

# ============================================================================
# CONSOLE HELPER
# ============================================================================

class ConsoleHelper:
    """Wrapper for pretty console output"""
    
    def __init__(self):
        self.console = Console() if RICH_AVAILABLE else None
    
    def print(self, text: str, style: str = ""):
        if self.console:
            self.console.print(text, style=style)
        else:
            print(text)
    
    def print_panel(self, text: str, title: str = "", style: str = ""):
        if self.console:
            self.console.print(Panel(text, title=title, style=style))
        else:
            print(f"\n{'='*60}\n{title}\n{'='*60}\n{text}\n")
    
    def print_table(self, title: str, headers: List[str], rows: List[List[str]]):
        if self.console:
            table = Table(title=title)
            for header in headers:
                table.add_column(header)
            for row in rows:
                table.add_row(*row)
            self.console.print(table)
        else:
            print(f"\n{title}")
            print("  ".join(headers))
            for row in rows:
                print("  ".join(str(x) for x in row))

console = ConsoleHelper()

# ============================================================================
# ERROR PARSING
# ============================================================================

def parse_build_log(log_path: str) -> List[BuildError]:
    """Parse build log and extract errors"""
    
    errors = []
    
    if not os.path.exists(log_path):
        return errors
    
    with open(log_path, 'r') as f:
        lines = f.readlines()
    
    # Common compiler error patterns
    patterns = [
        # GCC/Clang: file.cpp:123:45: error: message
        r'^(.+?):(\d+):(\d+):\s*(error|warning):\s*(.+)$',
        # Alternative: file.cpp:123: error: message
        r'^(.+?):(\d+):\s*(error|warning):\s*(.+)$',
    ]
    
    for line in lines:
        line = line.strip()
        
        for pattern in patterns:
            match = re.match(pattern, line)
            if match:
                groups = match.groups()
                
                if len(groups) == 5:
                    file_path, line_num, col, error_type, message = groups
                elif len(groups) == 4:
                    file_path, line_num, error_type, message = groups
                    col = None
                else:
                    continue
                
                error = BuildError(
                    file_path=file_path,
                    line_number=int(line_num),
                    column=int(col) if col else None,
                    error_type=error_type,
                    message=message,
                    full_line=line
                )
                errors.append(error)
    
    return errors

def get_file_context(file_path: str, line_number: int, context_lines: int = 50) -> str:
    """Get source context around error line"""
    
    if not os.path.exists(file_path):
        return f"// File not found: {file_path}"
    
    with open(file_path, 'r') as f:
        lines = f.readlines()
    
    start = max(0, line_number - context_lines)
    end = min(len(lines), line_number + context_lines)
    
    context = []
    for i in range(start, end):
        marker = " >>> " if i == line_number - 1 else "     "
        context.append(f"{marker}{i+1:4d}| {lines[i].rstrip()}")
    
    return "\n".join(context)

# ============================================================================
# BUILD & TEST
# ============================================================================

def run_build(config: Config) -> Tuple[bool, str]:
    """Run build command and return (success, log_path)"""
    
    log_path = "build.log"
    
    console.print(f"[⚙️  Building] Running: {config.build_command}", style="cyan")
    
    try:
        result = subprocess.run(
            config.build_command,
            shell=True,
            capture_output=True,
            text=True,
            timeout=300  # 5 minute timeout
        )
        
        # Write log
        with open(log_path, 'w') as f:
            f.write(result.stdout)
            f.write(result.stderr)
        
        success = result.returncode == 0
        
        if success:
            console.print("✅ Build succeeded!", style="green bold")
        else:
            console.print(f"❌ Build failed (exit code {result.returncode})", style="red bold")
        
        return success, log_path
        
    except subprocess.TimeoutExpired:
        console.print("❌ Build timeout (>5 minutes)", style="red bold")
        return False, log_path
    except Exception as e:
        console.print(f"❌ Build error: {e}", style="red bold")
        return False, log_path

def run_tests(config: Config) -> Tuple[bool, str]:
    """Run test command and return (success, log_path)"""
    
    if not config.test_command:
        return True, ""  # No tests configured
    
    log_path = "test.log"
    
    console.print(f"[🧪 Testing] Running: {config.test_command}", style="cyan")
    
    try:
        result = subprocess.run(
            config.test_command,
            shell=True,
            capture_output=True,
            text=True,
            timeout=300
        )
        
        with open(log_path, 'w') as f:
            f.write(result.stdout)
            f.write(result.stderr)
        
        success = result.returncode == 0
        
        if success:
            console.print("✅ All tests passed!", style="green bold")
        else:
            console.print(f"❌ Tests failed", style="red bold")
        
        return success, log_path
        
    except Exception as e:
        console.print(f"❌ Test error: {e}", style="red bold")
        return False, log_path

# ============================================================================
# BACKUP & GIT
# ============================================================================

def create_backup(files: List[str], iteration: int):
    """Create backup of files before modification"""
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_dir = f"backups/autofix_{timestamp}_iter{iteration}"
    os.makedirs(backup_dir, exist_ok=True)
    
    for file_path in files:
        if os.path.exists(file_path):
            dest = os.path.join(backup_dir, file_path)
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            shutil.copy2(file_path, dest)
    
    console.print(f"📦 Backed up {len(files)} files to {backup_dir}", style="yellow")
    return backup_dir

def git_commit(message: str, config: Config):
    """Commit current changes to git"""
    
    if not config.enable_git_commits or config.dry_run:
        return
    
    try:
        subprocess.run(["git", "add", "-A"], check=False, capture_output=True)
        subprocess.run(["git", "commit", "-m", message], check=False, capture_output=True)
        console.print(f"📝 Git commit: {message}", style="dim")
    except Exception as e:
        console.print(f"⚠️  Git commit failed: {e}", style="yellow")

# ============================================================================
# AI FIX GENERATION
# ============================================================================

def generate_fix_prompt(errors: List[BuildError], context_map: Dict[str, str]) -> str:
    """Generate prompt for AI to fix errors"""
    
    prompt = """You are an expert C++ systems engineer fixing build errors.

Below are compilation errors from MELVIN v2 codebase. Fix each error by providing the corrected code.

CRITICAL INSTRUCTIONS:
1. Return ONLY the corrected code blocks, no explanations
2. Use this format:
   ```cpp:file_path
   // corrected code here
   ```
3. Include enough context (10+ lines) around the fix so I can locate it
4. Fix ALL errors shown below
5. Maintain existing code style and formatting
6. Do NOT add new dependencies or libraries

ERRORS TO FIX:

"""
    
    for i, error in enumerate(errors, 1):
        prompt += f"\nError #{i}:\n"
        prompt += f"File: {error.file_path}\n"
        prompt += f"Line: {error.line_number}\n"
        prompt += f"Type: {error.error_type}\n"
        prompt += f"Message: {error.message}\n"
        prompt += f"\nContext:\n{context_map.get(error.file_path, '// No context available')}\n"
        prompt += "-" * 80 + "\n"
    
    prompt += "\n\nProvide fixes now:"
    
    return prompt

def call_openai_api(prompt: str, config: Config) -> Optional[str]:
    """Call OpenAI API to get fix suggestions"""
    
    if not OPENAI_AVAILABLE:
        console.print("❌ OpenAI library not available", style="red")
        return None
    
    api_key = os.getenv("OPENAI_API_KEY")
    if not api_key:
        console.print("❌ OPENAI_API_KEY not set", style="red")
        return None
    
    if config.dry_run:
        console.print("🔍 [DRY RUN] Would call OpenAI API", style="yellow")
        return "// Mock fix for dry run"
    
    try:
        console.print(f"🤖 Calling OpenAI ({config.model})...", style="magenta")
        
        client = openai.OpenAI(api_key=api_key)
        
        response = client.chat.completions.create(
            model=config.model,
            messages=[
                {"role": "system", "content": "You are an expert C++ systems engineer."},
                {"role": "user", "content": prompt}
            ],
            temperature=config.temperature,
            max_tokens=config.max_tokens
        )
        
        fix_content = response.choices[0].message.content
        
        console.print(f"✅ Received {len(fix_content)} chars of fixes", style="green")
        
        return fix_content
        
    except Exception as e:
        console.print(f"❌ OpenAI API error: {e}", style="red")
        return None

# ============================================================================
# PATCH APPLICATION
# ============================================================================

def extract_code_blocks(ai_response: str) -> List[Tuple[str, str]]:
    """Extract code blocks from AI response"""
    
    # Pattern: ```cpp:file_path or ```cpp
    pattern = r'```(?:cpp|c\+\+)(?::(.+?))?\n(.*?)```'
    matches = re.findall(pattern, ai_response, re.DOTALL)
    
    code_blocks = []
    for file_path, code in matches:
        if file_path:
            code_blocks.append((file_path.strip(), code.strip()))
        else:
            # Try to infer from previous context
            code_blocks.append(("unknown", code.strip()))
    
    return code_blocks

def apply_fix_to_file(file_path: str, new_code: str, config: Config) -> bool:
    """Apply AI-generated fix to file"""
    
    if not os.path.exists(file_path):
        console.print(f"⚠️  File not found: {file_path}", style="yellow")
        return False
    
    if config.dry_run:
        console.print(f"🔍 [DRY RUN] Would apply fix to {file_path}", style="yellow")
        return True
    
    try:
        # Read original
        with open(file_path, 'r') as f:
            original_content = f.read()
        
        # Try to intelligently merge the fix
        # Strategy: Find matching context in file, replace that section
        
        # For now, simple approach: if new_code looks complete, replace file
        # In production, use more sophisticated patch matching
        
        # Check if this looks like a complete file or a snippet
        if len(new_code.split('\n')) > 20 or '#pragma once' in new_code or '#include' in new_code[:100]:
            # Looks like complete file
            with open(file_path, 'w') as f:
                f.write(new_code)
            console.print(f"✏️  Replaced {file_path}", style="green")
        else:
            # Looks like snippet - try to find and replace matching section
            # This is simplified - production would use fuzzy matching
            console.print(f"⚠️  Snippet too small, skipping {file_path}", style="yellow")
            return False
        
        return True
        
    except Exception as e:
        console.print(f"❌ Failed to apply fix to {file_path}: {e}", style="red")
        return False

def apply_fixes(ai_response: str, errors: List[BuildError], config: Config) -> List[str]:
    """Apply all fixes from AI response"""
    
    code_blocks = extract_code_blocks(ai_response)
    modified_files = []
    
    console.print(f"📝 Found {len(code_blocks)} code blocks to apply", style="cyan")
    
    for file_path, code in code_blocks:
        if file_path == "unknown":
            # Try to infer from errors
            if errors:
                file_path = errors[0].file_path
        
        if apply_fix_to_file(file_path, code, config):
            modified_files.append(file_path)
    
    return modified_files

# ============================================================================
# MAIN AUTO-FIX LOOP
# ============================================================================

class AutoFixer:
    """Main auto-fix orchestrator"""
    
    def __init__(self, config: Config):
        self.config = config
        self.attempts: List[FixAttempt] = []
        self.start_time = time.time()
        self.seen_errors: set = set()
    
    def run(self) -> bool:
        """Run auto-fix loop until success or max iterations"""
        
        console.print_panel(
            f"🤖 MELVIN Autonomous Build & Repair Tool\n"
            f"Model: {self.config.model}\n"
            f"Max Iterations: {self.config.max_iters}\n"
            f"Build: {self.config.build_command}",
            title="Starting Auto-Fix",
            style="bold blue"
        )
        
        for iteration in range(1, self.config.max_iters + 1):
            console.print(f"\n{'='*60}", style="bold")
            console.print(f"Iteration {iteration}/{self.config.max_iters}", style="bold cyan")
            console.print(f"{'='*60}\n", style="bold")
            
            # Run build
            build_success, log_path = run_build(self.config)
            
            if build_success:
                console.print("🎉 Build succeeded! Checking tests...", style="green bold")
                
                # Run tests
                test_success, test_log = run_tests(self.config)
                
                if test_success:
                    console.print("🎉 All tests passed!", style="green bold")
                    self.print_summary(success=True)
                    return True
                else:
                    console.print("⚠️  Build OK but tests failed", style="yellow")
                    # Could parse test failures here
            
            # Parse errors
            errors = parse_build_log(log_path)
            
            if not errors:
                console.print("⚠️  Build failed but no parseable errors found", style="yellow")
                break
            
            # Filter to errors only (skip warnings for now)
            errors = [e for e in errors if e.error_type == "error"]
            
            console.print(f"Found {len(errors)} errors to fix:", style="yellow")
            for error in errors[:5]:  # Show first 5
                console.print(f"  • {error}", style="red")
            if len(errors) > 5:
                console.print(f"  ... and {len(errors)-5} more", style="dim")
            
            # Detect runaway (same errors repeating)
            error_signature = tuple(sorted(str(e) for e in errors))
            if self.config.detect_runaway and error_signature in self.seen_errors:
                console.print("🛑 Runaway detected (same errors repeating)", style="red bold")
                break
            self.seen_errors.add(error_signature)
            
            # Get context for errors
            context_map = {}
            unique_files = set(e.file_path for e in errors)
            for file_path in unique_files:
                file_errors = [e for e in errors if e.file_path == file_path]
                # Get context around first error in file
                if file_errors:
                    context_map[file_path] = get_file_context(
                        file_path,
                        file_errors[0].line_number,
                        self.config.context_lines
                    )
            
            # Backup files
            if self.config.backup_before_fix:
                backup_dir = create_backup(list(unique_files), iteration)
            
            # Generate fix prompt
            prompt = generate_fix_prompt(errors, context_map)
            
            # Save prompt
            prompt_log = f"logs/auto_fix/iteration_{iteration}_prompt.txt"
            os.makedirs(os.path.dirname(prompt_log), exist_ok=True)
            with open(prompt_log, 'w') as f:
                f.write(prompt)
            
            # Call AI
            ai_response = call_openai_api(prompt, self.config)
            
            if not ai_response:
                console.print("❌ Failed to get AI response", style="red")
                break
            
            # Save response
            response_log = f"logs/auto_fix/iteration_{iteration}_response.txt"
            with open(response_log, 'w') as f:
                f.write(ai_response)
            
            # Apply fixes
            modified_files = apply_fixes(ai_response, errors, self.config)
            
            # Git commit
            if modified_files and self.config.enable_git_commits:
                commit_msg = f"Auto-fix iteration #{iteration}: {len(errors)} errors"
                git_commit(commit_msg, self.config)
            
            # Record attempt
            attempt = FixAttempt(
                iteration=iteration,
                errors=errors,
                files_modified=modified_files,
                fix_applied=len(modified_files) > 0,
                build_success=False,
                timestamp=datetime.now().isoformat()
            )
            self.attempts.append(attempt)
            
            # Brief pause before retry
            time.sleep(1)
        
        # Max iterations reached
        console.print(f"\n⚠️  Max iterations ({self.config.max_iters}) reached", style="yellow bold")
        self.print_summary(success=False)
        return False
    
    def print_summary(self, success: bool):
        """Print final summary"""
        
        elapsed = time.time() - self.start_time
        
        console.print("\n" + "="*60, style="bold")
        console.print("FINAL SUMMARY", style="bold cyan")
        console.print("="*60 + "\n", style="bold")
        
        rows = [
            ["Status", "✅ SUCCESS" if success else "❌ FAILED"],
            ["Total Iterations", str(len(self.attempts))],
            ["Total Time", f"{elapsed:.1f}s"],
            ["Errors Fixed", str(sum(len(a.errors) for a in self.attempts))],
            ["Files Modified", str(len(set(f for a in self.attempts for f in a.files_modified)))],
        ]
        
        console.print_table("Auto-Fix Summary", ["Metric", "Value"], rows)
        
        if self.attempts:
            console.print("\nIteration History:", style="bold")
            for attempt in self.attempts:
                status = "✅" if attempt.fix_applied else "❌"
                console.print(
                    f"  {status} Iter {attempt.iteration}: {len(attempt.errors)} errors, "
                    f"{len(attempt.files_modified)} files modified"
                )

# ============================================================================
# CONFIGURATION
# ============================================================================

def load_config(config_path: Optional[str] = None) -> Config:
    """Load configuration from YAML file or use defaults"""
    
    config = Config()
    
    if config_path and os.path.exists(config_path):
        with open(config_path, 'r') as f:
            yaml_config = yaml.safe_load(f)
        
        # Update config from YAML
        if yaml_config:
            for key, value in yaml_config.items():
                if hasattr(config, key):
                    setattr(config, key, value)
        
        console.print(f"📋 Loaded config from {config_path}", style="cyan")
    else:
        console.print("📋 Using default config", style="cyan")
    
    return config

# ============================================================================
# MAIN
# ============================================================================

def main():
    parser = argparse.ArgumentParser(description="MELVIN Autonomous Build & Repair Tool")
    parser.add_argument("--config", type=str, help="Path to config YAML file")
    parser.add_argument("--dry-run", action="store_true", help="Simulate without making changes")
    parser.add_argument("--max-iters", type=int, help="Maximum iterations")
    parser.add_argument("--model", type=str, help="OpenAI model to use")
    
    args = parser.parse_args()
    
    # Load config
    config = load_config(args.config)
    
    # Override with CLI args
    if args.dry_run:
        config.dry_run = True
    if args.max_iters:
        config.max_iters = args.max_iters
    if args.model:
        config.model = args.model
    
    # Check prerequisites
    if not config.dry_run and not OPENAI_AVAILABLE:
        console.print("❌ OpenAI library required. Install with: pip install openai", style="red bold")
        sys.exit(1)
    
    if not config.dry_run and not os.getenv("OPENAI_API_KEY"):
        console.print("❌ OPENAI_API_KEY environment variable not set", style="red bold")
        console.print("Set it with: export OPENAI_API_KEY=your_key_here", style="yellow")
        sys.exit(1)
    
    # Create auto-fixer
    fixer = AutoFixer(config)
    
    # Run!
    try:
        success = fixer.run()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        console.print("\n\n⚠️  Interrupted by user", style="yellow bold")
        fixer.print_summary(success=False)
        sys.exit(2)
    except Exception as e:
        console.print(f"\n\n❌ Fatal error: {e}", style="red bold")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()

