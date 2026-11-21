#pragma once
// 同道录页（第 7 章第二期 2a·路 U，施工图 docs/ch07-tongdaolu.md 第 3 节；施工记录 docs/ch07-tongdaolu-u.md）。
//
// 顶替主菜单里原来的「名册」页（MenuScene::Page::Roster 这个值不变，只换了字与内容）。三层：
//   卡片：data/companions 里的每一位一张卡，三列两行、翻页。遇到过的（met_flag 置了）画立绘的头肩（有立绘时）
//         或像素小人；没遇到的是剪影、名字写「？？？」、底下一句提示（tongdao.<id>.hint，缺了退回缺省）。
//         升档后还没叙话的那一位卡角标「可叙话」。
//   详情：左边大图，右边档名与五格档位（封顶时最高那一格写「机缘未到」）、默契进度、已有 / 下一档给什么
//         （从伙伴表、角色表、法术表生成）、喜好（二档起全列）、在册加成（说明由种类与数值生成，生效没有、名额）、
//         一列动作：赠礼、切磋、请教、叙话、带上在册加成。能不能做一律问规则层（rules/Tongdao.h）；
//         地点由这里判——只有人在百药园（kTongdaoMap）时四种相处可用，别处置灰写「回百药园后可相处」。
//   赠礼：储物袋里送得出手的每一堆，二档起行尾标「合意 / 不喜」；送完说一句加了多少。
//
// 切磋、请教、叙话要关掉菜单再演（开仗、演事件脚本）：update 返回 CloseMenu，菜单那一帧出栈，压上去的东西接着来。
// 事件脚本的约定（路 N 照同一条写）：叙话第 N 档 scripts/ch07/tongdao_<id>_t<N>.lua（N = 2–5），
// 请教 scripts/ch07/tongdao_<id>_qingjiao.lua；脚本末尾自己调 tongdao.xuhua_mark / tongdao.qingjiao 记账。
// 文件不在时那一项置灰写「（内容未到）」。
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "engine/Engine.h"
#include "ui/Widgets.h"

namespace fanren::game {

class Application;

// 相处只在这张图上（施工图 3.4「地点在百药园」）。
inline constexpr const char* kTongdaoMap = "ch06_baiyaoyuan";

// 叙话 / 请教的事件脚本（相对 scripts/）。
[[nodiscard]] std::string tongdaoXuhuaScript(const std::string& companionId, int tier);
[[nodiscard]] std::string tongdaoQingjiaoScript(const std::string& companionId);
// 这段脚本在游戏根的 scripts/ 下有没有。
[[nodiscard]] bool tongdaoScriptExists(Application& app, const std::string& relative);

// 把 {slot} 换成 value（文案里的占位）。
[[nodiscard]] std::string tongdaoFill(std::string text, const std::string& slot, const std::string& value);

// 一条被动 / 在册加成的说明（种类 + 数值生成，例「追击伤害 +20%」）；kind None 返回空串。
[[nodiscard]] std::string perkText(const core::GameData& data, const core::CompanionPerk& perk, int value);

// 第 tier 档（2–5）给什么；tier 1 给的是「可同行、赠礼、同修」与她一档就会的法术。按画面上的次序。
[[nodiscard]] std::vector<std::string> tierGains(Application& app, const core::Companion& companion, int tier);

// 一位在同道录页上的样子（卡片与详情共用），每次重建时从状态算一遍。
struct TongdaoEntry {
    std::string id;
    bool met = false;
    bool person = true;
    std::string roleId;     // 此刻的角色（没遇到的取第一段）：名字、立绘、像素小人都按它
    std::string name;       // 没遇到的是「？？？」
    std::string intro;      // 卡片上那一行：遇到过的写 tongdao.<id>.intro（缺了写默契），没遇到的写提示
    bool introIsBond = false;   // intro 是退回的「默契 N」（详情页进度那一行已经写了，不重复）
    int bond = 0;
    int tier = 1;
    int capTier = 5;        // rules::bondTierCap
    std::string tierName;
    bool xuhuaReady = false;   // 升了档、该演的那段脚本也在：卡角标「可叙话」
};
[[nodiscard]] std::vector<TongdaoEntry> tongdaoEntries(Application& app);

// 详情页那一列动作（次序固定：赠礼、切磋、请教、叙话、带上在册加成）。不能做的 enabled = false、disabledReason 写为什么。
enum class TongdaoAction { Gift, Spar, Qingjiao, Xuhua, Bonus };
inline constexpr int kTongdaoActionCount = 5;
[[nodiscard]] std::vector<ui::ListItem> tongdaoActionRows(Application& app, const std::string& companionId);

class TongdaoPage {
public:
    enum class Focus { Cards, Detail, Gift };
    enum class Outcome { Stay, Back, CloseMenu };

