/*
 * LP Error Reporter - Python-like error messages
 * Implementation
 */
#include "error_reporter.h"
#include <ctype.h>

/* ANSI color codes */
#define COLOR_RED     "\033[1;31m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_WHITE   "\033[1;37m"
#define COLOR_DIM     "\033[2m"
#define COLOR_RESET   "\033[0m"

/* Unicode symbols */
#define SYMBOL_ERROR    "❌"
#define SYMBOL_WARNING  "⚠️"
#define SYMBOL_HINT     "💡"
#define SYMBOL_ARROW    "│"
#define SYMBOL_POINTER  "^"

/* Initialize error list */
void lp_error_list_init(LpErrorList *list) {
    list->head = NULL;
    list->tail = NULL;
    list->error_count = 0;
    list->warning_count = 0;
}

/* Add error to list */
void lp_error_list_add(LpErrorList *list, LpErrorCode code, LpErrorSeverity severity,
                       int line, int column, const char *message, const char *hint) {
    lp_error_list_add_range(list, code, severity, line, column, line, column, message, hint);
}

/* Add error with range */
void lp_error_list_add_range(LpErrorList *list, LpErrorCode code, LpErrorSeverity severity,
                             int line, int column, int end_line, int end_column,
                             const char *message, const char *hint) {
    LpError *err = (LpError *)malloc(sizeof(LpError));
    if (!err) return;
    
    err->code = code;
    err->severity = severity;
    err->line = line;
    err->column = column;
    err->end_line = end_line;
    err->end_column = end_column;
    err->message = message ? strdup(message) : NULL;
    err->hint = hint ? strdup(hint) : NULL;
    err->file_path = NULL;
    err->next = NULL;
    
    if (list->tail) {
        list->tail->next = err;
        list->tail = err;
    } else {
        list->head = err;
        list->tail = err;
    }
    
    if (severity == SEVERITY_ERROR) {
        list->error_count++;
    } else if (severity == SEVERITY_WARNING) {
        list->warning_count++;
    }
}

