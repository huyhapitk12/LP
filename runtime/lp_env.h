/*
 * LP Env Module — Environment variable access
 * Maps to C getenv/setenv with Python-like os.environ API.
 */
#ifndef LP_ENV_H
#define LP_ENV_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline const char *lp_env_get(const char *name) {
    const char *v = getenv(name);
    return v ? v : "";
}

static inline const char *lp_env_get_default(const char *name, const char *def) {
    const char *v = getenv(name);
    return v ? v : def;
}

static inline int lp_env_set(const char *name, const char *value) {
#ifdef _WIN32
    char buf[4096];
    snprintf(buf, sizeof(buf), "%s=%s", name, value);
    return _putenv(buf) == 0;
#else
    return setenv(name, value, 1) == 0;
#endif
}

static inline int lp_env_unset(const char *name) {
#ifdef _WIN32
    char buf[4096];
    snprintf(buf, sizeof(buf), "%s=", name);
    return _putenv(buf) == 0;
#else
    return unsetenv(name) == 0;
#endif
}

static inline int lp_env_has(const char *name) {
    return getenv(name) != NULL;
}

#endif /* LP_ENV_H */