    // ---- 版面的数（1280×720，主菜单右栏 kPagePanel 的正文区里；施工记录第 2 节）----
    static constexpr int kCols = 3;
    static constexpr int kRows = 2;
    static constexpr int kPerPage = kCols * kRows;
    static constexpr int kCardW = 260;
    static constexpr int kCardH = 248;
    static constexpr int kCardGap = 16;
    static constexpr int kCardTop = 36;        // 正文区顶往下：上面一行写提示与「外传」
    static constexpr int kCardArtH = 168;      // 卡片上半截的画
    static constexpr int kDetailArtW = 300;    // 详情页左栏的大图
    static constexpr int kActionRows = kTongdaoActionCount;

    // 正文区（MenuScene 右栏扣掉标题带）里第 slot（0–5）张卡的位置。
    [[nodiscard]] static engine::Rect cardRect(const engine::Rect& area, int slot);
    // 详情页：左栏、右栏、动作列表、赠礼列表。
    [[nodiscard]] static engine::Rect detailArtRect(const engine::Rect& area);
    [[nodiscard]] static engine::Rect detailInfoRect(const engine::Rect& area);
    [[nodiscard]] static engine::Rect actionListRect(const engine::Rect& area, const ui::Theme& theme);
    [[nodiscard]] static engine::Rect giftListRect(const engine::Rect& area);

    // 重读一遍状态（打开菜单、送完礼、回到这一页时）。光标尽量留在原来那一位上。
    void rebuild(Application& app);
    // 进来：从卡片层起，光标停在第一位遇到过的人上（没有就第 0 位）。
    void enter(Application& app);
    // 截图口与测试：直接停在某一位的详情（id 不在表里或没遇到返回 false）。
    bool openDetail(Application& app, const std::string& companionId);
    bool openGift(Application& app, const std::string& companionId);

    Outcome update(Application& app);
    // focused 为假：菜单左栏光标停在这一页上的预览（卡片层、不亮光标）。
    void render(Application& app, const engine::Rect& area, bool focused) const;

    [[nodiscard]] Focus focus() const { return focus_; }
    [[nodiscard]] int cursor() const { return cursor_; }
    [[nodiscard]] int page() const { return cursor_ / kPerPage; }
    [[nodiscard]] int pageCount() const;
    [[nodiscard]] const std::vector<TongdaoEntry>& entries() const { return entries_; }
    [[nodiscard]] const ui::ListView& actions() const { return actions_; }
    [[nodiscard]] const ui::ListView& gifts() const { return gifts_; }
    [[nodiscard]] std::string selectedId() const;

    // 详情页「喜好」那一块的行：一行放得下就「喜欢：…　不喜：…」一行；放不下就把「不喜：…」整段挪到第二行
    //（不在词中间断开）；某一段自己还放不下才截断加「…」。measure 量一段字的像素宽。
    [[nodiscard]] static std::vector<std::string> likesLines(const std::string& like, const std::string& dislike,
                                                             int width,
                                                             const std::function<int(const std::string&)>& measure);

    // 这一页可能画出来的全部固有字（UiStyleTests 那种禁词扫描用）。
    [[nodiscard]] static std::vector<std::string> strings(const core::GameData& data);

private:
    struct GiftChoice {
        std::string itemId;
        int herbAge = 0;
    };
    // 一张卡或详情的画：立绘（线性）或像素小人；都没有时空着（剪影画一个「？」）。
    struct Art {
        engine::TextureId bust = engine::kInvalidTexture;
        engine::RectF bustSrc{};          // 整格（840×1260 那种）
        engine::TextureId sprite = engine::kInvalidTexture;
        engine::Rect spriteFrame{};
        int spriteScale = 4;
        int spriteFootY = 23;
    };

    void loadArt(Application& app);
    void rebuildActions(Application& app);
    void rebuildGifts(Application& app);
    Outcome activate(Application& app);
    void giveSelected(Application& app);
    void moveCursor(Application& app, int dx, int dy);
    void stepPerson(Application& app, int direction);

    void drawCards(Application& app, const engine::Rect& area, bool focused) const;
    void drawCard(Application& app, const engine::Rect& box, std::size_t index, bool selected) const;
    void drawDetail(Application& app, const engine::Rect& area) const;
    void drawInfo(Application& app, const engine::Rect& info, const TongdaoEntry& entry) const;
    void drawArtInto(Application& app, const engine::Rect& box, std::size_t index, bool full, bool silhouette) const;
    void drawWaizhuan(Application& app, const engine::Rect& area) const;

    std::vector<TongdaoEntry> entries_;
    std::vector<Art> art_;
    int cursor_ = 0;
    Focus focus_ = Focus::Cards;
    ui::ListView actions_;
    ui::ListView gifts_;
    std::vector<GiftChoice> giftChoices_;
};

}  // namespace fanren::game
