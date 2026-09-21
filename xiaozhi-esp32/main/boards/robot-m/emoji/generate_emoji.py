#!/usr/bin/env python3
"""大头 robot-m 像素机甲表情包生成器。

输出 21 个 192x192 透明底循环 GIF，文件名与固件情绪键一一对应。
角色：枪灰像素机甲头 + 青色光学目镜 + 霓虹紫角灯（赛博朋克像素风）。

重新生成：python3 generate_emoji.py
"""
import os
from PIL import Image

# ---------------------------------------------------------------- 调色板
NONE = None
FILL = (52, 62, 80, 255)        # 机身填充（提亮，保证头型轮廓）
METAL = FILL
METAL_D = (16, 22, 33, 255)     # 描边/挖空
CYAN = (0, 240, 255, 255)
CYAN_SOFT = (160, 235, 255, 255)
PURPLE = (178, 80, 255, 255)
PURPLE_DIM = (96, 52, 140, 255)
YELLOW = (255, 220, 60, 255)
WHITE = (225, 245, 255, 255)

N = 16          # 16x16 逻辑像素
CELL = 12       # 每像素放大到 12px → 192x192（320 宽屏占 60%）
S = N * CELL    # 画布边长
FRAMES = 6
DURATION = 120

# ---------------------------------------------------------------- 基础头型
HEAD_ROWS = {
    4: range(2, 16),
    5: range(1, 15),
    6: range(0, 16),
    7: range(0, 16),
    12: range(0, 16),
    13: range(0, 16),
    14: range(1, 15),
    15: range(2, 14),
}
VISOR = [(x, y) for y in range(8, 12) for x in range(2, 14)]
HORN_STALK = [(2, 2), (2, 3), (1, 3), (13, 2), (13, 3), (14, 3)]
HORN_TIP = [(2, 1), (13, 1)]

LE, RE = (5, 9), (10, 9)

# ---------------------------------------------------------------- 眉眼嘴图元
EYES = {
    "bar":   [(-1, 0), (0, 0), (1, 0)],
    "dot":   [(-1, -1), (0, -1), (-1, 0), (0, 0)],
    "round": [(-1, -1), (0, -1), (1, -1), (-1, 0), (1, 0)],
    "up":    [(-1, 1), (0, 0), (1, 1)],
    "down":  [(-1, 0), (0, 1), (1, 0)],
    "shut":  [(-1, 0), (0, 0), (1, 0)],
    "x":     [(-1, -1), (1, -1), (0, 0), (-1, 1), (1, 1)],
    "half":  [(-1, 1), (0, 1), (1, 1)],
    "heart": [(-1, -1), (1, -1), (-1, 0), (0, 0), (1, 0), (0, 1)],
    "looku": [(-1, -1), (0, -1)],
    "lookd": [(-1, 1), (0, 1)],
}

BROWS = {
    "angry":      ([(3, 7), (4, 7), (5, 6)], [(10, 6), (11, 7), (12, 7)]),
    "sad":        ([(3, 6), (4, 7), (5, 7)], [(10, 7), (11, 7), (12, 6)]),
    "raised":     ([(4, 6), (5, 6)], [(10, 6), (11, 6)]),
    "think":      (None, [(10, 6), (11, 6), (12, 6)]),
    "confident":  ([(3, 7), (4, 7), (5, 7)], [(10, 7), (11, 7), (12, 7)]),
}

