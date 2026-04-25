/*
 * LP Path Module — File path operations
 * Cross-platform path manipulation (like Python's os.path).
 */
#ifndef LP_PATH_H
#define LP_PATH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
  #include <direct.h>
  #include <windows.h>
  #define LP_PATH_SEP '\\'
  #define LP_PATH_SEP_STR "\\"
#else
  #include <unistd.h>
  #define LP_PATH_SEP '/'
  #define LP_PATH_SEP_STR "/"
#endif

static inline int lp_path_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static inline int lp_path_isfile(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return (st.st_mode & S_IFMT) == S_IFREG;
}

static inline int lp_path_isdir(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return (st.st_mode & S_IFMT) == S_IFDIR;
}

static inline int64_t lp_path_getsize(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return (int64_t)st.st_size;
}

static inline char *lp_path_basename(const char *path) {
    const char *p = strrchr(path, '/');
    const char *q = strrchr(path, '\\');
    const char *last = (p > q) ? p : q;
    return strdup(last ? last + 1 : path);
}

static inline char *lp_path_dirname(const char *path) {
    const char *p = strrchr(path, '/');
    const char *q = strrchr(path, '\\');
    const char *last = (p > q) ? p : q;
    if (!last) return strdup(".");
    size_t len = last - path;
    char *d = (char *)malloc(len + 1);
    if (d) { memcpy(d, path, len); d[len] = '\0'; }
    return d;
}

static inline char *lp_path_extension(const char *path) {
    const char *dot = strrchr(path, '.');
    const char *sep = strrchr(path, '/');
    const char *bsep = strrchr(path, '\\');
    const char *last_sep = (sep > bsep) ? sep : bsep;
    if (!dot || (last_sep && dot < last_sep)) return strdup("");
    return strdup(dot);
}

static inline char *lp_path_join(const char *a, const char *b) {
    size_t la = strlen(a), lb = strlen(b);
    char *r = (char *)malloc(la + lb + 2);
    if (!r) return NULL;
    memcpy(r, a, la);
    if (la > 0 && a[la-1] != '/' && a[la-1] != '\\') { r[la] = LP_PATH_SEP; la++; }
    memcpy(r + la, b, lb);
    r[la + lb] = '\0';
    return r;
}

static inline char *lp_path_abspath(const char *path) {
    char *buf = (char *)malloc(4096);
    if (!buf) return NULL;
#ifdef _WIN32
    if (!_fullpath(buf, path, 4096)) { free(buf); return strdup(path); }
#else
    if (!realpath(path, buf)) { free(buf); return strdup(path); }
#endif
    return buf;
}

static inline char *lp_path_getcwd(void) {
    char *buf = (char *)malloc(4096);
    if (!buf) return NULL;
#ifdef _WIN32
    _getcwd(buf, 4096);
#else
    if (!getcwd(buf, 4096)) { buf[0] = '\0'; }
#endif
    return buf;
}

#endif /* LP_PATH_H */
