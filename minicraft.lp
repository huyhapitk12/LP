# ================================================================
#  MiniCraft - Minecraft-style voxel game in LP Language
#  ----------------------------------------------------------------
#  Controls:
#    WASD        = Move            Mouse Move  = Look around
#    Space       = Jump / Fly up   LShift      = Fly down
#    Left Click  = Break block     Right Click = Place block
#    1-9         = Select block    F           = Toggle fly mode
#    ESC         = Quit
#  ----------------------------------------------------------------
#  Run:  lp minicraft.lp
#  GCC:  lp minicraft.lp --gcc
# ================================================================

import gui
import math

# ================================================================
#  CONSTANTS
# ================================================================
WW = 32
WH = 20
WD = 32
SCR_W = 1024
SCR_H = 768
SENS = 0.20
SPEED = 0.10
FLY_SPD = 0.18
GRAV = -0.012
JUMP_V = 0.16
PW = 0.28
PH = 1.62
EYE_H = 1.52

PI = math.pi

# ================================================================
#  BLOCK TYPES
# ================================================================
B_AIR = 0
B_GRASS = 1
B_DIRT = 2
B_STONE = 3
B_SAND = 4
B_WOOD = 5
B_LEAVES = 6
B_PLANKS = 7
B_COBBLE = 8
B_BRICK = 9
B_SNOW = 10
B_BEDROCK = 11

block_name = ["Air", "Grass", "Dirt", "Stone", "Sand", "Oak Log",
              "Leaves", "Planks", "Cobble", "Bricks", "Snow", "Bedrock"]

# Block colors: [top_r, top_g, top_b, side_r, side_g, side_b, bot_r, bot_g, bot_b]
block_col = [
    [0,0,0, 0,0,0, 0,0,0],
    [77,199,46, 140,89,38, 128,76,31],
    [128,76,31, 128,76,31, 115,71,26],
    [133,133,133, 122,122,122, 107,107,107],
    [230,217,148, 224,209,133, 209,191,115],
    [107,71,31, 97,61,26, 89,56,20],
    [56,158,26, 46,133,20, 51,148,23],
    [184,133,56, 173,122,46, 166,115,38],
    [107,107,107, 97,97,97, 89,89,89],
    [179,71,56, 166,64,51, 153,56,46],
    [242,242,250, 230,230,237, 217,217,224],
    [51,51,51, 46,46,46, 38,38,38]
]

# ================================================================
#  GLOBAL STATE
# ================================================================
win = 0
canvas = 0
px = 0.0
py = 0.0
pz = 0.0
yaw = 0.0
pitch = 0.0
vy = 0.0
on_ground = 0
fly_mode = 1
mouse_dx = 0
mouse_dy = 0
last_mx = 0
last_my = 0
game_active = 0
screen_cx = 0
screen_cy = 0
selected_slot = 0
hotbar = [B_GRASS, B_DIRT, B_STONE, B_SAND, B_WOOD,
          B_LEAVES, B_PLANKS, B_COBBLE, B_BRICK]
display_fps = 0
frame_count = 0
last_fps_time = 0
last_tick = 0
mesh_dirty = 1
last_chunk_x = -1
last_chunk_z = -1

# Edge-detection state for polled input
prev_esc = 0
prev_f = 0
prev_lmb = 0
prev_rmb = 0
prev_keys = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

# ================================================================
#  WORLD DATA  (3D list)
# ================================================================
world = []

def world_init():
    world = []
    for x in range(WW):
        plane = []
        for y in range(WH):
            row = []
            for z in range(WD):
                row.append(0)
            plane.append(row)
        world.append(plane)

def in_bounds(x, y, z):
    if x >= 0 and x < WW and y >= 0 and y < WH and z >= 0 and z < WD:
        return 1
    return 0

def get_blk(x, y, z):
    if in_bounds(x, y, z) == 0:
        if y < 0:
            return B_STONE
        return B_AIR
    return int(world[x][y][z])

def set_blk(x, y, z, t):
    if in_bounds(x, y, z):
        world[x][y][z] = t

def is_solid(x, y, z):
    return get_blk(x, y, z) != B_AIR

