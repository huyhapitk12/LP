#include <cstdio>
#include <ctime>
#include <vector>
int main(){
    int n=5000000;
    std::vector<char>s(n+1,1);s[0]=s[1]=0;
    struct timespec t0,t1;
    clock_gettime(CLOCK_MONOTONIC,&t0);
    for(int i=2;(long long)i*i<=n;i++)if(s[i])for(int j=i*i;j<=n;j+=i)s[j]=0;
    int cnt=0;for(int i=0;i<=n;i++)if(s[i])cnt++;
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%d\n%.6f\n",cnt,e);
}
