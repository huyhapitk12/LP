#include <cstdint>
#include <cstdio>
#include <ctime>
int64_t dfs(int node,int d,int mx){if(d>=mx)return 1;return dfs(node*2,d+1,mx)+dfs(node*2+1,d+1,mx);}
int main(){
    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);
    int64_t r=dfs(1,0,20);
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%lld\n%.6f\n",r,e);
}
