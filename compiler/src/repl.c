/*
 * LP REPL (Read-Eval-Print Loop) — Interactive Mode
 *
 * Strategy:
 *   - Accumulate all state (variables, functions, classes) as C source
 *   - Each new input is appended, then the whole program is recompiled & run
 *   - Multi-line blocks detected by trailing colon (:) on def/class/if/for/while
 *   - Special commands: .help, .clear, .exit
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <process.h>
#endif
#include "parser.h"
#include "codegen.h"
#include "repl.h"
#include "process_utils.h"

#if defined(_WIN32)
#define LP_HOST_EXE_EXT ".exe"
#define LP_PATH_SEP_STR "\\"
#else
#define LP_HOST_EXE_EXT ""
#define LP_PATH_SEP_STR "/"
#endif

/* ─── ANSI color codes ─── */
#define C_RESET   "\033[0m"
#define C_GREEN   "\033[32m"
#define C_CYAN    "\033[36m"
#define C_YELLOW  "\033[33m"
#define C_RED     "\033[31m"
#define C_BOLD    "\033[1m"
#define C_DIM     "\033[2m"

/* ─── Dynamic string buffer (for accumulating source) ─── */
typedef struct {
    char *data;
    int len;
    int cap;
} ReplBuf;

static int repl_copy_str(char *dst, size_t dst_size, const char *src) {
    size_t len;
    if (!dst || dst_size == 0 || !src) return 0;
    len = strlen(src);
    if (len >= dst_size) {
        memcpy(dst, src, dst_size - 1);
        dst[dst_size - 1] = '\0';
        return 0;
    }
    memcpy(dst, src, len + 1);
    return 1;
}

static int repl_append_str(char *dst, size_t dst_size, const char *suffix) {
    size_t used, add;
    if (!dst || dst_size == 0 || !suffix) return 0;
    used = strlen(dst);
    add = strlen(suffix);
    if (used >= dst_size) return 0;
    if (used + add >= dst_size) {
        size_t room = dst_size - used - 1;
        if (room > 0) memcpy(dst + used, suffix, room);
        dst[dst_size - 1] = '\0';
        return 0;
    }
    memcpy(dst + used, suffix, add + 1);
    return 1;
}

static int repl_join2(char *dst, size_t dst_size, const char *a, const char *b) {
    return repl_copy_str(dst, dst_size, a) && repl_append_str(dst, dst_size, b);
}

static int repl_make_include_flag(char *dst, size_t dst_size, const char *runtime_inc) {
    return repl_join2(dst, dst_size, "-I", runtime_inc);
}

static void rbuf_init(ReplBuf *b) {
    b->cap = 4096;
    b->data = (char *)malloc(b->cap);
    b->data[0] = '\0';
    b->len = 0;
}

static void rbuf_append(ReplBuf *b, const char *s) {
    int slen = (int)strlen(s);
    while (b->len + slen + 1 > b->cap) {
        b->cap *= 2;
        b->data = (char *)realloc(b->data, b->cap);
    }
    memcpy(b->data + b->len, s, slen);
    b->len += slen;
    b->data[b->len] = '\0';
}

static void rbuf_free(ReplBuf *b) {
    free(b->data);
    b->data = NULL;
    b->len = b->cap = 0;
}

/* ─── Mockable hooks for testing ─── */
typedef char* (*ReadLineFunc)(const char *prompt);
typedef int (*EvalFunc)(const char *source, const char *gcc, const char *runtime_inc);

static char *default_read_line(const char *prompt);
static int repl_eval(const char *source, const char *gcc, const char *runtime_inc);

static ReadLineFunc current_read_line;
static EvalFunc current_eval;
static int repl_suppress_output = 0;

/* ─── GCC finder (same logic as main.c) ─── */
static const char *repl_find_gcc(void) {
#ifdef _WIN32
    FILE *f = fopen("C:\\msys64\\ucrt64\\bin\\gcc.exe", "rb");
    if (f) {
        fclose(f);
        const char *old = getenv("PATH");
        size_t cap = strlen(old ? old : "") + 100;
        char *new_path = (char *)malloc(cap);
        if (!new_path) return "C:\\msys64\\ucrt64\\bin\\gcc.exe";
        snprintf(new_path, cap, "PATH=C:\\msys64\\ucrt64\\bin;%s", old ? old : "");
        _putenv(new_path);
        free(new_path);
        return "C:\\msys64\\ucrt64\\bin\\gcc.exe";
    }
#endif
    {
        const char *gcc_argv[] = {"gcc", "--version", NULL};
        if (lp_run_silent("gcc", gcc_argv)) return "gcc";

        const char *cc_argv[] = {"cc", "--version", NULL};
        if (lp_run_silent("cc", cc_argv)) return "cc";
    }
    return NULL;
}

