#ifndef LP_COMPAT_H
#define LP_COMPAT_H

#include <stdlib.h>
#include <string.h>

static inline char *lp_strdup(const char *src) {
    size_t len;
    char *copy;

    if (!src) return NULL;

    len = strlen(src) + 1;
    copy = (char *)malloc(len);
    if (!copy) return NULL;

    memcpy(copy, src, len);
    return copy;
}

#ifndef strdup
#define strdup lp_strdup
#endif

#endif
