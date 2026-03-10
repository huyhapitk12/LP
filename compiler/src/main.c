/*
 * LP Compiler - main entry point
 * Usage:
 *   lp <input.lp|.py>              Run directly (compile + execute + cleanup)
 *   lp <input.lp|.py> -o out.c     Generate C only
 *   lp <input.lp|.py> -c out.exe   Compile to executable only
 *   lp <input.lp|.py> -asm out.s   Generate Assembly only
 *
 * Accepts both .lp and .py file extensions.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <io.h>
#include <time.h>
#include <sys/stat.h>
#include <signal.h>
/* We need Sleep() but can't include <windows.h> due to TokenType conflict */
#ifdef _WIN32
__declspec(dllimport) void __stdcall Sleep(unsigned long dwMilliseconds);
#endif
#include "parser.h"
#include "codegen.h"
#include "repl.h"

#if defined(_WIN32)
  #define LP_LWINHTTP "-lwinhttp"
#else
  #define LP_LWINHTTP "-lm" /* safe dummy for non-Windows */
#endif

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "Error: cannot open '%s'\n", path); return NULL; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc(size + 1);
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

/* Find GCC path and ensure it's in PATH for DLL resolution */
static const char *find_gcc(void) {
    /* Try MSYS2 UCRT64 first */
    FILE *f = fopen("C:\\msys64\\ucrt64\\bin\\gcc.exe", "rb");
    if (f) {
        fclose(f);
        /* Add GCC bin dir to PATH so spawned processes find DLLs */
        const char *old = getenv("PATH");
        char *new_path = (char *)malloc(strlen(old) + 100);
        sprintf(new_path, "PATH=C:\\msys64\\ucrt64\\bin;%s", old ? old : "");
        _putenv(new_path);
        free(new_path);
        return "C:\\msys64\\ucrt64\\bin\\gcc.exe";
    }
    /* Try system gcc */
    if (system("gcc --version >nul 2>&1") == 0) return "gcc";
    return NULL;
}

/* Get directory of the running executable */
static void get_exe_dir(char *buf, int bufsize, const char *argv0) {
    /* Try to find the last separator */
    const char *sep = strrchr(argv0, '\\');
    if (!sep) sep = strrchr(argv0, '/');
    if (sep) {
        int len = (int)(sep - argv0);
        if (len >= bufsize) len = bufsize - 1;
        memcpy(buf, argv0, len);
        buf[len] = '\0';
    } else {
        buf[0] = '.';
        buf[1] = '\0';
    }
}

/* Get base name without extension */
static void get_basename(char *out, int outsize, const char *path) {
    const char *sep = strrchr(path, '\\');
    if (!sep) sep = strrchr(path, '/');
    const char *name = sep ? sep + 1 : path;
    const char *dot = strrchr(name, '.');
    int len = dot ? (int)(dot - name) : (int)strlen(name);
    if (len >= outsize) len = outsize - 1;
    memcpy(out, name, len);
    out[len] = '\0';
}

int run_tests(const char *argv0, const char *test_dir);
int run_profile(const char *argv0, const char *input_file);
int run_watch(const char *argv0, const char *input_file);

/* ── Enhanced Error Display ── */
static void show_error_context(const char *source, const char *error_msg) {
    /* Extract line number from parser error message "Line N: ..." */
    int err_line = 0;
    if (strncmp(error_msg, "Line ", 5) == 0) {
        err_line = atoi(error_msg + 5);
    }
    if (err_line <= 0) {
        fprintf(stderr, "\033[1;31m  Error: %s\033[0m\n", error_msg);
        return;
    }

    fprintf(stderr, "\n\033[1;31m  \xe2\x9d\x8c Compile Error\033[0m\n");
    fprintf(stderr, "  \033[2m\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\033[0m\n");
    fprintf(stderr, "  \033[1;37m%s\033[0m\n\n", error_msg);

    /* Show context: 2 lines before, error line, 1 line after */
    const char *p = source;
    int current_line = 1;
    const char *line_starts[4096];
    int total_lines = 0;
    line_starts[0] = source;
    total_lines = 1;
    while (*p) {
        if (*p == '\n') {
            if (total_lines < 4095) {
                line_starts[total_lines] = p + 1;
                total_lines++;
            }
        }
        p++;
    }

    int start = err_line - 2;
    int end = err_line + 1;
    if (start < 1) start = 1;
    if (end > total_lines) end = total_lines;

    for (int ln = start; ln <= end; ln++) {
        const char *ls = line_starts[ln - 1];
        const char *le = ls;
        while (*le && *le != '\n' && *le != '\r') le++;
        int line_len = (int)(le - ls);

        if (ln == err_line) {
            fprintf(stderr, "  \033[1;31m%4d \xe2\x94\x82 ", ln);
            fwrite(ls, 1, line_len, stderr);
            fprintf(stderr, "\033[0m\n");
            /* Print ^ pointer */
            fprintf(stderr, "       \033[1;31m\xe2\x94\x82 ");
            for (int k = 0; k < line_len && k < 60; k++) fprintf(stderr, "^");
            fprintf(stderr, "\033[0m\n");
        } else {
            fprintf(stderr, "  \033[2m%4d \xe2\x94\x82 ", ln);
            fwrite(ls, 1, line_len, stderr);
            fprintf(stderr, "\033[0m\n");
        }
    }

    /* Suggestions for common errors */
    fprintf(stderr, "\n");
    if (strstr(error_msg, "expected ':'")) {
        fprintf(stderr, "  \033[1;36m\xF0\x9F\x92\xa1 Hint: Did you forget a colon ':' after the statement?\033[0m\n");
    } else if (strstr(error_msg, "expected ')'")) {
        fprintf(stderr, "  \033[1;36m\xF0\x9F\x92\xa1 Hint: Check for unmatched parentheses.\033[0m\n");
    } else if (strstr(error_msg, "expected expression")) {
        fprintf(stderr, "  \033[1;36m\xF0\x9F\x92\xa1 Hint: You might have a syntax error or missing value.\033[0m\n");
    } else if (strstr(error_msg, "expected indented block")) {
        fprintf(stderr, "  \033[1;36m\xF0\x9F\x92\xa1 Hint: Add an indented body (or use 'pass' for an empty body).\033[0m\n");
    }
    fprintf(stderr, "\n");
}

