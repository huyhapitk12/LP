#ifndef LP_PLATFORM_H
#define LP_PLATFORM_H

#include "lp_runtime.h"

#if defined(_WIN32)
  #include <windows.h>
#else
  #include <unistd.h>
#endif

static inline const char* lp_platform_os() {
#if defined(_WIN32)
    return "windows";
#elif defined(__APPLE__)
    return "macos";
#elif defined(__linux__)
    return "linux";
#else
    return "unknown";
#endif
}

static inline const char* lp_platform_arch() {
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__i386) || defined(_M_IX86)
    return "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
#elif defined(__arm__) || defined(_M_ARM)
    return "arm";
#else
    return "unknown";
#endif
}

static inline int64_t lp_platform_cores() {
#if defined(_WIN32)
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#else
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (cores < 1) return 1;
    return cores;
#endif
}

#endif
