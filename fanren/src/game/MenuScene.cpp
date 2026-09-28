#include "game/MenuScene.h"

#include <algorithm>
#include <cstdio>
#include <utility>

#include "core/rules/Calendar.h"
#include "core/rules/Objectives.h"
#include "core/rules/Realm.h"
#include "engine/TextLayout.h"
#include "game/Application.h"
#include "game/BoardScene.h"
#include "game/CultivationScene.h"
#include "game/SettingsScene.h"
#include "game/ShopScene.h"
#include "game/SpriteAtlas.h"
#include "io/VisualLoader.h"

namespace fanren::game {
namespace {

constexpr float kW = static_cast<float>(engine::kLogicalWidth);
constexpr float kH = static_cast<float>(engine::kLogicalHeight);

// 摆位。左栏两块（七项命令、左下角的盘缠日期所在），右栏一块大的放当前页。
// 命令块的高 = 七行列表（ui::listAreaHeight(7) = 258）+ 上下各 12；加「设置」那一项时从 252 长到 282，
// 离下面的盘缠块（y = 540）还远。
constexpr engine::Rect kCommandPanel{48, 48, 300, 282};
constexpr engine::Rect kInfoPanel{48, 540, 300, 132};
constexpr engine::Rect kPagePanel{372, 48, 860, 624};
constexpr int kPageCount = 7;

// 「设置」页的摘要列哪几行（系统设置面板的行下标）：有值的那八项，按键设置、恢复默认不列。
constexpr int kSettingsSummaryRows = SettingsScene::kTextSpeed + 1;

// 右栏列表页下方留给说明的那一截。
constexpr int kDescriptionH = 150;

// 状态页一人一行：小像框 96×132，里面贴 16×24 的行走帧 ×4（64×96，施工图 1.1 的战斗尺度）。
constexpr int kMemberRowH = 188;
constexpr int kPortraitW = 96;
constexpr int kPortraitH = 132;
constexpr int kSpriteScale = 4;
// 人物表四向走路帧里「下」（面朝镜头）的下标，与 GameState::facing 同一套。
constexpr std::size_t kFacingDown = 2;

// 模糊底图上再压一层墨色：菜单的字要压得住，底下的世界只剩一个「还在那儿」的印象。
constexpr std::uint8_t kBackdropDim = 150;

// 音效 id（docs/audio.md 第 4 节）。
constexpr const char* kSfxOpen = "ui_open";
constexpr const char* kSfxClose = "ui_close";
constexpr const char* kSfxCursor = "ui_cursor";
constexpr const char* kSfxConfirm = "ui_confirm";
constexpr const char* kSfxCancel = "ui_cancel";
constexpr const char* kSfxError = "ui_error";
constexpr const char* kSfxSaved = "save";

[[nodiscard]] const char* pageKey(MenuScene::Page page, PanelStage stage) {
    // **不要加 default:**：新增一页时让编译器告警，而不是悄悄画成空标签。
    switch (page) {
        case MenuScene::Page::Status: return "ui.menu.status";
        case MenuScene::Page::Items: return "ui.menu.items";
        case MenuScene::Page::Magic:
            // 「法术」是凡人篇的禁词（第 2 章硬约束 #8）；火弹术那几页纸，第 4 章的文案
            // 自己叫它「法子」。菜单跟着阶段换一个说法。
            return stage == PanelStage::Mortal ? "ui.menu.magic.mortal" : "ui.menu.magic.immortal";
        case MenuScene::Page::Journal: return "ui.menu.journal";
        case MenuScene::Page::Save: return "ui.menu.save";
        case MenuScene::Page::Settings: return "ui.menu.settings";
        case MenuScene::Page::Back: return "ui.menu.back";
    }
    return "";
}

[[nodiscard]] const char* magicEmptyKey(PanelStage stage) {
    return stage == PanelStage::Mortal ? "ui.menu.magic.empty.mortal"
                                       : "ui.menu.magic.empty.immortal";
}

// 与修炼面板同一个写法（CultivationScene::renderStatus）：第几年几月几日。
[[nodiscard]] std::string dateText(int day) {
    const rules::Calendar calendar{day};
    return "第 " + std::to_string(calendar.year()) + " 年 " + std::to_string(calendar.monthOfYear()) +
           " 月 " + std::to_string(calendar.dayOfMonth()) + " 日";
}

[[nodiscard]] engine::RectF toRectF(const engine::Rect& r) {
    return engine::RectF{static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.w),
                         static_cast<float>(r.h)};
}

}  // namespace

