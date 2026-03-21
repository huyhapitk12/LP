# Feature Overview

> Last updated: March 2026 | Version 0.1.0

This page is the short overview of LP feature support.

Use this page when you want a quick answer. Use [Feature Status](Feature-Status) when you need the detailed matrix, examples, and caveats for a specific feature.

## Stable Public Surface

These areas are the most reliable parts of the current language and toolchain:

- Core syntax: variables, functions, classes, `if`, `for`, `while`, `try`, `raise`, `with`
- String support: common string methods and f-strings
- Core collections: lists, dicts, tuples
- Module usage: `math`, `random`, `time`, `os`, `http`, `json`, `sqlite`, `memory`, `platform`, `dsa`
- Tooling: REPL, C output, GCC-backed build flow, packaging, C API export

## Partial Or Strict Features

These features exist, but the current build has limitations that matter in practice:

- `thread.spawn`: documented conservatively; named zero-argument workers are the verified path today
- Pattern matching: fully stable, including capture-pattern guards (`case n if n > 10`) and variable bindings
- Set literals: experimental in the current release
- List comprehensions: numeric-focused and still limited
- `numpy`/parallel/GPU flows: available, but success depends more heavily on backend and toolchain configuration
- Advanced type system features such as unions and generics: parsed, but not fully enforced or specialized

## How To Read The Docs

- Read [Language Basics](Language-Basics) first for normal syntax.
- Use [Language Reference](Language-Reference) for compact API lookup.
- Use [Feature Status](Feature-Status) as the canonical detailed status page.
- Use [Runtime Modules](Runtime-Modules) for module-by-module public contracts.
- Use [Concurrency and Parallelism](Concurrency-and-Parallelism) and [Parallel and GPU Computing](Parallel-Computing) for threading/OpenMP/GPU details.

## Recommended Reading By Goal

### I want to know what is safe to ship with today

- [Feature Status](Feature-Status)
- [Runtime Modules](Runtime-Modules)
- [Troubleshooting](Troubleshooting)

### I want the shortest syntax summary

- [Quick Reference](Quick-Reference)
- [Language Reference](Language-Reference)

### I want advanced runtime or systems features

- [Concurrency and Parallelism](Concurrency-and-Parallelism)
- [Parallel and GPU Computing](Parallel-Computing)
- [Security Reference](Security-Module-Reference)
