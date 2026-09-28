#pragma once
// 主菜单（施工图 docs/octopath-overhaul.md 1.5 / 第 4 节）：行走时 Tab / Esc 打开。
//
//   左栏：状态 · 物品 · 法术 · 记事 · 存盘 · 设置 · 返回，下面一块写盘缠、日期、所在。
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
// 用词走 Wording：第 1–5 章 story.xiuxian_known 不置，钱叫「碎银」、境界说「口诀第几层」、
// 法力叫「气力」、「法术」一栏叫「法门」。面板上能出现的固有字全部由 menuStrings 列出，
// tests/UiStyleTests.cpp 拿禁词表扫它。
#include <memory>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "engine/PostFx.h"
#include "game/Scene.h"
#include "game/Wording.h"
#include "ui/Widgets.h"

namespace fanren::game {

class MenuScene : public Scene {
public:
    // 左栏的七项，次序即画面上的次序。「设置」插在「存盘」与「返回」之间（docs/settings.md 5.1）。
    enum class Page { Status, Items, Magic, Journal, Save, Settings, Back };

    // initial：打开时光标停在哪一项；focusList：物品 / 法术页直接把光标放进右栏的列表，
    // 记事页直接翻开告示板（截图口拍那几页用；游戏里按 Tab 打开时一律停在「状态」）。
    explicit MenuScene(Page initial = Page::Status, bool focusList = false);
    ~MenuScene() override;

    // 这一阶段菜单上可能出现的全部固有字（左栏、页眉、空表提示、状态页的标签、
    // 盘缠与日期的前缀）。与 render 取的是同一批 key 与同一张用词表，禁词扫描才有意义。
    [[nodiscard]] static std::vector<std::string> menuStrings(const core::GameData& data,
                                                              PanelStage stage);

    // 左栏七项的字。法术那一栏按阶段换说法。
    [[nodiscard]] static std::string pageLabel(const core::GameData& data, PanelStage stage,
                                               Page page);

    // 物品页的行：背包里除了钱（钱在左下角另写）以外的每一堆，灵草带年份。
    [[nodiscard]] static std::vector<ui::ListItem> itemRows(const core::GameData& data,
                                                            const core::GameState& state);

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;

    // 快照拍到之前要让世界层在底下画一帧（快照拍的就是它）；拍到之后只画模糊底图。
    [[nodiscard]] bool opaque() const override { return backdropReady_; }
    [[nodiscard]] std::string name() const override { return "Menu"; }

private:
    enum class Focus { Commands, Items, Magic };

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
    };

    [[nodiscard]] Page page() const;
    void rebuildLists(Application& app);
    void loadMembers(Application& app);
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
    void drawJournal(Application& app, const engine::Rect& area) const;
    void drawSave(Application& app, const engine::Rect& area) const;
    void drawSettings(Application& app, const engine::Rect& area) const;

    Page initial_;
    bool focusList_;
    Focus focus_ = Focus::Commands;
    ui::ListView commands_;
    ui::ListView items_;
    ui::ListView magics_;
    std::vector<std::string> itemIds_;    // items_ 的第 i 行是哪件东西
    std::vector<Member> members_;
    std::string saveNote_;                // 最近一次存盘的结果
    bool saveOk_ = false;
    std::unique_ptr<engine::PostFx> fx_;
    bool backdropReady_ = false;
    // 告示板正压在菜单上面。那段时间菜单只画模糊底图：告示板是半透明的，两层面板的字
    // 叠在一起谁也读不清。告示板一收，菜单重新成为顶层、update 再被调到时清掉。
    bool journalOpen_ = false;
    // 系统设置面板正压在菜单上面：同上（docs/settings.md 5.1）。
    bool settingsOpen_ = false;
};

}  // namespace fanren::game