MenuScene::MenuScene(Page initial, bool focusList) : initial_(initial), focusList_(focusList) {}

MenuScene::~MenuScene() = default;

std::string MenuScene::pageLabel(const core::GameData& data, PanelStage stage, Page page) {
    return data.lookupText(pageKey(page, stage));
}

std::vector<std::string> MenuScene::menuStrings(const core::GameData& data, PanelStage stage) {
    std::vector<std::string> out;
    for (int i = 0; i < kPageCount; ++i) out.push_back(pageLabel(data, stage, static_cast<Page>(i)));
    for (const char* key : {"ui.menu.items.empty", "ui.menu.journal.now", "ui.menu.journal.hint",
                            "ui.menu.save.hint", "ui.menu.save.ok", "ui.menu.save.failed",
                            "ui.menu.settings.hint", "ui.menu.keys", "ui.menu.money", "ui.menu.date",
                            "ui.menu.place", "ui.status.attack", "ui.status.defence", "ui.status.speed"}) {
        out.push_back(data.lookupText(key));
    }
    out.push_back(data.lookupText(magicEmptyKey(stage)));
    // 状态页借修炼面板的用词表说「气血 / 气力」「口诀 / 境界」，钱借 Wording 的说法：
    // 同一个词只有一个定义处，扫描也就扫到同一处。
    const CultivationLexicon& words = cultivationLexicon(stage);
    out.push_back(words.hpLabel);
    out.push_back(words.mpLabel);
    out.push_back(words.realmLabel);
    out.push_back(currencyAmount(stage, 28));
    return out;
}

std::vector<ui::ListItem> MenuScene::itemRows(const core::GameData& data,
                                              const core::GameState& state) {
    std::vector<ui::ListItem> rows;
    for (const core::BagEntry& entry : state.bag) {
        // 钱是背包里的一件东西（ShopScene 那条口径），但它的物品名是修仙界的叫法。
        // 钱在左下角按 Wording 另说一遍，这里不列。
        if (entry.itemId == kSpiritStoneItemId || entry.count <= 0) continue;
        const core::Item* item = data.findItem(entry.itemId);
        ui::ListItem row;
        row.label = item != nullptr ? item->name : entry.itemId;
        if (entry.herbAge > 0) row.label += "（" + std::to_string(entry.herbAge) + " 年）";
        row.detail = "×" + std::to_string(entry.count);
        rows.push_back(std::move(row));
    }
    return rows;
}

void MenuScene::onEnter(Application& app) {
    fx_ = std::make_unique<engine::PostFx>(app.engine());
    backdropReady_ = false;

    const PanelStage stage = wordingStage(app.state());
    std::vector<ui::ListItem> commands;
    for (int i = 0; i < kPageCount; ++i) {
        ui::ListItem row;
        row.label = pageLabel(app.data(), stage, static_cast<Page>(i));
        commands.push_back(std::move(row));
    }
    commands_.reset();
    commands_.setPageSize(kPageCount);
    commands_.setItems(std::move(commands));
    for (int i = 0; i < static_cast<int>(initial_); ++i) commands_.moveDown();

    rebuildLists(app);
    loadMembers(app);
    focus_ = Focus::Commands;
    journalOpen_ = false;
    if (focusList_ && initial_ == Page::Items && items_.hasSelectable()) focus_ = Focus::Items;
    if (focusList_ && initial_ == Page::Magic && magics_.hasSelectable()) focus_ = Focus::Magic;
    app.engine().playSfx(kSfxOpen);
}

