#!/usr/bin/env python3
"""
LP Language Linter - Static Code Analysis Tool
Like pylint, flake8, mypy for Python

Usage:
    python lp_linter.py file.lp
    python lp_linter.py file.lp --strict
    python lp_linter.py file.lp --json
"""

import re
import sys
import argparse
from dataclasses import dataclass
from enum import Enum
from typing import List, Dict, Set, Tuple, Optional

class Severity(Enum):
    ERROR = "ERROR"
    WARNING = "WARNING"
    INFO = "INFO"
    STYLE = "STYLE"

@dataclass
class LintIssue:
    line: int
    col: int
    code: str
    severity: Severity
    message: str
    suggestion: str = ""

class LPLinter:
    """LP Language Static Analyzer"""
    
    VALID_TYPES = {
        'int', 'float', 'str', 'bool', 'list', 'dict', 'set', 'tuple',
        'void', 'arena', 'pool', 'thread', 'lock', 'val'
    }
    
    BUILTIN_MODULES = {
        'dsa', 'math', 'random', 'time', 'os', 'sys', 'http', 'json',
        'sqlite', 'memory', 'thread', 'parallel', 'numpy', 'platform'
    }
    
    KEYWORDS = {
        'import', 'from', 'as', 'def', 'class', 'return', 'if', 'elif',
        'else', 'for', 'while', 'break', 'continue', 'pass', 'try',
        'except', 'finally', 'raise', 'with', 'lambda', 'yield',
        'match', 'case', 'async', 'await', 'True', 'False', 'null',
        'and', 'or', 'not', 'in', 'is', 'parallel'
    }
    
    BUILTIN_FUNCS = {
        'print', 'len', 'range', 'input', 'int', 'float', 'str', 'bool',
        'type', 'list', 'dict', 'set', 'tuple', 'sorted', 'enumerate',
        'zip', 'map', 'filter', 'sum', 'min', 'max', 'abs', 'round'
    }
    
    def __init__(self, strict: bool = False):
        self.strict = strict
        self.issues: List[LintIssue] = []
        self.functions: Dict[str, int] = {}
        self.classes: Dict[str, int] = {}
        self.variables: Set[str] = set()
        self.imports: Set[str] = set()
    
    def lint(self, code: str) -> List[LintIssue]:
        lines = code.split('\n')
        
        # Pass 1: Collect definitions
        for i, line in enumerate(lines, 1):
            self._collect_definitions(i, line.strip())
        
        # Pass 2: Check for issues
        for i, line in enumerate(lines, 1):
            self._check_line(i, line)
        
        # Global checks
        self._check_dsa_flush(code)
        self._check_main_call(code)
        
        return sorted(self.issues, key=lambda x: (x.line, x.col))
    
    def _collect_definitions(self, line_num: int, line: str):
        # Function definition
        match = re.match(r'def\s+(\w+)\s*\(', line)
        if match:
            self.functions[match.group(1)] = line_num
        
        # Class definition
        match = re.match(r'class\s+(\w+)', line)
        if match:
            self.classes[match.group(1)] = line_num
    
    def _check_line(self, line_num: int, line: str):
        stripped = line.strip()
        col = len(line) - len(line.lstrip()) + 1
        
        if not stripped or stripped.startswith('#'):
            return
        
        self._check_import(line_num, col, stripped)
        self._check_var_decl(line_num, col, stripped)
        self._check_none_vs_null(line_num, col, stripped)
        self._check_syntax(line_num, col, stripped)
        self._check_common_mistakes(line_num, col, stripped)
        
        if self.strict:
            self._check_style(line_num, col, line)
    
    def _check_import(self, line_num: int, col: int, line: str):
        match = re.match(r'import\s+(\w+)', line)
        if match:
            module = match.group(1)
            self.imports.add(module)
            
            if module not in self.BUILTIN_MODULES:
                self.issues.append(LintIssue(
                    line=line_num, col=col, code="E001",
                    severity=Severity.WARNING,
                    message=f"Unknown module '{module}'",
                    suggestion=f"Known modules: {', '.join(sorted(self.BUILTIN_MODULES))}"
                ))
    
    def _check_var_decl(self, line_num: int, col: int, line: str):
        match = re.match(r'(\w+)\s*:\s*(\w+)', line)
        if match:
            name, type_name = match.groups()
            self.variables.add(name)
            
            if type_name not in self.VALID_TYPES and type_name not in self.classes:
                self.issues.append(LintIssue(
                    line=line_num, col=col, code="E002",
                    severity=Severity.ERROR,
                    message=f"Unknown type '{type_name}'",
                    suggestion=f"Valid types: {', '.join(sorted(self.VALID_TYPES))}"
                ))
    
    def _check_none_vs_null(self, line_num: int, col: int, line: str):
        if 'None' in line and 'null' not in line:
            self.issues.append(LintIssue(
                line=line_num, col=col, code="E003",
                severity=Severity.ERROR,
                message="Use 'null' instead of 'None' in LP",
                suggestion="Replace 'None' with 'null'"
            ))
    
    def _check_syntax(self, line_num: int, col: int, line: str):
        keywords = ['if', 'elif', 'else', 'for', 'while', 'def', 'class', 'try', 'except', 'with']
        
        for kw in keywords:
            pattern = rf'^{kw}\s+[^:#]*$'
            if re.match(pattern, line):
                self.issues.append(LintIssue(
                    line=line_num, col=len(line),
                    code="E004", severity=Severity.ERROR,
                    message=f"Missing ':' after '{kw}'",
                    suggestion=f"Add ':' at the end"
                ))
    
    def _check_common_mistakes(self, line_num: int, col: int, line: str):
        # Assignment in if
        if re.search(r'if\s+\w+\s*=\s*', line):
            self.issues.append(LintIssue(
                line=line_num, col=col, code="W001",
                severity=Severity.WARNING,
                message="Assignment in if condition, did you mean '=='?",
                suggestion="Use '==' for comparison"
            ))
        
        # ptr type
        if re.search(r':\s*ptr\s*=', line):
            self.issues.append(LintIssue(
                line=line_num, col=col, code="E005",
                severity=Severity.ERROR,
                message="'ptr' type is not directly usable",
                suggestion="Use 'list' for arrays or 'arena' for memory"
            ))
    
    def _check_style(self, line_num: int, col: int, line: str):
        if len(line) > 100:
            self.issues.append(LintIssue(
                line=line_num, col=100, code="S001",
                severity=Severity.STYLE,
                message=f"Line too long ({len(line)} > 100)",
                suggestion="Break into multiple lines"
            ))
        
        if line != line.rstrip():
            self.issues.append(LintIssue(
                line=line_num, col=len(line.rstrip()) + 1,
                code="S002", severity=Severity.STYLE,
                message="Trailing whitespace",
                suggestion="Remove trailing spaces"
            ))
    
    def _check_dsa_flush(self, code: str):
        has_dsa_import = 'import dsa' in code
        has_dsa_write = 'dsa.write' in code
        has_flush = 'dsa.flush()' in code
        
        if has_dsa_import and has_dsa_write and not has_flush:
            lines = code.split('\n')
            last_write = 0
            for i, line in enumerate(lines, 1):
                if 'dsa.write' in line:
                    last_write = i
            
            self.issues.append(LintIssue(
                line=last_write, col=1, code="E006",
                severity=Severity.ERROR,
                message="dsa module requires dsa.flush() before exit",
                suggestion="Add 'dsa.flush()' at the end"
            ))
    
    def _check_main_call(self, code: str):
        has_main = 'def main(' in code or 'def solve(' in code
        has_call = re.search(r'\b(main|solve)\s*\(\s*\)', code)
        
        if has_main and not has_call:
            lines = code.split('\n')
            for i, line in enumerate(lines, 1):
                if 'def main(' in line or 'def solve(' in line:
                    self.issues.append(LintIssue(
                        line=i, col=1, code="W002",
                        severity=Severity.WARNING,
                        message="Function defined but never called",
                        suggestion="Add 'main()' or 'solve()' at the end"
                    ))
                    break

