#include <cstdio>
#include <ctime>
#include <cmath>
int main(){
    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);
    double tot=0;
    for(int i=0;i<1000000;i++){double x=(i+1)*1e-6;tot+=sin(x)*cos(x)+sqrt(x)*log(x+1);}
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%g\n%.6f\n",tot,e);
}
