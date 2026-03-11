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
3. `08_Runtime_Modules.md`
4. `09_CLI_and_Tooling.md`
5. `10_Build_Distribution_and_Cross_Targets.md`
6. `11_C_API_Export.md`
7. `14_Language_Reference.md`

## Coverage Status

LP docs use the following status language:

- Supported: implemented and documented as normal public behavior.
- Partially supported: implemented with important restrictions or platform dependencies.
- Unsupported: explicitly rejected or missing in the current compiler/runtime.
- Internal or experimental: code paths exist, but they are not presented as stable public workflow.

Current high-value status notes:

- `http.get(...)`: supported.
- `http.post(...)`: unsupported.
- `json.loads(...)` and `json.dumps(...)`: supported.
- `json.parse(...)`: unsupported.
- `thread.spawn(...)`: partially supported with strict worker restrictions.
- `parallel for`: partially supported because toolchain OpenMP behavior is platform-dependent.
- Dictionary comprehensions: not documented as supported public syntax in the current compiler.
- `yield`: reserved in the lexer/AST but not documented as public syntax yet.

## Common errors and limitations

- If a command example in the docs says `lp ...`, that means either an installed `lp` binary or the repo build output such as `build\lp.exe` or `./build/lp`.
- Some advanced source paths exist internally, especially for Python interop and `numpy`, but they are not yet treated as stable public workflow in these docs.

## See also

- [Installation and Setup](01_Installation_and_Setup.md)
- [CLI and Tooling](09_CLI_and_Tooling.md)
- [Language Reference](14_Language_Reference.md)
