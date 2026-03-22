#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <vector>
#include <climits>
int main(){
    int n=500,INF=1000000000;
    std::vector<int>adj(n*n,INF),dist(n,INF),vis(n,0);
    for(int i=0;i<n;i++)for(int j=0;j<n;j++){
        if(i==j)adj[i*n+j]=0;
        else if(abs(i-j)<=2)adj[i*n+j]=(i*7+j*13)%100+1;
    }
    dist[0]=0;
    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);
    for(int _=0;_<n;_++){
        int u=-1;for(int v=0;v<n;v++)if(!vis[v]&&(u==-1||dist[v]<dist[u]))u=v;
        if(u==-1||dist[u]==INF)break;vis[u]=1;
        for(int v=0;v<n;v++)if(adj[u*n+v]<INF&&dist[u]+adj[u*n+v]<dist[v])dist[v]=dist[u]+adj[u*n+v];
    }
    clock_gettime(CLOCK_MONOTONIC,&t1);
    double e=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)*1e-9;
    printf("%d\n%.6f\n",dist[n-1],e);
}
