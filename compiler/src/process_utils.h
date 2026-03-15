#ifndef LP_PROCESS_UTILS_H
#define LP_PROCESS_UTILS_H

#ifndef _WIN32
#include <sys/types.h>

#ifndef _P_WAIT
#define _P_WAIT 0
#endif

int _spawnv(int mode, const char *path, const char *const argv[]);
int _spawnvp(int mode, const char *file, const char *const argv[]);
int _spawnl(int mode, const char *path, const char *arg0, ...);

#endif /* _WIN32 */

/* Cross-platform: runs a program with arguments and suppresses stdout/stderr.
 * Returns 1 if successful (exit code 0), 0 otherwise. */
int lp_run_silent(const char *file, const char *const argv[]);

/* 
 * Security: Safe command execution functions
 * These functions avoid shell injection by using execve/execvp directly
 * instead of system() which invokes a shell.
 */

/* Run a command with arguments, capturing output to a buffer.
 * This is a safe alternative to popen() that avoids shell interpretation.
 * Returns the exit code, or -1 on error.
 * output_buf can be NULL if output is not needed.
 */
int lp_run_capture(const char *file, const char *const argv[], 
                   char *output_buf, size_t output_size);

/* Validate a filename/path for safety.
 * Returns 1 if safe, 0 if potentially dangerous.
 * Checks for shell metacharacters and path traversal attempts.
 */
int lp_path_is_safe(const char *path);

#endif /* LP_PROCESS_UTILS_H */
