# Examples Cookbook

## What you'll learn

This guide walks through the example and regression files in `tests/regression/`, explains what each one demonstrates, and shows the fastest command to run it.

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
| `06_tsp_ils.lp` | ILS, 2-opt, double bridge, `dsa.write_float_prec` |
| `test_global_multiline_lambda.lp` | top-level multiline lambda |
| `test_inheritance_access.lp` | `protected` inheritance rules |
| `test_json.lp` | JSON round-trip |
| `test_memory_control.lp` | arena and pool helpers |
| `test_multiline_lambda.lp` | local multiline lambda |
| `test_string_operations.lp` | string methods |
| `test_thread_sqlite.lp` | thread join + nested subscript + SQLite |
| `test_type_inference.lp` | local type inference |

## `06_tsp_ils.lp`

### What it teaches

- ILS (Iterated Local Search) pattern
- Correct double bridge perturbation (4-cut reconnection)
- 2-opt local search with don't-look bits (DLB)
- OR-opt-1 (single-node relocation)
- `dsa.write_float_prec(x, 4)` for formatted float output
- Pre-allocated flat arrays for distance matrix

### Run it

```bash
lp tests/regression/06_tsp_ils.lp < input.txt
```

### Input format

```
N
x1 y1
x2 y2
...
xN yN
```

### What to notice

- Double bridge cuts at three positions `a < b < c` and reconnects as `[0..a) + [c..N) + [b..c) + [a..b)` — this is qualitatively different from a simple segment reversal and is essential for escaping local optima
- The distance matrix is stored as a flat list `D[i * N + j]` — pre-allocated once and reused throughout
- `dsa.write_float_prec(best, 4)` prints the tour length with exactly 4 decimal places, matching competitive programming judge expectations
- OR-opt-1 relocates individual nodes to a better position in the tour and is run after 2-opt in each ILS iteration

### Performance note

LP runs this approximately 2× slower than an equivalent C++ implementation due to LpVal overhead on the distance matrix. When typed arrays (`int[]` / `float[]`) land in a future release, the inner loop cost will drop substantially. See [Performance Guide](Performance-Guide).

## `01_web_fetcher_thread.lp`

### What it teaches

- `http.get(...)`
- `thread.spawn(...)`
- `thread.join(...)`
- SQLite writes from worker threads

### Run it

```bash
lp tests/regression/01_web_fetcher_thread.lp
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
lp tests/regression/02_oop_game_engine.lp
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
lp tests/regression/03_memory_arena.lp
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
lp tests/regression/04_parallel_for.lp
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
lp tests/regression/05_system_sqlite.lp
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
