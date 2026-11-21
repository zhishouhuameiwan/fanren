"""把一张 3×3 行走表切成 128×120 的帧，按脚底与头部中心做亚像素对齐，拼成 4×4 的走路表，并出预览动图。

用法：python build_walk.py <行走表.png> <输出目录> [目标身高=98]
行：0 正面 / 1 左侧面 / 2 背面；列：0 站 / 1 迈步A / 2 迈步B。
"""
import json
import sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont

SRC, OUT = sys.argv[1], sys.argv[2].rstrip('/') + '/'
TARGET_H = float(sys.argv[3]) if len(sys.argv) > 3 else 98.0
FW, FH = 128, 120
FOOT_Y = 113                       # 脚底最后一行像素
CX = 64.0                          # 头部中心落在这一列的中线

im = Image.open(SRC).convert('RGBA')
arr = np.array(im)
H, W = arr.shape[:2]
CH, CW = H // 3, W // 3

# 预乘透明度，缩放时边缘不发黑
PAD = 600                          # 四周补透明边，取景框越出原图也不怕
f = arr.astype(np.float32) / 255.0
pm = np.dstack([f[:, :, :3] * f[:, :, 3:4], f[:, :, 3:4]])
pm = np.pad(pm, ((PAD, PAD), (PAD, PAD), (0, 0)))


def cell_planes(r, c):
    """只留这一格的像素：取景框比一格大，不然会框进邻格小人的脚或头。"""
    m = np.zeros_like(pm)
    y0, x0 = PAD + r * CH, PAD + c * CW
    m[y0:y0 + CH, x0:x0 + CW] = pm[y0:y0 + CH, x0:x0 + CW]
    return [Image.fromarray(np.ascontiguousarray(m[:, :, k]), 'F') for k in range(4)]


def measure(r, c):
    a = arr[r * CH:(r + 1) * CH, c * CW:(c + 1) * CW, 3] > 40
    ys, xs = np.nonzero(a)
    top, bot = ys.min(), ys.max()
    head = a[top:top + int((bot - top) * 0.33)]
    _, hx = np.nonzero(head)
    return dict(top=top + r * CH, bot=bot + r * CH, hcx=hx.mean() + c * CW, h=bot - top)


meas = {(r, c): measure(r, c) for r in range(3) for c in range(3)}
scale = TARGET_H / meas[(0, 0)]['h']          # 四个方向共用一个比例（以正面站立帧定）


def frame(r, c):
    m = meas[(r, c)]
    # 输出 (CX, FOOT_Y+1) 对应源图 (头部中心, 脚底下沿)
    x0 = m['hcx'] - CX / scale
    y0 = (m['bot'] + 1) - (FOOT_Y + 1) / scale
    box = (x0 + PAD, y0 + PAD, x0 + PAD + FW / scale, y0 + PAD + FH / scale)
    ch = [p.resize((FW, FH), Image.LANCZOS, box=box) for p in cell_planes(r, c)]
    out = np.dstack([np.array(x).astype(np.float32) for x in ch])
    al = out[:, :, 3:4]
    rgb = np.where(al > 1e-4, out[:, :, :3] / np.maximum(al, 1e-4), 0)
    res = (np.clip(np.dstack([rgb, al]), 0, 1) * 255).round().astype(np.uint8)
    res[res[:, :, 3] < 6] = 0
    return Image.fromarray(res, 'RGBA')


src_frames = {(r, c): frame(r, c) for r in range(3) for c in range(3)}
# 每一向 4 帧：站、迈步A、站、迈步B（与现有手绘表 walk_cycle [0,1,2,3]、站立取第 0 帧的约定一致）
rows = {
    'down': [src_frames[(0, 0)], src_frames[(0, 1)], src_frames[(0, 0)], src_frames[(0, 2)]],
    'left': [src_frames[(1, 0)], src_frames[(1, 1)], src_frames[(1, 0)], src_frames[(1, 2)]],
    'up': [src_frames[(2, 0)], src_frames[(2, 1)], src_frames[(2, 0)], src_frames[(2, 2)]],
}
rows['right'] = [fr.transpose(Image.FLIP_LEFT_RIGHT) for fr in rows['left']]   # 镜像：剑换到另一侧
order = ['down', 'left', 'right', 'up']
sheet = Image.new('RGBA', (FW * 4, FH * 4), (0, 0, 0, 0))
for ri, d in enumerate(order):
    for ci, fr in enumerate(rows[d]):
        sheet.paste(fr, (ci * FW, ri * FH))
sheet.save(OUT + 'chen_q_walk.png')

# 量一下对齐后的结果：每帧头顶、脚底、头部中心
report = {}
for d in order:
    rr = []
    for fr in rows[d]:
        a = np.array(fr)[:, :, 3] > 40
        ys, xs = np.nonzero(a)
        top, bot = int(ys.min()), int(ys.max())
        _, hx = np.nonzero(a[top:top + int((bot - top) * 0.33)])
        rr.append(dict(top=top, bot=bot, h=bot - top + 1, headcx=round(float(hx.mean()), 2),
                       left=int(xs.min()), right=int(xs.max())))
    report[d] = rr
heads = [x['top'] for d in order for x in report[d]]
json.dump(dict(scale=scale, foot_y=FOOT_Y, head_y=min(heads), frames=report), open(OUT + 'align_report.json', 'w'),
          indent=1, ensure_ascii=False)
print('scale', round(scale, 4), 'head_y', min(heads))
for d in order:
    print(d, [(x['top'], x['bot'], x['headcx']) for x in report[d]])