void MenuScene::rebuildLists(Application& app) {
    const core::GameState& state = app.state();
    const PanelStage stage = wordingStage(state);
    const int rows = ui::listRowsThatFit(
        kPagePanel.h - ui::panelTitleBandHeight(true, app.theme()) - kDescriptionH, app.theme());

    itemIds_.clear();
    for (const core::BagEntry& entry : state.bag) {
        if (entry.itemId == kSpiritStoneItemId || entry.count <= 0) continue;
        itemIds_.push_back(entry.itemId);
    }
    items_.reset();
    items_.setEmptyHint(app.text("ui.menu.items.empty"));
    items_.setPageSize(rows);
    items_.setItems(itemRows(app.data(), state));

    const CultivationLexicon& words = cultivationLexicon(stage);
    std::vector<ui::ListItem> magicRows;
    for (const std::string& id : state.learnedMagics) {
        const core::Magic* magic = app.data().findMagic(id);
        ui::ListItem row;
        row.label = magic != nullptr ? magic->name : id;
        if (magic != nullptr) row.detail = words.mpLabel + std::to_string(magic->needMp);
        magicRows.push_back(std::move(row));
    }
    magics_.reset();
    magics_.setEmptyHint(app.text(magicEmptyKey(stage)));
    magics_.setPageSize(rows);
    magics_.setItems(std::move(magicRows));
}

void MenuScene::loadMembers(Application& app) {
    engine::Engine& e = app.engine();
    const core::GameData& data = app.data();
    const core::GameState& state = app.state();

    // 精灵索引：美术还没到（或无头）时就是空的，小像退回「印章」画法。
    // 与世界画面读的是同一张表、走同一条选外观的规则（game/SpriteAtlas.h）。
    SpriteIndex sprites;
    const std::vector<std::string> found = e.findAssets("art/sprites", "index.json");
    if (!found.empty()) {
        core::Result<SpriteIndex> index = io::loadSpriteIndex(found.front());
        if (index) {
            sprites = std::move(index.value);
        } else {
            std::fprintf(stderr, "[menu] 精灵索引读不了，小像退回旧画法：%s\n", index.error.c_str());
        }
    }
    const ChapterTable& chapters = app.chapterTable();
    const auto chapterDone = [&](int n) { return chapters.chapterDone(state, n); };
    const auto portraitOf = [&](Member& member) {
        const CharacterSheet* sheet = sheetForRole(sprites, member.roleId, chapterDone);
        if (sheet == nullptr) return;
        member.portrait = e.loadTexture("art/sprites/" + sheet->file);
        // 小像取「行走·下」（面朝镜头）的站立帧；四向走路帧非空由读取器把关。帧矩形都是整像素。
        const engine::RectF frame = sheetFrameRect(*sheet, sheet->walk[kFacingDown].front());
        member.portraitFrame = engine::Rect{static_cast<int>(frame.x), static_cast<int>(frame.y),
                                            static_cast<int>(frame.w), static_cast<int>(frame.h)};
    };

    members_.clear();
    Member hero;
    hero.roleId = "hanli";
    hero.name = app.speakerName("hanli");
    hero.hp = state.hp;
    hero.maxHp = state.maxHp;
    hero.mp = state.mp;
    hero.maxMp = state.maxMp;
    // 攻防取自境界，与战斗里给他的是同一个数（BattleScene 的 makeHero，技术债 G-10）。
    hero.attack = rules::realmAttack(state.realm);
    hero.defence = rules::realmDefence(state.realm);
    if (const core::RoleTemplate* role = data.findRole("hanli")) hero.speed = role->speed;
    hero.hero = true;
    portraitOf(hero);
    members_.push_back(std::move(hero));

    for (const core::PartyMember& member : state.party) {
        const core::RoleTemplate* role = data.findRole(member.roleId);
        if (!member.active || role == nullptr) continue;
        Member row;
        row.roleId = role->id;
        row.name = role->name;
        // hp < 0 是「还没打过仗、按模板满血」的记号（PartyMember）。同伴的法力不进存档，
        // 每场仗按模板满额开场，这里也就照模板写。
        row.maxHp = role->maxHp;
        row.hp = member.hp < 0 ? role->maxHp : member.hp;
        row.maxMp = role->maxMp;
        row.mp = role->maxMp;
        row.attack = role->attack;
        row.defence = role->defence;
        row.speed = role->speed;
        portraitOf(row);
        members_.push_back(std::move(row));
    }
}

