#include <cstdio>
#include <ctime>
#include <vector>
int main(){
    int rows=1000,cols=1000,total=rows*cols;
    std::vector<int>dist(total,-1),q(total);
    dist[0]=0;int head=0,tail=0;q[tail++]=0;
    int dx[]={0,0,1,-1},dy[]={1,-1,0,0};
    struct timespec t0,t1;
    clock_gettime(CLOCK_MONOTONIC,&t0);
    while(head<tail){
        int cur=q[head++];int r=cur/cols,c=cur%cols;
        for(int d=0;d<4;d++){
            int nr=r+dx[d],nc=c+dy[d];
            if(nr>=0&&nr<rows&&nc>=0&&nc<cols){
                int nb=nr*cols+nc;
                if(dist[nb]==-1){dist[nb]=dist[cur]+1;q[tail++]=nb;}
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%d\n%.6f\n",dist[total-1],e);
}
