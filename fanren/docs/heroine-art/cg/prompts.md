# 陈巧倩五档「红颜」事件图·出图记录（同道录 2a·路 N，2026-11-21）

- 工具：主检出的 `tools/artgen/seedream_run.js`（只用不改；Seedream 5.5，`--offline-models --optimize false --watermark false`），每次 1 张，`--size 2560x1440`。
- 参考图：`docs/heroine-art/final/chen_shimei/normal.png`（立绘定稿基准图），要求同一个人、同一身杏黄衣裙。
- 出图 2 张（上限 8）。入选 `raw/cg_02.png` → 仓库 `assets/art/cg/chen_qiaoqian_hongyan.png`（原尺寸 2560×1440，PIL 重存 PNG，不改像素）。
- 底线自查（`docs/heroine-art.md` 第 3、7 节、施工图 3.5）：两人衣着整齐；隔着一步多的空地，没有任何肢体接触；韩立只有背影与一线侧脸；她是成年女子的脸与身量，神情羞涩不安，没有撩人姿势；衣料不透；手、五官放大看过没画坏。

| 编号 | 结果 | 理由 |
| --- | --- | --- |
| cg_01 | 弃（`rejected/cg_01.png`） | 合底线；但提示词里「裙角被露水打湿」画成了裙摆下段一大片褐黄色水渍，像脏污。韩立露出的侧脸也多一些。 |
| cg_02 | 入选 | 去掉「露水打湿」改「衣裙洁净」后重出：裙子干净，韩立几乎全是背影，两人之间的空当清楚，她低着头、话说一半的神情对。 |

提示词原文见 `prompt_01.txt`、`prompt_02.txt`（两份只差那一句）。
