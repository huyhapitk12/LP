#ifndef LP_CSV_H
#define LP_CSV_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#endif
/* We assume LpList and LpVal are defined in lp_dict.h included via lp_runtime.h */

static char lp_csv_delimiter = ',';

static inline void lp_csv_set_delimiter(const char* delim) {
    if (delim && delim[0] != '\0') lp_csv_delimiter = delim[0];
}

/* Parse a single CSV line into a list of strings */
static inline LpList* lp_csv_parse_line(const char* line) {
    LpList* row = lp_list_new();
    if (!line) return row;
    
    int len = (int)strlen(line);
    char* token = (char*)malloc(len + 1);
    int t_i = 0;
    int in_quotes = 0;
    
    for (int i = 0; i <= len; i++) {
        char c = (i < len) ? line[i] : '\0';
        
        if (c == '"') {
            if (in_quotes && i + 1 < len && line[i + 1] == '"') {
                token[t_i++] = '"'; i++;
            } else {
                in_quotes = !in_quotes;
            }
        } else if ((c == lp_csv_delimiter && !in_quotes) || c == '\0') {
            token[t_i] = '\0';
            lp_list_append(row, lp_val_str(token));
            t_i = 0;
        } else {
            token[t_i++] = c;
        }
    }
    
    free(token);
    return row;
}

static inline LpList* lp_csv_parse(const char* text) {
    LpList* result = lp_list_new();
    if (!text) return result;
    
    int len = (int)strlen(text);
    int in_quotes = 0;
    char* line = (char*)malloc(len + 1);
    int l_i = 0;
    
    for (int i = 0; i <= len; i++) {
        char c = (i < len) ? text[i] : '\0';
        
        if (c == '"') { in_quotes = !in_quotes; line[l_i++] = c; }
        else if ((c == '\n' && !in_quotes) || c == '\0') {
            line[l_i] = '\0';
            if (l_i > 0 && line[l_i-1] == '\r') line[l_i-1] = '\0';
            if (strlen(line) > 0 || c != '\0') {
                lp_list_append(result, lp_val_list(lp_csv_parse_line(line)));
            }
            l_i = 0;
        } else {
            line[l_i++] = c;
        }
    }
    
    free(line);
    return result;
}

static inline LpList* lp_csv_read(const char* filename) {
    LpList* result = lp_list_new();
    if (!filename) return result;

#ifdef _WIN32
    /* Memory-mapped I/O on Windows */
    HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return result;
    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0 || fileSize == INVALID_FILE_SIZE) { CloseHandle(hFile); return result; }
    HANDLE hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMap) { CloseHandle(hFile); return result; }
    const char* mapped = (const char*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (mapped) {
        char* buffer = (char*)malloc(fileSize + 1);
        if (buffer) {
            memcpy(buffer, mapped, fileSize);
            buffer[fileSize] = '\0';
            LpList* parsed = lp_csv_parse(buffer);
            free(buffer);
            UnmapViewOfFile(mapped);
            CloseHandle(hMap); CloseHandle(hFile);
            return parsed;
        }
        UnmapViewOfFile(mapped);
    }
    CloseHandle(hMap); CloseHandle(hFile);
    return result;
#else
    FILE* file = fopen(filename, "rb");
    if (!file) return result;
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (length > 0) {
        char* buffer = (char*)malloc(length + 1);
        if (buffer) {
            size_t read_len = fread(buffer, 1, length, file);
            buffer[read_len] = '\0';
            LpList* parsed = lp_csv_parse(buffer);
            free(buffer);
            fclose(file);
            return parsed;
        }
    }
    fclose(file);
    return result;
#endif
}

static inline int64_t lp_csv_row_count(LpList* data) {
    if (!data) return 0;
    return data->len;
}

static inline LpList* lp_csv_get_row(LpList* data, int64_t index) {
    if (!data || index < 0 || index >= data->len) return lp_list_new();
    if (data->items[index].type == LP_VAL_LIST) return data->items[index].as.l;
    return lp_list_new();
}

static inline const char* lp_csv_get_field(LpList* row, int64_t index) {
    if (!row || index < 0 || index >= row->len) return strdup("");
    if (row->items[index].type == LP_VAL_STRING) return row->items[index].as.s;
    return "";
}

static inline LpList* lp_csv_read_with_header(const char* filename) {
    return lp_csv_read(filename);
}

static inline const char* lp_csv_to_string(LpList* rows) {
    if (!rows) return strdup("");
    int cap = 4096, len = 0;
    char* buffer = (char*)malloc(cap);
    buffer[0] = '\0';
    
    for (int64_t i = 0; i < rows->len; i++) {
        if (rows->items[i].type != LP_VAL_LIST) continue;
        LpList* row = rows->items[i].as.l;
        for (int64_t j = 0; j < row->len; j++) {
            const char* field = "";
            if (row->items[j].type == LP_VAL_STRING) field = row->items[j].as.s;
            int flen = (int)strlen(field);
            if (len + flen + 4 > cap) { cap = (cap + flen) * 2; buffer = (char*)realloc(buffer, cap); }
            if (j > 0) buffer[len++] = lp_csv_delimiter;
            memcpy(buffer + len, field, flen); len += flen;
        }
        if (len + 2 > cap) { cap *= 2; buffer = (char*)realloc(buffer, cap); }
        buffer[len++] = '\n';
    }
    buffer[len] = '\0';
    return buffer;
}

static inline int lp_csv_write(const char* filename, LpList* rows) {
    if (!filename || !rows) return 0;
    FILE* file = fopen(filename, "wb");
    if (!file) return 0;
    const char* str = lp_csv_to_string(rows);
    if (str) { fwrite(str, 1, strlen(str), file); free((void*)str); }
    fclose(file);
    return 1;
}

#endif