MenuScene::Page MenuScene::page() const {
    const int index = std::clamp(commands_.selection(), 0, kPageCount - 1);
    return static_cast<Page>(index);
}

void MenuScene::close(Application& app) {
    app.engine().playSfx(kSfxClose);
}

bool MenuScene::activate(Application& app) {
    engine::Engine& e = app.engine();
    switch (page()) {
        case Page::Status:
            e.playSfx(kSfxConfirm);
            return true;
        case Page::Items:
        case Page::Magic: {
            const bool items = page() == Page::Items;
            const ui::ListView& list = items ? items_ : magics_;
            if (!list.hasSelectable()) {
                e.playSfx(kSfxError);
                return true;
            }
            e.playSfx(kSfxConfirm);
            focus_ = items ? Focus::Items : Focus::Magic;
            return true;
        }
        case Page::Journal:
            e.playSfx(kSfxConfirm);
            app.pushScene(std::make_unique<BoardScene>());
            journalOpen_ = true;
            return true;
        case Page::Save: {
            // 与 F5、地图上的存档点同一个入口，成败都如实说一句。
            auto saved = app.quickSave();
            saveOk_ = saved.ok;
            saveNote_ = saved ? app.text("ui.menu.save.ok")
                              : app.text("ui.menu.save.failed") + saved.error;
            e.playSfx(saved ? kSfxSaved : kSfxError);
            return true;
        }
        case Page::Settings:
            // 与标题画面的「设置」同一个面板；菜单照「记事」的做法让开（只画模糊底图）。
            e.playSfx(kSfxConfirm);
            app.pushScene(std::make_unique<SettingsScene>());
            settingsOpen_ = true;
            return true;
        case Page::Back:
            close(app);
            return false;
    }
    return true;
}

bool MenuScene::update(Application& app, double) {
    engine::Engine& e = app.engine();
    // 能走到这里，说明菜单又是最上面那一层了：告示板、设置面板若开过，此刻已经收了。
    journalOpen_ = false;
    settingsOpen_ = false;
    // 截图口要「直接翻开记事」：放在第一次 update 里压，不在 onEnter 里压——onEnter 是
    // Application 正在遍历待压场景表的时候调的，那时再往表里塞东西会让遍历失效。
    if (focusList_ && initial_ == Page::Journal) {
        focusList_ = false;
        return activate(app);
    }
    // Tab 不论停在哪一级都是「合上菜单」：打开它的是这个键，合上它的也该是。
    if (e.keyPressed(engine::Engine::Key::Menu)) {
        close(app);
        return false;
    }

    if (focus_ == Focus::Commands) {
        const int before = commands_.selection();
        const bool confirmed = commands_.update(e);
        if (commands_.selection() != before) e.playSfx(kSfxCursor);
        if (commands_.cancelled()) {
            close(app);
            return false;
        }
        return confirmed ? activate(app) : true;
    }

    ui::ListView& list = focus_ == Focus::Items ? items_ : magics_;
    const int before = list.selection();
    list.update(e);
    if (list.selection() != before) e.playSfx(kSfxCursor);
    if (list.cancelled()) {
        // Esc 退一级：回到左栏，列表的光标一并复位（下次进来从第一件看起）。
        list.reset();
        focus_ = Focus::Commands;
        e.playSfx(kSfxCancel);
    }
    return true;
}

// ---------------------------------------------------------------------------
// 画
// ---------------------------------------------------------------------------

