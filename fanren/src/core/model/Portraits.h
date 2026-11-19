#pragma once
// 对白头像表 data/visual/portraits.json 读进来之后的样子（伙伴改版，docs/interfaces-companions.md 2.5、3.9；
// 形状见 docs/art-sprites.md 5.3）。
//
// 纯数据。读在 io 层（io/PortraitLoader.h，严格：缺项、类型错、认不出的键整份报错），画在
// game/Portraits.h 与 DialogueScene。键是**角色 id**，不是伙伴 id：同一个人的不同装束是不同的角色 id，
// 各登记一条（白衣女子戴面纱那一套挂在 baiyi_shaonv 上）。没登记的角色对白照旧不带头像。
//
// 每个角色挂的是一张**按章生效的列表**（与精灵索引的 role_variants 同一个意思）：每条可带 minChapter / maxChapter，
// 取第一条成立的；都不成立就不带头像。韩立第 1–2 章是十岁的孩子，青年的头像只登记在第 7 章，那两章说话就不出脸。
#include <array>
#include <map>
#include <string>
#include <vector>

namespace fanren::core {

// 四个表情，次序就是 PortraitSheet::faces 的下标。脚本 talk 的第三个参数取其中之一，缺省 normal。
inline constexpr std::array<const char*, 4> kPortraitFaces{"normal", "smile", "angry", "hurt"};

// 一个角色的头像：一张图集上 frameW×frameH 的格子，cols 列一行；四个表情各指一格。
// 帧号 → 像素与人物表同一条公式：x = (i % cols) * frameW，y = (i / cols) * frameH。
struct PortraitSheet {
    std::string file;            // 相对 assets/art/portraits/
    int frameW = 0;
    int frameH = 0;
    int cols = 1;
    std::array<int, 4> faces{};  // kPortraitFaces 次序的帧号
    // 在哪几章生效（0 = 这一端不限）：maxChapter 那一章还没演完、且 minChapter 的前一章已经演完。
    // 与 LookVariant（core/model/Visual.h）同一种判法，「演完没有」由调用方按章节表的 done_flag 给。
    int minChapter = 0;
    int maxChapter = 0;
};

// 角色 id → 头像列表（按表里的次序，第一条在这一章生效的算数）。读取器保证每个列表非空。
using PortraitTable = std::map<std::string, std::vector<PortraitSheet>>;

}  // namespace fanren::core
