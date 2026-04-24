#include "process_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#endif

/* 
 * Security: Validate path/filename for safety
 * Returns 1 if safe, 0 if potentially dangerous.
 * This prevents command injection and path traversal attacks.
 */
int lp_path_is_safe(const char *path) {
    if (!path || !*path) return 0;
    
    /* Check for shell metacharacters that could enable command injection */
    /* Dangerous chars: ; | & ` $ ( ) { } [ ] < > ! * ? ' " \ newline carriage-return */
    for (const char *p = path; *p; p++) {
        switch (*p) {
            case ';': case '|': case '&': case '`': case '$':
            case '(': case ')': case '{': case '}': case '[': case ']':
            case '<': case '>': case '!': case '*': case '?':
            case '\'': case '"': case '\\':
            case '\n': case '\r':
                return 0;
            default:
                break;
        }
    }
    
    /* Check for path traversal attempts */
    if (strstr(path, "..") != NULL) return 0;
    
    return 1;
}

#ifndef _WIN32

static int lp_has_path_sep(const char *s) {
    return s && (strchr(s, '/') != NULL || strchr(s, '\\') != NULL);
}

static int lp_wait_pid(pid_t pid) {
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) continue;
        return -1;
    }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return 1;
}

static void lp_exec_fallback(const char *file, char *const argv[], int search_path) {
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
    if (pid == 0) lp_exec_fallback(path, (char *const *)argv, 0);
    return lp_wait_pid(pid);
}

int _spawnvp(int mode, const char *file, const char *const argv[]) {
    pid_t pid;
    (void)mode;
    pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) lp_exec_fallback(file, (char *const *)argv, 1);
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

int lp_run_silent(const char *file, const char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) return 0;
    if (pid == 0) {
        int fd = open("/dev/null", O_WRONLY);
        if (fd >= 0) {
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        lp_exec_fallback(file, (char *const *)argv, !lp_has_path_sep(file));
        _exit(127);
    }
    return lp_wait_pid(pid) == 0;
}

#else

int lp_run_silent(const char *file, const char *const argv[]) {
    (void)file;
    /* For Windows, _spawnvp doesn't have an easy way to suppress stdout/stderr directly
       without duplicating handles. We use CreateProcess directly. */
    char cmdline[2048] = {0};
    size_t current_len = 0;
    int i = 0;
    
    while (argv[i]) {
        size_t arg_len = strlen(argv[i]);
        int space_needed = (i > 0) ? 1 : 0;
        
        if (current_len + space_needed + arg_len >= sizeof(cmdline)) {
            fprintf(stderr, "Fatal error: Command line too long in lp_run_silent\n");
            return -1;
        }
        
        if (i > 0) {
            cmdline[current_len++] = ' ';
        }
        memcpy(cmdline + current_len, argv[i], arg_len);
        current_len += arg_len;
        cmdline[current_len] = '\0';
        
        i++;
    }

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE hNull = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hNull;
    si.hStdError = hNull;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        if (hNull != INVALID_HANDLE_VALUE) CloseHandle(hNull);
        return 0;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    if (hNull != INVALID_HANDLE_VALUE) CloseHandle(hNull);

    return exit_code == 0;
}

#endif

/* 
 * lp_run_capture: Safe alternative to popen()
 * Executes a command and captures its output without shell interpretation.
 * This prevents command injection vulnerabilities.
 */
int lp_run_capture(const char *file, const char *const argv[], 
                   char *output_buf, size_t output_size) {
    if (!file || !argv || !argv[0]) return -1;
    
#ifdef _WIN32
    /* Windows implementation using CreateProcess with pipes */
    SECURITY_ATTRIBUTES sa;
    HANDLE hReadPipe, hWritePipe;
    
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) return -1;
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);
    
    char cmdline[2048] = {0};
    size_t len = 0;
    for (int i = 0; argv[i] && len < sizeof(cmdline) - 1; i++) {
        if (i > 0 && len < sizeof(cmdline) - 1) cmdline[len++] = ' ';
        size_t arglen = strlen(argv[i]);
        if (len + arglen >= sizeof(cmdline) - 1) break;
        memcpy(cmdline + len, argv[i], arglen);
        len += arglen;
    }
    cmdline[len] = '\0';
    
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));
    
    BOOL success = CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    CloseHandle(hWritePipe);
    
    if (!success) {
        CloseHandle(hReadPipe);
        return -1;
    }
    
    DWORD bytes_read = 0;
    DWORD total_read = 0;
    if (output_buf && output_size > 0) {
        while (ReadFile(hReadPipe, output_buf + total_read, (DWORD)(output_size - 1 - total_read), &bytes_read, NULL) && bytes_read > 0) {
            total_read += bytes_read;
            if (total_read >= output_size - 1) break;
        }
        output_buf[total_read] = '\0';
    }
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);
    
    return (int)exit_code;
    
#else
    /* Unix implementation using fork/exec with pipes */
    int pipefd[2];
    pid_t pid;
    
    if (pipe(pipefd) < 0) return -1;
    
    pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
    
    if (pid == 0) {
        /* Child process */
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        
        /* Use execvp for PATH search, execv for absolute path */
        if (strchr(file, '/') || strchr(file, '\\')) {
            execv(file, (char *const *)argv);
        } else {
            execvp(file, (char *const *)argv);
        }
        _exit(127);
    }
    
    /* Parent process */
    close(pipefd[1]);
    
    if (output_buf && output_size > 0) {
        ssize_t total = 0;
        ssize_t n;
        while ((n = read(pipefd[0], output_buf + total, output_size - 1 - total)) > 0) {
            total += n;
            if ((size_t)total >= output_size - 1) break;
        }
        output_buf[total] = '\0';
    }
    
    close(pipefd[0]);
    
    int status;
    waitpid(pid, &status, 0);
    
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
#endif
}
