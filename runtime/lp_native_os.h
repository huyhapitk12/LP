/*
 * LP Native OS Module — Zero-Overhead C Implementation
 * Maps Python's os/os.path module to native C calls.
 */

#ifndef LP_NATIVE_OS_H
#define LP_NATIVE_OS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
  #include <windows.h>
  #include <direct.h>
  #include <io.h>
  #define LP_PATH_SEP '\\'
  #define LP_PATH_SEP_STR "\\"
#else
  #include <unistd.h>
  #include <dirent.h>
  #include <sys/stat.h>
  #define LP_PATH_SEP '/'
  #define LP_PATH_SEP_STR "/"
#endif

/* ================================================================
 * os.path functions
 * ================================================================ */

/* os.path.join(a, b) */
static inline const char *lp_os_path_join(const char *a, const char *b) {
    size_t la = strlen(a);
    size_t lb = strlen(b);
    /* Check if a already ends with separator */
    int has_sep = (la > 0 && (a[la-1] == '/' || a[la-1] == '\\'));
    char *r = (char *)malloc(la + lb + 2);
    memcpy(r, a, la);
    if (!has_sep && la > 0) {
        r[la] = LP_PATH_SEP;
        memcpy(r + la + 1, b, lb + 1);
    } else {
        memcpy(r + la, b, lb + 1);
    }
    return r;
}

/* os.path.exists(path) */
static inline int lp_os_path_exists(const char *path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES);
#else
    struct stat st;
    return (stat(path, &st) == 0);
#endif
}

/* os.path.isfile(path) */
static inline int lp_os_path_isfile(const char *path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
#endif
}

/* os.path.isdir(path) */
static inline int lp_os_path_isdir(const char *path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
#endif
}

/* os.path.basename(path) */
static inline const char *lp_os_path_basename(const char *path) {
    const char *sep = strrchr(path, '/');
    const char *sep2 = strrchr(path, '\\');
    if (sep2 && (!sep || sep2 > sep)) sep = sep2;
    if (sep) return sep + 1;
    return path;
}

/* os.path.dirname(path) */
static inline const char *lp_os_path_dirname(const char *path) {
    const char *sep = strrchr(path, '/');
    const char *sep2 = strrchr(path, '\\');
    if (sep2 && (!sep || sep2 > sep)) sep = sep2;
    if (!sep) {
        char *r = (char *)malloc(2);
        r[0] = '.'; r[1] = '\0';
        return r;
    }
    size_t len = sep - path;
    char *r = (char *)malloc(len + 1);
    memcpy(r, path, len);
    r[len] = '\0';
    return r;
}

/* os.path.getsize(path) */
static inline int64_t lp_os_path_getsize(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    int64_t size = (int64_t)ftell(f);
    fclose(f);
    return size;
}

/* ================================================================
 * os module functions
 * ================================================================ */

/* os.getcwd() */
static inline const char *lp_os_getcwd(void) {
    static char buf[4096];
#ifdef _WIN32
    _getcwd(buf, sizeof(buf));
#else
    getcwd(buf, sizeof(buf));
#endif
    return buf;
}

/* os.remove(path) */
static inline void lp_os_remove(const char *path) {
    remove(path);
}

/* os.rename(src, dst) */
static inline void lp_os_rename(const char *src, const char *dst) {
    rename(src, dst);
}

/* os.mkdir(path) */
static inline void lp_os_mkdir(const char *path) {
#ifdef _WIN32
    _mkdir(path);
#else
    mkdir(path, 0755);
#endif
}

/* os.sep */
static const char *lp_os_sep = LP_PATH_SEP_STR;

/* os.name */
#ifdef _WIN32
  static const char *lp_os_name = "nt";
#else
  static const char *lp_os_name = "posix";
#endif

#endif /* LP_NATIVE_OS_H */
