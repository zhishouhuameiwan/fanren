#pragma once
// 跨模块共享的数据模型。纯数据，无行为、无依赖。
// 字段按 P1 垂直切片所需的最小集给出，后续按章节需要扩充。
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "core/model/AttackCategory.h"
#include "core/rules/Bottle.h"
#include "core/rules/Encounter.h"
#include "core/rules/Field.h"
#include "core/rules/Realm.h"

namespace fanren::core {

struct Point {
    int x = 0;
    int y = 0;
    friend bool operator==(const Point&, const Point&) = default;
};

// 五行 bitmask。相克关系由 rules 层计算。
enum Element : int {
    kElementNone = 0,
    kElementMetal = 1,
    kElementWood = 2,
    kElementWater = 4,
    kElementFire = 8,
    kElementEarth = 16,
};

enum class ItemKind { Story, Artifact, Armor, Trinket, Pill, Herb, Talisman, Manual, Material };

struct Item {
    std::string id;
    std::string name;
    std::string descKey;
    ItemKind kind = ItemKind::Material;
    int grade = 0;          // 品阶
    int price = 0;          // 灵石价
    bool tradeable = true;
    int herbAge = 0;        // 灵草年份；非灵草为 0
    // 这一味灵草能长到的年份上限（绿液催熟与自然生长同受此限）；非灵草为 0。
    // data 里缺 maxAge 字段时由加载器按 grade 推默认值，见 rules::herbMaxAgeForGrade。
    int maxAge = 0;
    int restoreHp = 0;
    int restoreMp = 0;
    int addAttack = 0;
    int addDefence = 0;

    // ---- 用毒（P3 第 3 章增补，契约 docs/interfaces-p3-ch03.md 第 2 节）----
    // 一律追加在末尾，按字段顺序写的聚合初始化不受影响。
    //
    // 刻意做小：一个回合数加一个每回合掉血，不做抗性、不做毒的种类。
    // 毒药写 poison/poisonPower，解毒药写 curesPoison，两条路径都由数据给，
    // game 层不认识任何一个具体物品 id。
    //
    // **尸虫丸不走这套。** 它是剧情状态（旗标 `ch03.shichong_wan` 加一个期限），
    // 不在战斗里结算，也不该被解毒药解掉。把它做成战斗毒，玩家会以为吃颗解毒丹
    // 就能摆脱墨居仁的控制，而那是另一回事。后人不要「顺手统一」这两样。
    int poison = 0;          // 中毒回合数
    int poisonPower = 0;     // 每回合掉血
    bool curesPoison = false;

    // ---- 兵器给的攻击类别（八方旅人化改造，docs/octopath-battle.md 2.2）----
    // 同样追加在末尾。一件兵器揣在韩立身上，他的普攻就多一样可选的类别：
    // 玉带短剑给「剑」、制式佩刀给「刀」。只许一个类别位，且只许兵刃五类
    //（加载器查）；0 = 这件东西不是兵器。
    //
    // 从前这里是 useRange（使用距离，技术债 G-7）。横版战斗没有格子，
    // 「扔得多远」这件事整个不存在了，字段随之删掉，数据里也一并清掉
    //（tools/validate.py 见到还写着它的物品会报错，免得有人以为它还管用）。
    int weapon = 0;