# ================================================================
#  NOISE & TERRAIN
# ================================================================
def uhash(ax, az):
    x = int(ax)
    z = int(az)
    n = x * 374761393 + z * 668265263 + 1013904223
    n = ((n >> 13) ^ n) * 1274126177
    return (n >> 16) ^ n

def noise2d(ax, az):
    x = int(ax)
    z = int(az)
    ix = x >> 4
    iz = z >> 4
    fx = (x & 15) / 16.0
    fz = (z & 15) / 16.0
    fx = fx * fx * (3.0 - 2.0 * fx)
    fz = fz * fz * (3.0 - 2.0 * fz)
    v00 = (uhash(ix, iz) & 65535) / 65535.0
    v10 = (uhash(ix + 1, iz) & 65535) / 65535.0
    v01 = (uhash(ix, iz + 1) & 65535) / 65535.0
    v11 = (uhash(ix + 1, iz + 1) & 65535) / 65535.0
    return (v00 * (1.0 - fx) + v10 * fx) * (1.0 - fz) + (v01 * (1.0 - fx) + v11 * fx) * fz

def terrain_h(x, z):
    h = noise2d(x, z) * 14.0 + noise2d(x * 2, z * 2) * 7.0 + noise2d(x * 4, z * 4) * 3.0
    r = int(h + 3.0)
    if r < 1:
        r = 1
    if r >= WH:
        r = WH - 1
    return r

def place_tree(ax, ay, az):
    tx = int(ax)
    ty = int(ay)
    tz = int(az)
    trunk = 4 + (uhash(tx, tz) & 3)
    for i in range(trunk):
        if in_bounds(tx, ty + i, tz):
            world[tx][ty + i][tz] = B_WOOD
    for dx in range(-2, 3):
        for dz in range(-2, 3):
            for dy in range(trunk - 2, trunk + 2):
                adx = dx
                adz = dz
                if adx < 0:
                    adx = 0 - adx
                if adz < 0:
                    adz = 0 - adz
                if adx == 2 and adz == 2 and dy < trunk:
                    continue
                if dy == trunk + 1 and adx > 1 and adz > 1:
                    continue
                lx = tx + dx
                ly = ty + dy
                lz = tz + dz
                if in_bounds(lx, ly, lz) and world[lx][ly][lz] == B_AIR:
                    world[lx][ly][lz] = B_LEAVES

def generate_world():
    world_init()
    for x in range(WW):
        for z in range(WD):
            h = terrain_h(x, z)
            for y in range(WH):
                if y == 0:
                    world[x][y][z] = B_BEDROCK
                else:
                    if y < h - 4:
                        world[x][y][z] = B_STONE
                    else:
                        if y < h:
                            world[x][y][z] = B_DIRT
                        else:
                            if y == h:
                                if h < 7:
                                    world[x][y][z] = B_SAND
                                else:
                                    if h > 18:
                                        world[x][y][z] = B_SNOW
                                    else:
                                        world[x][y][z] = B_GRASS
    for x in range(3, WW - 3, 4):
        for z in range(3, WD - 3, 4):
            h = terrain_h(x, z)
            if h >= 7 and h <= 18 and (uhash(x * 7, z * 13) & 3) == 0:
                tx = x + (uhash(x, z) & 1)
                tz = z + (uhash(z, x) & 1)
                if in_bounds(tx, h + 1, tz):
                    place_tree(tx, h + 1, tz)

# ================================================================
#  COLLISION
# ================================================================
def player_collides(cx, cy, cz):
    cx = float(cx)
    cy = float(cy)
    cz = float(cz)
    x0 = int(cx - 0.28)
    x1 = int(cx + 0.28)
    y0 = int(cy)
    y1 = int(cy + 1.61)
    z0 = int(cz - 0.28)
    z1 = int(cz + 0.28)
    for bx in range(x0, x1 + 1):
        for by in range(y0, y1 + 1):
            for bz in range(z0, z1 + 1):
                if is_solid(bx, by, bz):
                    return 1
    return 0

# ================================================================
#  RAYCASTING  (DDA voxel traversal)
# ================================================================
face_nx = [1, -1, 0, 0, 0, 0]
face_ny = [0, 0, 1, -1, 0, 0]
face_nz = [0, 0, 0, 0, 1, -1]

