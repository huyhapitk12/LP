import math
import time
import random
import dsa

# ---------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------
def copy_arr(src: int[], dest: int[], n: int):
    for i in range(n):
        dest[i] = src[i]

def rebuild_pos(t: int[], pos: int[], N: int):
    for i in range(N):
        pos[t[i]] = i

def tour_len(t: int[], N: int, D: float[]) -> float:
    s: float = 0.0
    for i in range(N):
        s = s + D[t[i] * N + t[(i + 1) % N]]
    return s

# ---------------------------------------------------------------
# 2-opt with NN list + DLB + pos array
# ---------------------------------------------------------------
def two_opt(t: int[], pos: int[], N: int, D: float[], NN: int[], NK: int, dlb: int[]) -> int:
    improved: int = 0
    any_imp: int = 1
    while any_imp == 1:
        any_imp = 0
        for idx in range(N):
            t1: int = t[idx]
            if dlb[t1] == 1:
                continue
            i: int = pos[t1]
            i1: int = (i + 1) % N
            t2: int = t[i1]
            d12: float = D[t1 * N + t2]
            found: int = 0
            for k in range(NK):
                t3: int = NN[t1 * NK + k]
                if D[t1 * N + t3] >= d12:
                    break
                j: int = pos[t3]
                j1: int = (j + 1) % N
                t4: int = t[j1]
                if t4 == t1:
                    continue
                gain: float = d12 + D[t3 * N + t4] - D[t1 * N + t3] - D[t2 * N + t4]
                if gain > 0.000001:
                    lo: int = i1
                    hi: int = j
                    if i > j:
                        lo = j1
                        hi = i
                    while lo < hi:
                        tmp_v: int = t[lo]
                        t[lo] = t[hi]
                        t[hi] = tmp_v
                        pos[t[lo]] = lo
                        pos[t[hi]] = hi
                        lo = lo + 1
                        hi = hi - 1
                    dlb[t1] = 0
                    dlb[t2] = 0
                    dlb[t3] = 0
                    dlb[t4] = 0
                    any_imp = 1
                    improved = 1
                    found = 1
                    i = pos[t1]
                    i1 = (i + 1) % N
                    t2 = t[i1]
                    d12 = D[t1 * N + t2]
                    break
            if found == 0:
                dlb[t1] = 1
    return improved

# ---------------------------------------------------------------
# OR-opt-1 (relocate single node) with NN list
# ---------------------------------------------------------------
def or_opt_1(t: int[], pos: int[], N: int, D: float[], NN: int[], NK: int) -> int:
    improved: int = 0
    any_imp: int = 1
    while any_imp == 1:
        any_imp = 0
        for i in range(N):
            ip: int = (i - 1 + N) % N
            in_: int = (i + 1) % N
            node: int = t[i]
            pn: int = t[ip]
            nx: int = t[in_]
            rm: float = D[pn * N + node] + D[node * N + nx] - D[pn * N + nx]
            best_j: int = -1
            best_gain: float = 0.000001
            for k in range(NK):
                t3: int = NN[node * NK + k]
                j: int = pos[t3]
                if j == ip or j == i or j == in_:
                    continue
                j1: int = (j + 1) % N
                if j1 == i:
                    continue
                gain: float = rm + D[t[j] * N + t[j1]] - D[t[j] * N + node] - D[node * N + t[j1]]
                if gain > best_gain:
                    best_gain = gain
                    best_j = j
            if best_j < 0:
                continue
            seg0: int = t[i]
            if best_j > i:
                shift: int = best_j - i
                for pp in range(shift):
                    t[i + pp] = t[i + 1 + pp]
                t[best_j] = seg0
                for pp in range(i, best_j + 1):
                    pos[t[pp]] = pp
            else:
                shift2: int = i - best_j - 1
                for pp in range(shift2 - 1, -1, -1):
                    t[best_j + 2 + pp] = t[best_j + 1 + pp]
                t[best_j + 1] = seg0
                for pp in range(best_j + 1, i + 1):
                    pos[t[pp]] = pp
            any_imp = 1
            improved = 1
    return improved