/* ─── Find runtime include directory ─── */
static int repl_find_runtime(const char *argv0, char *out, int outsize) {
    char exe_dir[512];
    const char *sep = strrchr(argv0, '\\');
    if (!sep) sep = strrchr(argv0, '/');
    if (sep) {
        int len = (int)(sep - argv0);
        if (len >= (int)sizeof(exe_dir)) len = (int)sizeof(exe_dir) - 1;
        memcpy(exe_dir, argv0, len);
        exe_dir[len] = '\0';
    } else {
        exe_dir[0] = '.'; exe_dir[1] = '\0';
    }

    /* Try relative: ../runtime */
    char try_path[600];
    if (!repl_join2(try_path, sizeof(try_path), exe_dir, LP_PATH_SEP_STR ".." LP_PATH_SEP_STR "runtime" LP_PATH_SEP_STR "lp_runtime.h")) {
        return 0;
    }
    FILE *rf = fopen(try_path, "r");
    if (rf) {
        fclose(rf);
        return repl_join2(out, (size_t)outsize, exe_dir, LP_PATH_SEP_STR ".." LP_PATH_SEP_STR "runtime");
    }

    /* Try known paths */
    const char *candidates[] = { "runtime", "./runtime", "d:\\LP\\runtime", "d:/LP/runtime" };
    for (int i = 0; i < 4; i++) {
        if (!repl_join2(try_path, sizeof(try_path), candidates[i], LP_PATH_SEP_STR "lp_runtime.h")) continue;
        rf = fopen(try_path, "r");
        if (rf) { fclose(rf); return repl_copy_str(out, (size_t)outsize, candidates[i]); }
        snprintf(try_path, sizeof(try_path), "%s/lp_runtime.h", candidates[i]);
        rf = fopen(try_path, "r");
        if (rf) { fclose(rf); return repl_copy_str(out, (size_t)outsize, candidates[i]); }
    }
    return 0;
}

/* ─── Check if line starts a block (needs continuation) ─── */
static int is_block_starter(const char *line) {
    /* Skip leading whitespace */
    while (*line == ' ' || *line == '\t') line++;
    /* Skip empty lines and comments */
    if (*line == '\0' || *line == '#') return 0;
    /* Check for block-starting keywords */
    if (strncmp(line, "def ", 4) == 0) return 1;
    if (strncmp(line, "class ", 6) == 0) return 1;
    if (strncmp(line, "if ", 3) == 0) return 1;
    if (strncmp(line, "elif ", 5) == 0) return 1;
    if (strncmp(line, "else:", 5) == 0) return 1;
    if (strncmp(line, "for ", 4) == 0) return 1;
    if (strncmp(line, "while ", 6) == 0) return 1;
    if (strncmp(line, "try:", 4) == 0) return 1;
    if (strncmp(line, "except", 6) == 0) return 1;
    if (strncmp(line, "finally:", 8) == 0) return 1;
    if (strncmp(line, "with ", 5) == 0) return 1;
    return 0;
}

/* ─── Check if line is only whitespace ─── */
static int is_blank(const char *line) {
    while (*line) {
        if (*line != ' ' && *line != '\t' && *line != '\r' && *line != '\n') return 0;
        line++;
    }
    return 1;
}