def my_abs(v):
    v = float(v)
    if v < 0.0:
        return 0.0 - v
    return v

def raycast(ox, oy, oz, dx, dy, dz):
    ox = float(ox)
    oy = float(oy)
    oz = float(oz)
    dx = float(dx)
    dy = float(dy)
    dz = float(dz)
    res = [0, 0, 0, 0, 0]
    x = int(ox)
    y = int(oy)
    z = int(oz)
    face = 0
    sx = 1
    sy = 1
    sz = 1
    if dx <= 0:
        sx = -1
    if dy <= 0:
        sy = -1
    if dz <= 0:
        sz = -1
    tdx = 1e30
    tdy = 1e30
    tdz = 1e30
    if dx != 0:
        tdx = my_abs(1.0 / dx)
    if dy != 0:
        tdy = my_abs(1.0 / dy)
    if dz != 0:
        tdz = my_abs(1.0 / dz)
    tmx = 1e30
    tmy = 1e30
    tmz = 1e30
    if dx != 0:
        if dx > 0:
            tmx = (x + 1.0 - ox) * tdx
        else:
            tmx = (x - ox) * tdx
    if dy != 0:
        if dy > 0:
            tmy = (y + 1.0 - oy) * tdy
        else:
            tmy = (y - oy) * tdy
    if dz != 0:
        if dz > 0:
            tmz = (z + 1.0 - oz) * tdz
        else:
            tmz = (z - oz) * tdz
    for i in range(100):
        if in_bounds(x, y, z) and world[x][y][z] != B_AIR:
            res[0] = 1
            res[1] = x
            res[2] = y
            res[3] = z
            res[4] = face
            return res
        if tmx < tmy:
            if tmx < tmz:
                x = x + sx
                tmx = tmx + tdx
                if sx > 0:
                    face = 1
                else:
                    face = 0
            else:
                z = z + sz
                tmz = tmz + tdz
                if sz > 0:
                    face = 5
                else:
                    face = 4
        else:
            if tmy < tmz:
                y = y + sy
                tmy = tmy + tdy
                if sy > 0:
                    face = 3
                else:
                    face = 2
            else:
                z = z + sz
                tmz = tmz + tdz
                if sz > 0:
                    face = 5
                else:
                    face = 4
    return res

# ================================================================
#  BLOCK INTERACTION  (break / place)
# ================================================================
def do_break_block():
    yr = yaw * PI / 180.0
    pr = pitch * PI / 180.0
    fdx = math.sin(yr) * math.cos(pr)
    fdy = math.sin(pr)
    fdz = math.cos(yr) * math.cos(pr)
    result = raycast(px, py + EYE_H, pz, fdx, fdy, fdz)
    hit = result[0]
    if hit == 0:
        return
    hx = result[1]
    hy = result[2]
    hz = result[3]
    if world[hx][hy][hz] != B_BEDROCK:
        set_blk(hx, hy, hz, B_AIR)
        mesh_dirty = 1

def do_place_block():
    yr = yaw * PI / 180.0
    pr = pitch * PI / 180.0
    fdx = math.sin(yr) * math.cos(pr)
    fdy = math.sin(pr)
    fdz = math.cos(yr) * math.cos(pr)
    result = raycast(px, py + EYE_H, pz, fdx, fdy, fdz)
    hit = result[0]
    if hit == 0:
        return
    hx = result[1]
    hy = result[2]
    hz = result[3]
    face = result[4]
    nx = hx + int(face_nx[face])
    ny = hy + int(face_ny[face])
    nz = hz + int(face_nz[face])
    if in_bounds(nx, ny, nz) == 0:
        return
    if world[nx][ny][nz] != B_AIR:
        return
    px0 = int(px - 0.28)
    px1 = int(px + 0.28)
    py0 = int(py)
    py1 = int(py + 1.61)
    pz0 = int(pz - 0.28)
    pz1 = int(pz + 0.28)
    if nx >= px0 and nx <= px1 and ny >= py0 and ny <= py1 and nz >= pz0 and nz <= pz1:
        return
    set_blk(nx, ny, nz, hotbar[selected_slot])
    mesh_dirty = 1

