"""每个方向一张预览动图：游戏截图做底，新 Q 版小人与现有像素小人并排原地走，8 帧/秒，整张 2 倍最近邻放大。"""
import sys
from PIL import Image, ImageDraw, ImageFont

SHEET, OUTDIR = sys.argv[1], sys.argv[2].rstrip('/') + '/'
BG = sys.argv[3]                       # 游戏截图
BOX = tuple(int(v) for v in sys.argv[4].split(','))   # 截图里取哪一块当底
ROOT = 'E:/fanren/fanren/build-s7/sprite-test/root/assets/art/'
FONT = 'E:/fanren/fanren/assets/fonts/LXGWWenKai-Regular.ttf'
ZOOM = 2

new = Image.open(SHEET).convert('RGBA')
old = Image.open(ROOT + 'sprites/hd/chen_shimei_walk.png').convert('RGBA')
shadow_mask = Image.open(ROOT + 'fx/shadow.png').convert('RGBA').getchannel('A').resize((50, 16), Image.BILINEAR)
bg = Image.open(BG).convert('RGBA').crop(BOX)
W, H = bg.size
order = ['down', 'left', 'right', 'up']


def put_shadow(c, fx, fy):
    blk = Image.new('RGBA', (50, 16), (0, 0, 0, 255))
    blk.putalpha(shadow_mask)
    c.alpha_composite(blk, (fx - 25, fy - 10))


for ri, d in enumerate(order):
    frames = []
    for step in range(4):
        c = bg.copy()
        fy = H - 22
        # 现有像素小人：64×60 帧，2 倍最近邻，脚底 foot_y=57
        ox = W // 3
        put_shadow(c, ox, fy)
        of = old.crop((step * 64, ri * 60, step * 64 + 64, ri * 60 + 60)).resize((128, 120), Image.NEAREST)
        c.alpha_composite(of, (ox - 64, fy - 58 * 2))
        # 新小人：128×120 帧，1 倍，脚底 foot_y=113
        nx = W * 2 // 3
        put_shadow(c, nx, fy)
        nf = new.crop((step * 128, ri * 120, step * 128 + 128, ri * 120 + 120))
        c.alpha_composite(nf, (nx - 64, fy - 114))
        dr = ImageDraw.Draw(c)
        f = ImageFont.truetype(FONT, 13)
        dr.text((ox, 6), '现有像素', font=f, fill=(255, 245, 210, 255), anchor='mt', stroke_width=2, stroke_fill=(0, 0, 0, 255))
        dr.text((nx, 6), '非像素 Q 版', font=f, fill=(255, 245, 210, 255), anchor='mt', stroke_width=2, stroke_fill=(0, 0, 0, 255))
        frames.append(c.convert('RGB').resize((W * ZOOM, H * ZOOM), Image.NEAREST))
    frames[0].save(OUTDIR + f'walk_{d}.gif', save_all=True, append_images=frames[1:], duration=125, loop=0)
    # 同时存一张四帧并排的静图，方便不播放也能看
    strip = Image.new('RGB', (W * ZOOM * 4, H * ZOOM))
    for i, fr in enumerate(frames):
        strip.paste(fr, (i * W * ZOOM, 0))
    strip.save(OUTDIR + f'walk_{d}_strip.png')
print('ok', W, H)