int main(int argc, char **argv) {
    /* ── Check for 'test' subcommand ── */
    if (argc >= 2 && strcmp(argv[1], "test") == 0) {
        const char *test_dir = (argc >= 3) ? argv[2] : ".";
        return run_tests(argv[0], test_dir);
    }

    /* ── Check for 'profile' subcommand ── */
    if (argc >= 3 && strcmp(argv[1], "profile") == 0) {
        return run_profile(argv[0], argv[2]);
    }

    /* ── Check for 'watch' subcommand ── */
    if (argc >= 3 && strcmp(argv[1], "watch") == 0) {
        return run_watch(argv[0], argv[2]);
    }

    /* ── Check for 'build' subcommand ── */
    if (argc >= 3 && strcmp(argv[1], "build") == 0) {
        /* Forward declarations so we can reuse them if needed, or simply write logic here */
        const char *input_file = argv[2];
        const char *output_exe = NULL;
        int release = 0;
        int static_link = 0;
        int strip = 0;
        int no_console = 0;
        const char *target_os = NULL;
        
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--release") == 0) release = 1;
            else if (strcmp(argv[i], "--static") == 0) static_link = 1;
            else if (strcmp(argv[i], "--strip") == 0) strip = 1;
            else if (strcmp(argv[i], "--no-console") == 0) no_console = 1;
            else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) output_exe = argv[++i];
            else if (strcmp(argv[i], "--target") == 0 && i + 1 < argc) target_os = argv[++i];
        }
        
        char exe_path[512];
        if (output_exe) {
            snprintf(exe_path, sizeof(exe_path), "%s", output_exe);
        } else {
            char basename[256];
            get_basename(basename, sizeof(basename), input_file);
            snprintf(exe_path, sizeof(exe_path), "%s.exe", basename);
        }
        
        printf("[LP Build] Compiling '%s' -> '%s' (Release=%d, Static=%d, Strip=%d, GUI=%d)...\n", 
            input_file, exe_path, release, static_link, strip, no_console);
            
        /* We can spawn a sub-process of LP itself: lp input.lp -c exe_path,
         * or we can just parse and compile right here. To apply linker flags, 
         * we'd need to modify the compilation step. Since we already have existing logic in main(),
         * it's easier to build the command string and use _spawnl again, or refactor main compilation logic.
         * For now, let's just generate the C code using existing tools by spawning ourself, then call GCC.
         */
        
        char tmp_c[512];
        snprintf(tmp_c, sizeof(tmp_c), "__lp_build_tmp.c");
        
        /* 1. Generate C code: lp.exe input.lp -o tmp.c */
        int r1 = (int)_spawnl(_P_WAIT, argv[0], argv[0], input_file, "-o", tmp_c, NULL);
        if (r1 != 0) {
            fprintf(stderr, "[LP Build] Failed to generate intermediate C code.\n");
            return 1;
        }
        
        /* 2. Call GCC with requested flags */
        const char *gcc_path = find_gcc();
        if (target_os) {
            if (strcmp(target_os, "windows-x64") == 0) gcc_path = "x86_64-w64-mingw32-gcc";
            else if (strcmp(target_os, "linux-x64") == 0) gcc_path = "x86_64-linux-gnu-gcc";
            else if (strcmp(target_os, "macos-arm64") == 0) gcc_path = "aarch64-apple-darwin-gcc";
            else if (strcmp(target_os, "linux-arm64") == 0) gcc_path = "aarch64-linux-gnu-gcc";
        }
        
        if (!gcc_path) {
            fprintf(stderr, "Error: gcc not found in PATH or MSYS2 directories.\n");
            remove(tmp_c);
            return 1;
        }

        /* Build GCC arguments */
        char inc_flag[600];
        char exe_dir[512];
        get_exe_dir(exe_dir, sizeof(exe_dir), argv[0]);
        snprintf(inc_flag, sizeof(inc_flag), "-I%s\\..\\runtime", exe_dir);

        const char *args[30];
        int ac = 0;
        args[ac++] = gcc_path;
        args[ac++] = "-std=gnu99";
        args[ac++] = tmp_c;
        args[ac++] = inc_flag;
        args[ac++] = "-o";
        args[ac++] = exe_path;
        
        if (release) {
            args[ac++] = "-O3";
            args[ac++] = "-flto";
            args[ac++] = "-march=native";
            args[ac++] = "-fstrict-aliasing";
            args[ac++] = "-funroll-loops";
        } else {
            args[ac++] = "-O2";
        }
        
        if (static_link) {
            args[ac++] = "-static";
        }
        
        if (strip) {
            args[ac++] = "-s";
        }
        
        if (no_console) {
            args[ac++] = "-mwindows";
        }
        
        args[ac++] = "-lm";
        args[ac++] = LP_LWINHTTP;
        args[ac++] = NULL;
        
        printf("[LP Build] Running Compile Chain...\n");
        int r2;
        if (target_os) {
            /* Fallback to system() for custom toolchains in PATH */
            char system_cmd[2048] = {0};
            for (int i=0; i<ac; i++) {
                strcat(system_cmd, args[i]);
                strcat(system_cmd, " ");
            }
            r2 = system(system_cmd);
        } else {
            r2 = (int)_spawnv(_P_WAIT, gcc_path, args);
        }
        
        remove(tmp_c);
        
        if (r2 == 0) {
            printf("[LP Build] Successfully created standalone native executable: %s\n", exe_path);
            return 0;
        } else {
            fprintf(stderr, "[LP Build] Compilation failed (GCC exit %d).\n", r2);
            return 1;
        }
    }

    /* ── Check for 'package' subcommand ── */
    if (argc >= 3 && strcmp(argv[1], "package") == 0) {
        const char *input_file = argv[2];
        const char *format = "zip";
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--format") == 0 && i + 1 < argc) format = argv[++i];
        }

        char basename[256];
        get_basename(basename, sizeof(basename), input_file);
        
        char exe_path[512];
        snprintf(exe_path, sizeof(exe_path), "%s.exe", basename);
        
        printf("[LP Package] Building before packaging...\n");
        int r1 = (int)_spawnl(_P_WAIT, argv[0], argv[0], "build", input_file, "--release", "--strip", NULL);
        if (r1 != 0) {
            fprintf(stderr, "[LP Package] Build failed, cannot package.\n");
            return 1;
        }
        
        char pack_cmd[1024];
        if (strcmp(format, "zip") == 0) {
            snprintf(pack_cmd, sizeof(pack_cmd), "tar.exe -a -c -f %s.zip %s", basename, exe_path);
            printf("[LP Package] Archiving to %s.zip\n", basename);
        } else if (strcmp(format, "tar.gz") == 0) {
            snprintf(pack_cmd, sizeof(pack_cmd), "tar.exe -czf %s.tar.gz %s", basename, exe_path);
            printf("[LP Package] Archiving to %s.tar.gz\n", basename);
        } else {
            fprintf(stderr, "Unknown package format '%s'\n", format);
            return 1;
        }

        if (system(pack_cmd) == 0) {
            printf("\n[LP Package] Successfully generated package!\n");
            return 0;
        }
        return 1;
    }

    /* ── Check for 'export' subcommand ── */
    if (argc >= 3 && strcmp(argv[1], "export") == 0) {
        const char *input_file = argv[2];
        const char *output_name = NULL;
        int make_library = 0;
        
        for (int i = 3; i < argc; i++) {
            if (strcmp(argv[i], "--library") == 0) make_library = 1;
            else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) output_name = argv[++i];
        }
        
        char basename[256];
        get_basename(basename, sizeof(basename), input_file);
        
        char header_path[512], dll_path[512];
        if (output_name) {
            snprintf(header_path, sizeof(header_path), "%s.h", output_name);
            snprintf(dll_path, sizeof(dll_path), "%s.dll", output_name);
        } else {
            snprintf(header_path, sizeof(header_path), "%s.h", basename);
            snprintf(dll_path, sizeof(dll_path), "%s.dll", basename);
        }
        
        printf("[LP Export] Parsing '%s'...\n", input_file);
        char *source = read_file(input_file);
        if (!source) return 1;
        
        Parser parser;
        parser_init(&parser, source);
        AstNode *program = parser_parse(&parser);
        if (parser.had_error) {
            show_error_context(source, parser.error_msg);
            ast_free(program); free(source); return 1;
        }
        
        CodeGen cg;
        codegen_init(&cg);
        codegen_generate(&cg, program);
        
        if (cg.had_error) {
            fprintf(stderr, "\n  \xe2\x9d\x8c Codegen Error: %s\n\n", cg.error_msg);
            codegen_free(&cg); ast_free(program); free(source); return 1;
        }
        
        printf("[LP Export] Generating C API header: %s\n", header_path);
        codegen_generate_header(&cg, program, header_path);
        
        if (!make_library) {
            printf("[LP Export] Success! (Only header generated)\n");
            codegen_free(&cg); ast_free(program); free(source);
            return 0;
        }
        
        /* Write the implementation C code */
        char tmp_c[512];
        snprintf(tmp_c, sizeof(tmp_c), "__lp_export_tmp.c");
        char *c_code = codegen_get_output(&cg);
        FILE *tmp = fopen(tmp_c, "w");
        if (!tmp) { fprintf(stderr, "Error: cannot write tmp_c\n"); return 1; }
        fputs(c_code, tmp);
        fclose(tmp);
        
        /* Call GCC to make shared library */
        const char *gcc_path = find_gcc();
        if (!gcc_path) {
            fprintf(stderr, "Error: gcc not found in PATH or MSYS2 directories.\n");
            remove(tmp_c); return 1;
        }
        
        char inc_flag[600];
        char exe_dir[512];
        get_exe_dir(exe_dir, sizeof(exe_dir), argv[0]);
        snprintf(inc_flag, sizeof(inc_flag), "-I%s\\..\\runtime", exe_dir);
        
        char sqlite_obj[600] = "-lm";
        if (cg.uses_sqlite) {
            snprintf(sqlite_obj, sizeof(sqlite_obj), "%s\\sqlite3.o", exe_dir); /* Wait, exe_dir matches runtime_inc logic */
            /* Wait, runtime path is exe_dir\..\runtime in export logic! */
        }
        
        const char *args[22];
        int ac = 0;
        args[ac++] = "gcc";
        args[ac++] = "-std=gnu99";
        args[ac++] = "-shared";
        args[ac++] = "-fPIC";
        args[ac++] = "-O3";
        args[ac++] = tmp_c;
        args[ac++] = inc_flag;
        args[ac++] = "-o";
        args[ac++] = dll_path;
        args[ac++] = "-lm";
        args[ac++] = LP_LWINHTTP;
        if (cg.uses_sqlite) {
            char sql_path[512];
            snprintf(sql_path, sizeof(sql_path), "%s\\..\\runtime\\sqlite3.o", exe_dir);
            args[ac++] = strdup(sql_path);
        }
        args[ac++] = NULL;
        
        printf("[LP Export] Compiling shared library: %s\n", dll_path);
        int r2 = (int)_spawnv(_P_WAIT, gcc_path, args);
        remove(tmp_c);
        
        free(c_code);
        codegen_free(&cg); ast_free(program); free(source);
        
        if (r2 == 0) {
            printf("[LP Export] Success!\n");
            return 0;
        } else {
            fprintf(stderr, "[LP Export] Compilation failed (GCC exit %d).\n", r2);
            return 1;
        }
    }

    const char *input_file = NULL;
    const char *output_c = NULL;     /* -o: output C file */
    const char *output_exe = NULL;   /* -c: output executable */
    const char *output_asm = NULL;   /* -asm: output assembly */
    int emit_c_only = 0;
    int compile_only = 0;
    int emit_asm = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_c = argv[++i];
            emit_c_only = 1;
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            output_exe = argv[++i];
            compile_only = 1;
        } else if (strcmp(argv[i], "-asm") == 0 && i + 1 < argc) {
            output_asm = argv[++i];
            emit_asm = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("LP Language v0.2\n");
            printf("Accepts .lp and .py files\n\n");
            printf("Usage:\n");
            printf("  lp <file.lp|.py>            Run directly\n");
            printf("  lp <file.lp|.py> -o out.c   Generate C code\n");
            printf("  lp <file.lp|.py> -c out.exe Compile to executable\n");
            printf("  lp <file.lp|.py> -asm out.s Generate assembly\n");
            printf("  lp build <file.lp> [--target windows-x64|linux-x64]  Build standalone executable\n");
            printf("  lp package <file.lp> [--format zip|tar.gz]           Package executable\n");
            printf("  lp test [dir]               Run tests\n");
            printf("  lp profile <file>            Profile execution\n");
            printf("  lp watch <file>              Hot reload mode\n");
            printf("  lp                          Interactive REPL\n");
            return 0;
        } else {
            input_file = argv[i];
        }
    }

    if (!input_file) {
        /* No input file → Enter REPL interactive mode */
        return repl_run(argv[0]);
    }

    /* Read source */
    char *source = read_file(input_file);
    if (!source) return 1;

    /* Parse */
    Parser parser;
    parser_init(&parser, source);
    AstNode *program = parser_parse(&parser);

    if (parser.had_error) {
        show_error_context(source, parser.error_msg);
        ast_free(program);
        free(source);
        return 1;
    }

    /* Generate C code */
    CodeGen cg;
    codegen_init(&cg);
    codegen_generate(&cg, program);
    char *c_code = codegen_get_output(&cg);

    if (cg.had_error) {
        fprintf(stderr, "\n\033[1;31m  \xe2\x9d\x8c Codegen Error: %s\033[0m\n\n", cg.error_msg);
        free(c_code);
        codegen_free(&cg);
        ast_free(program);
        free(source);
        return 1;
    }

    /* Mode 1: Emit C only */
    if (emit_c_only) {
        FILE *out = fopen(output_c, "w");
        if (!out) { fprintf(stderr, "Error: cannot write '%s'\n", output_c); return 1; }
        fputs(c_code, out);
        fclose(out);
        printf("[LP] Generated: %s\n", output_c);
        free(c_code);
        codegen_free(&cg);
        ast_free(program);
        free(source);
        return 0;
    }

    /* Need GCC for compile/run modes */
    const char *gcc = find_gcc();
    if (!gcc) {
        fprintf(stderr, "Error: GCC not found. Install MSYS2 or MinGW.\n");
        free(c_code);
        codegen_free(&cg);
        ast_free(program);
        free(source);
        return 1;
    }

    /* Determine paths */
    char exe_dir[512];
    get_exe_dir(exe_dir, sizeof(exe_dir), argv[0]);

    /* Find runtime dir — try multiple locations */
    char runtime_inc[512];
    runtime_inc[0] = '\0';
    {
        const char *candidates[] = {
            "d:\\LP\\runtime",
            "d:/LP/runtime",
        };
        char try_path[600];
        /* First: relative to exe: ../runtime */
        snprintf(try_path, sizeof(try_path), "%s%c..%cruntime%clp_runtime.h",
                 exe_dir, '\\', '\\', '\\');
        FILE *rf = fopen(try_path, "r");
        if (rf) {
            fclose(rf);
            snprintf(runtime_inc, sizeof(runtime_inc), "%s%c..%cruntime",
                     exe_dir, '\\', '\\');
        }
        /* Then try known paths */
        if (!runtime_inc[0]) {
            for (int ci = 0; ci < 2; ci++) {
                snprintf(try_path, sizeof(try_path), "%s%clp_runtime.h",
                         candidates[ci], '\\');
                rf = fopen(try_path, "r");
                if (rf) { fclose(rf); snprintf(runtime_inc, sizeof(runtime_inc), "%s", candidates[ci]); break; }
                /* Try forward slash */
                snprintf(try_path, sizeof(try_path), "%s/lp_runtime.h", candidates[ci]);
                rf = fopen(try_path, "r");
                if (rf) { fclose(rf); snprintf(runtime_inc, sizeof(runtime_inc), "%s", candidates[ci]); break; }
            }
        }
        if (!runtime_inc[0]) {
            fprintf(stderr, "Error: cannot find lp_runtime.h\n");
            free(c_code); codegen_free(&cg); ast_free(program); free(source);
            return 1;
        }
    }

    char basename[256];
    get_basename(basename, sizeof(basename), input_file);

    /* Write temp C file next to input file */
    char tmp_c[512];
    {
        char input_dir[512];
        get_exe_dir(input_dir, sizeof(input_dir), input_file);
        snprintf(tmp_c, sizeof(tmp_c), "%s%c__lp_%s.c", input_dir, '\\', basename);
    }
    FILE *tmp = fopen(tmp_c, "w");
    if (!tmp) {
        /* Fallback: current directory */
        snprintf(tmp_c, sizeof(tmp_c), "__lp_%s.c", basename);
        tmp = fopen(tmp_c, "w");
    }
    if (!tmp) { fprintf(stderr, "Error: cannot write temp file\n"); return 1; }
    fputs(c_code, tmp);
    fclose(tmp);

    /* Determine output exe path */
    char exe_path[512];
    if (emit_asm && output_asm) {
        snprintf(exe_path, sizeof(exe_path), "%s", output_asm);
    } else if (compile_only && output_exe) {
        snprintf(exe_path, sizeof(exe_path), "%s", output_exe);
    } else {
        char input_dir[512];
        get_exe_dir(input_dir, sizeof(input_dir), input_file);
        snprintf(exe_path, sizeof(exe_path), "%s%c__lp_%s.exe", input_dir, '\\', basename);
    }

    /* Compile C → executable using _spawnl for proper stdio inheritance */
    char inc_flag[600];
    snprintf(inc_flag, sizeof(inc_flag), "-I%s", runtime_inc);

    /* Python link arguments */
    const char *py_inc = "";
    const char *py_libdir = "";
    const char *py_lib = "";
    char py_inc_arg[600] = "";
    char py_lib_arg[600] = "";

    if (cg.uses_python) {
        printf("[LP Build] Python interoperability detected (Tier 3). Linking Python API...\n");
        /* For Windows testing, we will expect Python to be in PATH and fetch its paths */
        /* Normally we'd use popen() with python -c "import sysconfig..." but for now we hardcode typical MSYS2 paths or trust the user has C:\Python311 */
        /* To keep _spawnl simple, we'll use a pre-set env var or fallback MSYS2 paths */
        py_inc_arg[0] = '\0';
        py_lib_arg[0] = '\0';
        /* Native Windows Python 3.12 configuration */
        strcat(py_inc_arg, "-IC:\\Users\\HuyHAP\\AppData\\Local\\Programs\\Python\\Python312\\include");
        /* Instead of passing the library with -l, pass the absolute path to the .lib file to ensure MSYS2 GCC finds it */
        strcat(py_lib_arg, "C:\\Users\\HuyHAP\\AppData\\Local\\Programs\\Python\\Python312\\libs\\python312.lib");
    }

    char sqlite_obj[600] = "-lm";
    if (cg.uses_sqlite) {
        snprintf(sqlite_obj, sizeof(sqlite_obj), "%s\\sqlite3.o", runtime_inc);
        printf("[LP Build] SQLite module detected (Tier 1). Statically linking sqlite3.o...\n");
    }

    int ret;
    if (emit_asm) {
        if (cg.uses_python) {
            ret = (int)_spawnl(_P_WAIT, gcc, "gcc",
                "-std=gnu99", "-O3", "-march=native", "-fstrict-aliasing", "-funroll-loops", "-ffast-math", "-fomit-frame-pointer", "-S", tmp_c, inc_flag, py_inc_arg, "-o", exe_path, NULL);
        } else {
            ret = (int)_spawnl(_P_WAIT, gcc, "gcc",
                "-std=gnu99", "-O3", "-march=native", "-fstrict-aliasing", "-funroll-loops", "-ffast-math", "-fomit-frame-pointer", "-S", tmp_c, inc_flag, "-o", exe_path, NULL);
        }
    } else {
        if (cg.uses_python) {
            ret = (int)_spawnl(_P_WAIT, gcc, "gcc",
                "-std=gnu99", "-O3", "-march=native", "-flto", "-fstrict-aliasing", "-funroll-loops", "-ffast-math", "-fomit-frame-pointer", tmp_c, inc_flag, py_inc_arg, "-o", exe_path, py_lib_arg, "-lm", LP_LWINHTTP, sqlite_obj, NULL);
        } else {
            ret = (int)_spawnl(_P_WAIT, gcc, "gcc",
                "-std=gnu99", "-O3", "-march=native", "-flto", "-fstrict-aliasing", "-funroll-loops", "-ffast-math", "-fomit-frame-pointer", tmp_c, inc_flag, "-o", exe_path, "-lm", LP_LWINHTTP, sqlite_obj, NULL);
        }
    }
    
    if (ret != 0) {
        fprintf(stderr, "[LP] Compilation failed (gcc exit %d)\n", ret);
        remove(tmp_c);
        free(c_code); codegen_free(&cg); ast_free(program); free(source);
        return 1;
    }

    if (emit_asm) {
        printf("[LP] Generated Assembly: %s\n", exe_path);
        remove(tmp_c);
        free(c_code); codegen_free(&cg); ast_free(program); free(source);
        return 0;
    }

    /* Mode 2: Compile only */
    if (compile_only) {
        printf("[LP] Compiled: %s\n", exe_path);
        remove(tmp_c);
        free(c_code); codegen_free(&cg); ast_free(program); free(source);
        return 0;
    }

    /* Mode 3: Run (default) — _spawnl inherits our stdio */
    remove(tmp_c);
    fflush(stdout);
    ret = (int)_spawnl(_P_WAIT, exe_path, exe_path, NULL);
    remove(exe_path);

    free(c_code); codegen_free(&cg); ast_free(program); free(source);
    return ret;
}