void MenuScene::render(Application& app) {
    drawBackdrop(app);
    if (journalOpen_ || settingsOpen_) return;
    drawCommands(app);
    drawInfo(app);
    drawPage(app);

    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const std::string keys = app.text("ui.menu.keys");
    const engine::Point size = e.measureText(keys, 16);
    e.drawText(keys, kPagePanel.x + kPagePanel.w - size.x, kPagePanel.y + kPagePanel.h + 14, 16,
               theme.paperDim, theme.bodyStyle);
}

void MenuScene::drawBackdrop(Application& app) {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const engine::RectF screen{0.f, 0.f, kW, kH};
    // 打开后的第一帧：世界层刚在底下画完，拍下来当底图。之后 opaque() 为真，世界层不再重画。
    // 拍不到（无头、建不出渲染目标）就只压一层墨色，世界层照旧在底下画着。
    if (!backdropReady_) backdropReady_ = fx_->snapshot() != engine::kInvalidTexture;
    if (backdropReady_) {
        e.drawTexture(fx_->blurredSnapshot(), engine::RectF{}, screen, engine::DrawOptions{});
    }
    e.fillRect(screen, ui::withAlpha(theme.inkDeep, kBackdropDim), engine::BlendMode::Alpha);
}

void MenuScene::drawCommands(Application& app) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    ui::drawPanel(e, kCommandPanel, std::string{}, theme);
    const engine::Rect list{kCommandPanel.x, kCommandPanel.y + 12, kCommandPanel.w,
                            kCommandPanel.h - 24};
    // 光标进了右栏的列表时，左栏只做「目录」不亮光标：两处都亮，方向键推的是哪一边就说不清了。
    if (focus_ == Focus::Commands) {
        commands_.render(e, list, theme);
    } else {
        commands_.renderPreview(e, list, theme);
    }
}

void MenuScene::drawInfo(Application& app) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const core::GameState& state = app.state();
    ui::drawPanel(e, kInfoPanel, std::string{}, theme);

    const std::string mapKey = core::mapDisplayNameKey(state.mapId);
    const std::string place = mapKey.empty() ? state.mapId : app.text(mapKey);
    const std::pair<std::string, std::string> lines[] = {
        {app.text("ui.menu.money"), currencyAmount(state, spiritStones(state))},
        {app.text("ui.menu.date"), dateText(state.day)},
        {app.text("ui.menu.place"), place},
    };
    int y = kInfoPanel.y + 18;
    for (const auto& [label, value] : lines) {
        const int x = kInfoPanel.x + theme.padding;
        e.drawText(label, x, y, 18, theme.gold, theme.bodyStyle);
        e.drawText(value, x + e.measureText(label, 18).x + 14, y - 1, 20, theme.paper,
                   theme.bodyStyle);
        y += 34;
    }
}

void MenuScene::drawPage(Application& app) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const PanelStage stage = wordingStage(app.state());
    const Page current = page();
    const std::string title = pageLabel(app.data(), stage, current);
    ui::drawPanel(e, kPagePanel, current == Page::Back ? std::string{} : title, theme);
    const engine::Rect area = ui::panelContentArea(kPagePanel, title, theme);

    switch (current) {
        case Page::Status:
            drawStatus(app, area);
            break;
        case Page::Items: {
            std::string description;
            const int index = items_.selection();
            if (focus_ == Focus::Items && index >= 0 &&
                index < static_cast<int>(itemIds_.size())) {
                if (const core::Item* item =
                        app.data().findItem(itemIds_[static_cast<std::size_t>(index)])) {
                    if (!item->descKey.empty()) description = app.text(item->descKey);
                }
            }
            drawListPage(app, area, items_, focus_ == Focus::Items, description);
            break;
        }
        case Page::Magic: {
            std::string description;
            const int index = magics_.selection();
            const std::vector<std::string>& learned = app.state().learnedMagics;
            if (focus_ == Focus::Magic && index >= 0 && index < static_cast<int>(learned.size())) {
                if (const core::Magic* magic =
                        app.data().findMagic(learned[static_cast<std::size_t>(index)])) {
                    if (!magic->descKey.empty()) description = app.text(magic->descKey);
                }
            }
            drawListPage(app, area, magics_, focus_ == Focus::Magic, description);
            break;
        }
        case Page::Journal:
            drawJournal(app, area);
            break;
        case Page::Save:
            drawSave(app, area);
            break;
        case Page::Settings:
            drawSettings(app, area);
            break;
        case Page::Back:
            break;
    }
}

