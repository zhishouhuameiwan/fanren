"""统一的投影与接触阴影（左上光）。

各画法只负责把「这里站着一块实体」登记进 ctx.solid_px / ctx.tall_px，影子在这里一次
做完：实体整体往右下平移几像素、压在地面上。屋、墙、崖这些高的东西影子长一些；
篱笆、家具矮，影子短。再在立面脚下压一道 1–3px 的接触阴影（墙根、篱笆根发暗）。

影子一律用明度级减法 + 抖动过渡，不用 alpha：像素画的影子该是一片抖动的暗格，
而不是一层半透明灰。
"""

from __future__ import annotations

import numpy as np

import pix


def cast(ctx, mask: np.ndarray, dx: int, dy: int, strength: float, soft: int = 1) -> None:
    """把 mask 往 (dx,dy) 平移后压在「不是实体」的像素上，边缘软化 soft 像素。"""
    sh = pix.shift(mask, dx, dy).astype(np.float32)
    if soft > 0:
        sh = pix.box_blur(sh, soft)
    sh = np.where(ctx.solid_px, 0.0, sh)
    ctx.below.shade(0, 0, -strength * sh, sh > 0.05)


def contact(ctx, mask: np.ndarray, depth: int = 3, strength: float = 1.0) -> None:
    """实体正下方的地面：第一行最暗，往下渐淡。"""
    below = ctx.below
    free = ~ctx.solid_px
    cur = mask
    for k in range(depth):
        cur = pix.shift(cur, 0, 1)
        m = cur & free
        below.shade(0, 0, -strength * (1.0 - k / depth), m)


def apply(ctx) -> None:
    tall = ctx.tall_px
    low = ctx.solid_px & ~tall
    # 影子里面是整一档暗（强度取整数），只有边上一两像素抖动过渡：
    # 强度 1.1 / 0.8 会让整片影子里零星跳出一成、两成的抖点，×3 下是一片麻点。
    cast(ctx, tall, 5, 4, 1.0, 1)
    cast(ctx, low, 2, 2, 1.0, 1)
    contact(ctx, tall, 3, 1.0)
    contact(ctx, low, 2, 0.7)