/* ─── Try to compile and run accumulated source ─── */
static int repl_eval(const char *source, const char *gcc, const char *runtime_inc) {
    /* Parse */
    LpArena *arena = lp_memory_arena_new(1024 * 1024);
    Parser parser;
    parser_init(&parser, source, arena);
    AstNode *program = parser_parse(&parser);

    if (parser.had_error) {
        if (!repl_suppress_output) fprintf(stderr, C_RED "  Parse error: %s" C_RESET "\n", parser.error_msg);
        ast_free(program); lp_memory_arena_free(arena);
        return -1;
    }

    /* Generate C code */
    CodeGen cg;
    codegen_init(&cg);
    codegen_generate(&cg, program);
    char *c_code = codegen_get_output(&cg);

    if (cg.had_error) {
        fprintf(stderr, C_RED "  Codegen error: %s" C_RESET "\n", cg.error_msg);
        free(c_code);
        codegen_free(&cg);
        ast_free(program); lp_memory_arena_free(arena);
        return -1;
    }

    /* Write temp C file */
    const char *tmp_c = "__lp_repl.c";
    FILE *tmp = fopen(tmp_c, "w");
    if (!tmp) {
        fprintf(stderr, C_RED "  Error: cannot write temp file" C_RESET "\n");
        free(c_code);
        codegen_free(&cg);
        ast_free(program); lp_memory_arena_free(arena);
        return -1;
    }
    fputs(c_code, tmp);
    fclose(tmp);

    /* Compile */
    char inc_flag[600];
    if (!repl_make_include_flag(inc_flag, sizeof(inc_flag), runtime_inc)) {
        fprintf(stderr, C_RED "  Include path too long" C_RESET "\n");
        remove(tmp_c);
        free(c_code);
        codegen_free(&cg);
        ast_free(program); lp_memory_arena_free(arena);
        return -1;
    }

    char exe_path[64];
    if (!repl_join2(exe_path, sizeof(exe_path), "__lp_repl", LP_HOST_EXE_EXT)) {
        fprintf(stderr, C_RED "  Output path too long" C_RESET "\n");
        remove(tmp_c);
        free(c_code);
        codegen_free(&cg);
        ast_free(program); lp_memory_arena_free(arena);
        return -1;
    }
    int ret = (int)_spawnl(_P_WAIT, gcc, "gcc",
        "-std=c99", "-O2", "-w", tmp_c, inc_flag, "-o", exe_path, "-lm", (const char *)NULL);

    if (ret != 0) {
        fprintf(stderr, C_RED "  Compilation failed" C_RESET "\n");
        remove(tmp_c);
        free(c_code);
        codegen_free(&cg);
        ast_free(program); lp_memory_arena_free(arena);
        return -1;
    }

    /* Execute */
    fflush(stdout);
    ret = (int)_spawnl(_P_WAIT, exe_path, exe_path, (const char *)NULL);

    /* Cleanup */
    remove(tmp_c);
    remove(exe_path);
    free(c_code);
    codegen_free(&cg);
    ast_free(program); lp_memory_arena_free(arena);
    return ret;
}

/* ─── Print help ─── */
static void repl_help(void) {
    printf("\n");
    printf(C_BOLD "  LP Interactive Commands:" C_RESET "\n");
    printf("    .help     Show this help message\n");
    printf("    .clear    Clear all accumulated state\n");
    printf("    .show     Show accumulated source code\n");
    printf("    .exit     Exit the REPL (or Ctrl+C)\n");
    printf("\n");
    printf(C_BOLD "  LP Syntax:" C_RESET "\n");
    printf("    x: int = 42            Variable declaration\n");
    printf("    print(\"Hello!\")         Print output\n");
    printf("    def foo(n: int) -> int: Function definition\n");
    printf("        return n * 2        (multi-line with indentation)\n");
    printf("                            (blank line to end block)\n");
    printf("\n");
}

/* ─── Print banner ─── */
static void repl_banner(void) {
    printf("\n");
    printf(C_BOLD C_CYAN "  ╔═══════════════════════════════════════╗" C_RESET "\n");
    printf(C_BOLD C_CYAN "  ║" C_RESET C_BOLD "     LP Language v0.2 — Interactive     " C_CYAN "║" C_RESET "\n");
    printf(C_BOLD C_CYAN "  ║" C_RESET C_DIM "     Type .help for commands            " C_CYAN "║" C_RESET "\n");
    printf(C_BOLD C_CYAN "  ╚═══════════════════════════════════════╝" C_RESET "\n");
    printf("\n");
}

/* ─── Read a line from stdin (with prompt) ─── */
static char *default_read_line(const char *prompt) {
    printf("%s", prompt);
    fflush(stdout);

    static char line[4096];
    if (!fgets(line, sizeof(line), stdin)) return NULL;

    /* Remove trailing newline */
    int len = (int)strlen(line);
    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
        line[--len] = '\0';

    return line;
}

