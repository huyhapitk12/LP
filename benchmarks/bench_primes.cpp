#include <cstdint>
#include <cstdio>
#include <ctime>
int cf(int64_t n){int c=0;for(int64_t i=2;i*i<=n;i++)while(n%i==0){c++;n/=i;}if(n>1)c++;return c;}
int main(){
    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);
    int64_t tot=0;for(int i=2;i<100002;i++)tot+=cf(i);
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%lld\n%.6f\n",tot,e);
}
