"""正面迈步B = 迈步A 的裙摆下缘到鞋底镜像（以那一截的中线为轴），接缝羽化。写出新的 3×3 表（其余格照抄）。"""
import sys
import numpy as np
from PIL import Image
src, out = sys.argv[1], sys.argv[2]
CUT, FEATHER = int(sys.argv[3]), 12
a = np.array(Image.open(src).convert('RGBA')).astype(np.float32)
cell = a[0:512, 512:1024].copy()                     # 第一行第 2 格：迈步A
low = cell[CUT:] 
cols = np.nonzero(low[:, :, 3].max(axis=0) > 40)[0]
axis = (cols.min() + cols.max()) / 2.0
mir = cell.copy()
xs = np.arange(512)
srcx = np.clip(np.round(2 * axis - xs).astype(int), 0, 511)
mir[:, :] = cell[:, srcx]
w = np.clip((np.arange(512) - CUT) / FEATHER, 0, 1)[:, None, None]
# 预乘后混合，免得半透明边发灰
def pm(x): return np.dstack([x[:, :, :3] * x[:, :, 3:4] / 255, x[:, :, 3:4]])
mixed = pm(cell) * (1 - w) + pm(mir) * w
al = mixed[:, :, 3:4]
rgb = np.where(al > 0, mixed[:, :, :3] * 255 / np.maximum(al, 1e-3), 0)
newcell = np.dstack([rgb, al])
b = a.copy()
b[0:512, 1024:1536] = newcell
Image.fromarray(np.clip(b, 0, 255).round().astype(np.uint8), 'RGBA').save(out)
print('axis', axis)