/* ─── Check if line starts a definition (def/class/import) ─── */
static int is_definition(const char *line) {
    while (*line == ' ' || *line == '\t') line++;
    if (strncmp(line, "def ", 4) == 0) return 1;
    if (strncmp(line, "class ", 6) == 0) return 1;
    if (strncmp(line, "import ", 7) == 0) return 1;
    return 0;
}

/* ─── Main REPL loop ─── */
int repl_run(const char *argv0) {
    if (!current_read_line) current_read_line = default_read_line;
    if (!current_eval) current_eval = repl_eval;

    /* Setup */
    const char *gcc = repl_find_gcc();
    if (!gcc) {
        fprintf(stderr, C_RED "Error: GCC not found. Install MSYS2 or MinGW." C_RESET "\n");
        return 1;
    }

    char runtime_inc[512];
    if (!repl_find_runtime(argv0, runtime_inc, sizeof(runtime_inc))) {
        fprintf(stderr, C_RED "Error: Cannot find LP runtime headers." C_RESET "\n");
        return 1;
    }

    /* Enable ANSI colors on Windows */
#ifdef _WIN32
    system(""); /* Trick to enable VT100 sequences on Windows 10+ */
#endif

    if (!repl_suppress_output) repl_banner();

    /* 
     * Two-buffer approach:
     *   defs — function/class/import definitions (accumulated permanently)
     *   exec — executable statements (accumulated, all re-run each time)
     * 
     * To avoid duplicate output, we track which exec statements have already
     * been "seen" and suppress their output by wrapping old statements in a
     * devnull redirect. Actually, a simpler approach: only run the NEW input,
     * but inject variable initializations from previous execs.
     *
     * Simplest correct approach: accumulate everything, accept the re-run.
     * But improve UX by only showing output from the LAST statement.
     * We do this by redirecting stdout to /dev/null for old code, then
     * restoring it for new code.
     *
     * Actually, the cleanest approach for a compiled language REPL:
     * Keep ALL accumulated code and re-run everything. This is how
     * many compiled-language REPLs work (e.g., Cling for C++).
     * The key insight: each new input -> full recompile + run.
     * Users expect this behavior from compiled language REPLs.
     */
    ReplBuf accumulated;
    rbuf_init(&accumulated);

    char prompt_main[64];
    char prompt_cont[64];
    snprintf(prompt_main, sizeof(prompt_main), C_GREEN C_BOLD ">>> " C_RESET);
    snprintf(prompt_cont, sizeof(prompt_cont), C_YELLOW "... " C_RESET);

    while (1) {
        char *line = current_read_line(prompt_main);
        if (!line) { /* EOF (Ctrl+Z on Windows) */
            printf("\n");
            break;
        }

        /* Skip empty lines */
        if (is_blank(line)) continue;

        /* ── Handle dot commands ── */
        if (line[0] == '.') {
            if (strcmp(line, ".exit") == 0 || strcmp(line, ".quit") == 0) {
                break;
            } else if (strcmp(line, ".help") == 0) {
                if (!repl_suppress_output) repl_help();
                continue;
            } else if (strcmp(line, ".clear") == 0) {
                rbuf_free(&accumulated);
                rbuf_init(&accumulated);
                if (!repl_suppress_output) printf(C_DIM "  State cleared." C_RESET "\n");
                continue;
            } else if (strcmp(line, ".show") == 0) {
                if (!repl_suppress_output) {
                    if (accumulated.len == 0) {
                        printf(C_DIM "  (empty)" C_RESET "\n");
                    } else {
                        printf(C_DIM "──────────────────────" C_RESET "\n");
                        printf("%s\n", accumulated.data);
                        printf(C_DIM "──────────────────────" C_RESET "\n");
                    }
                }
                continue;
            } else {
                if (!repl_suppress_output) printf(C_RED "  Unknown command: %s" C_RESET "\n", line);
                continue;
            }
        }

        /* ── Collect multi-line block if this is a block starter ── */
        ReplBuf input;
        rbuf_init(&input);
        rbuf_append(&input, line);
        rbuf_append(&input, "\n");

        if (is_block_starter(line)) {
            /* Read continuation lines until blank line */
            while (1) {
                char *cont = current_read_line(prompt_cont);
                if (!cont) break;
                if (is_blank(cont)) break;
                rbuf_append(&input, cont);
                rbuf_append(&input, "\n");
            }
        }

        /* ── If it's a definition (def/class/import), just add it silently ── */
        if (is_definition(input.data)) {
            /* Try to compile definitions + new def to check for errors */
            ReplBuf full;
            rbuf_init(&full);
            rbuf_append(&full, accumulated.data);
            rbuf_append(&full, input.data);

            /* Quick parse check */
            LpArena *arena = lp_memory_arena_new(1024 * 1024);
            Parser parser;
            parser_init(&parser, full.data, arena);
            AstNode *program = parser_parse(&parser);

            if (parser.had_error) {
                if (!repl_suppress_output) fprintf(stderr, C_RED "  Parse error: %s" C_RESET "\n", parser.error_msg);
            } else {
                rbuf_append(&accumulated, input.data);
            }
            ast_free(program); lp_memory_arena_free(arena);
            rbuf_free(&full);
            rbuf_free(&input);
            continue;
        }

        /* ── Build full source = accumulated + new input ── */
        ReplBuf full;
        rbuf_init(&full);
        rbuf_append(&full, accumulated.data);
        rbuf_append(&full, input.data);

        /* ── Try to compile and run ── */
        int result = current_eval(full.data, gcc, runtime_inc);

        if (result >= 0) {
            /* Success: add new input to accumulated state */
            rbuf_append(&accumulated, input.data);
        }
        /* If error, don't add to accumulated (so user can retry) */

        rbuf_free(&input);
        rbuf_free(&full);
    }

    if (!repl_suppress_output) printf(C_DIM "  Goodbye!" C_RESET "\n\n");
    rbuf_free(&accumulated);
    return 0;
}

