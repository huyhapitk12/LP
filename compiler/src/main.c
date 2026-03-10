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
#include "parser.h"
#include "codegen.h"

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

int main(int argc, char **argv) {
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
            printf("LP Language v0.1\n");
            printf("Accepts .lp and .py files\n\n");
            printf("Usage:\n");
            printf("  lp <file.lp|.py>            Run directly\n");
            printf("  lp <file.lp|.py> -o out.c   Generate C code\n");
            printf("  lp <file.lp|.py> -c out.exe Compile to executable\n");
            printf("  lp <file.lp|.py> -asm out.s Generate assembly\n");
            return 0;
        } else {
            input_file = argv[i];
        }
    }

    if (!input_file) {
        printf("LP Language v0.1\n");
        printf("Usage: lp <file.lp|.py> [-o output.c] [-c output.exe] [-asm output.s]\n");
        return 1;
    }

    /* Read source */
    char *source = read_file(input_file);
    if (!source) return 1;

    /* Parse */
    Parser parser;
    parser_init(&parser, source);
    AstNode *program = parser_parse(&parser);

    if (parser.had_error) {
        fprintf(stderr, "Parse error: %s\n", parser.error_msg);
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
        fprintf(stderr, "Codegen error: %s\n", cg.error_msg);
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

    int ret;
    if (emit_asm) {
        if (cg.uses_python) {
            ret = (int)_spawnl(_P_WAIT, gcc, "gcc",
                "-std=c99", "-O3", "-march=native", "-fstrict-aliasing", "-funroll-loops", "-ffast-math", "-fomit-frame-pointer", "-S", tmp_c, inc_flag, py_inc_arg, "-o", exe_path, NULL);
        } else {
            ret = (int)_spawnl(_P_WAIT, gcc, "gcc",
                "-std=c99", "-O3", "-march=native", "-fstrict-aliasing", "-funroll-loops", "-ffast-math", "-fomit-frame-pointer", "-S", tmp_c, inc_flag, "-o", exe_path, NULL);
        }
    } else {
        if (cg.uses_python) {
            ret = (int)_spawnl(_P_WAIT, gcc, "gcc",
                "-std=c99", "-O3", "-march=native", "-flto", "-fstrict-aliasing", "-funroll-loops", "-ffast-math", "-fomit-frame-pointer", tmp_c, inc_flag, py_inc_arg, "-o", exe_path, py_lib_arg, "-lm", NULL);
        } else {
            ret = (int)_spawnl(_P_WAIT, gcc, "gcc",
                "-std=c99", "-O3", "-march=native", "-flto", "-fstrict-aliasing", "-funroll-loops", "-ffast-math", "-fomit-frame-pointer", tmp_c, inc_flag, "-o", exe_path, "-lm", NULL);
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

