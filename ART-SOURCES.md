# art-sources：美术源图（不并入 main）

这条分支与 main 没有共同历史，只用来在几台电脑之间带「不入库的美术源图」。**不要把它合进 main。**

2026-11-21 从旧电脑的主检出快照而来，内容是 main 的 `.gitignore` 里写明「只留在本地」的两个目录：

| 目录 | 内容 |
| --- | --- |
| `fanren/docs/heroine-art/` | 女角色立绘：四轮样张（`samples/`）、七套成品原图与提示词（`final/<角色>/`）、陈巧倩五档事件图的原图、弃图与提示词（`cg/`） |
| `fanren/docs/companions-overhaul/` | 第 7 章 Seedream 重做的原稿：地图（`maps/`）、人物与妖兽原图（`raw/`）、各类定稿（`*-final/`） |

游戏运行用的图都在 main 的 `fanren/assets/` 下，照常入库；这里的只在**重新打包美术**（`tools/artgen/busts.py`、`hd_pack.py --src`、`hd_maps.py`）和**出新图要参考原图**时才用得上。

## 取回到工作区

在 main 的检出里、仓库根目录执行：

```bash
git fetch origin art-sources
git restore --source=origin/art-sources --worktree -- fanren/docs/heroine-art fanren/docs/companions-overhaul
```

两个目录在 main 里是被忽略的，取回之后 `git status` 仍然是干净的。

## 没带上的

- `fanren/docs/heroine-art/cg/raw/` 下两份 `*_metadata.json`：里面有出图服务带签名的临时下载链接，没有上传；对应的图本身都在。

## 更新这条分支

源图有增改时，在有源图的那台电脑上重做一个快照提交、强制更新这条分支即可（它不需要历史）。做法见 main 的 `fanren/docs/handoff-2026-11-21.md`。
