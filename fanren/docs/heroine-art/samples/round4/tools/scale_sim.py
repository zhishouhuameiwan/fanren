"""窗口不是 1280×720 时的观感——离线模拟，不是游戏截图。
游戏固定按 1280×720 画，再交给 SDL 的逻辑呈现缩放到窗口（Engine.cpp presentationFor）：
缺省整数缩放（1920×1080 窗口 → 1 倍加黑边；2560×1440 → 2 倍），设置里可改「铺满」（1920×1080 → 1.5 倍）。
贴图是 PIXELART 采样：放大近似最近邻、像素边缘略抗锯齿。这里用最近邻模拟 2 倍与 1.5 倍。
"""
from PIL import Image, ImageDraw, ImageFont
R4 = 'docs/heroine-art/samples/round4/'
FONT = 'assets/fonts/LXGWWenKai-Regular.ttf'
src = Image.open(R4 + 'shots/battle_attack.png').convert('RGB')
crop = src.crop((620, 330, 960, 560))  # 340×230：她（刺剑）+ 左边一点像素地面
out = []
for k, t in [(1, '1 倍：1280×720；1920×1080 整数缩放'), (1.5, '1.5 倍：1920×1080 铺满'), (2, '2 倍：2560×1440 整数缩放')]:
    w, h = int(crop.width * k), int(crop.height * k)
    out.append((crop.resize((w, h), Image.NEAREST), t))
W = sum(im.width for im, _ in out) + 40
H = max(im.height for im, _ in out) + 70
c = Image.new('RGB', (W, H), (24, 24, 28))
d = ImageDraw.Draw(c)
f = ImageFont.truetype(FONT, 18)
x = 10
for im, t in out:
    c.paste(im, (x, 40 + H - 70 - im.height))
    d.text((x, 10), t, font=f, fill=(240, 226, 190))
    x += im.width + 10
d.text((10, H - 26), '离线模拟，不是游戏截图：截图口只读 1280×720 的逻辑画面，没有改窗口大小的参数。', font=f, fill=(200, 120, 120))
c.save(R4 + 'shots/window_scale_sim.png')
print(c.size)
