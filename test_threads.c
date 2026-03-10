#include "lp_runtime.h"
#include "lp_thread.h"
#include <stdio.h>

void* my_worker(void* arg) {
    const char* str = (const char*)arg;
    printf("Worker running with arg: %s\n", str);
    return NULL;
}

int main() {
    printf("Starting threads...\n");
    LpThread t1 = lp_thread_spawn(my_worker, "Hello 1");
    LpThread t2 = lp_thread_spawn(my_worker, "Hello 2");
    
    printf("Waiting for threads...\n");
    lp_thread_join(t1);
    lp_thread_join(t2);
    
    printf("Done!\n");
    return 0;
}
