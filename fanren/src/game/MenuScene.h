#pragma once
// 主菜单（施工图 docs/octopath-overhaul.md 1.5 / 第 4 节）：行走时 Tab / Esc 打开。
//
//   左栏：状态 · 物品 · 法术 · 记事 · 存盘 · 设置 · 返回首页 · 返回，下面一块写盘缠、日期、所在。
//   右栏：光标停在哪一项就预览哪一页；物品、法术按确认进到右栏的列表里看说明。
//   记事：直接翻开告示板那一套（BoardScene），不另写一份。
//   存盘：与 F5 同一条路（Application::quickSave → saves/quick.sav），成败都说一句。
//   设置：右栏写一句说明 + 当前几项的摘要；确认压系统设置面板（SettingsScene，与标题画面共用，
//         docs/settings.md 5.1），菜单照「记事」的做法只画模糊底图，面板一收再成为顶层。
//
// 底图是打开那一帧世界画面的模糊快照（PostFx::blurredSnapshot），之后每帧只画它，
// 世界层不再重画。音效：开 ui_open、合 ui_close、移动 ui_cursor、确认 ui_confirm、
// 退一级 ui_cancel；缺文件时引擎静默。
//
// 用词走 Wording：第 1–5 章 story.xiuxian_known 不置，第 6 章太南谷听完灵根之说时置
//（scripts/ch06/guaipo.lua；G-13 已按第 6 章拍板收口）。不置时钱叫「碎银」、境界说「口诀第几层」、
// 法力叫「气力」、「法术」一栏叫「法门」。面板上能出现的固有字全部由 menuStrings 列出，
// tests/UiStyleTests.cpp 拿禁词表扫它。
//
// 伙伴改版（docs/interfaces-companions.md 2.2、3.1）：名册上有人（rules::roster 非空）时，左栏「状态」下面
// 多出两项——
//   队伍：列出队里每个人。伙伴那一行确认后可以「上场 / 歇着」（改 PartyMember::active；3.1：active 只在
//         这里改）或「赠与」（挑一件东西，rules::giveGift 记账：每人每天一次，今天送过的不扣东西、说一句）。
//         不是伙伴的队员（傀儡、化身）显示「随行」，不能改、不能送。
//   名册：已结识的伙伴与默契的档（rules::bondTier）。
// 名册是空的（第 1–6 章、截图口的默认情形）时这两项不出现，菜单与改版前逐像素相同。
// 小像的倍数读每张人物表自己的 scale（menuSpriteScale）：旧表 scale 3 → ×4，与改版前一样。
#include <memory>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "engine/PostFx.h"
#include "game/ItemInfoView.h"
#include "game/Scene.h"
#include "game/Wording.h"
#include "ui/Widgets.h"

namespace fanren::game {

class MenuScene : public Scene {
public:
    // 前八项的次序即画面上的次序；返回首页需要确认，「返回」只关闭菜单。
    // Party（队伍）、Roster（名册）追加在末尾是为了不挪动前八项的值（截图口按名字取它们）；
    // 画面上它俩排在「状态」后面，且只在名册上有人时出现（见文件头）。
    // Map（地图）是第 7 章改版·路 M 追加的（docs/ch07-foundation.md 3.2）：画面上排在「记事」后面，确认打开区域地图。
    enum class Page { Status, Items, Magic, Journal, Save, Settings, Home, Back, Party, Roster, Map };

    // 小像按几倍画：人物表的世界层倍数的三分之四，取整（旧表 3 → 4，高清表 2 → 2），至少 1。
    // 菜单的小像一向比走在地图上的大三分之一（16×24 的旧表：地图 ×3、菜单 ×4）。
    [[nodiscard]] static int menuSpriteScale(int sheetScale);

    // 小像贴在框里的哪一块：帧按 scale 放大、左右居中，脚底（帧内 footY 那一行的下沿）落在框底往上
    // kPortraitFootGap 处；超出的部分连同源图上对应的那一截一起裁掉。裁的边界：左、右、下就是框；
    // 上沿许高出框 kPortraitHeadroom——旧的 24×32 表（曲魂、傀儡、熊）×4 是 128 高，改版前起头顶就高出
    // 框 12，裁掉就改了现有画面。行与行之间空着 22（上一行的分隔线在框顶往上 22），16 碰不到它。
    // 旧表 16×24 不裁，与改版前一模一样；高清表（64×60、×2）左右各裁 16。
    // dst 为空（宽或高 ≤ 0）= 整帧都在可画范围之外，不画。
    struct PortraitPlacement {
        engine::RectF src;
        engine::RectF dst;
    };
    static constexpr int kPortraitFootGap = 16;
    static constexpr int kPortraitHeadroom = 16;
    [[nodiscard]] static PortraitPlacement portraitPlacement(const engine::Rect& box, const engine::Rect& frame,
                                                             int scale, int footY);

