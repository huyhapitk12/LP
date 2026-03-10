/*
 * LP Native File I/O — Zero-Overhead C stdio.h Wrapper
 * Provides Python-like open(), read(), write(), close() functions.
 */

#ifndef LP_IO_H
#define LP_IO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lp_dict.h" /* For LpVal */

/* 
 * We define LpFile as a wrapper around FILE*.
 * This allows us to integrate it with LpVal later if needed or just return it as PyObj/pointer.
 */
typedef struct {
    FILE *fp;
    char *filename;
    char *mode;
} LpFile;

static inline LpFile* lp_io_open(const char *filename, const char *mode) {
    FILE *fp = fopen(filename, mode);
    if (!fp) {
        /* In Phase 4 we will throw an exception, but for now just print error and exit */
        fprintf(stderr, "IOError: No such file or directory: '%s'\n", filename);
        exit(1);
    }
    LpFile *f = (LpFile*)malloc(sizeof(LpFile));
    f->fp = fp;
    f->filename = strdup(filename);
    f->mode = strdup(mode);
    return f;
}

static inline void lp_io_close(LpFile *f) {
    if (f && f->fp) {
        fclose(f->fp);
        f->fp = NULL;
    }
}

static inline const char* lp_io_read(LpFile *f) {
    if (!f || !f->fp) return "";
    fseek(f->fp, 0, SEEK_END);
    long length = ftell(f->fp);
    fseek(f->fp, 0, SEEK_SET);

    char *buffer = (char*)malloc(length + 1);
    if (buffer) {
        fread(buffer, 1, length, f->fp);
        buffer[length] = '\0';
    }
    return buffer;
}

static inline void lp_io_write(LpFile *f, const char *data) {
    if (f && f->fp) {
        fputs(data, f->fp);
    }
}

/* Frees the struct wrapper entirely */
static inline void lp_io_free(LpFile *f) {
    if (!f) return;
    if (f->fp) fclose(f->fp);
    free(f->filename);
    free(f->mode);
    free(f);
}

#endif /* LP_IO_H */
