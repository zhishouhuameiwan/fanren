#pragma once
// 游戏根目录（data/ maps/ scripts/ assets/ 的上一级）的自动定位。
//
// 命令行上一直是 `--assets .` 从工程根启动；双击 build\fanren.exe 时工作目录却是 build\，
// 那里既没有 data/ 也没有 maps/，于是一启动就初始化失败——而 fanren.exe 是窗口程序，
// 失败原因打在 stderr 上，双击的人什么也看不见。所以不给 --assets 时由 main 调这里去找。
//
// 判据是「同时有 data/ 与 maps/ 两个目录」：只认一个的话，tests/ 底下就有 maps/，构建目录
// 往后若多出个 data/ 也会被错认成根。引擎找字体目录（Engine.cpp 的 findAssetRoot）是
// 同一套「从当前目录、exe 目录各往上几层」的走法，层数也取同一个值。
//
// 只用 std::filesystem、不碰 SDL：io 层不依赖 engine，起点（当前目录、SDL_GetBasePath）由调用方给。
#include <filesystem>
#include <vector>

namespace fanren::io {

// 每个起点本身之外再往上找几层。exe 在 <根>/build-<槽>/ 里，一层就够；多给两层是为了
// 多配置生成器那种 build/Release/ 的布局，与引擎找字体的层数一致。
inline constexpr int kGameRootSearchDepth = 3;

// 依次从每个起点出发，看它本身和它往上 kGameRootSearchDepth 层，返回第一个同时有 data/ 与
// maps/ 的目录（绝对路径）。起点可以是相对路径、可以带尾分隔符（SDL_GetBasePath 就带），
// 空路径跳过。一个都找不到返回空路径，由调用方决定怎么办——这里不退回任何缺省值，
// 免得一个猜出来的根把「找不到」伪装成「数据目录不存在」。
[[nodiscard]] std::filesystem::path locateGameRoot(const std::vector<std::filesystem::path>& starts);

}  // namespace fanren::io