void MenuScene::drawStatus(Application& app, const engine::Rect& area) const {
    const ui::Theme& theme = app.theme();
    int y = area.y + 8;
    for (std::size_t i = 0; i < members_.size(); ++i) {
        if (y + kPortraitH > area.y + area.h) break;   // 画不下的那一位不硬塞出面板
        drawMember(app, members_[i], area.x + theme.padding, y);
        if (i + 1 < members_.size()) {
            ui::drawRuleH(app.engine(), static_cast<float>(area.x + theme.padding),
                          static_cast<float>(y + kMemberRowH - 22),
                          static_cast<float>(area.w - theme.padding * 2), theme);
        }
        y += kMemberRowH;
    }
}

void MenuScene::drawMember(Application& app, const Member& member, int x, int y) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const PanelStage stage = wordingStage(app.state());
    const CultivationLexicon& words = cultivationLexicon(stage);

    drawPortrait(app, member, engine::Rect{x, y, kPortraitW, kPortraitH});

    const int textX = x + kPortraitW + 28;
    e.drawText(member.name, textX, y + 4, 28, theme.goldBright, theme.titleStyle);
    if (member.hero) {
        // 境界走 Wording：第 1–5 章说「口诀　第三层」，不说「炼气三层」。
        const int nameW = e.measureText(member.name, 28).x;
        e.drawText(words.realmLabel + realmText(stage, app.state().realm), textX + nameW + 28,
                   y + 11, 20, theme.paper, theme.bodyStyle);
    }

    constexpr int kGaugeX = 230;
    constexpr int kGaugeW = 330;
    const auto vital = [&](int row, const char* label, int current, int maximum,
                           const engine::Color& fill) {
        const int lineY = y + 50 + row * 30;
        e.drawText(label + std::to_string(current) + " / " + std::to_string(maximum), textX, lineY,
                   20, theme.paper, theme.bodyStyle);
        ui::drawGauge(e, engine::Rect{textX + kGaugeX, lineY + 6, kGaugeW, 12}, current, maximum,
                      fill, theme);
    };
    vital(0, words.hpLabel, member.hp, member.maxHp, theme.jade);
    int nextRow = 1;
    // 还没有气力的人（第 1 章的韩立是凡人）不画一根 0 / 0 的空条：那看起来像坏了。
    if (member.maxMp > 0) vital(nextRow++, words.mpLabel, member.mp, member.maxMp, theme.azure);

    const std::string stats = app.text("ui.status.attack") + " " + std::to_string(member.attack) +
                              "　" + app.text("ui.status.defence") + " " +
                              std::to_string(member.defence) + "　" + app.text("ui.status.speed") +
                              " " + std::to_string(member.speed);
    e.drawText(stats, textX, y + 50 + nextRow * 30 + 4, 20, theme.paperDim, theme.bodyStyle);
}