MOUTHS = {
    "flat":    ([(6, 14), (7, 14), (8, 14), (9, 14)], []),
    "smile":   ([(5, 13), (10, 13), (6, 14), (7, 14), (8, 14), (9, 14)], []),
    "frown":   ([(5, 14), (10, 14), (6, 13), (7, 13), (8, 13), (9, 13)], []),
    "open":    ([(6, 13), (7, 13), (8, 13), (9, 13), (6, 14), (9, 14)],
                [(7, 14), (8, 14)]),
    "o":       ([(7, 13), (8, 13), (7, 14), (8, 14)], []),
    "tall_o":  ([(7, 13), (8, 13), (7, 14), (8, 14), (7, 15), (8, 15)],
                [(7, 14), (8, 14)]),
    "smirk":   ([(5, 14), (6, 14), (7, 14), (8, 13), (9, 13)], []),
    "wavy":    ([(5, 14), (7, 14), (9, 14), (6, 13), (8, 13)], []),
    "grin":    ([(5, 13), (6, 13), (7, 13), (8, 13), (9, 13), (10, 13),
                 (5, 14), (10, 14)],
                [(6, 14), (7, 14), (8, 14), (9, 14)]),
    "tongue":  ([(6, 13), (7, 13), (8, 13), (9, 13), (6, 14), (9, 14)],
                [(7, 14), (8, 14), (8, 13)]),
    "grit":    ([(5, 14), (6, 14), (7, 13), (8, 14), (9, 13), (10, 14)], []),
    "heart":   ([(6, 13), (8, 13), (6, 14), (7, 14), (8, 14), (7, 15)], []),
    "small":   ([(7, 14), (8, 14)], []),
}

# ---------------------------------------------------------------- 迷你像素精灵
SPRITES = {
    "heart": [
        "X.X",
        "XXX",
        ".X.",
    ],
    "star": [
        ".X.",
        "XXX",
        ".X.",
    ],
    "drop": [
        ".X",
        "XX",
        ".X",
    ],
    "z": [
        "XXX",
        "..X",
        ".X.",
        "XXX",
    ],
    "?": [
        ".X.",
        "..X",
        ".X.",
        ".X.",
    ],
    "!": [
        "X",
        "X",
        ".",
        "X",
    ],
}


def new_grid():
    return [[NONE] * N for _ in range(N)]


def put(g, x, y, color):
    if 0 <= x < N and 0 <= y < N and color is not None:
        g[y][x] = color


def draw_eye(g, name, cx, cy, color=WHITE):
    for dx, dy in EYES[name]:
        put(g, cx + dx, cy + dy, color)
    if name == "round":
        put(g, cx, cy, METAL_D)


def draw_sunglasses(g):
    for y in range(8, 11):
        for x in range(2, 14):
            g[y][x] = METAL_D
    for x, y in [(3, 9), (4, 9), (8, 9), (9, 9)]:
        g[y][x] = WHITE
    g[8][7] = METAL_D
    g[9][7] = METAL_D
    g[10][7] = METAL_D


def build_base_grid(spec, f):
    g = new_grid()
    for y, xs in HEAD_ROWS.items():
        for x in xs:
            g[y][x] = METAL
    for x, y in VISOR:
        g[y][x] = CYAN
    for x, y in HORN_STALK:
        g[y][x] = METAL
    horn_mode = spec.get("horns", "purple")
    if horn_mode == "flash":
        horn_color = YELLOW if f % 2 == 0 else PURPLE
    elif horn_mode == "dim":
        horn_color = PURPLE_DIM
    else:
        horn_color = PURPLE
    for x, y in HORN_TIP:
        g[y][x] = horn_color

    brows = spec.get("brows")
    if brows:
        left, right = BROWS[brows]
        brow_color = YELLOW if brows == "angry" else PURPLE
        for pts in (left, right):
            if pts:
                for x, y in pts:
                    put(g, x, y, brow_color)

    if spec.get("sunglasses"):
        draw_sunglasses(g)
    else:
        blink = spec.get("blink", True) and f == 4
        eye_specs = spec["eyes"]
        if isinstance(eye_specs, str):
            eye_specs = (eye_specs, eye_specs)
        eye_colors = spec.get("eye_color", WHITE)
        if not isinstance(eye_colors, tuple) or (eye_colors and
                not isinstance(eye_colors[0], tuple)):
            eye_colors = (eye_colors, eye_colors)
        centers = spec.get("eye_centers", (LE, RE))
        for i, (cx, cy) in enumerate(centers):
            name = "shut" if blink else eye_specs[i]
            draw_eye(g, name, cx, cy,
                     eye_colors[i] if eye_colors[i] else WHITE)

    mouth = spec.get("mouth")
    if mouth:
        name = mouth[f % len(mouth)] if isinstance(mouth, list) else mouth
        lit, dark = MOUTHS[name]
        for x, y in lit:
            put(g, x, y, spec.get("mouth_color", PURPLE))
        for x, y in dark:
            put(g, x, y, METAL_D)

    if spec.get("blush"):
        for x, y in [(2, 12), (3, 12), (12, 12), (13, 12)]:
            put(g, x, y, PURPLE_DIM)

    # 自动描边：与透明区相邻的机身像素染深色，强化头型轮廓
    for y in range(N):
        for x in range(N):
            if g[y][x] == FILL:
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if not (0 <= nx < N and 0 <= ny < N) or g[ny][nx] is NONE:
                        g[y][x] = METAL_D
                        break
    return g