    // ---- 道具施法（P3 第 7 章增补，契约 docs/interfaces-p3-ch07.md 第 2 节）----
    // 同样追加在末尾。一件符箓在战斗里用出去 = 以这门法术施展一次：不耗法力、不吃蓄劲、
    // 扣一件。空串 = 不是施法的道具。与 restoreHp / restoreMp / poison / curesPoison
    // 互斥（加载器查）：一件东西要么是药、要么是符。
    std::string castMagic;
};

// 这件东西在战斗里用不用得上。
//
// 判据就是「战斗真的会拿它做点什么」：`BattleState::applyItem` 只读下面这六个
// 字段（带 castMagic 的那一件按那门法术施展一次），一件都不沾的东西登记到战场上，
// 玩家选中它只会换来一句「并无变化」，白丢一个回合——那不是少了个功能，那是个陷阱。
//
// **刻意不按 kind 分类，更不按物品 id。** 毒药、解毒药、丹药的真实条目由编剧写，
// game 层与 core 层都不该认识它们的名字（契约 docs/interfaces-p3-ch03.md 2.3 节
// 对下毒那条路径的要求，这里是同一条口径）。按 kind 分也不行：`Pill` 里既有
// 回血的金疮药，也有纯剧情用的筑基丹（restore 全是 0），而灵草里的黄精草是真能
// 当场嚼的——真正的分界线是效果，不是分类。
//
// `addAttack` / `addDefence` 不算：那是装备加成，战斗里没有「装备一下」这个动作。
// 将来给 Item 加了新的战斗效果字段，改这一处即可，登记范围会自动跟上。
[[nodiscard]] inline bool battleUsable(const Item& item) {
    return item.restoreHp > 0 || item.restoreMp > 0 || item.poison > 0 || item.curesPoison ||
           !item.castMagic.empty();
}

// 法术蓄劲时怎么变强（docs/octopath-battle.md 2.3）。
//   Power：威力 ×(1+N)，仍是一击——一击只削 1 点架势；
//   Hits ：连发 1+N 发，每发单独算伤害、单独判破绽——「弹」这一类用它。
enum class MagicBoost { Power, Hits };

// 法术不伤人的效果（契约 docs/interfaces-p3-ch06.md 第 1 节；Stagger 见 docs/interfaces-p3-ch07.md 第 2 节）。
//   None   ：没有（伤人的法术）；
//   Reveal ：天眼术「看破」——施展一次，本场所有敌人的破绽全部揭开。不伤人、不削架势、不吃蓄劲。
//   Stagger：削架势——一个敌人的架势减 Magic::stagger 点，削到 0 就破势。不判破绽、不伤人、不吃蓄劲。
// 与 power > 0 / poison > 0 互斥（加载器报错）：一门法术要么伤人，要么看破或削架势。追加在末尾。
enum class MagicEffect { None, Reveal, Stagger };

struct Magic {
    std::string id;
    std::string name;
    std::string descKey;
    int element = kElementNone;
    int needMp = 5;
    int power = 10;
    // castRange（施法距离）随格子一起删掉：横版战斗里人人够得着人人。
    rules::Realm needRealm = rules::Realm::QiRefining1;

    // 法术也能下毒（契约第 2.3 节要求物品与法术各一条路径）。同样追加在末尾。
    // 同上：尸虫丸不是这里的毒。
    int poison = 0;
    int poisonPower = 0;

    // 蓄劲的方式（数据字段 "boost": "hits" | "power"，缺省 power）。追加在末尾。
    MagicBoost boost = MagicBoost::Power;

    // 不伤人的效果（数据字段 "effect": "reveal" | "stagger"，缺省没有）。追加在末尾。
    MagicEffect effect = MagicEffect::None;

    // 削几点架势（数据字段 "stagger"，1–9，缺省 1；只对 effect == Stagger 有意义）。追加在末尾。
    int stagger = 1;
};

// 法术这一击是什么类别：它的五行；带毒的再加一个「毒」（docs/octopath-battle.md 2.2）。
// 带毒那一条是本批加的口径：一门会下毒的法术打在怕毒的人身上却不算破绽，
// 玩家只会以为是 bug。第 1–5 章我方没有带毒的法术，这一条眼下只管敌人那几门。
[[nodiscard]] inline int magicCategories(const Magic& magic) {
    int out = categoriesOfElement(magic.element);
    if (magic.poison > 0) out |= kCategoryPoison;
    return out;
}

// 这门法术能不能拿来打人：有威力或者会下毒。
//
// 御风决、护身罡这类 power 0 的辅助法术从前靠 castRange 0「够不着任何人」被挡在
// 施法之外，菜单上那句理由是「超出施法距离」——对一门辅助法术那是误导。
// 格子没了，挡它的就得是一条说得出口的规则：它本来就不是伤人的法术。
[[nodiscard]] inline bool offensiveMagic(const Magic& magic) {
    return magic.power > 0 || magic.poison > 0;
}

// 这门法术在战斗里能不能施展：伤人的，或者带一样不伤人的效果（天眼术的看破、削架势）。
// 菜单「能不能点」问它；凡是「伤害 / 下毒」的分支照旧问 offensiveMagic。
// 护身罡、御风决（power 0、没有 effect）两样都不是，照旧点不动（契约 docs/interfaces-p3-ch06.md 1.3）。
[[nodiscard]] inline bool castableMagic(const Magic& magic) {
    return offensiveMagic(magic) || magic.effect != MagicEffect::None;
}

// 队伍成员（P3 第 3 章增补，契约第 1.1 节）。
//
// **韩立不进队伍**：他的状态散在 GameState 的既有字段里（hp / realm /
// cultivation…），硬塞进来会立刻出现「两处都记着他的血量」这种双重真源。
struct PartyMember {
    std::string roleId;     // 对应 data/roles/
    int hp = -1;            // 当前气血；-1 表示「按角色模板满血」
    bool active = true;     // 是否参战。留着位置但不上场的同伴用得上
};

// 首领的「蓄势」（docs/octopath-battle.md 2.6）。every 为 0 表示不会蓄势。
//
// 节奏按它**自己出过手的回合**数：每到第 every 个出手回合，它那一回合的第一手
// 用来宣告蓄势（不打人），下一手就是重招——伤害 ×mult，all 为真时打我方全体。
// 重招出手之前把它打到破势，重招作废。
struct ChargeSpec {
    int every = 0;
    double mult = 1.0;
    bool all = false;
    std::string textKey;   // 宣告时那一句预告（文案 key）
};

struct RoleTemplate {
    std::string id;
    std::string name;
    rules::Realm realm = rules::Realm::Mortal;
    int maxHp = 10;
    int maxMp = 0;
    int attack = 1;
    int defence = 1;
    int speed = 1;
    int element = kElementNone;
    std::vector<std::string> magics;

