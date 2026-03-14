/*
 * LP Native Security Module - Security validation and sanitization utilities
 * 
 * This module provides runtime security features for LP programs:
 * - Input validation
 * - Output sanitization
 * - SQL injection prevention
 * - XSS prevention
 * - CSRF protection
 * - Rate limiting
 * - Access control
 */

#ifndef LP_SECURITY_H
#define LP_SECURITY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* Security levels */
#define LP_SECURITY_NONE     0
#define LP_SECURITY_LOW      1
#define LP_SECURITY_MEDIUM   2
#define LP_SECURITY_HIGH     3
#define LP_SECURITY_CRITICAL 4

/* Access levels */
#define LP_ACCESS_GUEST 0
#define LP_ACCESS_USER  1
#define LP_ACCESS_ADMIN 2
#define LP_ACCESS_SUPER 3

/* ============================================
 * GLOBAL SECURITY STATE
 * These persist across function calls
 * ============================================ */

/* Rate limiting state - array for different functions */
#define LP_MAX_RATE_LIMIT_SLOTS 64

typedef struct {
    char name[64];          /* Function name */
    int limit;              /* Max requests per minute */
    time_t window_start;    /* Start of current window */
    int count;              /* Requests in current window */
} LpRateLimitSlot;

static LpRateLimitSlot _lp_rate_limits[LP_MAX_RATE_LIMIT_SLOTS];
static int _lp_rate_limit_count = 0;

/* Global security context for current request */
typedef struct {
    int authenticated;          /* Is user authenticated? */
    int access_level;           /* Current user's access level */
    char *user_id;              /* Current user ID */
    char *auth_token;           /* Auth token */
} LpGlobalSecurityContext;

static LpGlobalSecurityContext _lp_global_sec = {0, LP_ACCESS_GUEST, NULL, NULL};

/* ============================================
 * GLOBAL CONTEXT MANAGEMENT
 * ============================================ */

/* Set current user as authenticated */
static inline void lp_set_authenticated(int access_level, const char *user_id) {
    _lp_global_sec.authenticated = 1;
    _lp_global_sec.access_level = access_level;
    if (user_id) {
        free(_lp_global_sec.user_id);
        _lp_global_sec.user_id = strdup(user_id);
    }
}

/* Set user as guest (logout) */
static inline void lp_set_guest(void) {
    _lp_global_sec.authenticated = 0;
    _lp_global_sec.access_level = LP_ACCESS_GUEST;
    free(_lp_global_sec.user_id);
    _lp_global_sec.user_id = NULL;
}

/* Get current access level */
static inline int lp_get_access_level(void) {
    return _lp_global_sec.access_level;
}

/* Check if authenticated */
static inline int lp_is_authenticated(void) {
    return _lp_global_sec.authenticated;
}

/* ============================================
 * RATE LIMITING
 * ============================================ */

/* Find or create rate limit slot */
static inline LpRateLimitSlot* _lp_find_rate_slot(const char *func_name) {
    /* Find existing */
    for (int i = 0; i < _lp_rate_limit_count; i++) {
        if (strcmp(_lp_rate_limits[i].name, func_name) == 0) {
            return &_lp_rate_limits[i];
        }
    }
    /* Create new */
    if (_lp_rate_limit_count < LP_MAX_RATE_LIMIT_SLOTS) {
        LpRateLimitSlot *slot = &_lp_rate_limits[_lp_rate_limit_count++];
        strncpy(slot->name, func_name, 63);
        slot->name[63] = '\0';
        return slot;
    }
    return NULL;
}

/* Check rate limit - returns 1 if allowed, 0 if exceeded */
static inline int lp_check_func_rate_limit(const char *func_name, int limit) {
    LpRateLimitSlot *slot = _lp_find_rate_slot(func_name);
    if (!slot) return 1;  /* No slot available, allow */
    
    time_t now = time(NULL);
    
    /* Reset window if more than a minute passed */
    if (now - slot->window_start >= 60) {
        slot->window_start = now;
        slot->count = 0;
        slot->limit = limit;
    }
    
    slot->count++;
    
    return slot->count <= limit;
}

