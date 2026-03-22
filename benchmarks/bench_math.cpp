#include <cstdio>
#include <ctime>
#include <cmath>
int main(){
    double total=0.0;
    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);
    for(long long i=0;i<5000000LL;i++)
        total+=sin(i*0.001)+cos(i*0.002)+sqrt(i+1);
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%.4f\n%.6f\n",total,e);
}
