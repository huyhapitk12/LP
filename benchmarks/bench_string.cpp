#include <cstdio>
#include <ctime>
#include <string>
#include <algorithm>
int main(){
    std::string s="abcdefghijklmnopqrstuvwxyz";
    long long total=0;
    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);
    for(int i=0;i<500000;i++){
        std::string u=s; for(auto&c:u)c=toupper(c);
        std::string l=u; for(auto&c:l)c=tolower(c);
        total+=(long long)l.find("xyz");
    }
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%lld\n%.6f\n",total,e);
}