# ================================================================
#  GAME UPDATE
# ================================================================
def game_update():
    if game_active == 0:
        return
    yaw = yaw + mouse_dx * SENS
    pitch = pitch - mouse_dy * SENS
    if pitch > 89.0:
        pitch = 89.0
    if pitch < -89.0:
        pitch = -89.0
    mouse_dx = 0
    mouse_dy = 0
    yr = yaw * PI / 180.0
    fwd_x = math.sin(yr)
    fwd_z = math.cos(yr)
    rgt_x = math.cos(yr)
    rgt_z = 0.0 - math.sin(yr)
    spd = SPEED
    if fly_mode:
        spd = FLY_SPD
    mvx = 0.0
    mvz = 0.0
    mvy = 0.0
    if gui.get_key_state(87):
        mvx = mvx + fwd_x * spd
        mvz = mvz + fwd_z * spd
    if gui.get_key_state(83):
        mvx = mvx - fwd_x * spd
        mvz = mvz - fwd_z * spd
    if gui.get_key_state(65):
        mvx = mvx - rgt_x * spd
        mvz = mvz - rgt_z * spd
    if gui.get_key_state(68):
        mvx = mvx + rgt_x * spd
        mvz = mvz + rgt_z * spd
    if fly_mode:
        if gui.get_key_state(32):
            mvy = FLY_SPD
        if gui.get_key_state(16):
            mvy = 0.0 - FLY_SPD
        vy = 0.0
    else:
        vy = vy + GRAV
        if gui.get_key_state(32) and on_ground:
            vy = JUMP_V
    nx = px + mvx
    if player_collides(nx, py, pz) == 0:
        px = nx
    nz = pz + mvz
    if player_collides(px, py, nz) == 0:
        pz = nz
    ny = py + mvy
    if player_collides(px, ny, pz) == 0:
        py = ny
        on_ground = 0
    else:
        if vy < 0:
            on_ground = 1
        vy = 0.0
    if py < 0:
        py = 0
        vy = 0
        on_ground = 1
    if px < PW:
        px = PW
    if px > WW - PW:
        px = WW - PW
    if pz < PW:
        pz = PW
    if pz > WD - PW:
        pz = WD - PW
    # Rebuild mesh when player crosses block boundary (for distance culling)
    cx = int(px)
    cz = int(pz)
    if cx != last_chunk_x or cz != last_chunk_z:
        last_chunk_x = cx
        last_chunk_z = cz