/* Get remaining requests for function */
static inline int lp_rate_limit_remaining(const char *func_name) {
    LpRateLimitSlot *slot = _lp_find_rate_slot(func_name);
    if (!slot) return -1;
    return slot->limit - slot->count;
}

/* ============================================
 * SECURITY CONTEXT FOR @security DECORATOR
 * ============================================ */

/* Security context structure for function-level security */
typedef struct {
    int enabled;
    int level;
    int require_auth;
    char *auth_type;
    int rate_limit;
    int validate_input;
    int sanitize_output;
    int prevent_injection;
    int prevent_xss;
    int prevent_csrf;
    int enable_cors;
    char *cors_origins;
    int secure_headers;
    int encrypt_data;
    char *hash_algorithm;
    int access_level;
    int readonly;
    time_t last_request_time;
    int request_count;
} LpSecurityContext;

/* Default security context */
static inline void lp_security_init(LpSecurityContext *ctx) {
    ctx->enabled = 1;
    ctx->level = LP_SECURITY_MEDIUM;
    ctx->require_auth = 0;
    ctx->auth_type = NULL;
    ctx->rate_limit = 0;
    ctx->validate_input = 1;
    ctx->sanitize_output = 1;
    ctx->prevent_injection = 1;
    ctx->prevent_xss = 1;
    ctx->prevent_csrf = 0;
    ctx->enable_cors = 0;
    ctx->cors_origins = NULL;
    ctx->secure_headers = 1;
    ctx->encrypt_data = 0;
    ctx->hash_algorithm = NULL;
    ctx->access_level = LP_ACCESS_USER;
    ctx->readonly = 0;
    ctx->last_request_time = 0;
    ctx->request_count = 0;
}

/* ===== Input Validation ===== */

/* Check if string contains only safe characters */
static inline int lp_is_safe_string(const char *str) {
    if (!str) return 0;
    for (const char *p = str; *p; p++) {
        /* Check for potentially dangerous characters */
        switch (*p) {
            case '\0': case '\n': case '\r':
            case '<': case '>': case '"': case '\'':
            case '&': case '|': case ';': case '$':
            case '(': case ')': case '{': case '}':
            case '[': case ']': case '`': case '\\':
                return 0;
            default:
                break;
        }
    }
    return 1;
}

/* Validate email format */
static inline int lp_validate_email(const char *email) {
    if (!email || !*email) return 0;
    
    int has_at = 0;
    int has_dot = 0;
    const char *p = email;
    
    while (*p) {
        if (*p == '@') has_at = 1;
        else if (has_at && *p == '.') has_dot = 1;
        p++;
    }
    
    return has_at && has_dot && p - email > 5;
}

/* Validate numeric string */
static inline int lp_validate_numeric(const char *str) {
    if (!str || !*str) return 0;
    
    int has_digit = 0;
    const char *p = str;
    
    if (*p == '-' || *p == '+') p++;
    while (*p) {
        if (!isdigit((unsigned char)*p)) {
            if (*p == '.' && has_digit) return 0; /* Only one decimal point */
            if (*p != '.') return 0;
        }
        has_digit = 1;
        p++;
    }
    return has_digit;
}

/* Validate alphanumeric string */
static inline int lp_validate_alphanumeric(const char *str) {
    if (!str || !*str) return 0;
    for (const char *p = str; *p; p++) {
        if (!isalnum((unsigned char)*p) && *p != '_' && *p != '-')
            return 0;
    }
    return 1;
}

/* Validate URL format */
static inline int lp_validate_url(const char *url) {
    if (!url) return 0;
    
    /* Must start with http:// or https:// */
    if (strncmp(url, "http://", 7) == 0) return 1;
    if (strncmp(url, "https://", 8) == 0) return 1;
    
    return 0;
}

/* Validate identifier (variable/function name) */
static inline int lp_validate_identifier(const char *id) {
    if (!id || !*id) return 0;
    
    /* Must start with letter or underscore */
    if (!isalpha((unsigned char)id[0]) && id[0] != '_') return 0;
    
    /* Rest must be alphanumeric or underscore */
    for (const char *p = id + 1; *p; p++) {
        if (!isalnum((unsigned char)*p) && *p != '_') return 0;
    }
    
    return 1;
}

/* ===== SQL Injection Prevention ===== */

