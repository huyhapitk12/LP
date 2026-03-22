#include <cstdio>
#include <ctime>
#include <string>
#include <algorithm>
int main(){
    std::string s="hello world foo bar baz qux";
    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);
    long long cnt=0;
    for(int i=0;i<200000;i++){
        std::string u=s; std::transform(u.begin(),u.end(),u.begin(),::toupper);
        std::string l=u; std::transform(l.begin(),l.end(),l.begin(),::tolower);
        auto f=l.find("world"); if(f!=std::string::npos)cnt++;
        auto p=l.find("foo"); if(p!=std::string::npos){l.replace(p,3,"xyz");}
        cnt++;
    }
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%lld\n%.6f\n",cnt,e);
}
