#include <cstdio>
#include <ctime>
#include <vector>
#include <algorithm>
int main(){
    int n=1000; std::vector<int>a(n),b(n);
    for(int i=0;i<n;i++){a[i]=(int)(((int64_t)i*1103515245+12345)%26);b[i]=(int)(((int64_t)i*22695477+1)%26);}
    std::vector<int>dp((n+1)*(n+1),0);
    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);
    for(int i=1;i<=n;i++)for(int j=1;j<=n;j++){
        if(a[i-1]==b[j-1])dp[i*(n+1)+j]=dp[(i-1)*(n+1)+(j-1)]+1;
        else dp[i*(n+1)+j]=std::max(dp[(i-1)*(n+1)+j],dp[i*(n+1)+(j-1)]);
    }
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%d\n%.6f\n",dp[n*(n+1)+n],e);
}
