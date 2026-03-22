#include <cstdint>
#include <cstdio>
#include <ctime>
#include <vector>
int bs(std::vector<int64_t>&a,int n,int64_t t){int lo=0,hi=n-1;while(lo<=hi){int m=(lo+hi)/2;if(a[m]==t)return m;else if(a[m]<t)lo=m+1;else hi=m-1;}return -1;}
int main(){
    int n=1000000; std::vector<int64_t>a(n);
    for(int i=0;i<n;i++)a[i]=(int64_t)i*2;
    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);
    long long found=0;
    for(int i=0;i<1000000;i++){int r=bs(a,n,(int64_t)(i*7)%(n*2));if(r>=0)found++;}
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%lld\n%.6f\n",found,e);
}
