#pragma once
// 脚本命令。ScriptHost 把 Lua 协程 yield 出来的表翻成这些结构，game 层执行完
// 再把 CommandResult 回填。纯数据：不含 Lua、不含 SDL，便于无头测试。
#include <string>
#include <vector>

namespace fanren::script {

enum class CommandKind {
    Talk, Choice, Battle, Teleport, FadeOut, FadeIn,
    GiveItem, TakeItem, SetFlag, Shop, Wait, PlaySfx, GameOver, Ending,
    // P3 第 2 章增补。一律追加在末尾：存档与测试按序号比对，插在中间会让
    // 旧存档里的每一条命令静默错位成另一条。
    AdvanceDays, BottleGrant, BottleUnlockMature, FieldUnlock,
    // P3 事后追加（契约 docs/interfaces-p3-script.md 第 6 节）：
    // 起草四条命令时漏了「把绿液用掉」这一半，于是第 2 章所有「倒出一滴」
    // 「浇了三滴」的动作在机制上都没发生过。同样追加在末尾。
    BottleSpend, BottleMature,
    // P3 第 3 章增补（契约 docs/interfaces-p3-ch03.md 第 1.3 节）：队伍。
    // 同样追加在末尾。
    PartyAdd, PartyRemove,
    // P3 第 4 章增补（契约 docs/interfaces-p3-ch04.md 第 1.3 节）：已习得法术。
    // 同样追加在末尾。读（knows / count）走 __host，不占命令位。
    MagicLearn, MagicForget,
    // P3 第 4 章增补（契约 docs/interfaces-p3-ch04.md 第 6 节）：提升境界。
    // 在这一条之前，第 3、4 两章的脚本**没有任何一处能改境界**，脚本 API 里
    // 只有查询 `realm_at_least`，于是「这一章他修为涨了」这件事在游戏里无处发生。
    // 同样追加在末尾。读（realm_at_least / realm_value）走 __host，不占命令位。
    RealmAdvance,
    // 第 4 章二次整改（技术债 G-14，契约 docs/interfaces-p3-script.md 第 7 节）：
    // 抬剧情给的境界上限。只升不降；玩家自己在打坐面板上按的突破越不过它。
    // 同样追加在末尾。
    RealmCap,
    // 八方旅人化改造（docs/interfaces-octo-pathactions.md 11 节、docs/audio.md）：脚本点播 BGM。
    // 同样追加在末尾。
    PlayBgm,
    // P3 第 7 章增补（契约 docs/interfaces-p3-ch07.md 第 4 节）：按年份下限扣物（take_aged）。
    // 同样追加在末尾。计数（item.count_aged）走 __host，不占命令位。
    TakeItemAged,
};

// bgm() 的这个 id 不是曲子，是「撤掉点播、放回地图曲」（api.lua 的 bgm("map")）。
// 选一个词而不是空串：bgm("") 读起来像「静音」，而它要说的恰恰相反。
inline constexpr const char* kMapBgm = "map";

// 字段是通用槽位，各命令的占用方式如下表。之所以不给每种命令单独开结构，
// 是因为命令要跨 C++/Lua 边界来回搬运，扁平表比变体更省事也更好序列化。
//
//   kind        a            b            x            y      options
//   ---------------------------------------------------------------------
//   Talk        文案 key     说话人 id     -            -      -
//   Choice      -            -            -            -      选项文案 key
//   Battle      战斗 id      -            -            -      -
//   Teleport    地图 id      -            目标格 x     目标格 y  -
//   FadeOut     -            -            时长 ms      -      -
//   FadeIn      -            -            时长 ms      -      -
//   GiveItem    物品 id      -            数量         灵草年份  -
//   TakeItem    物品 id      -            数量         灵草年份  -
//   SetFlag     旗标名       -            值           -      -
//   Shop        商店 id      -            -            -      -
//   Wait        -            -            毫秒         -      -
//   PlaySfx     音效 id      -            -            -      -
//   GameOver    -            -            -            -      -
//   Ending      标题 key     正文 key     -            -      -
//   AdvanceDays -            -            天数         -      -
//   BottleGrant -            -            -            -      -
//   BottleUnlockMature  -    -            -            -      -
//   FieldUnlock 灵田 id      -            槽位数       -      -
//   BottleSpend -            -            滴数         -      -
//   BottleMature 物品 id     -            -            当前年份  -
//   PartyAdd    角色 id      -            -            -      -
//   PartyRemove 角色 id      -            -            -      -
//   MagicLearn  法术 id      -            -            -      -
//   MagicForget 法术 id      -            -            -      -
//   RealmAdvance -          -            目标境界编号 -      -
//   RealmCap    -            -            上限境界编号 -      -
//   PlayBgm     曲子 id / "map" -          -            -      -
//   TakeItemAged 物品 id     -            数量         年份下限  -
//
// 注：Talk 的 a 放文案 key 而不是说话人，与契约里「a 是主参数：文案 key /
// 地图 id / 物品 id」的排列一致 —— 各命令的 a 一律是那条命令的主体。
struct Command {
    CommandKind kind{};
    std::string a;
    std::string b;
    int x{}, y{};
    std::vector<std::string> options;
};

// 命令执行结果，由 game 层回填后 resume 协程。
//
// 新字段一律追加在末尾，理由与 CommandKind 相同。
struct CommandResult {
    bool ok = true;
    int choiceIndex = -1;   // Choice：选中项，0 起算；取消为 -1
    bool battleWon = false; // Battle
    // 数值回填。目前只有 BottleMature 用它带回催熟后的新年份 —— 年份由
    // rules::matureHerb 算，脚本不许自己推，否则规则一改文案就对不上。
    int value = 0;
    // 失败原因的机器码（ASCII，如 "no_drops"）。成功时为空。
    //
    // Battle 另有一层用法：它用 code 带回**战斗是怎么结束的**
    // （"" 打赢 / "lost" 打输 / "escaped" 我方逃走 / "enemy_fled" 敌人逃走），
    // 并用 value 带回识海之战咬下的体积百分比。won 这一位分不出「输了」与
    // 「敌人跑了」，而这一章的后续剧情全押在这个分别上。
    //
    // 为什么不回填 rules 层那句中文 reason：那是给玩家看的提示，会跟着文案改，
    // 脚本拿它去比对等于把 UI 文案钉进逻辑里，改一个字所有分支一起哑掉。
    std::string code;
};

}  // namespace fanren::script