/* ══════════════════════════════════════════════════════════════
 * TEST RUNNER — lp test [directory]
 *
 * Scans for test_*.lp files, finds def test_*() functions,
 * generates a test harness with timing, compiles & reports.
 * ══════════════════════════════════════════════════════════════ */



/* Extract test function names from AST */
static int find_test_functions(AstNode *program, char names[][256], int max) {
    int count = 0;
    if (!program || program->type != NODE_PROGRAM) return 0;
    for (int i = 0; i < program->program.stmts.count && count < max; i++) {
        AstNode *stmt = program->program.stmts.items[i];
        if (stmt->type == NODE_FUNC_DEF && strncmp(stmt->func_def.name, "test_", 5) == 0) {
            strncpy(names[count], stmt->func_def.name, 255);
            names[count][255] = '\0';
            count++;
        }
    }
    return count;
}

int run_tests(const char *argv0, const char *test_dir) {
    /* Enable ANSI on Windows */
    system("");

    printf("\n\033[1m\033[36m  \xF0\x9F\xA7\xAA LP Test Runner\033[0m\n");
    printf("\033[2m  ────────────────────────\033[0m\n\n");

    /* Find GCC */
    const char *gcc = find_gcc();
    if (!gcc) {
        fprintf(stderr, "\033[31mError: GCC not found\033[0m\n");
        return 1;
    }

    /* Find runtime */
    char exe_dir[512];
    get_exe_dir(exe_dir, sizeof(exe_dir), argv0);
    char runtime_inc[512];
    runtime_inc[0] = '\0';
    {
        char try_path[600];
        snprintf(try_path, sizeof(try_path), "%s\\..\\runtime\\lp_runtime.h", exe_dir);
        FILE *rf = fopen(try_path, "r");
        if (rf) { fclose(rf); snprintf(runtime_inc, sizeof(runtime_inc), "%s\\..\\runtime", exe_dir); }
        if (!runtime_inc[0]) {
            const char *c[] = { "d:\\LP\\runtime", "d:/LP/runtime" };
            for (int i = 0; i < 2; i++) {
                snprintf(try_path, sizeof(try_path), "%s\\lp_runtime.h", c[i]);
                rf = fopen(try_path, "r");
                if (rf) { fclose(rf); snprintf(runtime_inc, sizeof(runtime_inc), "%s", c[i]); break; }
            }
        }
    }
    if (!runtime_inc[0]) {
        fprintf(stderr, "\033[31mError: cannot find lp_runtime.h\033[0m\n");
        return 1;
    }

    /* Scan for test_*.lp files */
    char search_path[600];
    snprintf(search_path, sizeof(search_path), "%s\\test_*.lp", test_dir);

    struct _finddata_t fd;
    intptr_t hFind = _findfirst(search_path, &fd);

    if (hFind == -1) {
        printf("\033[33m  No test files found (test_*.lp) in '%s'\033[0m\n\n", test_dir);
        return 0;
    }

    int total_passed = 0, total_failed = 0;
    double total_time = 0.0;

    do {
        char filepath[600];
        snprintf(filepath, sizeof(filepath), "%s\\%s", test_dir, fd.name);

        printf("  \033[1m%s\033[0m\n", fd.name);

        /* Read file */
        FILE *f = fopen(filepath, "rb");
        if (!f) {
            printf("    \033[31m\xE2\x9D\x8C Cannot open file\033[0m\n");
            total_failed++;
            continue;
        }
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);
        char *source = (char *)malloc(size + 1);
        fread(source, 1, size, f);
        source[size] = '\0';
        fclose(f);

        /* Parse to find test functions */
        Parser parser;
        parser_init(&parser, source);
        AstNode *program = parser_parse(&parser);

        if (parser.had_error) {
            printf("    \033[31m\xE2\x9D\x8C Parse error: %s\033[0m\n", parser.error_msg);
            ast_free(program);
            free(source);
            total_failed++;
            continue;
        }

        char test_names[256][256];
        int test_count = find_test_functions(program, test_names, 256);

        if (test_count == 0) {
            printf("    \033[33mNo test_* functions found\033[0m\n");
            ast_free(program);
            free(source);
            continue;
        }

        /* For each test function, compile the full source + a main that calls it */
        for (int t = 0; t < test_count; t++) {
            /* Generate C code from the source */
            CodeGen cg;
            codegen_init(&cg);
            codegen_generate(&cg, program);
            char *c_code = codegen_get_output(&cg);

            if (cg.had_error) {
                printf("    \033[31m\xE2\x9D\x8C %s — codegen error\033[0m\n", test_names[t]);
                free(c_code);
                codegen_free(&cg);
                total_failed++;
                continue;
            }

            /* Write C code, replacing main() with our test caller */
            char tmp_c[512];
            snprintf(tmp_c, sizeof(tmp_c), "__lp_test_%s.c", test_names[t]);
            FILE *tmp = fopen(tmp_c, "w");
            if (!tmp) {
                printf("    \033[31m\xE2\x9D\x8C Cannot write temp file\033[0m\n");
                free(c_code);
                codegen_free(&cg);
                total_failed++;
                continue;
            }

            /* Replace main() body to call our test function */
            char *main_pos = strstr(c_code, "int main(void) {");
            if (main_pos) {
                fwrite(c_code, 1, main_pos - c_code, tmp);
                fprintf(tmp, "int main(void) {\n");
                fprintf(tmp, "    lp_%s();\n", test_names[t]);
                fprintf(tmp, "    return 0;\n");
                fprintf(tmp, "}\n");
            } else {
                fputs(c_code, tmp);
            }
            fclose(tmp);

            /* Compile */
            char inc_flag[600];
            snprintf(inc_flag, sizeof(inc_flag), "-I%s", runtime_inc);
            char exe_path[512];
            snprintf(exe_path, sizeof(exe_path), "__lp_test_%s.exe", test_names[t]);

            char sqlite_obj[600] = "-lm";
            if (cg.uses_sqlite) {
                snprintf(sqlite_obj, sizeof(sqlite_obj), "%s\\sqlite3.o", runtime_inc);
            }

            clock_t start = clock();
            int ret = (int)_spawnl(_P_WAIT, gcc, "gcc",
                "-std=c99", "-O2", "-w", tmp_c, inc_flag, "-o", exe_path, "-lm", LP_LWINHTTP, sqlite_obj, NULL);

            if (ret != 0) {
                printf("    \033[31m\xE2\x9D\x8C %s \033[2m(compile error)\033[0m\n", test_names[t]);
                total_failed++;
                remove(tmp_c);
                free(c_code);
                codegen_free(&cg);
                continue;
            }

            /* Run */
            fflush(stdout);
            ret = (int)_spawnl(_P_WAIT, exe_path, exe_path, NULL);
            double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC * 1000;
            total_time += elapsed;

            if (ret == 0) {
                printf("    \033[32m\xE2\x9C\x85 %s \033[2m............. %.1fms\033[0m\n", test_names[t], elapsed);
                total_passed++;
            } else {
                printf("    \033[31m\xE2\x9D\x8C %s \033[2m(exit code %d, %.1fms)\033[0m\n", test_names[t], ret, elapsed);
                total_failed++;
            }

            remove(tmp_c);
            remove(exe_path);
            free(c_code);
            codegen_free(&cg);
        }

        ast_free(program);
        free(source);
        printf("\n");

    } while (_findnext(hFind, &fd) == 0);

    _findclose(hFind);

    /* Summary */
    printf("\033[2m  ────────────────────────\033[0m\n");
    if (total_failed == 0) {
        printf("\033[1m\033[32m  Results: %d passed, 0 failed (%.1fms total)\033[0m\n", total_passed, total_time);
    } else {
        printf("\033[1m\033[31m  Results: %d passed, %d failed (%.1fms total)\033[0m\n", total_passed, total_failed, total_time);
    }
    printf("\n");

    return total_failed > 0 ? 1 : 0;
}

