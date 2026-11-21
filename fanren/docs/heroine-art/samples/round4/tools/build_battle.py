"""把一张 2×4 战斗表（每格 640×640）切成 264×168 的帧，缩放、对齐后拼成 4×2 的战斗表。

用法：python build_battle.py <战斗表.png> <现有像素战斗表.png> <输出目录>
格序与 index.json 的 battle 一致：0 待机 1 备战 2 攻击 3 施法 / 4 受击 5 力竭 6 胜利 7 防御。
对齐规则：
- 一个缩放比例：待机帧（发髻顶到鞋底）缩到现有像素待机帧在屏幕上的高度（64×2=128px）。
- 竖直：每帧最低的不透明行对到 foot_y=163（现有 foot_y 81、battle_scale 2 → 屏幕第 163 行）。
- 水平：每帧头部（发髻往下一段黑发）的中心，对到现有像素表同一姿势头部中心的 2 倍处——新小人站在旧小人原来站的地方。
"""
import json
import sys
import numpy as np
from PIL import Image

SRC, OLD, OUT = sys.argv[1], sys.argv[2], sys.argv[3].rstrip('/') + '/'
NAMES = ['idle', 'ready', 'attack', 'cast', 'hurt', 'down', 'victory', 'guard']
FW, FH, FOOT_Y = 264, 168, 163
OFW, OFH = 132, 84

im = Image.open(SRC).convert('RGBA')
arr = np.array(im)
CH, CW = arr.shape[0] // 2, arr.shape[1] // 4
# 清掉游离的碎屑：每格里面积小于 200 源像素、与人不相连的不透明块（备战格右边有一个）
from scipy import ndimage
removed = []
for i in range(8):
    r, c = i // 4, i % 4
    sub = arr[r * CH:(r + 1) * CH, c * CW:(c + 1) * CW]
    lab, n = ndimage.label(sub[:, :, 3] > 0)
    sizes = ndimage.sum(np.ones_like(lab), lab, range(1, n + 1))
    for k, sz in enumerate(sizes, 1):
        if sz < 200:
            sub[lab == k] = 0
            removed.append((NAMES[i], int(sz)))
print('removed specks', removed)
PAD = 800
f = arr.astype(np.float32) / 255.0
pm = np.pad(np.dstack([f[:, :, :3] * f[:, :, 3:4], f[:, :, 3:4]]), ((PAD, PAD), (PAD, PAD), (0, 0)))


def hair_mask(a):
    rgb = a[:, :, :3].astype(np.int32)
    lum = rgb.mean(axis=2)
    sat = rgb.max(axis=2) - rgb.min(axis=2)
    return (a[:, :, 3] > 128) & (lum < 70) & (sat < 45)


def measure(a, head_span):
    al = a[:, :, 3] > 40
    ys, xs = np.nonzero(al)
    hm = hair_mask(a)
    hy, hx = np.nonzero(hm)
    top = hy.min()
    band = hm[top:top + head_span]
    by, bx = np.nonzero(band)
    return dict(top=int(ys.min()), hair_top=int(top), bot=int(ys.max()), hcx=float(bx.mean()),
                left=int(xs.min()), right=int(xs.max()))


def cell(i):
    r, c = i // 4, i % 4
    return arr[r * CH:(r + 1) * CH, c * CW:(c + 1) * CW], r * CH, c * CW


src_m = []
for i in range(8):
    a, y0, x0 = cell(i)
    m = measure(a, 90)
    m.update(top=m['top'] + y0, hair_top=m['hair_top'] + y0, bot=m['bot'] + y0, hcx=m['hcx'] + x0)
    src_m.append(m)

old = np.array(Image.open(OLD).convert('RGBA'))
old_m = []
for i in range(8):
    r, c = i // 4, i % 4
    old_m.append(measure(old[r * OFH:(r + 1) * OFH, c * OFW:(c + 1) * OFW], 14))

target_h = (old_m[0]['bot'] - old_m[0]['hair_top'] + 1) * 2
scale = target_h / (src_m[0]['bot'] - src_m[0]['hair_top'] + 1)
# 逐姿势修正：出图时胜利、防御两格整个人画大了（站直的身高 570、551 对待机的 528）。
# 这两格和施法一样是站直的，按身高把它们缩回待机的大小；不是站直的四格只能目测头的大小，不修。
FIX = {6: 528 / 570, 7: 528 / 551}
FIX = {k: (src_m[0]['bot'] - src_m[0]['hair_top'] + 1) / (src_m[k]['bot'] - src_m[k]['hair_top'] + 1) for k in FIX}


def planes(i):
    r, c = i // 4, i % 4
    m = np.zeros_like(pm)
    y0, x0 = PAD + r * CH, PAD + c * CW
    m[y0:y0 + CH, x0:x0 + CW] = pm[y0:y0 + CH, x0:x0 + CW]
    return [Image.fromarray(np.ascontiguousarray(m[:, :, k]), 'F') for k in range(4)]


frames, report = [], []
for i in range(8):
    m, om = src_m[i], old_m[i]
    sc = scale * FIX.get(i, 1.0)
    tx = (om['hcx'] + 0.5) * 2          # 旧表头部中心（像素中心）放大 2 倍后的屏幕位置
    x0 = m['hcx'] + 0.5 - tx / sc
    y0 = (m['bot'] + 1) - (FOOT_Y + 1) / sc
    box = (x0 + PAD, y0 + PAD, x0 + PAD + FW / sc, y0 + PAD + FH / sc)
    ch = [p.resize((FW, FH), Image.LANCZOS, box=box) for p in planes(i)]
    out = np.dstack([np.array(x).astype(np.float32) for x in ch])
    al = out[:, :, 3:4]
    rgb = np.where(al > 1e-4, out[:, :, :3] / np.maximum(al, 1e-4), 0)
    res = (np.clip(np.dstack([rgb, al]), 0, 1) * 255).round().astype(np.uint8)
    res[res[:, :, 3] < 6] = 0
    fr = Image.fromarray(res, 'RGBA')
    frames.append(fr)
    a = np.array(fr)
    edge = bool((a[0, :, 3] > 0).any() or (a[-1, :, 3] > 0).any() or (a[:, 0, 3] > 0).any() or (a[:, -1, 3] > 0).any())
    mm = measure(a, 22)
    report.append(dict(pose=NAMES[i], top=mm['top'], hair_top=mm['hair_top'], bot=mm['bot'],
                       head_cx=round(mm['hcx'], 1), want_cx=round(tx - 0.5, 1), left=mm['left'], right=mm['right'],
                       touches_edge=edge, src_h=m['bot'] - m['hair_top'] + 1, fix=round(FIX.get(i, 1.0), 4)))

sheet = Image.new('RGBA', (FW * 4, FH * 2), (0, 0, 0, 0))
for i, fr in enumerate(frames):
    sheet.paste(fr, ((i % 4) * FW, (i // 4) * FH))
sheet.save(OUT + 'chen_q_battle.png')
head_y = min(r['top'] for r in report if r['pose'] in ('idle',))
json.dump(dict(scale=scale, removed_specks=removed, target_h=target_h, foot_y=FOOT_Y, idle_top=head_y, frames=report),
          open(OUT + 'align_report.json', 'w'), indent=1, ensure_ascii=False)
print('scale', round(scale, 4), 'target_h', target_h, 'idle top', head_y)
for r in report:
    print(r)
