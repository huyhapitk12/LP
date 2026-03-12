#ifndef LP_PROCESS_UTILS_H
#define LP_PROCESS_UTILS_H

#ifndef _WIN32

#ifndef _P_WAIT
#define _P_WAIT 0
#endif

#include <sys/types.h>

int lp_has_path_sep(const char *s);
int lp_wait_pid(pid_t pid);
void lp_exec_fallback(const char *file, char *const argv[], int search_path);
int _spawnv(int mode, const char *path, const char *const argv[]);
int _spawnvp(int mode, const char *file, const char *const argv[]);
int _spawnl(int mode, const char *path, const char *arg0, ...);

#endif /* _WIN32 */

#endif /* LP_PROCESS_UTILS_H */