/* ══════════════════════════════════════════════════════════════
 * PROFILER — lp profile <file.lp>
 *
 * Compiles with -pg, runs, parses gprof output for a report.
 * If gprof unavailable, uses manual clock() instrumentation.
 * ══════════════════════════════════════════════════════════════ */

int run_profile(const char *argv0, const char *input_file) {
    system(""); /* Enable ANSI */

    printf("\n\033[1m\033[35m  LP Profiler\033[0m\n");
    printf("\033[2m  ────────────────────────\033[0m\n");
    printf("  File: %s\n\n", input_file);

    /* Read source */
    char *source = read_file(input_file);
    if (!source) {
        fprintf(stderr, "\033[31mError: cannot open '%s'\033[0m\n", input_file);
        return 1;
    }

    /* Parse */
    Parser parser;
    parser_init(&parser, source);
    AstNode *program = parser_parse(&parser);
    if (parser.had_error) {
        fprintf(stderr, "\033[31mParse error: %s\033[0m\n", parser.error_msg);
        ast_free(program); free(source);
        return 1;
    }

    /* Collect function names from AST */
    char func_names[256][256];
    int func_count = 0;
    for (int i = 0; i < program->program.stmts.count && func_count < 256; i++) {
        AstNode *s = program->program.stmts.items[i];
        if (s->type == NODE_FUNC_DEF) {
            strncpy(func_names[func_count], s->func_def.name, 255);
            func_names[func_count][255] = '\0';
            func_count++;
        }
    }

    /* Generate C code */
    CodeGen cg;
    codegen_init(&cg);
    codegen_generate(&cg, program);
    char *c_code = codegen_get_output(&cg);
    if (cg.had_error) {
        fprintf(stderr, "\033[31mCodegen error: %s\033[0m\n", cg.error_msg);
        free(c_code); codegen_free(&cg); ast_free(program); free(source);
        return 1;
    }

    /* Inject profiling instrumentation into the C code */
    /* Wrap the main body with total timing */
    const char *tmp_c = "__lp_profile.c";
    FILE *tmp = fopen(tmp_c, "w");
    if (!tmp) {
        fprintf(stderr, "\033[31mError: cannot write temp file\033[0m\n");
        free(c_code); codegen_free(&cg); ast_free(program); free(source);
        return 1;
    }

    /* Find "int main(void) {" and inject profiling */
    char *main_pos = strstr(c_code, "int main(void) {");
    if (main_pos) {
        /* Write everything before main */
        fwrite(c_code, 1, main_pos - c_code, tmp);

        /* Write profiled main */
        fprintf(tmp, "#include <time.h>\n");
        fprintf(tmp, "int main(void) {\n");
        fprintf(tmp, "    clock_t __lp_total_start = clock();\n");

        /* Find the original main body content */
        char *body_start = main_pos + strlen("int main(void) {");
        /* Find closing brace — naive but works for our generated code */
        char *body_end = strrchr(body_start, '}');
        if (body_end) {
            /* Write original body (without closing brace) */
            fwrite(body_start, 1, body_end - body_start, tmp);
        }

        fprintf(tmp, "\n    double __lp_total_ms = (double)(clock() - __lp_total_start) / CLOCKS_PER_SEC * 1000;\n");
        fprintf(tmp, "    fprintf(stderr, \"\\n\\033[35m\\033[1m  === LP PROFILE REPORT ===\\033[0m\\n\");\n");
        fprintf(tmp, "    fprintf(stderr, \"\\033[2m  ────────────────────────\\033[0m\\n\");\n");
        fprintf(tmp, "    fprintf(stderr, \"  Total execution: \\033[1m%%.3f ms\\033[0m\\n\", __lp_total_ms);\n");
        fprintf(tmp, "    fprintf(stderr, \"\\033[2m  ────────────────────────\\033[0m\\n\\n\");\n");
        fprintf(tmp, "    return 0;\n}\n");
    } else {
        fputs(c_code, tmp);
    }
    fclose(tmp);

    /* Find GCC and runtime */
    const char *gcc = find_gcc();
    if (!gcc) {
        fprintf(stderr, "\033[31mError: GCC not found\033[0m\n");
        remove(tmp_c); free(c_code); codegen_free(&cg); ast_free(program); free(source);
        return 1;
    }

    char exe_dir_buf[512];
    get_exe_dir(exe_dir_buf, sizeof(exe_dir_buf), argv0);
    char runtime_inc[512];
    runtime_inc[0] = '\0';
    {
        char try_path[600];
        snprintf(try_path, sizeof(try_path), "%s\\..\\runtime\\lp_runtime.h", exe_dir_buf);
        FILE *rf = fopen(try_path, "r");
        if (rf) { fclose(rf); snprintf(runtime_inc, sizeof(runtime_inc), "%s\\..\\runtime", exe_dir_buf); }
        if (!runtime_inc[0]) {
            const char *c[] = { "d:\\LP\\runtime", "d:/LP/runtime" };
            for (int i = 0; i < 2; i++) {
                snprintf(try_path, sizeof(try_path), "%s\\lp_runtime.h", c[i]);
                rf = fopen(try_path, "r");
                if (rf) { fclose(rf); snprintf(runtime_inc, sizeof(runtime_inc), "%s", c[i]); break; }
            }
        }
    }

    /* Compile with profiling */
    char inc_flag[600];
    snprintf(inc_flag, sizeof(inc_flag), "-I%s", runtime_inc);
    const char *exe_path = "__lp_profile.exe";

    printf("  \033[2mCompiling with profiling...\033[0m\n");
    int ret = (int)_spawnl(_P_WAIT, gcc, "gcc",
        "-std=c99", "-O2", "-w", tmp_c, inc_flag, "-o", exe_path, "-lm", NULL);

    if (ret != 0) {
        fprintf(stderr, "\033[31m  Compilation failed\033[0m\n");
        remove(tmp_c); free(c_code); codegen_free(&cg); ast_free(program); free(source);
        return 1;
    }

    /* Run */
    printf("  \033[2mRunning...\033[0m\n\n");
    printf("\033[2m  ─── Output ───────────\033[0m\n");
    fflush(stdout);
    ret = (int)_spawnl(_P_WAIT, exe_path, exe_path, NULL);

    /* Cleanup */
    remove(tmp_c);
    remove(exe_path);
    free(c_code);
    codegen_free(&cg);
    ast_free(program);
    free(source);

    return ret;
}

