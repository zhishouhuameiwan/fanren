# -*- coding: utf-8 -*-
"""全部条目的登记表：id → 生成函数，以及每个 id 的自检规格。

加新曲子 / 新音效只改两处：写生成函数，然后在对应模块末尾的 SONGS / SFX 里登记。
本文件按模块汇总，不需要改。
"""
from __future__ import annotations

import sfx_battle
import sfx_jingle
import sfx_ui
import songs_sect
import songs_special
import songs_world
from verify import Spec

BGM = {**songs_world.SONGS, **songs_sect.SONGS, **songs_special.SONGS}
assert len(BGM) == len(songs_world.SONGS) + len(songs_sect.SONGS) + len(songs_special.SONGS), \
    "两个曲目模块登记了同一个 id，后登记的会悄悄盖掉前一个"
SFX = {s.id: s for s in (*sfx_ui.SFX, *sfx_battle.SFX, *sfx_jingle.SFX)}
assert len(SFX) == len(sfx_ui.SFX) + len(sfx_battle.SFX) + len(sfx_jingle.SFX), "两条音效用了同一个 id"

# 地图属性里写死的 14 个 id：少一个，进图就是「找不到音频资源」。
# Chapter 8 map registration reuses these existing track IDs:
# ch08_dongfu -> bgm_mountain_path
# ch08_tianxing_fangshi -> bgm_town
# ch08_yanlingbao -> bgm_town
# ch08_lingkuang -> bgm_cliff
# ch08_jinguyuan -> bgm_wild
# ch08_jinmacheng -> bgm_town
# ch08_yuejing -> bgm_town
# ch08_jiayuan_shanlin -> bgm_night
# ch09_huangshan -> bgm_cliff
# ch09_yuanwu -> bgm_wild
# ch09_wumingshan -> bgm_mountain_path
# ch09_milin -> bgm_night
# ch10_gudao -> bgm_cliff
# ch10_haichuan -> bgm_river
# ch10_kuixing -> bgm_town
# ch10_xiaohuan -> bgm_valley
# ch10_tiandujie -> bgm_town
# ch10_jinhai -> bgm_wild
# ch10_haiyuandao -> bgm_cliff
MAP_BGM = ("bgm_village", "bgm_town", "bgm_mountain_path", "bgm_cliff", "bgm_qixuanmen", "bgm_valley",
           "bgm_indoor", "bgm_sect", "bgm_cave", "bgm_wild", "bgm_river", "bgm_inn", "bgm_manor", "bgm_night")

_BGM_SPEC = Spec("bgm", 60.0, 100.0, loop=True, lufs_range=(-17.0, -15.0))

assert not set(BGM) & set(SFX), "BGM 与音效的 id 重名"
assert all(i in BGM for i in MAP_BGM), "地图用到的 BGM 没有全部登记"


def spec_of(item_id: str) -> Spec:
    if item_id in BGM:
        return _BGM_SPEC
    sfx = SFX[item_id]
    return Spec("sfx", sfx.min_s, sfx.max_s, loop=False)
