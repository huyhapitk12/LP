#include <cstdio>
#include <ctime>
#include <fstream>
#include <string>
int main(){
    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);
    {std::ofstream f("/tmp/cpp_io_test.txt");for(int i=0;i<50000;i++)f<<i<<"\n";}
    std::string all,line; long long len=0;
    {std::ifstream f("/tmp/cpp_io_test.txt");while(std::getline(f,line))len+=line.size()+1;}
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%lld\n%.6f\n",len,e);
}
