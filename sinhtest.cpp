#include <bits/stdc++.h>
using namespace std;

static const int LIM = 100000;
static constexpr double PI = acos(-1.0);

static inline int clampi(long long v) {
    if (v < -LIM) return -LIM;
    if (v >  LIM) return  LIM;
    return (int)v;
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // Usage:
    //   ./gen [N=100] [mode=0] [seed=1]
    // Modes:
    // 0 uniform square
    // 1 grid + noise
    // 2 circle / ring (harder for greedy)
    // 3 two far clusters
    // 4 three clusters
    // 5 comb (two lines + teeth)
    // 6 spiral
    // 7 spokes (many rays from center)

    int N = 100, mode = 0;
    unsigned long long seed = 1;

    if (argc >= 2) N = atoi(argv[1]);
    if (argc >= 3) mode = atoi(argv[2]);
    if (argc >= 4) seed = strtoull(argv[3], nullptr, 10);
    N = max(1, min(100, N));

    mt19937_64 rng(seed);
    auto rnd_int = [&](int lo, int hi) {
        uniform_int_distribution<int> dist(lo, hi);
        return dist(rng);
    };
    auto rnd_real = [&](double lo, double hi) {
        uniform_real_distribution<double> dist(lo, hi);
        return dist(rng);
    };
    auto gauss = [&](double mean, double sd) {
        normal_distribution<double> dist(mean, sd);
        return dist(rng);
    };

    vector<pair<int,int>> pts;
    pts.reserve(N);
    unordered_set<long long> used;
    used.reserve(N * 3);

    auto add_pt = [&](int x, int y) {
        x = clampi(x); y = clampi(y);
        long long key = ( (long long)(x + LIM) << 20 ) ^ (long long)(y + LIM);
        if (used.insert(key).second) pts.push_back({x,y});
    };

    auto fill_uniform = [&]() {
        while ((int)pts.size() < N) add_pt(rnd_int(-LIM, LIM), rnd_int(-LIM, LIM));
    };

    if (mode == 0) {
        fill_uniform();
    }
    else if (mode == 1) {
        // grid + small noise
        int s = (int)ceil(sqrt((double)N));
        int step = max(1, (2*LIM) / max(1, s-1));
        int noise = max(1, step / 5);
        for (int i = 0; i < s && (int)pts.size() < N; i++) {
            for (int j = 0; j < s && (int)pts.size() < N; j++) {
                int x = -LIM + i*step + rnd_int(-noise, noise);
                int y = -LIM + j*step + rnd_int(-noise, noise);
                add_pt(x, y);
            }
        }
        while ((int)pts.size() < N) add_pt(rnd_int(-LIM, LIM), rnd_int(-LIM, LIM));
    }
    else if (mode == 2) {
        // ring / circle with jitter
        double R = 80000.0;
        double jitter = 1500.0;
        for (int i = 0; i < N; i++) {
            double ang = (2.0*PI*i)/N + rnd_real(-0.02, 0.02);
            double r = R + rnd_real(-jitter, jitter);
            int x = (int)llround(r*cos(ang) + rnd_real(-200.0, 200.0));
            int y = (int)llround(r*sin(ang) + rnd_real(-200.0, 200.0));
            add_pt(x,y);
        }
        while ((int)pts.size() < N) add_pt(rnd_int(-LIM, LIM), rnd_int(-LIM, LIM));
    }
    else if (mode == 3) {
        // two far clusters
        int n1 = N/2, n2 = N - n1;
        double d = 85000.0, sd = 4500.0;
        while ((int)pts.size() < n1) add_pt((int)llround(gauss(-d, sd)), (int)llround(gauss(0, sd)));
        while ((int)pts.size() < N)  add_pt((int)llround(gauss(+d, sd)), (int)llround(gauss(0, sd)));
    }
    else if (mode == 4) {
        // three clusters (triangle)
        vector<pair<double,double>> C = {{-70000, -10000}, {70000, -10000}, {0, 75000}};
        double sd = 3500.0;
        for (int i = 0; i < N; i++) {
            auto [cx,cy] = C[i % 3];
            add_pt((int)llround(gauss(cx, sd)), (int)llround(gauss(cy, sd)));
        }
        while ((int)pts.size() < N) add_pt(rnd_int(-LIM, LIM), rnd_int(-LIM, LIM));
    }
    else if (mode == 5) {
        // comb: 2 lines + alternating "teeth"
        int baseY1 = -40000, baseY2 = 40000;
        int x0 = -90000, x1 = 90000;
        int teeth = max(2, N/2);
        for (int i = 0; i < teeth && (int)pts.size() < N; i++) {
            double t = (teeth == 1) ? 0.0 : (double)i/(teeth-1);
            int x = (int)llround(x0 + t*(x1 - x0));
            int y = (i & 1) ? baseY1 : baseY2;
            add_pt(x, y);
            if ((int)pts.size() < N) {
                add_pt(x + rnd_int(-1500,1500),
                       y + ((i&1)? +9000 : -9000) + rnd_int(-800,800));
            }
        }
        while ((int)pts.size() < N) add_pt(rnd_int(-LIM, LIM), rnd_int(-LIM, LIM));
    }
    else if (mode == 6) {
        // spiral
        double ang = 0.0;
        for (int i = 0; i < N; i++) {
            double r = 2000.0 + (90000.0 * i) / max(1, N-1);
            ang += 0.45 + rnd_real(-0.03, 0.03);
            int x = (int)llround(r*cos(ang) + rnd_real(-300.0,300.0));
            int y = (int)llround(r*sin(ang) + rnd_real(-300.0,300.0));
            add_pt(x,y);
        }
        while ((int)pts.size() < N) add_pt(rnd_int(-LIM, LIM), rnd_int(-LIM, LIM));
    }
    else if (mode == 7) {
        // spokes: many rays from center
        int rays = 10;
        double R = 90000.0;
        for (int i = 0; i < N; i++) {
            int ray = i % rays;
            double ang = 2.0*PI*ray/rays + rnd_real(-0.01, 0.01);
            double rr = R * (0.2 + 0.8 * ((double)(i / rays) + 1) / (double)(N / rays + 1));
            rr += rnd_real(-2000.0, 2000.0);
            int x = (int)llround(rr*cos(ang));
            int y = (int)llround(rr*sin(ang));
            add_pt(x,y);
        }
        while ((int)pts.size() < N) add_pt(rnd_int(-LIM, LIM), rnd_int(-LIM, LIM));
    }
    else {
        fill_uniform();
    }

    cout << N << "\n";
    for (auto &p: pts) cout << p.first << " " << p.second << "\n";
    return 0;
}
