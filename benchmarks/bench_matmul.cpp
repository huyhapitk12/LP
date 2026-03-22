#include <cstdio>
#include <ctime>
#include <vector>
int main(){
    int n=300;
    std::vector<long long>a(n*n),b(n*n),c(n*n,0);
    for(int i=0;i<n*n;i++){a[i]=i%100;b[i]=(i*2)%100;}
    struct timespec t0,t1;
    clock_gettime(CLOCK_MONOTONIC,&t0);
    for(int i=0;i<n;i++)for(int k=0;k<n;k++)for(int j=0;j<n;j++)c[i*n+j]+=a[i*n+k]*b[k*n+j];
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%lld\n%.6f\n",c[n*n-1],e);
}