# ---------------------------------------------------------------- 动效
def motion_offset(spec, f):
    m = spec.get("motion", "bob")
    if m == "bounce":
        return ([0, -6, -10, -6, 0, 0][f], 0)
    if m == "bob":
        return ([0, -3, -5, -3, 0, 0][f], 0)
    if m == "slow":
        return ([0, -2, -3, -2, 0, 0][f], 0)
    if m == "shake":
        return (0, [0, 0, 2, 0, -2, 0][f])
    if m == "wobble":
        return (0, [0, 2, 0, -2, 0, 0][f])
    return (0, 0)


def draw_sprite(img, name, cx, cy, color):
    """cx/cy 单位为格（可为小数），精灵左上角对齐。"""
    rows = SPRITES[name]
    ox = int(round(cx * CELL))
    oy = int(round(cy * CELL))
    for r, row in enumerate(rows):
        for c, ch in enumerate(row):
            if ch == "X":
                x0, y0 = ox + c * CELL, oy + r * CELL
                for yy in range(y0, y0 + CELL):
                    for xx in range(x0, x0 + CELL):
                        if 0 <= xx < S and 0 <= yy < S:
                            img.putpixel((xx, yy), color)


def render_frame(spec, f):
    oy, ox = motion_offset(spec, f)
    g = build_base_grid(spec, f)
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    for y in range(N):
        for x in range(N):
            c = g[y][x]
            if c is not None:
                X, Y = x * CELL + ox, y * CELL + oy
                for dy in range(CELL):
                    for dx in range(CELL):
                        px, py = X + dx, Y + dy
                        if 0 <= px < S and 0 <= py < S:
                            img.putpixel((px, py), c)
    decos = spec.get("decor")
    if decos:
        for sprite, cx, cy, color, follows in decos(spec, f):
            draw_sprite(img, sprite,
                        cx + (ox / CELL if follows else 0),
                        cy + (oy / CELL if follows else 0), color)
    return img


def build_gif(spec, path):
    frames = [render_frame(spec, f) for f in range(FRAMES)]
    frames[0].save(path, save_all=True, append_images=frames[1:],
                   duration=spec.get("dur", DURATION), loop=0,
                   disposal=2, optimize=True)


# ---------------------------------------------------------------- 装饰套路
def deco_tear(spec, f):
    """眼泪从外眼角沿脸颊滑落。"""
    out = []
    y = 12.0 + ((f * 0.9) % 3.5)
    out.append(("drop", 3.4, y, CYAN_SOFT, True))
    if spec.get("two_tears"):
        y2 = 12.0 + (((f * 0.9) + 1.7) % 3.5)
        out.append(("drop", 11.6, y2, CYAN_SOFT, True))
    return out