# ================================================================
#  RENDERING
# ================================================================
def render():
    yr = yaw * PI / 180.0
    pr = pitch * PI / 180.0
    gui.clear3d(canvas, 0.53, 0.81, 0.92)
    ex = px
    ey = py + EYE_H
    ez = pz
    cx = px + math.sin(yr) * math.cos(pr)
    cy = py + EYE_H + math.sin(pr)
    cz = pz + math.cos(yr) * math.cos(pr)
    gui.look_at(ex, ey, ez, cx, cy, cz, 0.0, 1.0, 0.0)
    cw = gui.get_canvas_w(canvas)
    ch = gui.get_canvas_h(canvas)
    if cw <= 0:
        cw = SCR_W
    if ch <= 0:
        ch = SCR_H
    gui.perspective(70.0, float(cw) / float(ch), 0.1, 200.0)
    gui.enable_depth_test()
    gui.disable_lighting()
    if mesh_dirty:
        gui.begin3d(1)
        for x in range(WW):
            for y in range(WH):
                for z in range(WD):
                    bt = int(world[x][y][z])
                    if bt == B_AIR:
                        continue
                    ddx = x + 0.5 - px
                    ddz = z + 0.5 - pz
                    if ddx * ddx + ddz * ddz > 2500.0:
                        continue
                    col = block_col[bt]
                    tr = int(col[0]) / 255.0
                    tg = int(col[1]) / 255.0
                    tb = int(col[2]) / 255.0
                    sr = int(col[3]) / 255.0
                    sg = int(col[4]) / 255.0
                    sb = int(col[5]) / 255.0
                    br = int(col[6]) / 255.0
                    bg_ = int(col[7]) / 255.0
                    bb = int(col[8]) / 255.0
                    if is_solid(x + 1, y, z) == 0:
                        gui.color3d(sr * 0.85, sg * 0.85, sb * 0.85)
                        gui.normal3d(1.0, 0.0, 0.0)
                        gui.vertex3d(float(x + 1), float(y), float(z))
                        gui.vertex3d(float(x + 1), float(y + 1), float(z))
                        gui.vertex3d(float(x + 1), float(y + 1), float(z + 1))
                        gui.vertex3d(float(x + 1), float(y), float(z + 1))
                    if is_solid(x - 1, y, z) == 0:
                        gui.color3d(sr * 0.75, sg * 0.75, sb * 0.75)
                        gui.normal3d(-1.0, 0.0, 0.0)
                        gui.vertex3d(float(x), float(y), float(z + 1))
                        gui.vertex3d(float(x), float(y + 1), float(z + 1))
                        gui.vertex3d(float(x), float(y + 1), float(z))
                        gui.vertex3d(float(x), float(y), float(z))
                    if is_solid(x, y + 1, z) == 0:
                        gui.color3d(tr, tg, tb)
                        gui.normal3d(0.0, 1.0, 0.0)
                        gui.vertex3d(float(x), float(y + 1), float(z))
                        gui.vertex3d(float(x + 1), float(y + 1), float(z))
                        gui.vertex3d(float(x + 1), float(y + 1), float(z + 1))
                        gui.vertex3d(float(x), float(y + 1), float(z + 1))
                    if is_solid(x, y - 1, z) == 0:
                        gui.color3d(br * 0.60, bg_ * 0.60, bb * 0.60)
                        gui.normal3d(0.0, -1.0, 0.0)
                        gui.vertex3d(float(x), float(y), float(z + 1))
                        gui.vertex3d(float(x + 1), float(y), float(z + 1))
                        gui.vertex3d(float(x + 1), float(y), float(z))
                        gui.vertex3d(float(x), float(y), float(z))
                    if is_solid(x, y, z + 1) == 0:
                        gui.color3d(sr * 0.95, sg * 0.95, sb * 0.95)
                        gui.normal3d(0.0, 0.0, 1.0)
                        gui.vertex3d(float(x), float(y), float(z + 1))
                        gui.vertex3d(float(x + 1), float(y), float(z + 1))
                        gui.vertex3d(float(x + 1), float(y + 1), float(z + 1))
                        gui.vertex3d(float(x), float(y + 1), float(z + 1))
                    if is_solid(x, y, z - 1) == 0:
                        gui.color3d(sr * 0.70, sg * 0.70, sb * 0.70)
                        gui.normal3d(0.0, 0.0, -1.0)
                        gui.vertex3d(float(x + 1), float(y), float(z))
                        gui.vertex3d(float(x), float(y), float(z))
                        gui.vertex3d(float(x), float(y + 1), float(z))
                        gui.vertex3d(float(x + 1), float(y + 1), float(z))
        gui.end3d()
        ok = gui.save_batch()
        if ok:
            mesh_dirty = 0 
    else:
        gui.draw_cached(1)
    if game_active:
        fdx = math.sin(yr) * math.cos(pr)
        fdy = math.sin(pr)
        fdz = math.cos(yr) * math.cos(pr)
        result = raycast(px, py + EYE_H, pz, fdx, fdy, fdz)
        if result[0] == 1:
            hx = float(result[1])
            hy = float(result[2])
            hz = float(result[3])
            e = 0.005
            x0 = hx - e
            x1 = hx + 1.0 + e
            y0 = hy - e
            y1 = hy + 1.0 + e
            z0 = hz - e
            z1 = hz + 1.0 + e
            gui.begin3d(2)
            gui.color3d(0.0, 0.0, 0.0)
            gui.vertex3d(x0, y0, z0)
            gui.vertex3d(x1, y0, z0)
            gui.vertex3d(x1, y0, z0)
            gui.vertex3d(x1, y0, z1)
            gui.vertex3d(x1, y0, z1)
            gui.vertex3d(x0, y0, z1)
            gui.vertex3d(x0, y0, z1)
            gui.vertex3d(x0, y0, z0)
            gui.vertex3d(x0, y1, z0)
            gui.vertex3d(x1, y1, z0)
            gui.vertex3d(x1, y1, z0)
            gui.vertex3d(x1, y1, z1)
            gui.vertex3d(x1, y1, z1)
            gui.vertex3d(x0, y1, z1)
            gui.vertex3d(x0, y1, z1)
            gui.vertex3d(x0, y1, z0)
            gui.vertex3d(x0, y0, z0)
            gui.vertex3d(x0, y1, z0)
            gui.vertex3d(x1, y0, z0)
            gui.vertex3d(x1, y1, z0)
            gui.vertex3d(x1, y0, z1)
            gui.vertex3d(x1, y1, z1)
            gui.vertex3d(x0, y0, z1)
            gui.vertex3d(x0, y1, z1)
            gui.end3d()
    gui.present3d(canvas)
    mode_str = "Fly"
    if fly_mode == 0:
        mode_str = "Walk"
    bn = block_name[int(hotbar[selected_slot])]
    title = f"MiniCraft | FPS: {display_fps} | {mode_str} | [{px} {py} {pz}] | {bn} [{selected_slot + 1}]"
    if game_active == 0:
        title = title + "  -- Click to play --"
    gui.set_title(win, title)

