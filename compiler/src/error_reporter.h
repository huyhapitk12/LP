/*
 * LP Error Reporter - Python-like error messages
 * Provides beautiful, helpful error messages with context and suggestions
 */
#ifndef LP_ERROR_REPORTER_H
#define LP_ERROR_REPORTER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Error codes */
typedef enum {
    /* Syntax Errors E001-E010 */
    ERR_MISSING_COLON = 1,
    ERR_MISSING_INDENT,
    ERR_UNMATCHED_PAREN,
    ERR_UNMATCHED_BRACKET,
    ERR_UNMATCHED_BRACE,
    ERR_UNEXPECTED_TOKEN,
    ERR_INVALID_SYNTAX,
    ERR_EXPECTED_EXPRESSION,
    ERR_EXPECTED_IDENTIFIER,
    ERR_INVALID_ASSIGNMENT,
    
    /* Type Errors E011-E020 */
    ERR_INVALID_TYPE = 11,
    ERR_MISSING_TYPE_ANNOTATION,
    ERR_TYPE_MISMATCH,
    ERR_INVALID_TYPE_ARGS,
    ERR_UNKNOWN_TYPE,
    
    /* Name Errors E021-E030 */
    ERR_UNKNOWN_MODULE = 21,
    ERR_UNDEFINED_VAR,
    ERR_UNDEFINED_FUNC,
    ERR_REDEFINED_VAR,
    ERR_REDEFINED_FUNC,
    
    /* Semantic Errors E031-E040 */
    ERR_USE_NULL_NOT_NONE = 31,
    ERR_MISSING_DSA_FLUSH,
    ERR_FUNC_NOT_CALLED,
    ERR_INVALID_2D_ARRAY,
    ERR_DIVISION_BY_ZERO,
    ERR_ARRAY_OUT_OF_BOUNDS,
    ERR_INFINITE_RECURSION,
    
    /* Native Compilation Errors E051-E060 */
    ERR_NATIVE_COMPILATION = 51,
    
    /* Warnings W001-W010 */
    WARN_ASSIGN_IN_COND = 101,
    WARN_UNUSED_VAR,
    WARN_UNUSED_IMPORT,
    WARN_DEAD_CODE,
    WARN_UNREACHABLE_CODE,
    WARN_EMPTY_BODY,
    WARN_DEPRECATED,
    WARN_SHADOW_VAR
} LpErrorCode;

/* Error severity */
typedef enum {
    SEVERITY_ERROR,
    SEVERITY_WARNING,
    SEVERITY_NOTE,
    SEVERITY_HINT
} LpErrorSeverity;

/* Error structure */
typedef struct LpError {
    LpErrorCode code;
    LpErrorSeverity severity;
    int line;
    int column;
    int end_line;
    int end_column;
    char *message;
    char *hint;
    char *file_path;
    struct LpError *next;
} LpError;

/* Error list */
typedef struct {
    LpError *head;
    LpError *tail;
    int error_count;
    int warning_count;
} LpErrorList;

/* Initialize error list */
void lp_error_list_init(LpErrorList *list);

/* Add error to list */
void lp_error_list_add(LpErrorList *list, LpErrorCode code, LpErrorSeverity severity,
                       int line, int column, const char *message, const char *hint);

/* Add error with range */
void lp_error_list_add_range(LpErrorList *list, LpErrorCode code, LpErrorSeverity severity,
                             int line, int column, int end_line, int end_column,
                             const char *message, const char *hint);

/* Free error list */
void lp_error_list_free(LpErrorList *list);

/* Check if has errors */
int lp_error_list_has_errors(LpErrorList *list);

/* Print single error with context */
void lp_print_error_with_context(LpError *err, const char *source, FILE *out);

/* Print all errors */
void lp_print_all_errors(LpErrorList *list, const char *source, FILE *out);

/* Quick error functions - print immediately */
void lp_report_error(LpErrorCode code, int line, int column, 
                     const char *message, const char *hint,
                     const char *source);

void lp_report_warning(LpErrorCode code, int line, int column,
                       const char *message, const char *hint,
                       const char *source);

/* Get error code name */
const char *lp_error_code_name(LpErrorCode code);

/* Get severity name and color */
const char *lp_severity_name(LpErrorSeverity severity);
const char *lp_severity_color(LpErrorSeverity severity);

/* Built-in hints for common errors */
const char *lp_get_builtin_hint(LpErrorCode code);

/* Source line utilities */
char *lp_get_source_line(const char *source, int line);
int lp_count_source_lines(const char *source);
void lp_find_line_column(const char *source, int offset, int *line, int *column);

#endif /* LP_ERROR_REPORTER_H */
