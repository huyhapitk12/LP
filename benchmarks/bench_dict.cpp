#include <cstdio>
#include <ctime>
#include <unordered_map>
#include <string>
int main(){
    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);
    std::unordered_map<std::string,int64_t>d;
    for(int i=0;i<100000;i++){auto k=std::to_string(i%10000);d[k]++;}
    int64_t tot=0;
    for(int i=0;i<10000;i++){auto k=std::to_string(i);auto it=d.find(k);if(it!=d.end())tot+=it->second;}
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%lld\n%.6f\n",tot,e);
}
