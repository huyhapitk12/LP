#ifndef LP_SQLITE_H
#define LP_SQLITE_H

#include "sqlite3.h"
#include "lp_runtime.h"

static inline sqlite3* lp_sqlite_connect(const char* filename) {
    sqlite3* db;
    if (sqlite3_open(filename, &db)) {
        fprintf(stderr, "SQLite Error: %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    return db;
}

static inline void lp_sqlite_close(sqlite3* db) {
    if (db) sqlite3_close(db);
}

static inline void lp_sqlite_bind_params(sqlite3_stmt* stmt, LpList* l) {
    if (!l) return;
    for (int i = 0; i < l->len; i++) {
        LpVal v = l->items[i];
        if (v.type == LP_VAL_INT) sqlite3_bind_int64(stmt, i + 1, v.as.i);
        else if (v.type == LP_VAL_FLOAT) sqlite3_bind_double(stmt, i + 1, v.as.f);
        else if (v.type == LP_VAL_STRING) sqlite3_bind_text(stmt, i + 1, v.as.s, -1, SQLITE_TRANSIENT);
        else if (v.type == LP_VAL_NULL) sqlite3_bind_null(stmt, i + 1);
    }
}

static inline int lp_sqlite_execute(sqlite3* db, const char* sql, LpList* params) {
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "SQLite Error: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    lp_sqlite_bind_params(stmt, params);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        fprintf(stderr, "SQLite Error: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt);
    return 1;
}

/* Querying returns an LpVal wrapping an LpList of LpDict structures */
static inline LpVal lp_sqlite_query(sqlite3* db, const char* sql, LpList* params) {
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "SQLite Error: %s\n", sqlite3_errmsg(db));
        return lp_val_null();
    }
    lp_sqlite_bind_params(stmt, params);
    
    LpList* rows = lp_list_new();
    int cols = sqlite3_column_count(stmt);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        LpDict* row = lp_dict_new();
        for (int i = 0; i < cols; i++) {
            const char* col_name = sqlite3_column_name(stmt, i);
            int type = sqlite3_column_type(stmt, i);
            if (type == SQLITE_INTEGER) {
                lp_dict_set(row, col_name, lp_val_int(sqlite3_column_int64(stmt, i)));
            } else if (type == SQLITE_FLOAT) {
                lp_dict_set(row, col_name, lp_val_float(sqlite3_column_double(stmt, i)));
            } else if (type == SQLITE_TEXT) {
                lp_dict_set(row, col_name, lp_val_str((const char*)sqlite3_column_text(stmt, i)));
            } else if (type == SQLITE_NULL) {
                lp_dict_set(row, col_name, lp_val_null());
            }
        }
        lp_list_append(rows, lp_val_dict(row));
    }
    sqlite3_finalize(stmt);
    return lp_val_list(rows);
}

#endif