def deco_hearts(spec, f):
    out = []
    for i, col in enumerate((6.0, 9.0)):
        y = 0.8 - ((f - i * 3) % FRAMES) * 0.9
        if y > -0.5:
            color = PURPLE if (f + i) % 2 == 0 else PURPLE_DIM
            out.append(("heart", col, y, color, False))
    return out


def deco_z(spec, f):
    y = 0.6 - ((f - 3) % FRAMES) * 0.85
    if y > -0.5:
        return [("z", 9.8, y, CYAN if f % 2 == 0 else CYAN_SOFT, False)]
    return []


def deco_sweat(spec, f):
    return [("drop", 13.3, 5.0, CYAN_SOFT, True)]


def deco_q(spec, f):
    if f != 5:
        return [("?", 6.4, 0.0, PURPLE, False)]
    return []


def deco_bang(spec, f):
    if f in (0, 1, 2, 4):
        return [("!", 7.4, 0.0, YELLOW, False)]
    return []


# ---------------------------------------------------------------- 21 个情绪
def specs():
    return {
        "neutral":     dict(eyes="bar", mouth="flat"),
        "happy":       dict(eyes="up", mouth="smile", motion="bounce", horns="flash"),
        "laughing":    dict(eyes="x", mouth="grin", motion="bounce", horns="flash"),
        "funny":       dict(eyes=("x", "up"), mouth="open", motion="wobble",
                            horns="flash", blink=False),
        "sad":         dict(eyes="down", mouth="frown", brows="sad", motion="slow",
                            decor=deco_tear, blink=False),
        "angry":       dict(eyes="dot", mouth="grit", brows="angry", motion="shake",
                            horns="flash", eye_color=YELLOW, blink=False),
        "crying":      dict(eyes="up", mouth="grin", motion="shake",
                            decor=lambda s, f: deco_tear(dict(two_tears=True), f),
                            two_tears=True, blink=False),
        "loving":      dict(eyes="heart", mouth="smile", motion="slow",
                            eye_color=PURPLE, decor=deco_hearts, blink=False),
        "embarrassed": dict(eyes="lookd", mouth="wavy", motion="bob",
                            decor=deco_sweat, blink=False, blush=True),
        "surprised":   dict(eyes="round", mouth="o", brows="raised",
                            motion="still", decor=deco_bang, blink=False),
        "shocked":     dict(eyes="round", mouth="tall_o", brows="raised",
                            motion="shake", decor=deco_sweat, blink=False),
        "thinking":    dict(eyes=("dot", "looku"), mouth="small", brows="think",
                            motion="wobble", decor=deco_q, blink=False),
        "winking":     dict(eyes=("up", "round"), mouth="smirk", motion="bob",
                            blink=False),
        "cool":        dict(sunglasses=True, mouth="smirk", motion="bob",
                            blink=False),
        "relaxed":     dict(eyes="up", mouth="smile", motion="slow", horns="dim"),
        "delicious":   dict(eyes="up", mouth="tongue", motion="bob",
                            blink=False),
        "kissy":       dict(eyes="shut", mouth="heart", motion="bob",
                            decor=deco_hearts, blink=False),
        "confident":   dict(eyes="half", mouth="smirk", brows="confident",
                            motion="slow", blink=False),
        "sleepy":      dict(eyes="shut", mouth="small", motion="still",
                            horns="dim", decor=deco_z, blink=False),
        "silly":       dict(eyes="dot", mouth="tongue", motion="wobble",
                            horns="flash",
                            eye_centers=((6, 9), (9, 9)), blink=False),
        "confused":    dict(eyes=("dot", "round"), mouth="wavy", motion="wobble",
                            decor=deco_q, blink=False),
    }


def main():
    out_dir = os.path.dirname(os.path.abspath(__file__))
    for name, spec in specs().items():
        spec.setdefault("motion", "bob")
        spec.setdefault("horns", "purple")
        build_gif(spec, os.path.join(out_dir, f"{name}.gif"))
        print("ok", name)


if __name__ == "__main__":
    main()