/* Characters that need escaping for SQL */
static inline int lp_sql_needs_escape(const char *str) {
    if (!str) return 0;
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '\'': case '"': case '\\': case '\0':
            case '\n': case '\r': case '\x1a':
            case ';': case '--': case '/*':
                return 1;
            default:
                break;
        }
    }
    return 0;
}

/* Escape string for SQL (returns newly allocated string) */
static inline char* lp_sql_escape(const char *str) {
    if (!str) return strdup("");
    
    size_t len = strlen(str);
    size_t new_len = 0;
    
    /* Count characters that need escaping */
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '\'': case '"': case '\\': case '\0':
            case '\n': case '\r': case '\x1a':
                new_len += 2;
                break;
            default:
                new_len++;
                break;
        }
    }
    
    char *result = (char*)malloc(new_len + 1);
    if (!result) return NULL;
    
    char *out = result;
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '\'': *out++ = '\''; *out++ = '\''; break;
            case '"':  *out++ = '\\'; *out++ = '"'; break;
            case '\\': *out++ = '\\'; *out++ = '\\'; break;
            case '\0': *out++ = '\\'; *out++ = '0'; break;
            case '\n': *out++ = '\\'; *out++ = 'n'; break;
            case '\r': *out++ = '\\'; *out++ = 'r'; break;
            case '\x1a': *out++ = '\\'; *out++ = 'Z'; break;
            default: *out++ = *p; break;
        }
    }
    *out = '\0';
    
    return result;
}

/* Check for SQL injection patterns */
static inline int lp_detect_sql_injection(const char *str) {
    if (!str) return 0;
    
    /* Common SQL injection patterns */
    const char *patterns[] = {
        "' OR '", "' AND '", " OR '1'='1", " OR 1=1",
        "'; DROP ", "'; DELETE ", "'; UPDATE ", "'; INSERT ",
        " UNION ", " UNION SELECT ", "--", "/*", "*/",
        "EXEC(", "EXECUTE(", "xp_cmdshell",
        "CONCAT(", "CHAR(", "NCHAR(", "VARCHAR(",
        NULL
    };
    
    for (int i = 0; patterns[i]; i++) {
        if (strstr(str, patterns[i])) return 1;
    }
    
    return 0;
}

/* ===== XSS Prevention ===== */

/* Escape HTML special characters (returns newly allocated string) */
static inline char* lp_html_escape(const char *str) {
    if (!str) return strdup("");
    
    size_t len = strlen(str);
    size_t new_len = 0;
    
    /* Count space needed for escaped characters */
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '&': new_len += 5; break; /* &amp; */
            case '<': new_len += 4; break; /* &lt; */
            case '>': new_len += 4; break; /* &gt; */
            case '"': new_len += 6; break; /* &quot; */
            case '\'': new_len += 6; break; /* &#39; */
            default: new_len++; break;
        }
    }
    
    char *result = (char*)malloc(new_len + 1);
    if (!result) return NULL;
    
    char *out = result;
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '&': strcpy(out, "&amp;"); out += 5; break;
            case '<': strcpy(out, "&lt;"); out += 4; break;
            case '>': strcpy(out, "&gt;"); out += 4; break;
            case '"': strcpy(out, "&quot;"); out += 6; break;
            case '\'': strcpy(out, "&#39;"); out += 6; break;
            default: *out++ = *p; break;
        }
    }
    *out = '\0';
    
    return result;
}

/* Remove HTML tags (returns newly allocated string) */
static inline char* lp_strip_html(const char *str) {
    if (!str) return strdup("");
    
    size_t len = strlen(str);
    char *result = (char*)malloc(len + 1);
    if (!result) return NULL;
    
    char *out = result;
    int in_tag = 0;
    
    for (const char *p = str; *p; p++) {
        if (*p == '<') {
            in_tag = 1;
        } else if (*p == '>') {
            in_tag = 0;
        } else if (!in_tag) {
            *out++ = *p;
        }
    }
    *out = '\0';
    
    return result;
}

