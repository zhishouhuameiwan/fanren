"""两张对照：
1. compare_sheet.png：现有像素战斗表（2 倍最近邻）与新表（1 倍）八个姿势并排，屏幕大小相同，红线是脚底。
2. shots/ingame_poses.png：同一刻（深处蛇之战「中途」）里八个姿势的游戏截图裁切，拼成 2×4。
"""
import sys
from PIL import Image, ImageDraw, ImageFont
R4 = 'docs/heroine-art/samples/round4/'
OLD = 'build-s7/sprite-test/root/assets/art/sprites/hd/chen_shimei_battle.png'
FONT = 'assets/fonts/LXGWWenKai-Regular.ttf'
NAMES = ['待机 idle', '备战 ready', '攻击 attack', '施法 cast', '受击 hurt', '力竭 down', '胜利 victory', '防御 guard']
f = lambda s: ImageFont.truetype(FONT, s)
old = Image.open(OLD).convert('RGBA')
new = Image.open(R4 + 'frames/chen_q_battle.png').convert('RGBA')
FW, FH, LW, TH = 264, 168, 120, 34
W = LW + 8 * FW
c = Image.new('RGBA', (W, TH + 2 * FH + 10), (58, 62, 70, 255))
d = ImageDraw.Draw(c)
for i in range(8):
    x = LW + i * FW
    d.text((x + FW // 2, 8), NAMES[i], font=f(18), fill=(240, 226, 190, 255), anchor='mt')
    o = old.crop(((i % 4) * 132, (i // 4) * 84, (i % 4) * 132 + 132, (i // 4) * 84 + 84)).resize((FW, FH), Image.NEAREST)
    n = new.crop(((i % 4) * FW, (i // 4) * FH, (i % 4) * FW + FW, (i // 4) * FH + FH))
    for row, im in enumerate((o, n)):
        y = TH + row * (FH + 10)
        d.rectangle([x + 1, y, x + FW - 2, y + FH - 1], fill=(72, 77, 86, 255))
        c.alpha_composite(im, (x, y))
        d.line([x + 1, y + 164, x + FW - 2, y + 164], fill=(220, 70, 70, 255))
for row, t in enumerate(('现有像素\n132×84 ×2', '非像素 Q 版\n264×168 ×1')):
    d.multiline_text((10, TH + row * (FH + 10) + 60), t, font=f(18), fill=(240, 226, 190, 255), spacing=6)
c.convert('RGB').save(R4 + 'compare_sheet.png')

CROP = (360, 300, 1000, 610)
cw, chh = CROP[2] - CROP[0], CROP[3] - CROP[1]
g = Image.new('RGB', (4 * cw, 2 * (chh + 30)), (20, 20, 24))
dg = ImageDraw.Draw(g)
for i in range(8):
    sh = Image.open(R4 + f'shots/forced_{i}.png').convert('RGB').crop(CROP)
    x, y = (i % 4) * cw, (i // 4) * (chh + 30)
    g.paste(sh, (x, y + 30))
    dg.text((x + cw // 2, y + 5), NAMES[i], font=f(20), fill=(240, 226, 190), anchor='mt')
g.save(R4 + 'shots/ingame_poses.png')
print(c.size, g.size)