/* ─── Internal Unit Tests ─── */

static const char *mock_inputs[20];
static int mock_input_idx = 0;
static char last_eval_source[4096];
static int mock_eval_called = 0;
static int mock_eval_ret = 0;

static char *mock_read_line(const char *prompt) {
    (void)prompt;
    if (mock_inputs[mock_input_idx] == NULL) return NULL;
    static char buf[1024];
    strncpy(buf, mock_inputs[mock_input_idx++], sizeof(buf) - 1);
    return buf;
}

static int mock_eval(const char *source, const char *gcc, const char *runtime_inc) {
    (void)gcc; (void)runtime_inc;
    mock_eval_called++;
    strncpy(last_eval_source, source, sizeof(last_eval_source) - 1);
    return mock_eval_ret;
}

/* A dummy argv0 for repl_run tests */
static const char *test_argv0 = "build/lp";

/* Setup mocks */
static void setup_mocks(void) {
    current_read_line = mock_read_line;
    current_eval = mock_eval;
    repl_suppress_output = 1;
    mock_input_idx = 0;
    mock_eval_called = 0;
    mock_eval_ret = 0;
    memset(mock_inputs, 0, sizeof(mock_inputs));
    memset(last_eval_source, 0, sizeof(last_eval_source));
}

void run_repl_tests(void) {
    char dst[10];
    int res;

    /* Normal copy */
    res = repl_copy_str(dst, sizeof(dst), "hello");
    if (res != 1 || strcmp(dst, "hello") != 0) {
        fprintf(stderr, C_RED "Test failed: repl_copy_str normal copy" C_RESET "\n");
        exit(1);
    }

    /* Exact fit */
    res = repl_copy_str(dst, sizeof(dst), "123456789");
    if (res != 1 || strcmp(dst, "123456789") != 0) {
        fprintf(stderr, C_RED "Test failed: repl_copy_str exact fit" C_RESET "\n");
        exit(1);
    }

    /* Overflow copy */
    res = repl_copy_str(dst, sizeof(dst), "1234567890123");
    if (res != 0 || strcmp(dst, "123456789") != 0) {
        fprintf(stderr, C_RED "Test failed: repl_copy_str overflow" C_RESET "\n");
        exit(1);
    }

    /* Null destination */
    res = repl_copy_str(NULL, sizeof(dst), "hello");
    if (res != 0) {
        fprintf(stderr, C_RED "Test failed: repl_copy_str null dst" C_RESET "\n");
        exit(1);
    }

    /* Zero size */
    res = repl_copy_str(dst, 0, "hello");
    if (res != 0) {
        fprintf(stderr, C_RED "Test failed: repl_copy_str zero size" C_RESET "\n");
        exit(1);
    }

    /* Null source */
    res = repl_copy_str(dst, sizeof(dst), NULL);
    if (res != 0) {
        fprintf(stderr, C_RED "Test failed: repl_copy_str null src" C_RESET "\n");
        exit(1);
    }

    printf("  \033[1mC Utils\033[0m\n");
    printf("    \033[32m[OK] repl_copy_str\033[0m\n");

    /* Test is_block_starter */
    if (!is_block_starter("def foo():")) { fprintf(stderr, "Test failed: is_block_starter def\n"); exit(1); }
    if (!is_block_starter("  class Bar:")) { fprintf(stderr, "Test failed: is_block_starter class\n"); exit(1); }
    if (is_block_starter("x = 1")) { fprintf(stderr, "Test failed: is_block_starter x=1\n"); exit(1); }
    if (is_block_starter("# def foo()")) { fprintf(stderr, "Test failed: is_block_starter comment\n"); exit(1); }

    printf("    \033[32m[OK] is_block_starter\033[0m\n");

    /* Test is_definition */
    if (!is_definition("def myfunc():")) { fprintf(stderr, "Test failed: is_definition def\n"); exit(1); }
    if (!is_definition("import math")) { fprintf(stderr, "Test failed: is_definition import\n"); exit(1); }
    if (is_definition("x = 5")) { fprintf(stderr, "Test failed: is_definition x=5\n"); exit(1); }

    printf("    \033[32m[OK] is_definition\033[0m\n");

    /* Test REPL Run with Mocks */
    printf("\n  \033[1mREPL State Machine\033[0m\n");

    /* 1. Basic command and clear */
    setup_mocks();
    mock_inputs[0] = "x = 10";
    mock_inputs[1] = ".clear";
    mock_inputs[2] = "y = 20";
    mock_inputs[3] = ".exit";
    mock_eval_ret = 0; /* success */
    repl_run(test_argv0);

    if (mock_eval_called != 2) {
        fprintf(stderr, "Test failed: REPL loop eval count %d != 2\n", mock_eval_called);
        exit(1);
    }
    if (strcmp(last_eval_source, "y = 20\n") != 0) {
        fprintf(stderr, "Test failed: REPL loop accumulation failed after .clear\n");
        exit(1);
    }
    printf("    \033[32m[OK] REPL .clear and accumulation\033[0m\n");

    /* 2. Block collection */
    setup_mocks();
    mock_inputs[0] = "if True:";
    mock_inputs[1] = "  print(1)";
    mock_inputs[2] = ""; /* end of block */
    mock_inputs[3] = ".exit";
    repl_run(test_argv0);

    if (mock_eval_called != 1) {
        fprintf(stderr, "Test failed: REPL block collection eval count %d != 1\n", mock_eval_called);
        exit(1);
    }
    if (strstr(last_eval_source, "if True:\n  print(1)\n") == NULL) {
        fprintf(stderr, "Test failed: REPL block collection source mismatch\n");
        exit(1);
    }
    printf("    \033[32m[OK] REPL block collection\033[0m\n");

    /* 3. Definitions are not evaluated, just accumulated */
    setup_mocks();
    mock_inputs[0] = "def add(a: int, b: int) -> int:";
    mock_inputs[1] = "  return a + b";
    mock_inputs[2] = ""; /* end of block */
    mock_inputs[3] = "x = add(1, 2)";
    mock_inputs[4] = ".exit";
    repl_run(test_argv0);

    if (mock_eval_called != 1) {
        fprintf(stderr, "Test failed: REPL definition eval count %d != 1\n", mock_eval_called);
        exit(1);
    }
    if (strstr(last_eval_source, "def add") == NULL || strstr(last_eval_source, "x = add") == NULL) {
        fprintf(stderr, "Test failed: REPL definition accumulation source mismatch\n");
        exit(1);
    }
    printf("    \033[32m[OK] REPL definition accumulation\033[0m\n");

    /* Restore defaults */
    current_read_line = default_read_line;
    current_eval = repl_eval;
    repl_suppress_output = 0;

    printf("\n");
}
