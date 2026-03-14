# Documentation Map

## What you'll learn

This file explains how the LP documentation is organized, which pages are tutorials versus references, and which parts of the language are fully supported, partially supported, or intentionally documented as limitations.

## Prerequisites

None.

## Documentation Types

LP uses four documentation modes:

- Tutorial: step-by-step onboarding and feature walkthroughs.
- Reference: compact syntax and API lookup.
- Cookbook: example-driven walkthroughs tied to files in `examples/`.
- Troubleshooting: symptom-to-fix guides for common build and compiler problems.

## Source Of Truth

When an older document and the current implementation disagree, use the implementation in this repository as the source of truth.

Primary source files:

- `README.md`
- `task_progress.md`
- `examples/`
- `compiler/src/main.c`
- `compiler/src/repl.c`
- `compiler/src/parser.c`
- `compiler/src/codegen.c`
- `runtime/*.h`

## Document Map

| File | Type | Audience | Purpose |
| --- | --- | --- | --- |
| `FEATURES.md` | Reference | Everyone | **Comprehensive feature status: supported, partial, and unsupported.** |
| `01_Installation_and_Setup.md` | Tutorial | Everyone | Build the compiler and verify the environment. |
| `02_First_Programs.md` | Tutorial | Beginners | Run LP files, use the REPL, generate C, compile executables. |
| `03_Language_Basics.md` | Tutorial | Beginners | Variables, functions, control flow, inference, annotations. |
| `04_Expressions_and_Collections.md` | Tutorial | Beginners to intermediate | Strings, collections, indexing, lambdas, collection limitations. |
| `05_Object_Oriented_Programming.md` | Tutorial | Intermediate | Classes, inheritance, polymorphism, access control. |
| `06_Error_Handling_and_Advanced_Control_Flow.md` | Tutorial | Intermediate | `try`, `except`, `finally`, `raise`, `with`, parser/codegen diagnostics. |
| `07_Concurrency_and_Parallelism.md` | Tutorial | Intermediate to advanced | `thread.*`, `parallel for`, restrictions and anti-patterns. |
| `08_Runtime_Modules.md` | Reference + tutorial | Intermediate | Public runtime module surface and examples. |
| `09_CLI_and_Tooling.md` | Reference | Everyone | Every public `lp` command and its behavior. |
| `10_Build_Distribution_and_Cross_Targets.md` | Reference | Advanced | Release flags, packaging, host builds, cross-target builds. |
| `11_C_API_Export.md` | Tutorial + reference | Advanced | `lp export`, generated headers, and shared library workflow. |
| `12_Examples_Cookbook.md` | Cookbook | Everyone | Guided walkthroughs of `examples/`. |
| `13_Troubleshooting.md` | Troubleshooting | Everyone | Fast fixes for setup, compiler, and tooling errors. |
| `14_Language_Reference.md` | Reference | Everyone | Compact syntax, builtins, runtime surface, CLI summary. |
| `15_Parallel_Computing.md` | Tutorial + Reference | Intermediate to advanced | OpenMP integration, GPU computing, parallel algorithms. |
| `guide_vi.md` | Tutorial | Vietnamese readers | Fast onboarding in Vietnamese. |

## Learning Paths

### Beginner path

Read in this order:

1. `01_Installation_and_Setup.md`
2. `02_First_Programs.md`
3. `03_Language_Basics.md`
4. `04_Expressions_and_Collections.md`
5. `05_Object_Oriented_Programming.md`
6. `09_CLI_and_Tooling.md`
7. `13_Troubleshooting.md`

### App developer path

Read in this order:

1. `01_Installation_and_Setup.md`
2. `02_First_Programs.md`
3. `03_Language_Basics.md`
4. `04_Expressions_and_Collections.md`
5. `06_Error_Handling_and_Advanced_Control_Flow.md`
6. `08_Runtime_Modules.md`
7. `12_Examples_Cookbook.md`

### Systems and power-user path

Read in this order:

1. `01_Installation_and_Setup.md`
2. `07_Concurrency_and_Parallelism.md`
3. `15_Parallel_Computing.md`
4. `08_Runtime_Modules.md`
5. `09_CLI_and_Tooling.md`
6. `10_Build_Distribution_and_Cross_Targets.md`
7. `11_C_API_Export.md`
8. `14_Language_Reference.md`

## Coverage Status

LP docs use the following status language:

- ✅ **Supported**: implemented and documented as normal public behavior.
- ⚠️ **Partially supported**: implemented with important restrictions or platform dependencies.
- ❌ **Not supported**: explicitly rejected or missing in the current compiler/runtime.
- 🔧 **Internal or experimental**: code paths exist, but they are not presented as stable public workflow.

For a comprehensive feature status table, see **[FEATURES.md](FEATURES.md)**.

### Quick Status Reference

| Category | Status |
|----------|--------|
| Core syntax (variables, functions, classes) | ✅ Full |
| Control flow (if, for, while, try) | ✅ Full |
| Operators (arithmetic, bitwise, comparison) | ✅ Full |
| String methods | ✅ Full |
| Collections (list, dict, set, tuple) | ✅ Full |
| OOP (classes, inheritance, access control) | ✅ Full |
| Lambda functions | ✅ Full |
| `parallel for` | ✅ Full (needs OpenMP) |
| Dictionary comprehensions | ✅ Full |
| `yield` / Generators | ✅ Full |
| `http.post` | ✅ Full |
| `json.parse` | ✅ Full |
| List comprehensions | ⚠️ Partial (numeric-focused) |
| `thread.spawn` | ⚠️ Partial (worker restrictions) |
| `numpy` module | ⚠️ Partial (basic functions) |
| Async/await | ❌ Not yet |
| Decorators | ❌ Not yet |
| F-strings | ❌ Not yet |

### Key Limitations

- `thread.spawn(...)`: Worker must be a named LP function, 0 or 1 argument, return `int` or `void`
- `parallel for`: Requires OpenMP-capable toolchain for actual parallelism
- Integer division `//` and modulo `%` follow Python semantics
- Cross-target builds require matching external toolchains

## Common errors and limitations

- If a command example in the docs says `lp ...`, that means either an installed `lp` binary or the repo build output such as `build\lp.exe` or `./build/lp`.
- Some advanced source paths exist internally, especially for Python interop and `numpy`, but they are not yet treated as stable public workflow in these docs.

## See also

- [FEATURES.md](FEATURES.md) - Comprehensive feature status
- [Installation and Setup](01_Installation_and_Setup.md)
- [CLI and Tooling](09_CLI_and_Tooling.md)
- [Language Reference](14_Language_Reference.md)
