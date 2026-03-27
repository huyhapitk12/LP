#ifndef LP_LOGGING_H
#define LP_LOGGING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

typedef enum {
    LP_LOG_DEBUG = 0,
    LP_LOG_INFO = 1,
    LP_LOG_WARNING = 2,
    LP_LOG_ERROR = 3,
    LP_LOG_CRITICAL = 4
} LpLogLevel;

static LpLogLevel lp_log_current_level = LP_LOG_INFO;
static FILE* lp_log_file = NULL;
static int lp_log_enabled = 1;

/* Buffered logging — reduces syscalls in tight loops */
#define LP_LOG_BUFSIZE 4096
#define LP_LOG_FLUSH_COUNT 32
static char lp_log_buffer[LP_LOG_BUFSIZE];
static int lp_log_buf_len = 0;
static int lp_log_buf_count = 0;

/* Cached timestamp — refreshes at most once per second */
static time_t lp_log_last_time = 0;
static char lp_log_time_cache[64] = {0};

static inline const char* lp_log_level_to_str(LpLogLevel level) {
    switch (level) {
        case LP_LOG_DEBUG: return "DEBUG";
        case LP_LOG_INFO: return "INFO";
        case LP_LOG_WARNING: return "WARNING";
        case LP_LOG_ERROR: return "ERROR";
        case LP_LOG_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

static inline const char* lp_log_level_to_color(LpLogLevel level) {
    switch (level) {
        case LP_LOG_DEBUG: return "\033[90m";
        case LP_LOG_INFO: return "\033[36m";
        case LP_LOG_WARNING: return "\033[33m";
        case LP_LOG_ERROR: return "\033[31m";
        case LP_LOG_CRITICAL: return "\033[1;31m";
        default: return "\033[0m";
    }
}

static inline void lp_log_set_level_int(LpLogLevel level) {
    lp_log_current_level = level;
}

static inline void lp_logging_set_level(const char* level_str) {
    if (!level_str) return;
    if (strcmp(level_str, "DEBUG") == 0) lp_log_current_level = LP_LOG_DEBUG;
    else if (strcmp(level_str, "INFO") == 0) lp_log_current_level = LP_LOG_INFO;
    else if (strcmp(level_str, "WARNING") == 0) lp_log_current_level = LP_LOG_WARNING;
    else if (strcmp(level_str, "ERROR") == 0) lp_log_current_level = LP_LOG_ERROR;
    else if (strcmp(level_str, "CRITICAL") == 0) lp_log_current_level = LP_LOG_CRITICAL;
}

static inline const char* lp_logging_get_level() {
    return lp_log_level_to_str(lp_log_current_level);
}

static inline void lp_logging_set_file(const char* filepath) {
    if (lp_log_file && lp_log_file != stdout && lp_log_file != stderr) fclose(lp_log_file);
    if (!filepath || strlen(filepath) == 0) { lp_log_file = NULL; return; }
    lp_log_file = fopen(filepath, "a");
}

static inline void lp_logging_disable() { lp_log_enabled = 0; }
static inline void lp_logging_enable() { lp_log_enabled = 1; }

static inline void lp_logging_flush() {
    if (lp_log_file) fflush(lp_log_file);
    fflush(stdout);
}

static inline void lp_log_write(LpLogLevel level, const char* msg) {
    if (!lp_log_enabled) return;
    if (level < lp_log_current_level) return;
    
    /* Cached timestamp — only call time()/localtime() once per second */
    time_t now; time(&now);
    if (now != lp_log_last_time) {
        lp_log_last_time = now;
        struct tm *info = localtime(&now);
        strftime(lp_log_time_cache, sizeof(lp_log_time_cache), "%Y-%m-%d %H:%M:%S", info);
    }
    
    printf("%s[%s]\033[0m %s | %s\n", lp_log_level_to_color(level), 
           lp_log_level_to_str(level), lp_log_time_cache, msg ? msg : "NULL");
    
    /* Buffered file write */
    if (lp_log_file) {
        int written = snprintf(lp_log_buffer + lp_log_buf_len, LP_LOG_BUFSIZE - lp_log_buf_len,
                               "[%s] %s | %s\n", lp_log_level_to_str(level), lp_log_time_cache, msg ? msg : "NULL");
        if (written > 0 && lp_log_buf_len + written < LP_LOG_BUFSIZE) {
            lp_log_buf_len += written;
            lp_log_buf_count++;
        }
        if (lp_log_buf_len >= LP_LOG_BUFSIZE - 256 || lp_log_buf_count >= LP_LOG_FLUSH_COUNT || level >= LP_LOG_WARNING) {
            fwrite(lp_log_buffer, 1, lp_log_buf_len, lp_log_file);
            lp_log_buf_len = 0;
            lp_log_buf_count = 0;
        }
    }
}

static inline void lp_logging_debug(const char* msg) { lp_log_write(LP_LOG_DEBUG, msg); }
static inline void lp_logging_info(const char* msg) { lp_log_write(LP_LOG_INFO, msg); }
static inline void lp_logging_warning(const char* msg) { lp_log_write(LP_LOG_WARNING, msg); }
static inline void lp_logging_error(const char* msg) { lp_log_write(LP_LOG_ERROR, msg); }
static inline void lp_logging_critical(const char* msg) { lp_log_write(LP_LOG_CRITICAL, msg); }

#endif