    // ---- 破势与蓄劲（八方旅人化改造，docs/octopath-battle.md 第 5 节）----
    // 一律追加在末尾。数据里写的是类别的中文名，加载器换成位掩码。
    //
    // 普攻可用的兵刃。**缺省空手（拳）**：谁都打得出一拳，没写这一项的角色
    //（绝大多数敌人、也包括码头打手这种敌友两用的）就是空手。
    int weapons = kCategoryFist;
    // 架势。0 = 没有架势（我方、不上场的 NPC）；上场的敌人由门禁要求至少 1。
    int toughness = 0;
    // 破绽（位掩码）。
    int weaknesses = 0;
    // 每回合行动次数（首领可以不止一次）。
    int actions = 1;
    ChargeSpec charge;
    // 只有这几类要得了他的命（位掩码，0 = 什么都要得了）。只对敌人生效，见 Unit::killableBy。
    int killableBy = 0;
};

// 背包条目。灵草按年份分堆，故年份进 key 的一部分。
struct BagEntry {
    std::string itemId;
    int count = 0;
    int herbAge = 0;
};

// ---- 静态数据仓库（从 data/ 加载，运行期只读）----
// 一步主线目标。
//
// 「下一步该做什么、该去哪」此前只存在于脚本的触发器与旗标里，玩家在画面上
// 看不到任何一个字——引导写在对白里（「三叔支开他去街上打听」），说完就没了。
// 这张表把每一步显式写出来：完成与否看 doneFlag，去哪看 targetMap/targetObject。
//
// **不新造一套进度**：doneFlag 就是剧情脚本本来就要置的那个完成旗标，
// 所以目标链不可能与剧情走岔——脚本演完，这一步自动算完。
struct Objective {
    std::string id;           // 章内唯一，形如 n3_baoming
    std::string textKey;      // 玩家看到的那一行
    std::string doneFlag;     // 置位即视为已完成
    std::string targetMap;    // 去哪张图
    std::string targetObject; // 那张图上的哪个对象（npc_/trigger_…）
    int chapter = 1;
};

// ---- 任务（P3 第 5 章增补，契约 docs/interfaces-p3-ch05.md 第 1 节）----
//
// **任务只是一张「怎么从旗标和背包读出进度」的表。** 它不存任何东西、不发任何东西、
// 不开新的存档字段：接、推进、交、过期，一律是剧情脚本本来就要做的置旗标与给物品，
// 任务系统只负责把它们读成四种状态之一（rules/Quests.h）。所以这里全是只读的静态数据。
//
// 主线任务不是另写一份：它就是 data/objectives/chNN.json 那条目标链，加载器在读目标链的
// 同一趟里原地生成（契约 1.5）。支线来自 data/quests/<id>.json，一任务一文件。
enum class QuestKind { Main, Side };

// 一条谓词。契约 1.2 只给了三种，刻意不多：设计第 6 节三条支线用不到第四种，
// 而多一种写法就多一处「两种写法说的是不是同一件事」要人判断。
struct QuestCondition {
    enum class Op {
        FlagAtLeast,   // flag(subject) >= value
        FlagEquals,    // flag(subject) == value
        ItemAtLeast,   // itemCount(subject) >= value，各年份合计
    };
    Op op = Op::FlagAtLeast;
    std::string subject;   // 旗标名或物品 id，由 op 决定是哪一种
    int value = 1;
};

struct QuestStep {
    std::string id;
    std::string textKey;
    // 全部成立即这一步做完了。**主线任务的步骤这里是空的**：主线的完成只认目标链判据
    // （Objective::doneFlag 置位，rules::currentObjective），不在这里另写一份——两份判据
    // 早晚会走散，而走散的那一刻 HUD 与告示板就说着两件不同的事。
    std::vector<QuestCondition> done;
    std::string targetMap;     // 可选，与 targetObject 成对
    std::string targetObject;
};

struct Quest {
    std::string id;
    std::string name;          // 给维护者看的，不上屏
    int chapter = 0;
    QuestKind kind = QuestKind::Side;
    std::string titleKey;
    std::string summaryKey;
    std::vector<QuestStep> steps;
    std::vector<QuestCondition> accept;
    std::vector<QuestCondition> complete;
    // 空表 = 这条任务不会过期（不是「空 AND 恒真 = 恒失败」，见 rules::questStatus）。
    std::vector<QuestCondition> fail;
    std::string failTextKey;
    // 只给测试对账脚本里的 give（契约 Q4：任务系统不发奖励）。
    std::vector<BagEntry> rewards;
    std::string origin;
    std::string note;
};

// 路径行动（契约 docs/interfaces-octo-pathactions.md 第 4.3 节）。定义在本文件末尾那一段追加里；
// 这里先声明一句，GameData 才放得下它的 vector（C++17 起 vector 容许元素类型此刻还不完整，
// 只要用到成员之前补全——本文件读完时它就完整了）。
struct PathAction;

struct GameData {
    std::map<std::string, Item> items;
    std::map<std::string, Magic> magics;
    std::map<std::string, RoleTemplate> roles;
    // 文案：key → UTF-8 文本。脚本只引 key，不写字面量。
    std::map<std::string, std::string> text;
    // 主线目标链，按章号与章内顺序排好。顺序就是推进顺序，选型只认这一个次序。
    std::vector<Objective> objectives;
    // 任务（契约 docs/interfaces-p3-ch05.md 第 1 节）：主线（每章一条，由目标链原地生成）
    // 按章号在前，支线按（章号，id）在后。objectives 仍是 HUD 与告示板上半截的唯一来源，
    // 这里的主线任务与它逐步对应、同一趟加载出来，不会走散。
    std::vector<Quest> quests;
    // 路径行动，按章号排好（io/PathActionLoader.h 的 loadPathActionDir，只查形状；
    // 引用由门禁与 tests/PathActionTests.cpp 在真树上查）。
    std::vector<PathAction> pathActions;

