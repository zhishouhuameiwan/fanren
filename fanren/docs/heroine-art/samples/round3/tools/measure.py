import sys
import numpy as np
from PIL import Image
im = np.array(Image.open(sys.argv[1]).convert('RGBA'))
n = int(sys.argv[2]) if len(sys.argv) > 2 else 3
H, W = im.shape[:2]
ch, cw = H // n, W // 3
for r in range(n):
    for c in range(3):
        a = im[r*ch:(r+1)*ch, c*cw:(c+1)*cw, 3] > 40
        ys, xs = np.nonzero(a)
        if len(ys) == 0: print(r, c, 'empty'); continue
        top, bot, l, rr = ys.min(), ys.max(), xs.min(), xs.max()
        hh = bot - top
        head = a[top:top + int(hh * 0.33)]
        hy, hx = np.nonzero(head)
        print(f'r{r}c{c} top={top} bot={bot} h={hh} l={l} r={rr} headcx={hx.mean():.1f} edge={ys.min()==0 or ys.max()==ch-1 or xs.min()==0 or xs.max()==cw-1}')