/* Check for XSS patterns */
static inline int lp_detect_xss(const char *str) {
    if (!str) return 0;
    
    /* Common XSS patterns */
    const char *patterns[] = {
        "<script", "</script", "javascript:", "onerror=",
        "onload=", "onclick=", "onmouseover=", "onfocus=",
        "onblur=", "eval(", "document.cookie", "document.write",
        "window.location", "innerHTML", "outerHTML",
        "vbscript:", "expression(", "data:text/html",
        NULL
    };
    
    for (int i = 0; patterns[i]; i++) {
        if (strcasestr(str, patterns[i])) return 1;
    }
    
    return 0;
}

/* Helper: case-insensitive string search */
#ifndef strcasestr
static inline char* strcasestr(const char *haystack, const char *needle) {
    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
            h++; n++;
        }
        if (!*n) return (char*)haystack;
    }
    return NULL;
}
#endif

/* ===== Rate Limiting ===== */

/* Check if request is within rate limit */
static inline int lp_check_rate_limit(LpSecurityContext *ctx) {
    if (!ctx->enabled || ctx->rate_limit <= 0) return 1;
    
    time_t now = time(NULL);
    
    /* Reset counter if more than a minute has passed */
    if (now - ctx->last_request_time >= 60) {
        ctx->request_count = 0;
        ctx->last_request_time = now;
    }
    
    ctx->request_count++;
    
    return ctx->request_count <= ctx->rate_limit;
}

/* ===== Access Control ===== */

/* Check if access level is sufficient */
static inline int lp_check_access(LpSecurityContext *ctx, int required_level) {
    if (!ctx->enabled) return 1;
    return ctx->access_level >= required_level;
}

/* Check if write operation is allowed */
static inline int lp_check_write_allowed(LpSecurityContext *ctx) {
    if (!ctx->enabled) return 1;
    return !ctx->readonly;
}

/* ===== Security Context Management ===== */

/* Apply security context from decorator */
static inline void lp_apply_security_settings(
    LpSecurityContext *ctx,
    int level,
    int require_auth,
    const char *auth_type,
    int rate_limit,
    int validate_input,
    int sanitize_output,
    int prevent_injection,
    int prevent_xss,
    int prevent_csrf,
    int enable_cors,
    const char *cors_origins,
    int secure_headers,
    int encrypt_data,
    const char *hash_algorithm,
    int access_level,
    int readonly
) {
    ctx->enabled = 1;
    ctx->level = level;
    ctx->require_auth = require_auth;
    ctx->auth_type = auth_type ? strdup(auth_type) : NULL;
    ctx->rate_limit = rate_limit;
    ctx->validate_input = validate_input;
    ctx->sanitize_output = sanitize_output;
    ctx->prevent_injection = prevent_injection;
    ctx->prevent_xss = prevent_xss;
    ctx->prevent_csrf = prevent_csrf;
    ctx->enable_cors = enable_cors;
    ctx->cors_origins = cors_origins ? strdup(cors_origins) : NULL;
    ctx->secure_headers = secure_headers;
    ctx->encrypt_data = encrypt_data;
    ctx->hash_algorithm = hash_algorithm ? strdup(hash_algorithm) : NULL;
    ctx->access_level = access_level;
    ctx->readonly = readonly;
}

/* Validate input based on security context */
static inline int lp_security_validate_input(LpSecurityContext *ctx, const char *input) {
    if (!ctx->enabled || !ctx->validate_input) return 1;
    if (!input) return 0;
    
    /* Check for SQL injection if enabled */
    if (ctx->prevent_injection && lp_detect_sql_injection(input)) {
        return 0;
    }
    
    /* Check for XSS if enabled */
    if (ctx->prevent_xss && lp_detect_xss(input)) {
        return 0;
    }
    
    /* High security: check for any unsafe characters */
    if (ctx->level >= LP_SECURITY_HIGH && !lp_is_safe_string(input)) {
        return 0;
    }
    
    return 1;
}

/* Sanitize output based on security context */
static inline char* lp_security_sanitize_output(LpSecurityContext *ctx, const char *output) {
    if (!ctx->enabled || !ctx->sanitize_output) {
        return output ? strdup(output) : strdup("");
    }
    
    /* Always escape HTML */
    return lp_html_escape(output);
}

/* Free security context resources */
static inline void lp_security_cleanup(LpSecurityContext *ctx) {
    free(ctx->auth_type);
    free(ctx->cors_origins);
    free(ctx->hash_algorithm);
}

#endif /* LP_SECURITY_H */
