#pragma once
// 境界规则。纯逻辑，不依赖 SDL / JSON / Lua / 文件 IO。
//
// 数值沿用已验证的 fanren-kys 原型（编号留有间隔便于后续插入大境界），
// 境界跨度按 docs/大纲.md v3.1 第 4.3 节的曲线：
// 凡人 → 炼气一至十三层 → 筑基初/中/后 → 结丹初/中/后（乱星海篇终点为结丹后期顶峰）。
#include <cstdint>
#include <string_view>

namespace fanren::rules {

// 境界编号。刻意不连续：大境界之间留出空段，日后插入元婴等阶段不会
// 冲掉已存档的数值。
enum class Realm : std::int32_t {
    Mortal = 0,

    QiRefining1 = 1,
    QiRefining2 = 2,
    QiRefining3 = 3,
    QiRefining4 = 4,
    QiRefining5 = 5,
    QiRefining6 = 6,
    QiRefining7 = 7,
    QiRefining8 = 8,
    QiRefining9 = 9,
    QiRefining10 = 10,
    QiRefining11 = 11,
    QiRefining12 = 12,
    QiRefining13 = 13,

    FoundationEarly = 21,
    FoundationMid = 22,
    FoundationLate = 23,

    CoreEarly = 31,
    CoreMid = 32,
    CoreLate = 33,
};

// 大境界分档，用于境界压制、法术可用性与 UI 分组。
enum class RealmTier : std::int32_t {
    Mortal = 0,
    QiRefining = 1,
    Foundation = 2,
    Core = 3,
};

[[nodiscard]] constexpr std::int32_t toValue(Realm realm) noexcept {
    return static_cast<std::int32_t>(realm);
}

[[nodiscard]] constexpr Realm fromValue(std::int32_t value) noexcept {
    return static_cast<Realm>(value);
}

// 该编号是否对应一个合法境界。存档读入与脚本传值都要先过这道闸。
[[nodiscard]] bool isValid(Realm realm) noexcept;

// 大境界分档。非法编号返回 Mortal。
[[nodiscard]] RealmTier tierOf(Realm realm) noexcept;

// 显示名。非法编号返回「未知」。
[[nodiscard]] std::string_view nameOf(Realm realm) noexcept;

// 下一境界。已达本作上限（结丹后期）或编号非法时返回 std::nullopt 的等价物：
// 这里用 Realm::Mortal 会与合法值混淆，因此改用 bool 出参风格的重载。
[[nodiscard]] bool tryNext(Realm realm, Realm& out) noexcept;

// 突破到下一境界所需修为。已达上限或编号非法返回 -1。
[[nodiscard]] std::int32_t cultivationNeeded(Realm realm) noexcept;

// 境界跌落。第 9 章「遁走」中主角真元被夺，自筑基中期跌回炼气期，
// 本函数按大境界逐级回落，下限为炼气一层（不会掉回凡人：修为根基仍在）。
[[nodiscard]] Realm demote(Realm realm, std::int32_t levels) noexcept;

// a 相对 b 的境界压制系数。跨大境界压制显著，同档内按层数微调。
// 返回值用于伤害与命中计算，1.0 为势均力敌。
[[nodiscard]] double suppressionFactor(Realm attacker, Realm defender) noexcept;

// ---------------------------------------------------------------------------
// 境界给出的气血 / 法力基准
// ---------------------------------------------------------------------------
//
// 在这两条函数出现之前，**全作没有任何一处让 maxHp / maxMp 增长**：突破只改
// 境界编号，`GameState` 的 maxHp 从第 1 章的 10 点一路带到第 14 章还是 10 点。
// 修为于是只是一道剧情闸门，不是「变强」。
//
// 数值不是拍的，是**从 data/roles 已有的敌人曲线上反解出来的**——那份数据是
// 编剧与关卡定好的内容，这里只负责让主角落在同一条曲线上：
//
//   境界          data/roles 里同境界角色的 maxHp        本表取值
//   炼气四层      jiexiu 72                              72
//   炼气五层      daoyu_husui 82                         84
//   炼气六层      mofu_shigui 88 / 黄枫谷外门弟子 92      96
//   炼气八层      zhaoze_yaoshou 112                     120
//   炼气十层      huadaowu_dizi 132                      144
//   炼气十一层    —                                      156
//   炼气十二层    qingxumen_gaotu 178 / lu_shixiong 180  168
//   炼气十三层    heishajiao_shigui 176                  180
//   筑基初期      240 / 262 / 268                        260
//   筑基中期      326 / 340 / 352 / 428 / 430            380
//   筑基后期      wang_chan 470                          480
//   结丹初期      680 / 700 / 720 / 760                  700
//   结丹中期      wu_chou 900                            900
//   结丹后期      feng_xi 1450（第 14 章的终局对手）      1300
//
// 炼气期于是收成一条直线 **24 + 12 × 层数**（法力 10 × 层数），它同时穿过
// jiexiu(四层 72)、lu_shixiong(十二层 180，差 7%) 与 heishajiao_shigui(十三层 176≈180)
// 三个点（陆师兄第 7 章改成十二层、maxHp 钉 180，docs/interfaces-p3-ch07.md 5.3）。只钉一个点的曲线随便一条都能穿过去（见 docs/README.md 那张空转法表里
// 的「只钉一个数据点」），所以单测是**扫一片**：逐层与 data 的同境界角色比对。
//
// 凡人一档刻意保持 10，与 `GameState` 的默认值一字不差：
//   1. 第 1、2 章一场战斗都没有，改它只会平白改掉两章的既有行为；
//   2. 10 点是**韩立这个少年**被写弱的样子，不是「凡人」这一档的代表值——
//      data 里的凡人杂兵在 18（恶狼、韩母）到 64（野狼帮头目）之间；
//   3. 于是「感气入道」那一次突破本身就是最大的一跳（10 → 36），正对得上
//      大纲里凡人与修士之间那条线。
//
// 攻防见下面的 `realmAttack` / `realmDefence`（技术债 G-10 已销账）。
[[nodiscard]] std::int32_t realmMaxHp(Realm realm) noexcept;
[[nodiscard]] std::int32_t realmMaxMp(Realm realm) noexcept;

// ---------------------------------------------------------------------------
// 境界给出的攻 / 防基准（技术债 G-10）
// ---------------------------------------------------------------------------
//
// 在这两条函数出现之前，韩立的攻 6 / 防 3 **写死在 `src/game/BattleScene.cpp`**，
// 从第 1 章到第 14 章一个数也不动。后果不是「少涨了一点」，是把整章的设计
// 反过来了：第 4 章硬约束 7 写的是「全歼来犯者靠算计与毒，不是境界压制」，
// 而在攻防不随境界动的出厂数值里，境界唯一能给的就是法力池（能放几发火弹），
// 于是实测下来**只有抬境界能翻盘**——正好是那条硬约束的反面
//（证据见 docs/ch04-review.md §4.2）。
//
// 数值与 maxHp/maxMp 同一个来路：**从 data/roles 已有的敌人曲线上反解**，
// 主角只是落在同一条线上。炼气期两条线是
//
//     攻 ≈ 3 + 1.8 × 层数      data：四层 12、八层 18、十一层 22、十三层 26
//     防 ≈ 4 + 层数            data：四层 8、六层 10、八层 12、十一层 15、十三层 17
//
// 但**整条曲线锚在「炼气三层 = 攻 6 / 防 3」上**，也就是两条线整体下移一个
// 固定位移（攻 −2.4、防 −4）。锚定是硬要求，不是口味：
//
//   * **不锚定会当场打坏第 3 章的两场教学战。** `data/roles/jiang_shou.json` 与
//     `data/battles/b03_andao_shishou.json` 的说明里白纸黑字写着「韩立平砍
//     6*2-9=3」「本章的尺度是韩立攻 6」，教学战二整场戏建在这个数上；
//     按未锚定的曲线炼气三层该是攻 10，一刀砍僵兽从 5 点跳到 18 点，两下就完，
//     「砍不动就下毒」当场作废。
//   * 防那一侧同样敏感：伤害是 `攻×2 − 防` 的减法，防从 3 抬到 7，
//     教学战一的恶狼（攻 4）一刀只剩保底 1 点，那一场随之作废。
//
// 锚定之后**第 1-3 章一个数不动**（凡人与炼气一至三层全是 6/3），
// 涨的部分全落在还没施工的章节里。
//
// 逐层取值（单测逐层钉死，不是只钉一两个点——「只钉一个数据点」是
// docs/README.md 那张空转法表上的一行）：
//
//     层   1  2  3  4  5  6  7  8  9 10 11 12 13
//     攻   6  6  6  8 10 11 13 15 17 19 20 22 24
//     防   3  3  3  4  5  6  7  8  9 10 11 12 13
//
// 凡人一律 6 / 3，与写死的现值一字不差：第 1、2 章与识海那一场都靠它。
// 注意这与 `realmMaxHp` 的凡人档口径不同——那里 10 点是「韩立这个少年」被写弱
// 的样子，而 6/3 是三章的战斗数值建在上面的那个尺度，改它等于改那三章。
//
// **筑基以上是暂定值，到那一章再标定。** 取值与 data/roles 同境界角色的攻防
// 同量级，并整体下移同一个锚定位移；但那几档一场仗都还没编，没有任何一场
// 战斗能验证它们：
//
//     境界        data/roles 同境界的攻 / 防（样本）        本表取值
//     筑基初期    34/24、31/22、38/26                        32 / 20
//     筑基中期    43/30、38/29、33/25、39/31、52/36          37 / 26
//     筑基后期    wang_chan 46/34                            44 / 30
//     结丹初期    64/47、66/50、62/56、61/48                 62 / 46
//     结丹中期    wu_chou 78/62                              76 / 58
//     结丹后期    feng_xi 104/86                            102 / 82
//
// 非法境界返回凡人值（与 `realmMaxHp` 同一个兜底口径）。
[[nodiscard]] std::int32_t realmAttack(Realm realm) noexcept;
[[nodiscard]] std::int32_t realmDefence(Realm realm) noexcept;

// 一对「当前值 / 上限」。气血与法力共用，免得同一条补齐规则写两遍。
struct Vitals {
    std::int32_t current = 0;
    std::int32_t max = 0;
};

// 把上限抬到境界基准，**只补不削**。
//
// 补上去的那一截当作「他本来就该有的根基」，因此当前值同步抬高相同的量，而不是
// 停在原处：一份炼气三层、10/10 满血的老存档若只抬上限，玩家读档后拿到的是
// 10/60，一进战斗就死——那不是修好了，那是换了个坏法。
//
// 只补不削有两层理由。一是安全：读档时按境界**重算**（而不是取下限）会把日后
// 丹药、功法、装备给的任何一点上限悄悄抹掉，而这种损失在存档里看不出来、也找
// 不回来。二是第 9 章的境界跌落（`demote`，真元被夺自筑基中期跌回炼气期）——
// 掉境界要不要跟着削气血是那一章的设计决定，不该由一条读档时的补齐规则替它定。
[[nodiscard]] Vitals liftToFloor(Vitals v, std::int32_t floor) noexcept;

}  // namespace fanren::rules
