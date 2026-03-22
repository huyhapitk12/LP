#include <cstdint>
#include <cstdio>
#include <ctime>
#include <vector>
int part(std::vector<int64_t>&a,int lo,int hi){int64_t pv=a[hi];int i=lo-1;for(int j=lo;j<hi;j++)if(a[j]<=pv)std::swap(a[++i],a[j]);std::swap(a[i+1],a[hi]);return i+1;}
void qs(std::vector<int64_t>&a,int lo,int hi){if(lo<hi){int p=part(a,lo,hi);qs(a,lo,p-1);qs(a,p+1,hi);}}
int main(){
    int n=100000; std::vector<int64_t>a(n);
    for(int i=0;i<n;i++)a[i]=((int64_t)i*1103515245+12345)%1000000;
    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);
    qs(a,0,n-1);
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%lld\n%.6f\n",a[n/2],e);
}