# ---------------------------------------------------------------
# OR-opt-2 (relocate 2-node segment)
# ---------------------------------------------------------------
def or_opt_2(t: int[], pos: int[], N: int, D: float[], NN: int[], NK: int) -> int:
    improved: int = 0
    any_imp: int = 1
    while any_imp == 1:
        any_imp = 0
        for i in range(N - 1):
            ie: int = i + 1
            t_front: int = t[i]
            t_back: int = t[ie]
            ip: int = (i - 1 + N) % N
            in_: int = (ie + 1) % N
            rm: float = D[t[ip] * N + t_front] + D[t_back * N + t[in_]] - D[t[ip] * N + t[in_]]
            best_j: int = -1
            best_gain: float = 0.000001
            for k in range(NK):
                t3: int = NN[t_front * NK + k]
                j: int = pos[t3]
                if j >= i and j <= ie:
                    continue
                j1: int = (j + 1) % N
                if j1 >= i and j1 <= ie:
                    continue
                gain: float = rm + D[t[j] * N + t[j1]] - D[t[j] * N + t_front] - D[t_back * N + t[j1]]
                if gain > best_gain:
                    best_gain = gain
                    best_j = j
            if best_j < 0:
                continue
            s0: int = t[i]
            s1: int = t[ie]
            if i < best_j:
                shift: int = best_j - ie
                for pp in range(shift):
                    t[i + pp] = t[ie + 1 + pp]
                t[best_j - 1] = s0
                t[best_j] = s1
                for pp in range(i, best_j + 1):
                    pos[t[pp]] = pp
            else:
                shift2: int = i - best_j - 1
                for pp in range(shift2 - 1, -1, -1):
                    t[best_j + 3 + pp] = t[best_j + 1 + pp]
                t[best_j + 1] = s0
                t[best_j + 2] = s1
                for pp in range(best_j + 1, ie + 1):
                    pos[t[pp]] = pp
            any_imp = 1
            improved = 1
    return improved

# ---------------------------------------------------------------
# Double Bridge (true 4-cut)
# ---------------------------------------------------------------
def double_bridge(t: int[], tmp: int[], N: int):
    if N < 4:
        return
    a: int = 1 + random.randint(0, N - 2)
    b: int = 1 + random.randint(0, N - 2)
    c: int = 1 + random.randint(0, N - 2)
    while a == b or b == c or a == c:
        a = 1 + random.randint(0, N - 2)
        b = 1 + random.randint(0, N - 2)
        c = 1 + random.randint(0, N - 2)
    if a > b:
        sv: int = a
        a = b
        b = sv
    if b > c:
        sv = b
        b = c
        c = sv
    if a > b:
        sv = a
        a = b
        b = sv
    idx: int = 0
    for i in range(0, a):
        tmp[idx] = t[i]
        idx = idx + 1
    for i in range(c, N):
        tmp[idx] = t[i]
        idx = idx + 1
    for i in range(b, c):
        tmp[idx] = t[i]
        idx = idx + 1
    for i in range(a, b):
        tmp[idx] = t[i]
        idx = idx + 1
    for i in range(N):
        t[i] = tmp[i]