def format_output(issues: List[LintIssue]) -> str:
    if not issues:
        return "✅ No issues found!"
    
    errors = sum(1 for i in issues if i.severity == Severity.ERROR)
    warnings = sum(1 for i in issues if i.severity == Severity.WARNING)
    styles = sum(1 for i in issues if i.severity == Severity.STYLE)
    
    output = [f"🔍 Found {len(issues)} issue(s): {errors} error(s), {warnings} warning(s), {styles} style\n"]
    
    for issue in issues:
        if issue.severity == Severity.ERROR:
            icon = "❌"
        elif issue.severity == Severity.WARNING:
            icon = "⚠️"
        else:
            icon = "ℹ️"
        
        output.append(f"{icon} [{issue.code}] Line {issue.line}:{issue.col}: {issue.message}")
        if issue.suggestion:
            output.append(f"    💡 Suggestion: {issue.suggestion}")
    
    return '\n'.join(output)

def main():
    parser = argparse.ArgumentParser(description='LP Linter')
    parser.add_argument('file', help='LP file to lint')
    parser.add_argument('--strict', action='store_true', help='Enable style checks')
    parser.add_argument('--json', action='store_true', help='JSON output')
    parser.add_argument('--quiet', action='store_true', help='Only show errors')
    
    args = parser.parse_args()
    
    try:
        with open(args.file, 'r', encoding='utf-8') as f:
            code = f.read()
    except FileNotFoundError:
        print(f"❌ Error: File '{args.file}' not found")
        sys.exit(1)
    
    linter = LPLinter(strict=args.strict)
    issues = linter.lint(code)
    
    if args.quiet:
        issues = [i for i in issues if i.severity == Severity.ERROR]
    
    if args.json:
        import json
        print(json.dumps([{
            'line': i.line, 'col': i.col, 'code': i.code,
            'severity': i.severity.value, 'message': i.message,
            'suggestion': i.suggestion
        } for i in issues], indent=2))
    else:
        print(format_output(issues))
    
    sys.exit(1 if any(i.severity == Severity.ERROR for i in issues) else 0)

if __name__ == '__main__':
    main()
