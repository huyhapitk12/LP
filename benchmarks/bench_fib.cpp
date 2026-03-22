#include <cstdio>
#include <ctime>
long long fib(int n){return n<=1?n:fib(n-1)+fib(n-2);}
int main(){
    struct timespec t0,t1;
    clock_gettime(CLOCK_MONOTONIC,&t0);
    long long r=fib(38);
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%lld\n%.6f\n",r,e);
}
