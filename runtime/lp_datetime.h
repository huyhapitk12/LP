#ifndef LP_DATETIME_H
#define LP_DATETIME_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Cached localtime — avoids redundant calls when extracting multiple fields */
static time_t _lp_dt_cached_ts = 0;
static struct tm _lp_dt_cached_tm;

static inline struct tm* _lp_dt_localtime(double ts) {
    time_t rawtime = (time_t)ts;
    if (rawtime == _lp_dt_cached_ts && _lp_dt_cached_ts != 0) {
        return &_lp_dt_cached_tm;
    }
    struct tm *info = localtime(&rawtime);
    if (info) {
        _lp_dt_cached_ts = rawtime;
        _lp_dt_cached_tm = *info;
        return &_lp_dt_cached_tm;
    }
    return NULL;
}

/* Cross-platform strptime fallback for Windows */
#ifdef _WIN32
static inline char* lp_strptime(const char* s, const char* f, struct tm* tm) {
    int year=1970, month=1, day=1, hour=0, min=0, sec=0;
    if (strcmp(f, "%Y-%m-%d %H:%M:%S") == 0) {
        if (sscanf(s, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec) == 6) {
            tm->tm_year=year-1900; tm->tm_mon=month-1; tm->tm_mday=day;
            tm->tm_hour=hour; tm->tm_min=min; tm->tm_sec=sec;
            return (char*)(s + strlen(s));
        }
    } else if (strcmp(f, "%Y-%m-%d") == 0) {
        if (sscanf(s, "%d-%d-%d", &year, &month, &day) == 3) {
            tm->tm_year=year-1900; tm->tm_mon=month-1; tm->tm_mday=day;
            tm->tm_hour=0; tm->tm_min=0; tm->tm_sec=0;
            return (char*)(s + strlen(s));
        }
    }
    return NULL;
}
#else
#define lp_strptime strptime
#endif

static inline const char* lp_datetime_now() {
    time_t rawtime; struct tm *info;
    static char buffer[80];
    time(&rawtime); info = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", info);
    return strdup(buffer);
}

static inline const char* lp_datetime_today() {
    time_t rawtime; struct tm *info;
    static char buffer[80];
    time(&rawtime); info = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", info);
    return strdup(buffer);
}

static inline double lp_datetime_timestamp() {
    return (double)time(NULL);
}

static inline const char* lp_datetime_format(double ts, const char* fmt) {
    time_t rawtime = (time_t)ts;
    struct tm *info = localtime(&rawtime);
    static char buffer[256];
    if (info && fmt) { strftime(buffer, sizeof(buffer), fmt, info); return strdup(buffer); }
    return strdup("");
}

static inline double lp_datetime_parse(const char* str, const char* fmt) {
    struct tm tm_info = {0};
    if (str && fmt) {
        if (lp_strptime(str, fmt, &tm_info)) { tm_info.tm_isdst=-1; return (double)mktime(&tm_info); }
    }
    return 0.0;
}

static inline double lp_datetime_diff(double ts1, double ts2) { return ts1 - ts2; }
static inline double lp_datetime_add_seconds(double ts, int n) { return ts + n; }
static inline double lp_datetime_add_days(double ts, int n) { return ts + (n * 86400.0); }
static inline double lp_datetime_add_hours(double ts, int n) { return ts + (n * 3600.0); }

static inline int lp_datetime_year(double ts) {
    struct tm *info = _lp_dt_localtime(ts);
    return info ? info->tm_year + 1900 : 0;
}
static inline int lp_datetime_month(double ts) {
    struct tm *info = _lp_dt_localtime(ts);
    return info ? info->tm_mon + 1 : 0;
}
static inline int lp_datetime_day(double ts) {
    struct tm *info = _lp_dt_localtime(ts);
    return info ? info->tm_mday : 0;
}
static inline int lp_datetime_weekday(double ts) {
    struct tm *info = _lp_dt_localtime(ts);
    if (!info) return 0;
    int wd = info->tm_wday - 1;
    if (wd < 0) wd = 6;
    return wd;
}

static inline int lp_datetime_is_leap_year(int year) {
    if (year % 400 == 0) return 1;
    if (year % 100 == 0) return 0;
    if (year % 4 == 0) return 1;
    return 0;
}

/* Pre-computed month table — O(1) days-in-month lookup */
static const int _lp_days_in_month[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static inline int lp_datetime_days_in_month(int year, int month) {
    if (month < 1 || month > 12) return 0;
    if (month == 2 && lp_datetime_is_leap_year(year)) return 29;
    return _lp_days_in_month[month];
}

static const int _lp_cumul_days[13] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

static inline int lp_datetime_day_of_year(double ts) {
    struct tm *info = _lp_dt_localtime(ts);
    if (!info) return 0;
    int month = info->tm_mon + 1, day = info->tm_mday, year = info->tm_year + 1900;
    int doy = _lp_cumul_days[month] + day;
    if (month > 2 && lp_datetime_is_leap_year(year)) doy++;
    return doy;
}

#endif
