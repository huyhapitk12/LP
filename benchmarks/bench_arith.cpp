#include <cstdio>
#include <ctime>
int main(){
    struct timespec t0,t1;
    clock_gettime(CLOCK_MONOTONIC,&t0);
    long long total=0;
    for(long long i=0;i<100000000LL;i++) total+=i;
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%lld\n%.6f\n",total,e);
}
