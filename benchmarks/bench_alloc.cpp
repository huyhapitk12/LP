#include <cstdint>
#include <cstdio>
#include <ctime>
#include <vector>
int main(){
    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);
    int64_t tot=0;
    for(int i=0;i<10000;i++){
        std::vector<int64_t>a(1000);
        for(int j=0;j<1000;j++)a[j]=(int64_t)j*i;
        tot+=a[999];
    }
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%lld\n%.6f\n",tot,e);
}