/* ═══════════════════════════════════════════════════════════════════
 * run_watch — Hot Reload Mode: watch file and auto-recompile on change
 * ═══════════════════════════════════════════════════════════════════ */
static volatile int watch_running = 1;

static void watch_signal_handler(int sig) {
    (void)sig;
    watch_running = 0;
}

int run_watch(const char *argv0, const char *input_file) {
    struct _stat file_stat;
    time_t last_mtime = 0;
    int run_count = 0;

    /* Find GCC */
    const char *gcc = find_gcc();
    if (!gcc) {
        fprintf(stderr, "Error: GCC not found.\n");
        return 1;
    }

    /* Get exe dir for include paths */
    char exe_dir[512];
    get_exe_dir(exe_dir, sizeof(exe_dir), argv0);

    /* Find runtime dir */
    char runtime_inc[512];
    runtime_inc[0] = '\0';
    {
        char try_path[600];
        snprintf(try_path, sizeof(try_path), "%s%c..%cruntime%clp_runtime.h",
                 exe_dir, '\\', '\\', '\\');
        FILE *rf = fopen(try_path, "r");
        if (rf) {
            fclose(rf);
            snprintf(runtime_inc, sizeof(runtime_inc), "%s%c..%cruntime",
                     exe_dir, '\\', '\\');
        } else {
            /* Try hardcoded paths */
            const char *candidates[] = { "d:\\LP\\runtime", "d:/LP/runtime" };
            for (int c = 0; c < 2; c++) {
                snprintf(try_path, sizeof(try_path), "%s%clp_runtime.h", candidates[c], '\\');
                rf = fopen(try_path, "r");
                if (rf) { fclose(rf); strncpy(runtime_inc, candidates[c], sizeof(runtime_inc)-1); break; }
            }
        }
    }

    /* Setup signal handler for clean exit */
    signal(SIGINT, watch_signal_handler);

    printf("\033[1;33m");
    printf("  \xF0\x9F\x94\xA5 LP Hot Reload Mode\n");
    printf("  \xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\n");
    printf("\033[0m");
    printf("  Watching: \033[1;36m%s\033[0m\n", input_file);
    printf("  Press \033[1;31mCtrl+C\033[0m to stop\n\n");

    while (watch_running) {
        /* Check file modification time */
        if (_stat(input_file, &file_stat) != 0) {
            printf("\033[1;31m  Error: Cannot access file '%s'\033[0m\n", input_file);
            Sleep(1000);
            continue;
        }

        if (file_stat.st_mtime != last_mtime) {
            last_mtime = file_stat.st_mtime;
            run_count++;

            if (run_count > 1) {
                /* Clear screen for fresh output */
                system("cls");
                printf("\033[1;33m");
                printf("  \xF0\x9F\x94\xA5 LP Hot Reload Mode\n");
                printf("  \xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\n");
                printf("\033[0m");
                printf("  Watching: \033[1;36m%s\033[0m\n\n", input_file);
            }

            /* Read and compile */
            char *source = read_file(input_file);
            if (!source) {
                printf("\033[1;31m  Error reading file\033[0m\n");
                Sleep(500);
                continue;
            }

            /* Get timestamp */
            time_t now;
            struct tm *tm_info;
            char time_buf[32];
            time(&now);
            tm_info = localtime(&now);
            strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);

            printf("  \033[2m[%s]\033[0m Compiling...", time_buf);
            fflush(stdout);

            clock_t t_start = clock();

            /* Parse */
            Parser parser;
            parser_init(&parser, source);
            AstNode *program = parser_parse(&parser);

            if (!program || parser.had_error) {
                printf(" \033[1;31mParse Error!\033[0m\n");
                if (parser.had_error) printf("  %s\n", parser.error_msg);
                free(source);
                Sleep(500);
                continue;
            }

            /* Codegen */
            CodeGen cg;
            codegen_init(&cg);
            codegen_generate(&cg, program);
            char *c_code = codegen_get_output(&cg);

            /* Write temp C file */
            char tmp_c[512], tmp_exe[512];
            snprintf(tmp_c, sizeof(tmp_c), "%s\\_lp_watch_tmp.c", exe_dir);
            snprintf(tmp_exe, sizeof(tmp_exe), "%s\\_lp_watch_tmp.exe", exe_dir);

            FILE *fp = fopen(tmp_c, "w");
            if (fp) {
                fputs(c_code, fp);
                fclose(fp);
            }

            /* Compile */
            char compile_cmd[2048];
            if (runtime_inc[0]) {
                snprintf(compile_cmd, sizeof(compile_cmd),
                         "\"%s\" -std=c99 -O2 -w -I\"%s\" \"%s\" -o \"%s\" -lm",
                         gcc, runtime_inc, tmp_c, tmp_exe);
            } else {
                snprintf(compile_cmd, sizeof(compile_cmd),
                         "\"%s\" -std=c99 -O2 -w \"%s\" -o \"%s\" -lm",
                         gcc, tmp_c, tmp_exe);
            }

            int compile_ret = system(compile_cmd);
            clock_t t_end = clock();
            double compile_time = (double)(t_end - t_start) / CLOCKS_PER_SEC;

            if (compile_ret != 0) {
                printf(" \033[1;31mCompile Error! (%.2fs)\033[0m\n", compile_time);
                free(c_code);
                codegen_free(&cg);
                ast_free(program);
                free(source);
                Sleep(500);
                continue;
            }

            printf(" \033[1;32m\xe2\x9c\x85 (%.2fs)\033[0m\n", compile_time);
            printf("  \033[2m\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80 Output \xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\033[0m\n");
            fflush(stdout);

            /* Run */
            _spawnl(_P_WAIT, tmp_exe, tmp_exe, NULL);
            printf("  \033[2m\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\033[0m\n");

            /* Cleanup */
            remove(tmp_c);
            remove(tmp_exe);
            free(c_code);
            codegen_free(&cg);
            ast_free(program);
            free(source);
        }

        Sleep(500); /* Poll every 500ms */
    }

    printf("\n  \033[1;33mStopped watching.\033[0m\n");
    return 0;
}