    [[nodiscard]] const Item* findItem(const std::string& id) const;
    [[nodiscard]] const Magic* findMagic(const std::string& id) const;
    [[nodiscard]] const RoleTemplate* findRole(const std::string& id) const;
    // 查不到时返回 key 本身，便于在画面上一眼看出缺哪条文案。
    [[nodiscard]] std::string lookupText(const std::string& key) const;
};

// ---- 存档状态（可变）----
struct GameState {
    std::string mapId;
    Point position;
    int facing = 2;               // 0 上 1 右 2 下 3 左

    rules::Realm realm = rules::Realm::Mortal;
    int cultivation = 0;
    int hp = 10, maxHp = 10;
    int mp = 0, maxMp = 0;

    int day = 1;                  // 游戏内日历
    int chapter = 1;              // 当前章（1-14）

    std::vector<BagEntry> bag;
    std::map<std::string, int> flags;     // 命名旗标，需在 data/flags.json 登记

    // ---- P2 系统的可持久化状态（追加在末尾，不破坏既有的聚合初始化）----

    // 掌天瓶。全作经济中枢：凝液 → 催熟灵草 → 炼丹/出售 → 资源 → 修为。
    // owned 与 matureKnown 是两个独立开关，对应原著里相隔四年的两个节点，
    // 不要合并成一个「有没有瓶子」。
    rules::Bottle bottle;

    // 灵田。可能同时有多处（谷中药圃、后来的药园、海岛洞府）。
    std::vector<rules::SpiritField> fields;

    // 四艺熟练度，0-100。炼制成功率按它算。
    int alchemyProficiency = 0;
    int talismanProficiency = 0;
    int forgeProficiency = 0;
    int formationProficiency = 0;

    // 资质。影响修炼效率与突破成功率，由剧情设定，不随打坐变化。
    int aptitude = 50;

    // 打坐收益的小数余数。炼气期升一层只要几十点，摊到每天不足 1 点，
    // 整数除法会把单日打坐的收益吃光。规则层刻意不设每日保底（那会被
    // 拆成一天一次打坐刷十倍），于是余数累加器放在这里。
    int cultivationRemainder = 0;

    // 日课已结算到的那一天。凡人（口诀）阶段每过去的日子都算他照旧默过一遍
    // 口诀，由修炼面板按水位懒结算——第 2 章的四年是剧情整年整年推过去的，
    // 只认玩家点击的话，那四年在修为账上是空的。详见 game/CultivationScene.h。
    //
    // 0 表示「还没起课」：新档从第一次打开修炼面板那天起算，老档同样，不倒补。
    int lastPracticeDay = 0;

