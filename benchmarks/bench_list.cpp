#include <cstdio>
#include <ctime>
#include <vector>
int main(){
    int n=200000;
    std::vector<long long>vals(n); std::vector<int>nexts(n);
    for(int i=0;i<n;i++){vals[i]=i*3+1;nexts[i]=i+1;}
    nexts[n-1]=-1;
    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);
    long long total=0;
    for(int p=0;p<20;p++){int cur=0;while(cur!=-1){total+=vals[cur];cur=nexts[cur];}}
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%lld\n%.6f\n",total,e);
}