    // initial：打开时光标停在哪一项；focusList：物品 / 法术页直接把光标放进右栏的列表，
    // 记事页直接翻开告示板，首页页直接进入确认（截图口用；游戏里按 Tab 打开时停在「状态」）。
    // listRow：物品 / 法术页聚焦后光标再往下挪几行（截图口 menu:items:<n> 用，看某一样的四行式说明）。
    explicit MenuScene(Page initial = Page::Status, bool focusList = false, int listRow = 0);
    ~MenuScene() override;

    // 这一阶段菜单上可能出现的全部固有字（左栏、页眉、空表提示、状态页的标签、
    // 盘缠与日期的前缀）。与 render 取的是同一批 key 与同一张用词表，禁词扫描才有意义。
    [[nodiscard]] static std::vector<std::string> menuStrings(const core::GameData& data,
                                                              PanelStage stage);

    // 左栏各项的字。法术那一栏按阶段换说法。
    [[nodiscard]] static std::string pageLabel(const core::GameData& data, PanelStage stage,
                                               Page page);
    // 画在屏上的那一份（左栏与右栏页眉都取它）：同上，另外物品那一栏按 ch06.chuwudai
    // 换成「储物袋」（Wording 的 bagWord，契约 docs/interfaces-p3-ch06.md 第 4 节）。
    [[nodiscard]] static std::string pageLabel(const core::GameData& data, const core::GameState& state,
                                               Page page);

    // 物品页的行：背包里除了钱（钱在左下角另写）以外的每一堆，灵草带年份。
    [[nodiscard]] static std::vector<ui::ListItem> itemRows(const core::GameData& data,
                                                            const core::GameState& state);

    // 名册页的行：名册上每位伙伴（rules::roster 的次序），左边是她此刻那个角色的名字，右边是默契的档名。
    [[nodiscard]] static std::vector<ui::ListItem> rosterRows(const core::GameData& data,
                                                              const core::GameState& state);

    // 左栏此刻有哪几项，按画面次序（测试读它：名册空着时就是改版前那八项）。
    [[nodiscard]] const std::vector<Page>& pages() const { return pages_; }

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;

    // 快照拍到之前要让世界层在底下画一帧（快照拍的就是它）；拍到之后只画模糊底图。
    [[nodiscard]] bool opaque() const override { return backdropReady_; }
    [[nodiscard]] std::string name() const override { return "Menu"; }

private:
    enum class Focus { Commands, Items, Magic, Home, Party, PartyActions, Gift, ItemTarget };

    // 一个在队的人：状态页一行。小像在打开菜单时就解析好（贴图句柄 + 取哪一帧），
    // 不在 render 里每帧去查索引、载图——缺图的那一行每帧都会打一次 WARNING。
    struct Member {
        std::string roleId;
        std::string name;
        int hp = 0, maxHp = 0, mp = 0, maxMp = 0;
        int attack = 0, defence = 0, speed = 0;
        bool hero = false;   // 韩立：多一行境界（同伴不练那段口诀）
        engine::TextureId portrait = engine::kInvalidTexture;
        engine::Rect portraitFrame{};
        int portraitScale = 4;   // menuSpriteScale(这张表的 scale)
        int portraitFootY = 23;  // 这张表的脚底（帧内的行）
    };
    // 赠与列表的一行是背包里的哪一堆（灵草按年份分堆）。
    struct GiftChoice {
        std::string itemId;
        int herbAge = 0;
    };

    [[nodiscard]] Page page() const;
    [[nodiscard]] engine::Rect commandPanel(const ui::Theme& theme) const;
    void rebuildLists(Application& app);
    void rebuildCompanionLists(Application& app);
    void loadMembers(Application& app);
    // 队伍页光标所在那一行的伙伴 id；不是伙伴（或没有选中）时为空串。
    [[nodiscard]] std::string selectedCompanion(Application& app) const;
    // 队伍页光标所在那一行对应 GameState::party 里的哪一位；没有选中、或下标对不上时为 nullptr。
    [[nodiscard]] core::PartyMember* selectedMember(Application& app) const;
    void updateParty(Application& app, bool confirmed);
    void updatePartyActions(Application& app, bool confirmed);
    void giveSelected(Application& app);
    // 战斗外用药（rules::FieldItems）：物品页按确认 → 不止一人时先选给谁 → 扣一件、说一句。
    void useSelectedItem(Application& app);
    void applyItemTo(Application& app, int target);   // target：-1 主角，否则 GameState::party 的下标
    void rebuildItemTargets(Application& app);
    // 确认了左栏的某一项。返回 false 表示菜单就此关闭。
    bool activate(Application& app);
    void close(Application& app);

