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

#endif /* LP_PROCESS_UTILS_H */