    [[nodiscard]] rules::SpiritField* findField(const std::string& fieldId);

    // 两个计时器，单位秒。见方案 2.2 的防刷口径。
    double playSecondsGameplay = 0.0;     // 计入 10h 门槛
    double playSecondsSystem = 0.0;       // 系统菜单，不计入

    [[nodiscard]] int flag(const std::string& name) const;
    void setFlag(const std::string& name, int value = 1);
    [[nodiscard]] int itemCount(const std::string& itemId) const;
    // 某一堆（同 itemId 且同年份）的数量。灵草按年份分堆，「有几株黄精」与
    // 「有几株四十四年的黄精」是两个问题，商店卖出要问的是后者。
    [[nodiscard]] int itemCountOfAge(const std::string& itemId, int herbAge) const;
    void addItem(const std::string& itemId, int count, int herbAge = 0);
    // 不指定年份时**先扣年份低的**，把高年份留给玩家（见 .cpp）。
    [[nodiscard]] bool removeItem(const std::string& itemId, int count);
    // 精确扣某一堆。商店卖出必须走这一条：玩家点的是「四十四年那一株」，
    // 走 removeItem 会转头扣掉一年份那株，账面上灵石照给、少的却是另一堆，
    // 而这种错在存档里看不出来，要到玩家回头找那株药时才暴露。
    [[nodiscard]] bool removeItemOfAge(const std::string& itemId, int count, int herbAge);
    // 按年份下限（P3 第 7 章增补，契约 docs/interfaces-p3-ch07.md 第 4 节）：马师伯收
    // 「四十四年以上」的黄精，上面两条都答不了——一条哪一堆都算，一条只认恰好那个年份。
    // 年份不低于 minAge 的有几件（minAge <= 0 即全部年份）。
    [[nodiscard]] int itemCountAtLeastAge(const std::string& itemId, int minAge) const;
    // 从年份不低于 minAge 的堆里扣 count 件，够格里年份低的先扣；不够就一件不扣。
    [[nodiscard]] bool removeItemAtLeastAge(const std::string& itemId, int count, int minAge);

    // ---- 队伍（P3 第 3 章增补，契约第 1.1 节）----
    //
    // 追加在**所有数据成员的末尾**：插在中间会破坏按字段顺序写的聚合初始化。
    // 不含韩立本人（见 PartyMember 的注释）。存档版本因此升到 3，见 io/SaveFile.h。
    std::vector<PartyMember> party;