    void drawBackdrop(Application& app);
    void drawCommands(Application& app) const;
    void drawInfo(Application& app) const;
    void drawPage(Application& app) const;
    void drawStatus(Application& app, const engine::Rect& area) const;
    void drawMember(Application& app, const Member& member, int x, int y) const;
    void drawPortrait(Application& app, const Member& member, const engine::Rect& box) const;
    void drawListPage(Application& app, const engine::Rect& area, const ui::ListView& list,
                      bool focused, const std::string& description) const;
    // 物品页、法术页：左边列表、右边说明块（block 为 nullptr 时写一句「选中一样……」）。
    void drawSplitPage(Application& app, const engine::Rect& area, const ui::ListView& list, bool focused,
                       const DetailBlock* block) const;
    void drawJournal(Application& app, const engine::Rect& area) const;
    void drawSave(Application& app, const engine::Rect& area) const;
    void drawSettings(Application& app, const engine::Rect& area) const;
    void drawHome(Application& app, const engine::Rect& area) const;
    void drawMapPage(Application& app, const engine::Rect& area) const;
    void drawParty(Application& app, const engine::Rect& area) const;
    // 女角色立绘（docs/heroine-art-build.md 3.2）：队里有人有立绘时，在页面右边那一栏（partyArtColumn）画选中那位的
    // 大图（没有立绘的画像素小人），返回留给名单、说明、动作的左边那一块；谁都没有立绘时什么也不画、原样返回 area。
    [[nodiscard]] engine::Rect drawPartyArt(Application& app, const engine::Rect& area) const;
public:
    static constexpr int kPartyArtW = 380;          // 右栏宽：高 560 的立绘取中间那块（600:924）宽 364
    static constexpr int kPartyArtBottom = 6;       // 立绘底边离面板框线
    static constexpr int kPartyFigureW = 200;       // 没有立绘的人：像素小人那一框
    static constexpr int kPartyFigureH = 280;
    static constexpr int kPartyFigureBottom = 40;
    [[nodiscard]] static engine::Rect partyArtColumn(const engine::Rect& area);
private:

    Page initial_;
    bool focusList_;
    int listRow_ = 0;
    Focus focus_ = Focus::Commands;
    ui::ListView commands_;
    ui::ListView items_;
    ui::ListView magics_;
    ui::ListView homeChoices_;
    std::vector<std::string> itemIds_;    // items_ 的第 i 行是哪件东西
    std::vector<int> itemAges_;           // 同一行的灵草年份（非灵草为 0），扣东西时按它扣
    ui::ListView itemTargets_;            // 用药：给谁用
    std::vector<int> itemTargetIndex_;    // itemTargets_ 的第 i 行：-1 主角，否则 party 下标
    std::string usingItemId_;             // 正在选「给谁用」的那件东西
    int usingItemAge_ = 0;
    std::vector<Member> members_;
    std::vector<Member> partyFigures_;   // 队伍页右栏的像素小人：GameState::party 的每一位，同次序
    std::vector<Page> pages_;             // 左栏此刻有哪几项，按画面次序
    ui::ListView party_;                  // 队伍页：队里每个人
    std::vector<std::size_t> partyIndex_; // party_ 的第 i 行是 GameState::party 的第几位
    ui::ListView partyActions_;           // 「上场 / 歇着」「赠与」
    ui::ListView gifts_;                  // 赠与：背包里送得出去的东西
    std::vector<GiftChoice> giftChoices_;
    ui::ListView roster_;                 // 名册页
    std::string saveNote_;                // 最近一次存盘的结果
    bool saveOk_ = false;
    std::unique_ptr<engine::PostFx> fx_;
    bool backdropReady_ = false;
    // 告示板正压在菜单上面。那段时间菜单只画模糊底图：告示板是半透明的，两层面板的字
    // 叠在一起谁也读不清。告示板一收，菜单重新成为顶层、update 再被调到时清掉。
    bool journalOpen_ = false;
    // 系统设置面板正压在菜单上面：同上（docs/settings.md 5.1）。
    bool settingsOpen_ = false;
    // 物品页、法术页右边那块说明（docs/ch07-foundation.md 3.3）：选中的那一样不变就不重算。
    mutable DetailCache detail_;
};

}  // namespace fanren::game
