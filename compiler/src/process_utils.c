#ifndef _WIN32
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include "process_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

int lp_has_path_sep(const char *s) {
    return s && (strchr(s, '/') != NULL || strchr(s, '\\') != NULL);
}

int lp_wait_pid(pid_t pid) {
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) continue;
        return -1;
    }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return 1;
}

void lp_exec_fallback(const char *file, char *const argv[], int search_path) {
    if (search_path) execvp(file, argv);
    else execv(file, argv);

    if (!lp_has_path_sep(file)) {
        char local_path[1024];
        if (snprintf(local_path, sizeof(local_path), "./%s", file) > 0) {
            execv(local_path, argv);
        }
    }
    _exit(127);
}

int _spawnv(int mode, const char *path, const char *const argv[]) {
    pid_t pid;
    (void)mode;
    pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        lp_exec_fallback(path, (char *const *)argv, 0);
    }
    return lp_wait_pid(pid);
}

int _spawnvp(int mode, const char *file, const char *const argv[]) {
    pid_t pid;
    (void)mode;
    pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        lp_exec_fallback(file, (char *const *)argv, 1);
    }
    return lp_wait_pid(pid);
}

int _spawnl(int mode, const char *path, const char *arg0, ...) {
    const char *arg;
    const char *argv[128];
    int argc = 0;
    va_list ap;

    argv[argc++] = arg0;
    va_start(ap, arg0);
    while (argc < 127) {
        arg = va_arg(ap, const char *);
        if (!arg) break;
        argv[argc++] = arg;
    }
    va_end(ap);
    argv[argc] = NULL;

    if (lp_has_path_sep(path)) return _spawnv(mode, path, argv);
    return _spawnvp(mode, path, argv);
}
#endif /* _WIN32 */
