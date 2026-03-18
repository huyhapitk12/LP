# Examples Cookbook

## What you'll learn

This guide walks through the example and regression files in `examples/`, explains what each one demonstrates, and shows the fastest command to run it.

## Prerequisites

- Read [First Programs](First-Programs) first.

## Cookbook Index

| File | Main topic |
| --- | --- |
| `01_web_fetcher_thread.lp` | HTTP + threads + SQLite |
| `02_oop_game_engine.lp` | classes, inheritance, methods |
| `03_memory_arena.lp` | manual arena allocation |
| `04_parallel_for.lp` | `parallel for` syntax |
| `05_system_sqlite.lp` | platform + SQLite |
| `test_global_multiline_lambda.lp` | top-level multiline lambda |
| `test_inheritance_access.lp` | `protected` inheritance rules |
| `test_json.lp` | JSON round-trip |
| `test_memory_control.lp` | arena and pool helpers |
| `test_multiline_lambda.lp` | local multiline lambda |
| `test_string_operations.lp` | string methods |
| `test_thread_sqlite.lp` | thread join + nested subscript + SQLite |
| `test_type_inference.lp` | local type inference |

## `01_web_fetcher_thread.lp`

### What it teaches

- `http.get(...)`
- `thread.spawn(...)`
- `thread.join(...)`
- SQLite writes from worker threads

### Run it

```bash
lp examples/01_web_fetcher_thread.lp
```

### What to notice

- each worker is a named function
- each worker returns `int`
- the main thread validates `thread.join(...)` results

## `02_oop_game_engine.lp`

### What it teaches

- class fields
- inherited movement logic
- subclass-specific behavior
- method calls on objects

### Run it

```bash
lp examples/02_oop_game_engine.lp
```

### What to notice

- both `Player` and `Enemy` inherit from `Entity`
- methods mutate object fields directly through `self`

## `03_memory_arena.lp`

### What it teaches

- `memory.arena_new(...)`
- `memory.arena_alloc(...)`
- `memory.cast(...)`
- explicit arena cleanup

### Run it

```bash
lp examples/03_memory_arena.lp
```

### What to notice

- memory is managed explicitly
- cleanup is part of the example, not optional decoration

## `04_parallel_for.lp`

### What it teaches

- the `parallel for` syntax
- numeric range iteration for heavy work
- current reduction caveats

### Run it

```bash
lp examples/04_parallel_for.lp
```

### What to notice

- the example uses `math.sin(...)` and `math.cos(...)`
- the code comments already warn that reductions are not automatic

## `05_system_sqlite.lp`

### What it teaches

- `platform.os()`
- `platform.arch()`
- `platform.cores()`
- SQLite table creation and querying

### Run it

```bash
lp examples/05_system_sqlite.lp
```

### What to notice

- platform helpers return runtime values that are easy to print with `str(...)`
- SQLite query results are accessed through nested subscript

## `test_global_multiline_lambda.lp`

### What it teaches

- top-level multiline lambda assignment

### Run it

```bash
lp test examples
```

## `test_inheritance_access.lp`

### What it teaches

- `protected` field access from a subclass
- `protected` method access from a subclass
- override behavior

### Run it

```bash
lp test examples
```

## `test_json.lp`

### What it teaches

- `json.loads(...)`
- `json.dumps(...)`
- indexed access into parsed JSON values

### Run it

```bash
lp test examples
```

## `test_memory_control.lp`

### What it teaches

- arena allocation
- pool allocation
- `memory.cast(...)` on both allocation paths

### Run it

```bash
lp test examples
```

## `test_multiline_lambda.lp`

### What it teaches

- multiline lambda inside local scope

### Run it

```bash
lp test examples
```

## `test_string_operations.lp`

### What it teaches

- `strip`, `lower`, `upper`
- `find`, `count`
- `split`, `join`
- `startswith`, `endswith`, `isdigit`

### Run it

```bash
lp test examples
```

## `test_thread_sqlite.lp`

### What it teaches

- thread return values through `join`
- SQLite use from multiple workers
- nested subscript access like `rows[0]["n"]`

### Run it

```bash
lp test examples
```

## `test_type_inference.lp`

### What it teaches

- inferred `int` and `str`
- inferred value flow into another function call

### Run it

```bash
lp test examples
```

## Common errors and limitations

- Some examples require network access, especially `01_web_fetcher_thread.lp`.
- SQLite examples create local `.db` files as part of normal execution.
- `parallel for` syntax is demonstrated in the examples, but actual parallel speedup still depends on toolchain configuration.

## See also

- [Runtime Modules](Runtime-Modules)
- [Troubleshooting](Troubleshooting)
