# 凡人修仙传 · 单机 RPG

按《凡人修仙传》（忘语 著）改编的单机修仙 RPG，目标是从韩家村一路走到乱星海篇结束（韩立传送回天南）。

C++20 + SDL3；剧情脚本用 Lua（sol2 协程），数据用 JSON，地图用 Tiled 格式。地图、美术、音乐音效都由仓库里的生成器程序产出，字体除外。

进度：第 1–5 章已完成，玩法与画面按《八方旅人》的路子改造过（横版「破势与蓄劲」战斗、像素美术烘焙地图、路径行动）；第 6–14 章未开工。当前状态与接手须知见 [fanren/docs/handoff.md](fanren/docs/handoff.md)。

## 目录

| 路径 | 内容 |
| --- | --- |
| `fanren/` | 游戏工程：`src/` 源码、`tests/` 单元测试、`data/` `maps/` `scripts/` `assets/` 游戏内容、`tools/` 生成器与内容门禁、`docs/` 实现文档（索引见 [fanren/docs/README.md](fanren/docs/README.md)） |
| `docs/` | 设计文档：`大纲.md`（章节内容的唯一权威）、`map_spec.md`（地图规范，`fanren/tools/validate.py` 按它实现）、`凡人修仙传RPG-重构方案.md`、`deps.md`（依赖与版本锁定）、`lore/`（原著设定词条库） |
| `tools/novel/` | 原著检索脚本，校对设定用。原著文本不在仓库里，需自备后放进 `novel/` |

## 构建

需要 Windows、Visual Studio（含 C++ 工具集；本机用 VS 2026 / MSVC v145）、CMake 3.25 以上、Ninja，以及 Python 3（内容门禁和生成器要 `numpy`、`Pillow`、`scipy`）。

```powershell
pip install numpy pillow scipy
cd fanren
.\build.bat
```

`build.bat` 依次做四件事：

1. 跑内容门禁（`validate.py`、`genmaps.py --check`、`validate_selftest.py`、`artgen.py --check`）；
2. `bootstrap.py` 把第三方依赖下载到 `vendor/`（SDL3、SDL3_ttf、SDL3_image、SDL3_mixer、Lua 5.4、sol2、nlohmann/json、GoogleTest），逐个校验 SHA256；
3. CMake + Ninja 构建；
4. ctest 跑单元测试。

产物在 `build\`。给一个参数就换一个构建目录：`.\build.bat myslot` 构建到 `build-myslot\`，几个人同时构建时不会互相踩。

## 运行

构建完直接双击 `fanren\build\fanren.exe` 即可：不给 `--assets` 时，它从当前目录和 exe 所在目录各往上找几层，找到同时有 `data/` 与 `maps/` 的那一层就当游戏目录；找不到或起不来会弹框说原因。命令行照旧可用：

```powershell
cd fanren
.\build\fanren.exe --assets .
```

默认键位：方向键 / WASD 移动，Enter / Z / 空格 确认，Esc / X / Tab 主菜单，E / Q 路径行动，Ctrl 快进，F5 存盘，Alt+Enter 切全屏。除方向键、Enter、Esc 以外，其余按键都能在「设置 → 按键设置」里改。

## 版权

- 这是一部同人改编作品。仓库不含原著文本；按项目的版权约定（[docs/lore/README.md](docs/lore/README.md) 第 1 节），游戏内文字为原创改编，设定词条只记要点、不摘原句。
- 字体霞鹜文楷（LXGW WenKai）采用 SIL Open Font License 1.1，见 [fanren/assets/fonts/LICENSE](fanren/assets/fonts/LICENSE)。