/* Free error list */
void lp_error_list_free(LpErrorList *list) {
    LpError *err = list->head;
    while (err) {
        LpError *next = err->next;
        if (err->message) free(err->message);
        if (err->hint) free(err->hint);
        if (err->file_path) free(err->file_path);
        free(err);
        err = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->error_count = 0;
    list->warning_count = 0;
}

/* Check if has errors */
int lp_error_list_has_errors(LpErrorList *list) {
    return list->error_count > 0;
}

/* Get error code name */
const char *lp_error_code_name(LpErrorCode code) {
    switch (code) {
        /* Syntax Errors */
        case ERR_MISSING_COLON: return "E001";
        case ERR_MISSING_INDENT: return "E002";
        case ERR_UNMATCHED_PAREN: return "E003";
        case ERR_UNMATCHED_BRACKET: return "E004";
        case ERR_UNMATCHED_BRACE: return "E005";
        case ERR_UNEXPECTED_TOKEN: return "E006";
        case ERR_INVALID_SYNTAX: return "E007";
        case ERR_EXPECTED_EXPRESSION: return "E008";
        case ERR_EXPECTED_IDENTIFIER: return "E009";
        case ERR_INVALID_ASSIGNMENT: return "E010";
        
        /* Type Errors */
        case ERR_INVALID_TYPE: return "E011";
        case ERR_MISSING_TYPE_ANNOTATION: return "E012";
        case ERR_TYPE_MISMATCH: return "E013";
        case ERR_INVALID_TYPE_ARGS: return "E014";
        case ERR_UNKNOWN_TYPE: return "E015";
        
        /* Name Errors */
        case ERR_UNKNOWN_MODULE: return "E021";
        case ERR_UNDEFINED_VAR: return "E022";
        case ERR_UNDEFINED_FUNC: return "E023";
        case ERR_REDEFINED_VAR: return "E024";
        case ERR_REDEFINED_FUNC: return "E025";
        
        /* Semantic Errors */
        case ERR_USE_NULL_NOT_NONE: return "E031";
        case ERR_MISSING_DSA_FLUSH: return "E032";
        case ERR_FUNC_NOT_CALLED: return "E033";
        case ERR_INVALID_2D_ARRAY: return "E034";
        case ERR_DIVISION_BY_ZERO: return "E035";
        case ERR_ARRAY_OUT_OF_BOUNDS: return "E036";
        case ERR_INFINITE_RECURSION: return "E037";
        
        /* Warnings */
        case WARN_ASSIGN_IN_COND: return "W001";
        case WARN_UNUSED_VAR: return "W002";
        case WARN_UNUSED_IMPORT: return "W003";
        case WARN_DEAD_CODE: return "W004";
        case WARN_UNREACHABLE_CODE: return "W005";
        case WARN_EMPTY_BODY: return "W006";
        case WARN_DEPRECATED: return "W007";
        case WARN_SHADOW_VAR: return "W008";
        
        default: return "???";
    }
}

/* Get severity name and color */
const char *lp_severity_name(LpErrorSeverity severity) {
    switch (severity) {
        case SEVERITY_ERROR: return "Error";
        case SEVERITY_WARNING: return "Warning";
        case SEVERITY_NOTE: return "Note";
        case SEVERITY_HINT: return "Hint";
        default: return "Unknown";
    }
}

const char *lp_severity_color(LpErrorSeverity severity) {
    switch (severity) {
        case SEVERITY_ERROR: return COLOR_RED;
        case SEVERITY_WARNING: return COLOR_YELLOW;
        case SEVERITY_NOTE: return COLOR_CYAN;
        case SEVERITY_HINT: return COLOR_GREEN;
        default: return COLOR_WHITE;
    }
}

/* Built-in hints for common errors */
const char *lp_get_builtin_hint(LpErrorCode code) {
    switch (code) {
        case ERR_MISSING_COLON:
            return "Add a colon ':' after the statement";
        case ERR_MISSING_INDENT:
            return "Add an indented block (use 4 spaces or tab)";
        case ERR_UNMATCHED_PAREN:
            return "Check for missing or extra parentheses '(' ')'";
        case ERR_UNMATCHED_BRACKET:
            return "Check for missing or extra brackets '[' ']'";
        case ERR_UNMATCHED_BRACE:
            return "Check for missing or extra braces '{' '}'";
        case ERR_INVALID_TYPE:
            return "Use 'int', 'float', 'str', 'list', 'dict', 'bool', or 'null'";
        case ERR_USE_NULL_NOT_NONE:
            return "LP uses 'null' instead of Python's 'None'";
        case ERR_MISSING_DSA_FLUSH:
            return "Add 'dsa.flush()' before program exit when using dsa.write_*";
        case ERR_FUNC_NOT_CALLED:
            return "Add a call to your function at the end of the file";
        case ERR_INVALID_2D_ARRAY:
            return "Use flat array: 'arr[i * row_size + j]' instead of 'arr[i][j]'";
        case ERR_UNKNOWN_MODULE:
            return "Available modules: dsa, math, os, time, random, json, http, sqlite";
        case ERR_MISSING_TYPE_ANNOTATION:
            return "Add type annotation: 'x: int = 5' or 'def f(x: int):'";
        case WARN_ASSIGN_IN_COND:
            return "Did you mean '==' for comparison instead of '=' for assignment?";
        case WARN_UNUSED_VAR:
            return "Remove the variable or use it in your code";
        case WARN_UNUSED_IMPORT:
            return "Remove the unused import statement";
        default:
            return NULL;
    }
}

/* Get source line */
char *lp_get_source_line(const char *source, int line) {
    if (!source || line < 1) return NULL;
    
    const char *p = source;
    int current_line = 1;
    
    /* Find the line */
    while (*p && current_line < line) {
        if (*p == '\n') current_line++;
        p++;
    }
    
    if (!*p) return NULL;
    
    /* Find end of line */
    const char *end = p;
    while (*end && *end != '\n' && *end != '\r') end++;
    
    /* Copy line */
    size_t len = end - p;
    char *result = (char *)malloc(len + 1);
    if (result) {
        memcpy(result, p, len);
        result[len] = '\0';
    }
    return result;
}

/* Count source lines */
int lp_count_source_lines(const char *source) {
    if (!source) return 0;
    int count = 1;
    while (*source) {
        if (*source == '\n') count++;
        source++;
    }
    return count;
}

/* Find line and column from offset */
void lp_find_line_column(const char *source, int offset, int *line, int *column) {
    if (!source || !line || !column) return;
    
    *line = 1;
    *column = 1;
    
    for (int i = 0; i < offset && source[i]; i++) {
        if (source[i] == '\n') {
            (*line)++;
            *column = 1;
        } else {
            (*column)++;
        }
    }
}

/* Print single error with context */
void lp_print_error_with_context(LpError *err, const char *source, FILE *out) {
    if (!err || !out) return;
    
    const char *color = lp_severity_color(err->severity);
    const char *symbol = (err->severity == SEVERITY_ERROR) ? SYMBOL_ERROR : SYMBOL_WARNING;
    const char *severity_text = lp_severity_name(err->severity);
    
    /* Header */
    fprintf(out, "\n");
    fprintf(out, "%s  %s %s%s%s\n", color, symbol, severity_text, 
            (err->severity == SEVERITY_ERROR) ? "" : "", COLOR_RESET);
    fprintf(out, "%s  ────────────────────────────────────%s\n", COLOR_DIM, COLOR_RESET);
    
    /* Error message with code */
    fprintf(out, "  %s[%s]%s Line %d: %s\n", 
            COLOR_WHITE, lp_error_code_name(err->code), COLOR_RESET,
            err->line, err->message ? err->message : "Unknown error");
    
    /* Show context: 2 lines before, error line, 1 line after */
    if (source) {
        fprintf(out, "\n");
        
        int start = err->line - 2;
        if (start < 1) start = 1;
        int end = err->line + 1;
        int total_lines = lp_count_source_lines(source);
        if (end > total_lines) end = total_lines;
        
        /* Calculate line number width */
        int line_num_width = 3;
        int temp = end;
        while (temp > 0) { temp /= 10; line_num_width++; }
        if (line_num_width < 4) line_num_width = 4;
        
        for (int ln = start; ln <= end; ln++) {
            char *line_str = lp_get_source_line(source, ln);
            
            /* Trim trailing whitespace for display */
            int line_len = line_str ? strlen(line_str) : 0;
            while (line_len > 0 && isspace(line_str[line_len - 1])) {
                line_str[--line_len] = '\0';
            }
            
            /* Limit line length for display */
            int display_len = line_len > 70 ? 70 : line_len;
            
            if (ln == err->line) {
                /* Error line - highlighted */
                fprintf(out, "  %s%*d %s ", COLOR_RED, line_num_width, ln, SYMBOL_ARROW);
                if (line_str) {
                    fprintf(out, "%.*s", display_len, line_str);
                }
                fprintf(out, "%s\n", COLOR_RESET);
                
                /* Print pointer */
                fprintf(out, "  %s%*s %s ", COLOR_RED, line_num_width, "", SYMBOL_ARROW);
                
                /* Calculate pointer position */
                int ptr_start = err->column > 0 ? err->column - 1 : 0;
                int ptr_len = 1;
                if (err->end_line == err->line && err->end_column > err->column) {
                    ptr_len = err->end_column - err->column;
                }
                
                /* Print spaces then carets */
                for (int i = 0; i < ptr_start && i < 60; i++) {
                    fprintf(out, " ");
                }
                for (int i = 0; i < ptr_len && (ptr_start + i) < 60; i++) {
                    fprintf(out, "%s", SYMBOL_POINTER);
                }
                fprintf(out, "%s\n", COLOR_RESET);
            } else {
                /* Non-error line - dimmed */
                fprintf(out, "  %s%*d %s ", COLOR_DIM, line_num_width, ln, SYMBOL_ARROW);
                if (line_str) {
                    fprintf(out, "%.*s", display_len, line_str);
                }
                fprintf(out, "%s\n", COLOR_RESET);
            }
            
            if (line_str) free(line_str);
        }
    }
    
    /* Print hint */
    const char *hint = err->hint ? err->hint : lp_get_builtin_hint(err->code);
    if (hint) {
        fprintf(out, "\n");
        fprintf(out, "  %s%s Hint: %s%s\n", COLOR_CYAN, SYMBOL_HINT, hint, COLOR_RESET);
    }
    
    fprintf(out, "\n");
}

/* Print all errors */
void lp_print_all_errors(LpErrorList *list, const char *source, FILE *out) {
    if (!list || !out) return;
    
    LpError *err = list->head;
    while (err) {
        lp_print_error_with_context(err, source, out);
        err = err->next;
    }
    
    /* Summary */
    if (list->error_count > 0 || list->warning_count > 0) {
        fprintf(out, "%s  Found %d error(s)", 
                list->error_count > 0 ? COLOR_RED : COLOR_YELLOW,
                list->error_count);
        if (list->warning_count > 0) {
            fprintf(out, ", %d warning(s)", list->warning_count);
        }
        fprintf(out, "%s\n\n", COLOR_RESET);
    }
}

/* Quick error function - print immediately */
void lp_report_error(LpErrorCode code, int line, int column, 
                     const char *message, const char *hint,
                     const char *source) {
    LpError err;
    err.code = code;
    err.severity = SEVERITY_ERROR;
    err.line = line;
    err.column = column;
    err.end_line = line;
    err.end_column = column;
    err.message = (char *)message;
    err.hint = (char *)hint;
    err.file_path = NULL;
    err.next = NULL;
    
    lp_print_error_with_context(&err, source, stderr);
}

/* Quick warning function - print immediately */
void lp_report_warning(LpErrorCode code, int line, int column,
                       const char *message, const char *hint,
                       const char *source) {
    LpError err;
    err.code = code;
    err.severity = SEVERITY_WARNING;
    err.line = line;
    err.column = column;
    err.end_line = line;
    err.end_column = column;
    err.message = (char *)message;
    err.hint = (char *)hint;
    err.file_path = NULL;
    err.next = NULL;
    
    lp_print_error_with_context(&err, source, stderr);
}
