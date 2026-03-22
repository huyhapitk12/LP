#include <cstdio>
#include <ctime>
#include <vector>
#include <algorithm>
int main(){
    int n=300,W=3000;
    std::vector<long long>wt(n+1),vl(n+1),dp(W+1,0);
    for(int i=1;i<=n;i++){wt[i]=(i*7+3)%50+1;vl[i]=(i*13+5)%100+1;}
    struct timespec t0,t1;
    clock_gettime(CLOCK_MONOTONIC,&t0);
    for(int i=1;i<=n;i++)for(int w=W;w>=wt[i];w--)dp[w]=std::max(dp[w],dp[w-wt[i]]+vl[i]);
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%lld\n%.6f\n",dp[W],e);
}