# ================================================================
#  GAME LOOP  (timer callback + polled input)
# ================================================================
def game_loop():
    # Tính FPS
    tc = gui.get_tick_count()
    if tc - last_fps_time > 1000:
        display_fps = frame_count
        frame_count = 0
        last_fps_time = tc
    frame_count = frame_count + 1

    if game_active:
        mx = gui.get_mouse_x()
        my = gui.get_mouse_y()
        mouse_dx = mouse_dx + mx - screen_cx
        mouse_dy = mouse_dy + my - screen_cy
        gui.set_mouse_pos(canvas, screen_cx, screen_cy)
    else:
        lmb = gui.get_key_state(1)
        if lmb and prev_lmb == 0:
            game_active = 1
            gui.show_cursor(0)
            screen_cx = gui.get_mouse_x()
            screen_cy = gui.get_mouse_y()
            gui.set_mouse_pos(canvas, screen_cx, screen_cy)
        prev_lmb = lmb

    game_update()
    render()

    if game_active:
        ks = gui.get_key_state(27)
        if ks and prev_esc == 0:
            gui.show_cursor(1)
            gui.close(win)
        prev_esc = ks

        kf = gui.get_key_state(70)
        if kf and prev_f == 0:
            fly_mode = 1 - fly_mode
        prev_f = kf

        for i in range(9):
            ks = gui.get_key_state(49 + i)
            if ks and prev_keys[i] == 0:
                selected_slot = i
            prev_keys[i] = ks

        lmb = gui.get_key_state(1)
        if lmb and prev_lmb == 0:
            do_break_block()
        prev_lmb = lmb

        rmb = gui.get_key_state(2)
        if rmb and prev_rmb == 0:
            do_place_block()
        prev_rmb = rmb

# ================================================================
#  MAIN
# ================================================================
def main():
    gui.prefer_discrete_gpu()
    print("Generating world...")
    generate_world()
    print("World generated!")
    win = gui.window("MiniCraft - Generating world...", SCR_W, SCR_H)
    canvas = gui.canvas3d(win, 0, 0, SCR_W, SCR_H)
    gui.set_vsync(0)
    gui.set_msaa(4)
    gui.on_timer(game_loop)
    gui.set_fps(win, 60)
    spawn_h = 0
    for sy in range(WH - 1, -1, -1):
        if world[WW // 2][sy][WD // 2] != B_AIR:
            spawn_h = sy + 1
            break
    px = WW / 2.0 + 0.5
    py = float(spawn_h) + 0.01
    pz = WD / 2.0 + 0.5
    last_mx = gui.get_mouse_x()
    last_my = gui.get_mouse_y()
    bname = gui.backend_name()
    print("=== MiniCraft ===")
    print("World: " + str(WW) + "x" + str(WH) + "x" + str(WD) + " | Backend: " + bname)
    print("WASD=Move  Mouse=Look  Space=Jump/FlyUp  Shift=FlyDown")
    print("LClick=Break  RClick=Place  1-9=Select  F=Fly  ESC=Quit")
    print("Click window to activate game.")
    gui.run_game(win)

main()
