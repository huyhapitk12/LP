/*
 * LP REPL (Read-Eval-Print Loop) — Interactive Mode
 * Usage: lp (no arguments) → enters interactive shell
 */
#ifndef LP_REPL_H
#define LP_REPL_H

/* Run the REPL interactive mode. argv0 = argv[0] for GCC path resolution. */
int repl_run(const char *argv0);

#endif
