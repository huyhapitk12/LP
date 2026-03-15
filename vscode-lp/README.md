# LP Language — VS Code Extension

Syntax highlighting, snippets, and run support for the LP programming language.

## Features

- **Syntax Highlighting** — Full TextMate grammar for `.lp` files
- **25+ Snippets** — `def`, `class`, `for`, `if`, `try`, `import`, and more
- **Run Button** — Click ▶ in editor title bar or press `Ctrl+Shift+R`
- **REPL** — Open LP interactive mode from command palette
- **Generate C** — View transpiled C code side-by-side
- **Compile** — Build native executable from `.lp` file

## Commands

| Command | Shortcut | Description |
|---|---|---|
| `LP: Run Current File` | `Ctrl+Shift+R` | Run the active `.lp` file |
| `LP: Open REPL` | — | Open LP interactive shell |
| `LP: Generate C Code` | — | Transpile to C and show side-by-side |
| `LP: Compile to Executable` | — | Compile to native `.exe` |

## Configuration

| Setting | Default | Description |
|---|---|---|
| `lp.compilerPath` | `""` (auto-detect) | Path to LP compiler |

## Installation

1. Copy this folder to `~/.vscode/extensions/lp-language`
2. Restart VS Code
3. Open any `.lp` file — done!

Or package as VSIX:
```bash
npm install -g @vscode/vsce
cd vscode-lp
vsce package
# Then: Extensions → Install from VSIX
```