# ---------------------------------------------------------------
# Main
# ---------------------------------------------------------------
def main():
    start_time: float = time.time()
    N: int = dsa.read_int()
    NK: int = 20
    if N == 1:
        dsa.write_str_ln("0.0000")
        dsa.write_int_ln(1)
        dsa.flush()
        return

    X = [0.0] * N
    Y = [0.0] * N
    for i in range(N):
        X[i] = dsa.read_float()
        Y[i] = dsa.read_float()

    D = [0.0] * (N * N)
    for i in range(N):
        for j in range(N):
            dx: float = X[i] - X[j]
            dy: float = Y[i] - Y[j]
            D[i * N + j] = math.sqrt(dx * dx + dy * dy)

    # Nearest-neighbor list — partial selection sort O(N*NK) per row
    NN = [0] * (N * NK)
    used = [0] * N
    for i in range(N):
        for j in range(N):
            used[j] = 0
        used[i] = 1
        for k in range(NK):
            best_nb: int = -1
            best_d: float = 999999999.0
            for j in range(N):
                if used[j] == 0 and D[i * N + j] < best_d:
                    best_d = D[i * N + j]
                    best_nb = j
            if best_nb < 0:
                break
            NN[i * NK + k] = best_nb
            used[best_nb] = 1

    best_t = [0] * N
    tmp_arr = [0] * N
    best: float = 999999999.0

    # Greedy nearest-neighbour init (all N starts)
    for s in range(N):
        vis = [0] * N
        tmp_arr[0] = s
        vis[s] = 1
        cur: float = 0.0
        for i in range(1, N):
            last: int = tmp_arr[i - 1]
            nxt: int = -1
            mn: float = 999999999.0
            # Try NN first
            for k in range(NK):
                nb: int = NN[last * NK + k]
                if vis[nb] == 0:
                    nxt = nb
                    mn = D[last * N + nb]
                    break
            if nxt == -1:
                for j in range(N):
                    if vis[j] == 0 and D[last * N + j] < mn:
                        mn = D[last * N + j]
                        nxt = j
            tmp_arr[i] = nxt
            vis[nxt] = 1
            cur = cur + mn
        cur = cur + D[tmp_arr[N - 1] * N + tmp_arr[0]]
        if cur < best:
            best = cur
            copy_arr(tmp_arr, best_t, N)

    t_arr = [0] * N
    pos = [0] * N
    dlb = [0] * N
    copy_arr(best_t, t_arr, N)
    rebuild_pos(t_arr, pos, N)
    for i in range(N):
        dlb[i] = 0

    # Initial local search
    imp: int = 1
    while imp == 1:
        imp = two_opt(t_arr, pos, N, D, NN, NK, dlb)
        imp = imp | or_opt_1(t_arr, pos, N, D, NN, NK)
        imp = imp | or_opt_2(t_arr, pos, N, D, NN, NK)

    best = tour_len(t_arr, N, D)
    copy_arr(t_arr, best_t, N)

    # ILS: check time every 16 iterations to minimize syscall overhead
    iter_count: int = 0
    while True:
        iter_count = iter_count + 1
        # Check time every 16 iterations
        if iter_count % 16 == 0:
            if (time.time() - start_time) > 0.85:
                break

        copy_arr(best_t, t_arr, N)
        double_bridge(t_arr, tmp_arr, N)
        rebuild_pos(t_arr, pos, N)
        for i in range(N):
            dlb[i] = 0
        imp = 1
        while imp == 1:
            imp = two_opt(t_arr, pos, N, D, NN, NK, dlb)
            imp = imp | or_opt_1(t_arr, pos, N, D, NN, NK)
            imp = imp | or_opt_2(t_arr, pos, N, D, NN, NK)

        cur_len: float = tour_len(t_arr, N, D)
        if cur_len < best:
            best = cur_len
            copy_arr(t_arr, best_t, N)

    # Rotate so node 0 is first
    p: int = 0
    for i in range(N):
        if best_t[i] == 0:
            p = i
            break

    final_t = [0] * N
    fi: int = 0
    for i in range(p, N):
        final_t[fi] = best_t[i]
        fi = fi + 1
    for i in range(0, p):
        final_t[fi] = best_t[i]
        fi = fi + 1

    dsa.write_float_prec(best, 4)
    dsa.writeln()
    for i in range(N):
        dsa.write_int(final_t[i] + 1)
        if i < N - 1:
            dsa.write_str(" ")
    dsa.writeln()
    dsa.flush()

main()
