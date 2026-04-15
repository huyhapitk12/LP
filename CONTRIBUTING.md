# Contributing to LP Language

First off, thank you for considering contributing to LP! It's people like you that make LP such a great tool.

## 📜 Table of Contents

- [Code of Conduct](#code-of-conduct)
- [How Can I Contribute?](#how-can-i-contribute)
- [Development Setup](#development-setup)
- [Coding Standards](#coding-standards)
- [Pull Request Process](#pull-request-process)
- [Reporting Bugs](#reporting-bugs)
- [Suggesting Features](#suggesting-features)

---

## 🤝 Code of Conduct

This project and everyone participating in it is governed by our [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. Please report unacceptable behavior to the project maintainers.

**Key principles:**
- Be respectful and inclusive
- Welcome newcomers and help them get started
- Focus on what is best for the community
- Show empathy towards other community members

---

## 🙋 How Can I Contribute?

### Report Bugs

Bug reports are incredibly helpful! When reporting a bug, please include:

1. **A clear title** - Summarize the bug concisely
2. **Steps to reproduce** - Detailed steps to trigger the bug
3. **Expected behavior** - What you expected to happen
4. **Actual behavior** - What actually happened
5. **Environment** - OS, compiler version, etc.
6. **Code sample** - Minimal code that reproduces the issue

### Suggest Features

Feature suggestions are welcome! Please:

1. **Check existing issues** - Avoid duplicates
2. **Describe the feature** - Clear and detailed description
3. **Explain the use case** - Why would this be useful?
4. **Provide examples** - How would the API look?

### Improve Documentation

Documentation improvements are always welcome:

- Fix typos or unclear sections
- Add missing documentation
- Translate documentation
- Improve examples

### Submit Pull Requests

We welcome code contributions! See the [Pull Request Process](#pull-request-process) below.

---

## 🛠️ Development Setup

### Prerequisites

- GCC or Clang compiler
- Make build tool
- binutils (`as`, `ld`)
- Git

### Clone and Build

```bash
# Clone your fork
git clone https://github.com/YOUR_USERNAME/LP.git
cd LP

# Build the compiler
make                # Linux / macOS
lp --build          # Windows (CMD or PowerShell)
./lp.sh --build     # Linux / macOS (alternative)

# Run tests
./build/lp test examples
```

### Project Structure

```
LP/
├── compiler/
│   ├── src/           # Source files (.c, .h)
│   └── tests/         # Compiler test programs
├── runtime/           # Runtime library (header-only C)
├── docs/              # Documentation assets
└── vscode-lp/         # VSCode extension
```

---

## 📏 Coding Standards

### C Code Style

```c
// Use 4 spaces for indentation
// Function names: snake_case
// Types: PascalCase with Lp prefix
// Constants: UPPER_SNAKE_CASE

// Good
int lp_calculate_sum(int a, int b) {
    return a + b;
}

typedef struct LpContext {
    int value;
} LpContext;

#define MAX_BUFFER_SIZE 1024

// Bad
int CalculateSum(int a, int b) {
    return a + b;
}
```

### Commit Messages

Follow conventional commits:

```
feat: Add new feature
fix: Fix a bug
docs: Update documentation
test: Add tests
refactor: Refactor code
perf: Improve performance
chore: Maintenance tasks
```

Examples:
```
feat: Add F-Strings support
fix: Fix buffer overflow in lexer
docs: Update installation guide
test: Add tests for pattern matching
```

### Code Comments

```c
// Single line comments for brief explanations

/*
 * Multi-line comments for detailed explanations
 * Include the "why" not just the "what"
 */

/**
 * @brief Calculate the factorial of a number
 * @param n The input number
 * @return The factorial of n
 */
int factorial(int n);
```

---

## 🔄 Pull Request Process

1. **Fork the repository** and create your branch from `master`
2. **Make your changes** following coding standards
3. **Add tests** for new functionality
4. **Update documentation** if needed
5. **Commit your changes** with clear commit messages
6. **Push to your fork** and submit a pull request
7. **Respond to review feedback**

### PR Checklist

- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] New features have tests
- [ ] Documentation is updated
- [ ] Commit messages follow conventions
- [ ] PR description is clear

### PR Title Format

```
<type>: <description>

# Examples:
feat: Add async/await support
fix: Resolve memory leak in parser
docs: Add Vietnamese language guide
```

---

## 🐛 Reporting Bugs

### Before Submitting

1. Check if the bug has already been reported
2. Try to reproduce with the latest version
3. Prepare a minimal code example

### Bug Report Template

```markdown
## Bug Description
[Clear description of the bug]

## Steps to Reproduce
1. Write code: `[your code]`
2. Run: `lp file.lp`
3. See error

## Expected Behavior
[What you expected]

## Actual Behavior
[What actually happened]

## Environment
- OS: [e.g., Ubuntu 22.04]
- LP Version: [e.g., 0.1.0]
- GCC Version: [e.g., 11.3.0]

## Code Sample
```lp
def main():
    # Your minimal reproducible code
    pass
```

## Error Output
```
[Paste error message]
```
```

---

## 💡 Suggesting Features

### Feature Request Template

```markdown
## Feature Description
[Clear description of the feature]

## Use Case
[Why would this be useful?]

## Proposed API
```lp
# How would it work?
def new_feature():
    pass
```

## Alternatives Considered
[Other approaches you've considered]

## Additional Context
[Any other relevant information]
```

---

## 🏷️ Issue Labels

| Label | Description |
|-------|-------------|
| `bug` | Something isn't working |
| `documentation` | Improvements or additions to documentation |
| `enhancement` | New feature or request |
| `good first issue` | Good for newcomers |
| `help wanted` | Extra attention is needed |
| `question` | Further information is requested |

---

## 📞 Getting Help

- **GitHub Issues**: For bug reports and feature requests
- **GitHub Discussions**: For questions and general discussion
- **Documentation**: Check the [GitHub Wiki](https://github.com/huyhapitk12/LP/wiki)

---

## 🙏 Recognition

Contributors are recognized in:

- The project README
- Release notes
- Git history

Thank you for contributing to LP! 🚀