void MenuScene::drawPortrait(Application& app, const Member& member, const engine::Rect& box) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const engine::RectF frame = toRectF(box);
    e.drawGradient(frame, theme.inkDeep, ui::withAlpha(theme.ink, 200), engine::BlendMode::Alpha);
    ui::drawFrame(e, frame, theme.goldDim);

    if (member.portrait != engine::kInvalidTexture) {
        const engine::Rect& src = member.portraitFrame;
        const float w = static_cast<float>(src.w * kSpriteScale);
        const float h = static_cast<float>(src.h * kSpriteScale);
        e.drawTexture(member.portrait, toRectF(src),
                      engine::RectF{frame.x + (frame.w - w) / 2.f, frame.y + frame.h - h - 16.f, w,
                                    h},
                      engine::DrawOptions{});
        return;
    }
    // 没有精灵（美术还在路上、或这个角色没进索引）：一方印章，刻着姓。
    // 这是完整的第二种画法，不是占位的洋红叉——状态页照样读得出是谁。
    const std::vector<std::string> glyphs = engine::splitGraphemes(member.name);
    if (glyphs.empty()) return;
    const engine::Point size = e.measureText(glyphs.front(), 44);
    e.drawText(glyphs.front(), box.x + (box.w - size.x) / 2, box.y + (box.h - size.y) / 2, 44,
               theme.gold, theme.titleStyle);
}

void MenuScene::drawListPage(Application& app, const engine::Rect& area, const ui::ListView& list,
                             bool focused, const std::string& description) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const engine::Rect listArea{area.x, area.y, area.w, std::max(0, area.h - kDescriptionH)};
    if (focused) {
        list.render(e, listArea, theme);
    } else {
        list.renderPreview(e, listArea, theme);
    }
    const int ruleY = area.y + area.h - kDescriptionH;
    ui::drawRuleH(e, static_cast<float>(area.x + theme.padding), static_cast<float>(ruleY),
                  static_cast<float>(area.w - theme.padding * 2), theme);
    ui::drawTextBlock(e, description, engine::Rect{area.x, ruleY + 4, area.w, kDescriptionH - 4}, 20,
                      theme);
}

void MenuScene::drawJournal(Application& app, const engine::Rect& area) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const int x = area.x + theme.padding;
    int y = area.y + theme.padding;
    e.drawText(app.text("ui.menu.journal.now"), x, y, theme.bodyFontSize, theme.goldBright,
               theme.bodyStyle);
    y += 36;
    // 与告示板上半截同一个判据（rules::currentObjective）：两处说的必须是同一件事。
    if (const core::Objective* step = rules::currentObjective(app.data().objectives, app.state())) {
        e.drawText(app.text(step->textKey), x, y, theme.bodyFontSize, theme.paper, theme.bodyStyle);
        const std::string key = core::mapDisplayNameKey(step->targetMap);
        e.drawText(key.empty() ? step->targetMap : app.text(key), x, y + 32, 18, theme.paperDim,
                   theme.bodyStyle);
    }
    e.drawText(app.text("ui.menu.journal.hint"), x, area.y + area.h - 48, 18, theme.paperDim,
               theme.bodyStyle);
}

void MenuScene::drawSave(Application& app, const engine::Rect& area) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    ui::drawTextBlock(e, app.text("ui.menu.save.hint"), engine::Rect{area.x, area.y, area.w, 120},
                      theme.bodyFontSize, theme);
    if (!saveNote_.empty()) {
        e.drawText(saveNote_, area.x + theme.padding, area.y + 130, theme.bodyFontSize,
                   saveOk_ ? theme.jade : theme.cinnabar, theme.bodyStyle);
    }
}

void MenuScene::drawSettings(Application& app, const engine::Rect& area) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    ui::drawTextBlock(e, app.text("ui.menu.settings.hint"), engine::Rect{area.x, area.y, area.w, 120},
                      theme.bodyFontSize, theme);
    // 当前几项的摘要：行名与值都问设置面板（同一张表、同一套叫法），这里不另写一份。
    const int x = area.x + theme.padding;
    int y = area.y + 110;
    const io::Settings& settings = app.settings();
    for (int row = 0; row < kSettingsSummaryRows; ++row) {
        const std::string label = SettingsScene::rowLabel(app.data(), row);
        e.drawText(label, x, y, 20, theme.gold, theme.bodyStyle);
        e.drawText(SettingsScene::valueText(app.data(), settings, row), x + 150, y, 20, theme.paper,
                   theme.bodyStyle);
        y += 32;
    }
}

}  // namespace fanren::game
