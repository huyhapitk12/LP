#include <cstdio>
#include <ctime>
#include <vector>
int main(){
    int n=8000;
    std::vector<long long>a(n);
    for(int i=0;i<n;i++)a[i]=n-i;
    struct timespec t0,t1;
    clock_gettime(CLOCK_MONOTONIC,&t0);
    for(int i=0;i<n;i++)for(int j=0;j<n-i-1;j++)if(a[j]>a[j+1])std::swap(a[j],a[j+1]);
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%lld\n%.6f\n",a[0],e);
}