    // 三个函数就地写成 inline，而不是像上面那些一样落到 Types.cpp：第 3 章三路
    // 并行作业，本次改动的文件白名单里没有 Types.cpp（并行边界见 docs/README.md）。
    // 都只有两三行，放在头里不值一提的编译代价，换的是不去碰别人正在改的文件。
    [[nodiscard]] bool partyHas(const std::string& roleId) const {
        for (const PartyMember& m : party) {
            if (m.roleId == roleId) return true;
        }
        return false;
    }
    // 已在队里则什么都不做（幂等）。返回是否真的加进去了。
    //
    // **不查 data**：core 层没有 GameData，「这个角色 id 存不存在」由 game 层在
    // 入队前判——那里才拿得到 data。见 Application::dispatch 的 PartyAdd。
    bool partyAdd(const std::string& roleId) {
        if (roleId.empty() || partyHas(roleId)) return false;
        party.push_back(PartyMember{roleId, -1, true});
        return true;
    }
    // 返回是否真的有人离队。不在队里时返回 false，让脚本能分辨
    // 「他走了」与「他本来就不在」。
    bool partyRemove(const std::string& roleId) {
        for (std::size_t i = 0; i < party.size(); ++i) {
            if (party[i].roleId != roleId) continue;
            party.erase(party.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
        return false;
    }

    // ---- 已习得法术（P3 第 4 章增补，契约 docs/interfaces-p3-ch04.md 第 1.1 节）----
    //
    // 追加在**所有数据成员的末尾**，与 party 同一个规矩：插在中间会破坏按字段
    // 顺序写的聚合初始化。存档版本因此升到 5，见 io/SaveFile.h。
    //
    // 这一项落地之前，「韩立学会了火弹术」这件事**无处存放**：magics 只在
    // RoleTemplate（data）上，而 data 是运行期只读的静态数据。技术债 G-4。
    //
    // **语义是集合，不是流水账。** 同一个 id 只该出现一次，理由不是洁癖：
    //   · 战斗菜单（BattleScene::buildMagicItems）逐条遍历这份清单建行，
    //     重复的 id 会让同一门法术在菜单里出现两行，两行一模一样、点哪行都一样，
    //     而玩家记住的是「第二行是火球术」；
    //   · count() 要回答的是「他会几门法术」，不是「learn 被调了几次」。
    // 去重发生在**写入侧**（learnMagic 幂等），所以 toJson 永远不会写出重复项；
    // 读入侧另有一道（见 io/SaveFile.cpp），挡的是手改过的存档。
    //
    // 顺序是学会的先后，刻意不排序：菜单的行序因此等于他习得的次序，
    // 火弹术在前、御风决在后——那正是本章两个节点的先后（设计第 1 节约束 1）。
    std::vector<std::string> learnedMagics;

    // 三个函数与 party 那三个同样就地写成 inline，理由也一样：本批的文件白名单里
    // 没有 Types.cpp（并行边界见 docs/README.md），而它们都只有两三行。
    [[nodiscard]] bool knowsMagic(const std::string& magicId) const {
        for (const std::string& id : learnedMagics) {
            if (id == magicId) return true;
        }
        return false;
    }
    // 已会则什么都不做（幂等）。返回是否真的**新**学会了一门 —— 调用方据此
    // 分辨「刚学会」与「早就会」（契约要求两种情形都回填 ok = true，所以这个
    // 返回值不是成败，是「有没有发生变化」，与 partyAdd 同一口径）。
    //
    // **不查 data**：core 层没有 GameData，「这个法术 id 存不存在」由 game 层在
    // 学之前判——那里才拿得到 data。见 Application::dispatch 的 MagicLearn。
    bool learnMagic(const std::string& magicId) {
        if (magicId.empty() || knowsMagic(magicId)) return false;
        learnedMagics.push_back(magicId);
        return true;
    }
    // 返回是否真的忘掉了一门。本来就不会时返回 false，让脚本分得清
    // 「忘了」与「他本来就不会」（与 partyRemove、take() 同一口径）。
    bool forgetMagic(const std::string& magicId) {
        for (std::size_t i = 0; i < learnedMagics.size(); ++i) {
            if (learnedMagics[i] != magicId) continue;
            learnedMagics.erase(learnedMagics.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
        return false;
    }

    // ---- 剧情给的境界上限（第 4 章二次整改，技术债 G-14）----
    //
    // 契约 docs/interfaces-p2.md 第 7 节。玩家自己在打坐面板上按的突破不能越过它；
    // 判定只有一个入口：rules::tryBreakthrough。它**只由剧情抬**——脚本命令 realm.cap，
    // 或者 realm.advance 顺带抬到目标层——而且只升不降。
    //
    // 追加在**所有数据成员的末尾**，与 party、learnedMagics 同一个规矩。存档因此升到 6，
    // 老档读进来取什么值见 io/SaveFile.h 的 legacyRealmCap。
    //
    // **缺省是凡人**：新开一局时韩立还没拿到口诀（第 1 章节点 9 才授），一层也不该按得上去。
    // 这是安全的一侧：哪一章忘了抬上限，玩家看到的是一句明明白白的「卡住了」，
    // 而不是悄悄越过剧情——前者当场就会有人报，后者要到下一章对不上账才发现。
    rules::Realm realmCap = rules::Realm::Mortal;

    // ---- 已揭开的破绽（八方旅人化改造，docs/octopath-battle.md 2.5）----
    //
    // role_id → 类别位掩码。一种敌人的破绽被打出来过一次，下一场再遇上它，
    // 那几格一开场就是亮的——八方旅人的做法，也是下游「路径行动·打探」提前揭破绽
    // 的落点（打探问出来的就往这里写）。
    //
    // 追加在**所有数据成员的末尾**，与 party、learnedMagics、realmCap 同一个规矩。
    // 存档因此升到 7；老档读进来是空表（全都不知道），见 io/SaveFile.h。
    //
    // 按 role_id 记而不是按「这一场的第几个单位」：知道的是「恶狼怕什么」，
    // 不是「上次那一条恶狼怕什么」。战斗开场从这里读进来，收场写回去（BattleScene）。
    std::map<std::string, int> knownWeaknesses;

    [[nodiscard]] int knownWeaknessesOf(const std::string& roleId) const {
        const auto it = knownWeaknesses.find(roleId);
        return it == knownWeaknesses.end() ? 0 : it->second;
    }
    // 只增不减：一样破绽知道了就是知道了。mask 为 0 时什么也不写——
    // 不留一条「知道了零样」的空账，免得存档里平白多出几十个 0。
    void learnWeaknesses(const std::string& roleId, int mask) {
        if (roleId.empty() || mask == 0) return;
        knownWeaknesses[roleId] |= mask;
    }

    // ---- 野外遭遇的计数器（八方旅人化改造，docs/interfaces-octo-encounters.md）----
    //
    // 全局一份，不按遭遇区、不按表分：每日上限防的是「这一天刷了几场」，按区分开的话，
    // 在两块区之间来回走就能把上限翻倍。只有站在遭遇区里迈的步才数（Application::stepEncounters）。
    //
    // 追加在**所有数据成员的末尾**，与 knownWeaknesses 同一个规矩。规则层的注释写明
    // 「随存档持久化」——不存的话，读一次档就把今天的上限清零，存读档成了刷怪的门路。
    // 存档因此升到 8；老档读进来是全 0（从没遇过），见 io/SaveFile.h。
    rules::EncounterState encounter;
};

// ---- 战斗配置（从 data/battles/ 读入，运行期只读）----
//
// 与 core::battle::BattleState 分开：那个是打起来之后的活状态，这个是开打前的
// 编成。混在一起会让「同一场战斗重开一次」变成要小心翻找哪些字段该重置。
struct BattleUnitSpec {
    std::string roleId;
    // position（站在哪一格）随格子一起删掉：横版战斗的站位由界面按敌我与顺序排，
    // 编成里写坐标已经没有任何东西会读它（tools/validate.py 见到 x / y 会报错）。
    bool ally = false;

    // ---- 波次（P3 第 4 章增补，契约 docs/interfaces-p3-ch04.md 第 2.2 节）----
    //
    // **缺省 0 = 开场就在场上**（加载器缺字段时也按 0 收，见 io/BattleLoader.cpp）。
    //
    // 为什么把波次号挂在**单位**上而不是在 BattleSetup 上另开一张
    // `waves: [[...], [...]]` 的表：
    //   · 另开一张表意味着 units 与 waves 两处都能写单位，加载器要判
    //     「两处都写了算谁的」，而那种规则没人记得住。
    //   · 波次只是「什么时候上场」，不是另一种单位。它与 faction 是同一层次的属性，
    //     就该和它并排。
    int wave = 0;
};

struct BattleReward {
    int cultivation = 0;
    int spiritStones = 0;
    std::vector<BagEntry> drops;
};

struct BattleSetup {
    std::string id;
    std::string name;
    int chapter = 1;
    std::string terrain;          // field / indoor / cave / secret / battlefield / sea / mind
    // width / height（战场几乘几）随格子一起删掉，见 BattleUnitSpec::position 的注释。
    bool canEscape = true;
    // 战败是否直接结束游戏。剧情硬仗为真；可重复的遭遇战为假，
    // 输了只是被打退，不该让玩家丢掉几十分钟进度。
    bool defeatIsFatal = true;
    // playerSpawn（韩立站哪一格）同上删掉。
    std::vector<BattleUnitSpec> units;
    BattleReward reward;
    std::string introKey;

    // ---- 韩立不在场（P3 第 5 章增补，契约 docs/interfaces-p3-ch05.md 第 2 节）----
    //
    // **缺省 false**（加载器缺字段时也按 false 收，见 io/BattleLoader.cpp）。
    //
    // 为真时：不建韩立这个单位、不带队伍里的同伴、不登记背包里的东西；我方就是编成里
    // 写死的 ally；战后存档的气血、法力、同伴气血一个字节不动（第 5 章夺帮那一夜他在
    // 客栈睡觉，ch122）。第 0 波里至少要有一个 ally，加载器查。
    bool heroAbsent = false;

    // ---- 战斗背景（八方旅人化改造，画面路用）----
    // 可选；空串 = 按 terrain 推（BattleScene::backdropOf）。规则层一个字节也不读它。
    std::string backdrop;
};

// ---- Tiled 地图 ----
struct MapObject {
    std::string name;
    std::string type;                         // spawn / portal / npc / trigger / encounter / facility
    Point position;                           // 以格为单位
    int width = 1, height = 1;                // 以格为单位
    std::map<std::string, std::string> properties;

    [[nodiscard]] std::string property(const std::string& key) const;
};

// 地图名那条文案 key，由地图 id 推出来：ch02_yaopu → ch02.map.yaopu.name。
//
// 为什么能推：全 18 张图的 display_name_key 都是这个形状，且 tools/validate.py
// 有一条规则钉着它必须是（规则 21）。**没有那条规则就不许这么推**——一条没人
// 守的命名约定，正是这个工程刚刚在瓦片编号上栽过的那种「两份约定分了家」。
//
// 用处是拿到一张**没有载入**的图的名字：目标在别的图上时，屏幕上要写的是
// 「七玄门外刃堂」而不是 ch02_wairentang。引擎同一时刻只载一张图，那张图的
// displayNameKey 只回答它自己。
[[nodiscard]] std::string mapDisplayNameKey(const std::string& mapId);

struct TileMap {
    std::string id;
    std::string displayNameKey;
    std::string region;
    std::string bgm;
    int chapter = 1;
    bool outdoor = true;
    // 走到地图边缘是否返回上级地图。室内场景与剧情关卡通常为 false，
    // 免得玩家在剧情中途从边缘走出去。
    bool canLeaveEdge = false;
    // canLeaveEdge 为真时的返回目标地图 id。
    std::string parentMap;
    int width = 0, height = 0;

    // 图层顺序固定，见 docs/map_spec.md 第 3 节。
    std::vector<int> ground, overlay, building, front, collision;
    std::vector<MapObject> objects;

    [[nodiscard]] bool inBounds(Point p) const;
    [[nodiscard]] bool walkable(Point p) const;
    [[nodiscard]] const MapObject* objectAt(Point p, const std::string& type) const;
    [[nodiscard]] const MapObject* defaultSpawn() const;
};

// ---- 路径行动（八方旅人化改造 P 路，契约 docs/interfaces-octo-pathactions.md）----
//
// 韩立对某个 NPC 的三种特别交互：打探（情报）、求购（物件）、切磋（过招）。
// 一条条目挂在（地图，NPC 对象名）上；什么时候挂着由 when / until 两张谓词表定，
// 能不能做由「阅历」门槛（境界）与碎银定，做成一次记在 doneFlag 上。
//
// 与任务同一个口径：这里全是只读的静态数据，规则层（rules/PathActions.h）只回答
// 「此刻能做什么、做了会怎样」，改存档的是游戏层。谓词沿用 QuestCondition——
// 同一套写法、同一套门禁，不另造第二种「旗标 / 物品 + op + value」。
//
// 追加在文件末尾：本轮 Types.h 只许追加，战斗路合回时要与这个文件做三方合并。
enum class PathActionKind {
    Inquire,     // 打探：说一段情报，头一回可附带揭破绽、给物件、置本文登记的旗标
    Purchase,    // 求购：拿碎银向这个人买一件他那里才有的东西，每条只买一次
    Challenge,   // 切磋：与这个人过招，赢了有奖励（只发一次），输了不死、可以再来
};

// 打探揭开的一类破绽（施工图 2.2 的攻击类别）。「已知破绽」写进存档那一步归战斗路，
// 这里只说揭开哪一个角色的哪一类。
struct PathReveal {
    std::string roleId;     // data/roles 里的角色 id
    std::string category;   // sword / blade / fist / dart / poison / metal / wood / water / fire / earth
};

struct PathAction {
    std::string id;         // 章内唯一：<npc 短名>_<dating | qiugou | qiecuo>[序号]
    int chapter = 0;
    PathActionKind kind = PathActionKind::Inquire;
    std::string mapId;      // 挂在哪张图
    std::string npc;        // 那张图上哪个 npc 对象（对象名，不是 role_id）
    // 挂出来的时段：when 全部成立（AND），且 until 无一成立（OR）。
    std::vector<QuestCondition> when;
    std::vector<QuestCondition> until;
    // 「阅历」门槛：韩立境界不到，对方就不肯说、不肯卖、不肯过招。凡人 = 没有门槛。
    rules::Realm minRealm = rules::Realm::Mortal;
    std::string doneFlag;   // chNN.path.<id>，做成一次即置 1
    std::string textKey;    // 打探：那段情报；求购：开价；切磋：邀战
    std::string refuseKey;  // 阅历不足时对方那一句（有门槛才有）

    // 打探：头一回才发，再按一次只是重看那段情报。
    std::vector<PathReveal> reveals;
    std::vector<BagEntry> gives;
    std::vector<std::string> setFlags;   // 只许本文登记的 chNN.path.*，不碰剧情旗标

    // 求购
    BagEntry goods;         // 买到什么（物品 / 件数 / 年份）
    int price = 0;          // 碎银，按块计
    std::string dealKey;    // 成交那一句
    std::string poorKey;    // 钱不够那一句

    // 切磋
    std::string battleId;
    // 编成尚未建好（等战斗路按横版格式补建）：加载器不查它的编成，规则层不把它挂出来。
    bool pending = false;
    std::string winKey;
    std::string loseKey;
    int rewardCultivation = 0;           // 奖励只由条目发一次：编成自己的 rewards 必须全 0
    std::vector<BagEntry> rewardItems;

    std::string origin;
    std::string note;
};

}  // namespace fanren::core
