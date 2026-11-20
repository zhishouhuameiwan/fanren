#include "game/MenuScene.h"

#include <algorithm>
#include <cstdio>
#include <utility>

#include "core/rules/Calendar.h"
#include "core/rules/Companions.h"
#include "core/rules/FieldItems.h"
#include "core/rules/Objectives.h"
#include "core/rules/Realm.h"
#include "core/rules/Roster.h"
#include "engine/TextLayout.h"
#include "game/Application.h"
#include "game/BoardScene.h"
#include "game/CultivationScene.h"
#include "game/DialogueScene.h"
#include "game/SettingsScene.h"
#include "game/ShopScene.h"
#include "game/SpriteAtlas.h"
#include "io/VisualLoader.h"

namespace fanren::game {
namespace {

constexpr float kW = static_cast<float>(engine::kLogicalWidth);
constexpr float kH = static_cast<float>(engine::kLogicalHeight);

// 左栏命令块容纳八行列表与上下各 12，下面的盘缠块仍从 y = 540 开始。
// 名册上有人时多出两行（队伍、名册），命令块按行高往下长（MenuScene::commandPanel），盘缠块不动。
constexpr engine::Rect kCommandPanel{48, 48, 300, 312};
constexpr int kCommandRows = 8;
constexpr engine::Rect kInfoPanel{48, 540, 300, 132};
constexpr engine::Rect kPagePanel{372, 48, 860, 624};
// Page 枚举里一共几项（menuStrings 逐项取字）。
constexpr int kPageCount = 10;

// 「设置」页的摘要列哪几行（系统设置面板的行下标）：有值的那九项（含手柄震动，docs/gamepad.md 第 8 节），
// 按键设置、恢复默认不列。
constexpr int kSettingsSummaryRows = SettingsScene::kPadRumble + 1;

// 右栏列表页下方留给说明的那一截。
constexpr int kDescriptionH = 150;

// 状态页一人一行：小像框 96×132，里面贴 16×24 的行走帧 ×4（64×96，施工图 1.1 的战斗尺度）。
// 倍数按表取（MenuScene::menuSpriteScale）：世界层倍数的 kMenuScaleNum / kMenuScaleDen，取整。
constexpr int kMemberRowH = 188;
constexpr int kPortraitW = 96;
constexpr int kPortraitH = 132;
constexpr int kMenuScaleNum = 4;
constexpr int kMenuScaleDen = 3;

// 队伍页下方「上场 / 歇着」「赠与」两行。
constexpr int kPartyActionToggle = 0;
constexpr int kPartyActionGift = 1;
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
        case MenuScene::Page::Home: return "ui.menu.home";
        case MenuScene::Page::Back: return "ui.menu.back";
        case MenuScene::Page::Party: return "ui.menu.party";
        case MenuScene::Page::Roster: return "ui.menu.roster";
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

std::string MenuScene::pageLabel(const core::GameData& data, const core::GameState& state, Page page) {
    // 物品那一栏领了储物袋之后改叫「储物袋」（O1，Wording 的 bagWord；未领时它与 ui.menu.items 同词）。
    if (page == Page::Items) return bagWord(state);
    return pageLabel(data, wordingStage(state), page);
}

std::vector<std::string> MenuScene::menuStrings(const core::GameData& data, PanelStage stage) {
    std::vector<std::string> out;
    for (int i = 0; i < kPageCount; ++i) out.push_back(pageLabel(data, stage, static_cast<Page>(i)));
    for (const char* key : {"ui.menu.items.empty", "ui.menu.journal.now", "ui.menu.journal.hint",
                            "ui.menu.save.hint", "ui.menu.save.ok", "ui.menu.save.failed",
                            "ui.menu.settings.hint", "ui.menu.home.warning", "ui.menu.home.cancel",
                            "ui.menu.keys", "ui.menu.money", "ui.menu.date",
                            "ui.menu.place", "ui.status.attack", "ui.status.defence", "ui.status.speed",
                            // 伙伴改版：队伍、名册两页与赠与的几句话
                            "ui.menu.party.hint", "ui.menu.party.on", "ui.menu.party.off",
                            "ui.menu.party.fixed", "ui.menu.party.toggle", "ui.menu.party.gift",
                            "ui.menu.party.gift.hint", "ui.menu.party.gift.empty", "ui.menu.party.empty",
                            "ui.menu.gift.liked", "ui.menu.gift.plain", "ui.menu.gift.disliked",
                            "ui.menu.gift.already", "ui.menu.roster.hint", "ui.bond.tier1", "ui.bond.tier2",
                            "ui.bond.tier3", "ui.bond.tier4", "ui.bond.tier5"}) {
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

int MenuScene::menuSpriteScale(int sheetScale) {
    return std::max(1, sheetScale * kMenuScaleNum / kMenuScaleDen);
}

MenuScene::PortraitPlacement MenuScene::portraitPlacement(const engine::Rect& box, const engine::Rect& frame,
                                                          int scale, int footY) {
    const float k = static_cast<float>(std::max(1, scale));
    const float w = static_cast<float>(frame.w) * k;
    const float h = static_cast<float>(frame.h) * k;
    const float boxX = static_cast<float>(box.x);
    const float boxY = static_cast<float>(box.y);
    const float boxR = boxX + static_cast<float>(box.w);
    const float boxB = boxY + static_cast<float>(box.h);
    // 改版前是「帧底边贴在框底往上 16」；旧表的脚底都在末行，按脚底对齐与它是同一个位置。
    const float x = boxX + (static_cast<float>(box.w) - w) / 2.f;
    const float y = boxB - static_cast<float>(kPortraitFootGap) - static_cast<float>(footY + 1) * k;
    const float x0 = std::max(x, boxX);
    const float y0 = std::max(y, boxY - static_cast<float>(kPortraitHeadroom));
    const float x1 = std::min(x + w, boxR);
    const float y1 = std::min(y + h, boxB);
    PortraitPlacement out;
    if (x1 <= x0 || y1 <= y0) return out;
    out.dst = engine::RectF{x0, y0, x1 - x0, y1 - y0};
    out.src = engine::RectF{static_cast<float>(frame.x) + (x0 - x) / k, static_cast<float>(frame.y) + (y0 - y) / k,
                            (x1 - x0) / k, (y1 - y0) / k};
    return out;
}

void MenuScene::onEnter(Application& app) {
    fx_ = std::make_unique<engine::PostFx>(app.engine());
    backdropReady_ = false;

    // 左栏有哪几项：前八项照旧；名册上有人才在「状态」后面插进队伍、名册（文件头）。
    pages_ = {Page::Status, Page::Items, Page::Magic, Page::Journal,
              Page::Save,   Page::Settings, Page::Home, Page::Back};
    if (!rules::roster(app.data().companions, app.state()).empty()) {
        pages_.insert(pages_.begin() + 1, {Page::Party, Page::Roster});
    }
    std::vector<ui::ListItem> commands;
    for (const Page entry : pages_) {
        ui::ListItem row;
        row.label = pageLabel(app.data(), app.state(), entry);
        commands.push_back(std::move(row));
    }
    commands_.reset();
    commands_.setPageSize(static_cast<int>(pages_.size()));
    commands_.setItems(std::move(commands));
    // 要停的那一项此刻不在左栏上（名册空着却要队伍页）：停在「状态」。
    const auto at = std::find(pages_.begin(), pages_.end(), initial_);
    if (at == pages_.end()) initial_ = Page::Status;
    for (auto it = pages_.begin(); it != pages_.end() && *it != initial_; ++it) commands_.moveDown();

    homeChoices_.setPageSize(2);
    homeChoices_.setItems({{app.text("ui.menu.home.cancel")}, {app.text("ui.menu.home")}});
    homeChoices_.reset();

    rebuildLists(app);
    rebuildCompanionLists(app);
    loadMembers(app);
    focus_ = Focus::Commands;
    journalOpen_ = false;
    if (focusList_ && initial_ == Page::Items && items_.hasSelectable()) focus_ = Focus::Items;
    if (focusList_ && initial_ == Page::Magic && magics_.hasSelectable()) focus_ = Focus::Magic;
    if (focusList_ && initial_ == Page::Home) focus_ = Focus::Home;
    if (focusList_ && initial_ == Page::Party && party_.hasSelectable()) focus_ = Focus::Party;
    app.engine().playSfx(kSfxOpen);
}

void MenuScene::rebuildCompanionLists(Application& app) {
    const core::GameData& data = app.data();
    const core::GameState& state = app.state();
    const int rows = ui::listRowsThatFit(
        kPagePanel.h - ui::panelTitleBandHeight(true, app.theme()) - kDescriptionH, app.theme());

    // 队伍：GameState::party 里的每一位（歇着的也列，不然改回上场无从下手）。
    std::vector<ui::ListItem> partyRows;
    partyIndex_.clear();
    for (std::size_t i = 0; i < state.party.size(); ++i) {
        const core::PartyMember& member = state.party[i];
        ui::ListItem row;
        const core::RoleTemplate* role = data.findRole(member.roleId);
        row.label = role != nullptr ? role->name : member.roleId;
        if (rules::companionOfRole(data.companions, member.roleId).empty()) {
            row.detail = app.text("ui.menu.party.fixed");
        } else {
            // 口径差（集成时核出来的，先不改）：这里只看 active，第 4 位起仍是 active 的伙伴也写「上场」；
            // 按契约 3.1 她不上场——战斗只带 active 的前 kMaxCompanionsOnField（3）位（BattleScene 的
            // fieldedPartyRoles）。P1 的名册不超过 3 人，这一行不会说错；名册超过 3 人的那一章（后备换人，P3）
            // 要改成问 fieldedPartyRoles，或写成「候补」。
            row.detail = app.text(member.active ? "ui.menu.party.on" : "ui.menu.party.off");
        }
        partyRows.push_back(std::move(row));
        partyIndex_.push_back(i);
    }
    const int partySelection = party_.selection();
    party_.reset();
    party_.setEmptyHint(app.text("ui.menu.party.empty"));
    party_.setPageSize(rows);
    party_.setItems(std::move(partyRows));
    for (int i = 0; i < partySelection; ++i) party_.moveDown();   // 改完一位，光标还停在她那一行

    partyActions_.setPageSize(2);
    partyActions_.setItems({{app.text("ui.menu.party.toggle")}, {app.text("ui.menu.party.gift")}});

    // 赠与：背包里除了钱、剧情物以外的每一堆。剧情物（ItemKind::Story）送出去，等着收它的那段戏就演不下去。
    std::vector<ui::ListItem> giftRows;
    giftChoices_.clear();
    std::vector<ui::ListItem> bagRows = itemRows(data, state);
    std::size_t bagRow = 0;
    for (const core::BagEntry& entry : state.bag) {
        if (entry.itemId == kSpiritStoneItemId || entry.count <= 0) continue;
        const ui::ListItem& row = bagRows[bagRow++];   // itemRows 与这里同一道过滤、同一个次序
        const core::Item* item = data.findItem(entry.itemId);
        if (item == nullptr || item->kind == core::ItemKind::Story) continue;
        giftRows.push_back(row);
        giftChoices_.push_back(GiftChoice{entry.itemId, entry.herbAge});
    }
    gifts_.reset();
    gifts_.setEmptyHint(app.text("ui.menu.party.gift.empty"));
    gifts_.setPageSize(rows);
    gifts_.setItems(std::move(giftRows));

    roster_.reset();
    roster_.setPageSize(rows);
    roster_.setItems(rosterRows(data, state));
}

std::vector<ui::ListItem> MenuScene::rosterRows(const core::GameData& data, const core::GameState& state) {
    std::vector<ui::ListItem> rows;
    for (const std::string& id : rules::roster(data.companions, state)) {
        // 名字取此刻的那个角色（问名之前是「白衣女子」，之后是「南宫婉」）。
        const std::string role = rules::currentRole(data.companions, id, state);
        const core::RoleTemplate* tmpl = role.empty() ? nullptr : data.findRole(role);
        ui::ListItem row;
        row.label = tmpl != nullptr ? tmpl->name : id;
        row.detail = data.lookupText("ui.bond.tier" + std::to_string(rules::bondTier(state.bondOf(id))));
        rows.push_back(std::move(row));
    }
    return rows;
}

core::PartyMember* MenuScene::selectedMember(Application& app) const {
    const int index = party_.selection();
    if (index < 0 || index >= static_cast<int>(partyIndex_.size())) return nullptr;
    std::vector<core::PartyMember>& party = app.state().party;
    const std::size_t at = partyIndex_[static_cast<std::size_t>(index)];
    return at < party.size() ? &party[at] : nullptr;
}

std::string MenuScene::selectedCompanion(Application& app) const {
    const core::PartyMember* member = selectedMember(app);
    return member == nullptr ? std::string{} : rules::companionOfRole(app.data().companions, member->roleId);
}

void MenuScene::rebuildLists(Application& app) {
    const core::GameState& state = app.state();
    const PanelStage stage = wordingStage(state);
    const int rows = ui::listRowsThatFit(
        kPagePanel.h - ui::panelTitleBandHeight(true, app.theme()) - kDescriptionH, app.theme());

    itemIds_.clear();
    itemAges_.clear();
    for (const core::BagEntry& entry : state.bag) {
        if (entry.itemId == kSpiritStoneItemId || entry.count <= 0) continue;
        itemIds_.push_back(entry.itemId);
        itemAges_.push_back(entry.herbAge);
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
        if (magic != nullptr) {
            if (stage == PanelStage::Immortal &&
                rules::toValue(magic->needRealm) > rules::toValue(state.realm)) {
                row.detail = "境界不足";
            } else {
                row.detail = words.mpLabel + std::to_string(magic->needMp);
            }
        }
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
        member.portraitScale = menuSpriteScale(sheet->scale);
        member.portraitFootY = sheet->footY;
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
    if (pages_.empty()) return Page::Status;
    const int index = std::clamp(commands_.selection(), 0, static_cast<int>(pages_.size()) - 1);
    return pages_[static_cast<std::size_t>(index)];
}

engine::Rect MenuScene::commandPanel(const ui::Theme& theme) const {
    // 八行时就是 kCommandPanel 原样（逐像素不变）；多一行，往下长一行的高度。
    const int rows = std::max(kCommandRows, static_cast<int>(pages_.size()));
    engine::Rect panel = kCommandPanel;
    panel.h += ui::listAreaHeight(rows, theme) - ui::listAreaHeight(kCommandRows, theme);
    return panel;
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
        case Page::Home:
            e.playSfx(kSfxConfirm);
            homeChoices_.reset();
            focus_ = Focus::Home;
            return true;
        case Page::Back:
            close(app);
            return false;
        case Page::Party:
            if (!party_.hasSelectable()) {
                e.playSfx(kSfxError);
                return true;
            }
            e.playSfx(kSfxConfirm);
            focus_ = Focus::Party;
            return true;
        case Page::Roster:
            // 只读的一页，与「状态」一样：确认只响一声。
            e.playSfx(kSfxConfirm);
            return true;
    }
    return true;
}

void MenuScene::updateParty(Application& app, bool confirmed) {
    engine::Engine& e = app.engine();
    if (party_.cancelled()) {
        party_.reset();
        focus_ = Focus::Commands;
        e.playSfx(kSfxCancel);
        return;
    }
    if (!confirmed) return;
    // 不是伙伴的队员（傀儡、化身）照老规矩全部上场（契约 3.1），也不收礼：确认只响一声错。
    if (selectedCompanion(app).empty()) {
        e.playSfx(kSfxError);
        return;
    }
    e.playSfx(kSfxConfirm);
    partyActions_.reset();
    focus_ = Focus::PartyActions;
}

void MenuScene::updatePartyActions(Application& app, bool confirmed) {
    engine::Engine& e = app.engine();
    if (partyActions_.cancelled()) {
        partyActions_.reset();
        focus_ = Focus::Party;
        e.playSfx(kSfxCancel);
        return;
    }
    if (!confirmed) return;
    if (partyActions_.selection() == kPartyActionToggle) {
        core::PartyMember* member = selectedMember(app);
        if (member == nullptr || selectedCompanion(app).empty()) {
            e.playSfx(kSfxError);
            focus_ = Focus::Party;
            return;
        }
        member->active = !member->active;
        e.playSfx(kSfxConfirm);
        rebuildCompanionLists(app);
        loadMembers(app);   // 状态页只列上场的人
        focus_ = Focus::Party;
        return;
    }
    if (partyActions_.selection() == kPartyActionGift) {
        e.playSfx(kSfxConfirm);
        gifts_.reset();
        focus_ = Focus::Gift;
    }
}

void MenuScene::giveSelected(Application& app) {
    engine::Engine& e = app.engine();
    const std::string companionId = selectedCompanion(app);
    const int index = gifts_.selection();
    const auto found = app.data().companions.find(companionId);
    if (found == app.data().companions.end() || index < 0 || index >= static_cast<int>(giftChoices_.size())) {
        e.playSfx(kSfxError);
        return;
    }
    const GiftChoice choice = giftChoices_[static_cast<std::size_t>(index)];
    core::GameState& state = app.state();
    // 次序：先核背包里还有没有，再记账，再扣——不会出现记了账却没扣成的半截。
    // 列表就是从背包这一刻拼出来的，没有了只能是别处动了背包：响一声错、重拼列表，账一笔不动。
    if (state.itemCountOfAge(choice.itemId, choice.herbAge) < 1) {
        e.playSfx(kSfxError);
        rebuildLists(app);
        rebuildCompanionLists(app);
        return;
    }
    // 今天送过了就一样不动：账不记、东西不扣、说一句（契约 2.2「每人每天一次」，记账在 rules::giveGift）。
    const rules::GiftOutcome outcome =
        rules::giveGift(state.bonds[companionId], found->second, choice.itemId, state.day);
    if (!outcome.given) {
        e.playSfx(kSfxError);
        app.showMessage("ui.menu.gift.already");
        focus_ = Focus::Party;
        return;
    }
    (void)state.removeItemOfAge(choice.itemId, 1, choice.herbAge);   // 上面核过数量，扣得成
    e.playSfx(kSfxConfirm);
    switch (outcome.taste) {
        case rules::GiftTaste::Liked: app.showMessage("ui.menu.gift.liked"); break;
        case rules::GiftTaste::Plain: app.showMessage("ui.menu.gift.plain"); break;
        case rules::GiftTaste::Disliked: app.showMessage("ui.menu.gift.disliked"); break;
    }
    rebuildLists(app);
    rebuildCompanionLists(app);
    focus_ = Focus::Party;
}

// ---------------------------------------------------------------------------
// 战斗外用药
// ---------------------------------------------------------------------------

namespace {

// 一句旁白（空说话人、进对话记录），与 WorldScene 开宝箱那一句同一个样子。
void narrateLine(Application& app, const std::string& body) {
    app.pushDialogueLine(std::string{}, body);
    app.pushScene(std::make_unique<DialogueScene>(std::string{}, body));
}

std::string fillSlot(std::string text, const std::string& slot, const std::string& value) {
    for (std::size_t at = text.find(slot); at != std::string::npos; at = text.find(slot, at + value.size())) {
        text.replace(at, slot.size(), value);
    }
    return text;
}

}  // namespace

void MenuScene::rebuildItemTargets(Application& app) {
    const core::GameData& data = app.data();
    const core::GameState& state = app.state();
    std::vector<ui::ListItem> rows;
    itemTargetIndex_.clear();
    const auto vitals = [&app](int hp, int maxHp, int mp, int maxMp) {
        std::string text = app.text("ui.menu.items.target_hp");
        text = fillSlot(text, "{hp}", std::to_string(hp));
        text = fillSlot(text, "{max}", std::to_string(maxHp));
        if (maxMp > 0) {
            std::string m = app.text("ui.menu.items.target_mp");
            m = fillSlot(m, "{mp}", std::to_string(mp));
            m = fillSlot(m, "{max}", std::to_string(maxMp));
            text += "  " + m;
        }
        return text;
    };
    ui::ListItem hero;
    hero.label = app.speakerName("hanli");
    hero.detail = vitals(state.hp, state.maxHp, state.mp, state.maxMp);
    rows.push_back(std::move(hero));
    itemTargetIndex_.push_back(-1);
    // 队里的每一位（歇着的也列：养伤不分上不上场）。同伴的法力不记账，只列气血。
    for (std::size_t i = 0; i < state.party.size(); ++i) {
        const core::PartyMember& member = state.party[i];
        const core::RoleTemplate* role = data.findRole(member.roleId);
        if (role == nullptr) continue;
        ui::ListItem row;
        row.label = role->name;
        row.detail = vitals(member.hp < 0 ? role->maxHp : member.hp, role->maxHp, 0, 0);
        rows.push_back(std::move(row));
        itemTargetIndex_.push_back(static_cast<int>(i));
    }
    const int keep = itemTargets_.selection();
    itemTargets_.reset();
    itemTargets_.setPageSize(std::max<int>(1, static_cast<int>(rows.size())));
    itemTargets_.setItems(std::move(rows));
    for (int i = 0; i < keep; ++i) itemTargets_.moveDown();
}

void MenuScene::useSelectedItem(Application& app) {
    engine::Engine& e = app.engine();
    const int index = items_.selection();
    if (index < 0 || index >= static_cast<int>(itemIds_.size())) {
        e.playSfx(kSfxError);
        return;
    }
    const core::Item* item = app.data().findItem(itemIds_[static_cast<std::size_t>(index)]);
    if (item == nullptr || !rules::fieldUsable(*item)) {
        // 不是不能点，是此刻用不上：说一句，不然玩家只会觉得按键坏了。
        e.playSfx(kSfxError);
        app.showMessage("ui.menu.items.cannot_use");
        return;
    }
    usingItemId_ = item->id;
    usingItemAge_ = itemAges_[static_cast<std::size_t>(index)];
    // 只有主角一个人时不必再问给谁。
    if (app.state().party.empty()) {
        applyItemTo(app, -1);
        return;
    }
    e.playSfx(kSfxConfirm);
    itemTargets_.reset();
    rebuildItemTargets(app);
    focus_ = Focus::ItemTarget;
}

void MenuScene::applyItemTo(Application& app, int target) {
    engine::Engine& e = app.engine();
    core::GameState& state = app.state();
    const core::Item* item = app.data().findItem(usingItemId_);
    // 列表是打开时拼的；别处动过背包（极少见）就重拼、不扣。
    if (item == nullptr || state.itemCountOfAge(usingItemId_, usingItemAge_) < 1) {
        e.playSfx(kSfxError);
        rebuildLists(app);
        rebuildCompanionLists(app);
        itemTargets_.reset();
        focus_ = Focus::Items;
        return;
    }
    std::string name;
    rules::FieldGain gain;
    core::PartyMember* member = nullptr;
    const core::RoleTemplate* role = nullptr;
    if (target < 0) {
        name = app.speakerName("hanli");
        gain = rules::fieldGain(*item, state.hp, state.maxHp, state.mp, state.maxMp);
    } else if (target < static_cast<int>(state.party.size())) {
        member = &state.party[static_cast<std::size_t>(target)];
        role = app.data().findRole(member->roleId);
        if (role != nullptr) {
            name = role->name;
            gain = rules::fieldGain(*item, member->hp < 0 ? role->maxHp : member->hp, role->maxHp, 0, 0);
        }
    }
    if (!gain.any()) {
        // 满着就不扣：白白吃掉一颗药比按了没反应更糟。
        e.playSfx(kSfxError);
        app.showMessage("ui.menu.items.full");
        return;
    }
    (void)state.removeItemOfAge(usingItemId_, 1, usingItemAge_);   // 上面核过数量，扣得成
    if (member != nullptr) {
        member->hp = (member->hp < 0 ? role->maxHp : member->hp) + gain.hp;
    } else {
        state.hp += gain.hp;
        state.mp += gain.mp;
    }
    e.playSfx(kSfxConfirm);
    std::string line = app.text(gain.hp > 0 && gain.mp > 0 ? "ui.menu.items.used_both"
                                : gain.hp > 0             ? "ui.menu.items.used_hp"
                                                          : "ui.menu.items.used_mp");
    line = fillSlot(line, "{name}", name);
    line = fillSlot(line, "{item}", item->name);
    line = fillSlot(line, "{hp}", std::to_string(gain.hp));
    line = fillSlot(line, "{mp}", std::to_string(gain.mp));
    narrateLine(app, line);
    const bool picking = focus_ == Focus::ItemTarget;
    rebuildLists(app);
    rebuildCompanionLists(app);
    loadMembers(app);   // 状态页的血条跟着变
    // 还剩、而且是从「给谁用」进来的，就停在那一层，好接着喂；用完了回物品列表。
    if (picking && state.itemCountOfAge(usingItemId_, usingItemAge_) > 0) {
        rebuildItemTargets(app);
    } else {
        itemTargets_.reset();
        focus_ = Focus::Items;
    }
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

    ui::ListView& list = focus_ == Focus::Home           ? homeChoices_
                         : focus_ == Focus::Items        ? items_
                         : focus_ == Focus::Party        ? party_
                         : focus_ == Focus::PartyActions ? partyActions_
                         : focus_ == Focus::Gift         ? gifts_
                         : focus_ == Focus::ItemTarget   ? itemTargets_
                                                         : magics_;
    const int before = list.selection();
    const bool confirmed = list.update(e);
    if (list.selection() != before) e.playSfx(kSfxCursor);
    if (focus_ == Focus::Party) {
        updateParty(app, confirmed);
        return true;
    }
    if (focus_ == Focus::PartyActions) {
        updatePartyActions(app, confirmed);
        return true;
    }
    if (focus_ == Focus::ItemTarget) {
        if (itemTargets_.cancelled()) {
            itemTargets_.reset();
            focus_ = Focus::Items;
            e.playSfx(kSfxCancel);
        } else if (confirmed) {
            const int row = itemTargets_.selection();
            if (row >= 0 && row < static_cast<int>(itemTargetIndex_.size())) {
                applyItemTo(app, itemTargetIndex_[static_cast<std::size_t>(row)]);
            } else {
                e.playSfx(kSfxError);
            }
        }
        return true;
    }
    if (focus_ == Focus::Items && confirmed && !list.cancelled()) {
        useSelectedItem(app);
        return true;
    }
    if (focus_ == Focus::Gift) {
        if (gifts_.cancelled()) {
            gifts_.reset();
            focus_ = Focus::PartyActions;
            e.playSfx(kSfxCancel);
        } else if (confirmed) {
            giveSelected(app);
        }
        return true;
    }
    if (list.cancelled()) {
        // Esc 退一级：回到左栏，列表的光标一并复位（下次进来从第一件看起）。
        list.reset();
        focus_ = Focus::Commands;
        e.playSfx(kSfxCancel);
    } else if (focus_ == Focus::Home && confirmed) {
        if (list.selection() == 1) {
            e.playSfx(kSfxConfirm);
            app.returnToTitle();
        } else {
            focus_ = Focus::Commands;
            e.playSfx(kSfxCancel);
        }
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
    const engine::Rect panel = commandPanel(theme);
    ui::drawPanel(e, panel, std::string{}, theme);
    const engine::Rect list{panel.x, panel.y + 12, panel.w, panel.h - 24};
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
    const Page current = page();
    const std::string title = pageLabel(app.data(), app.state(), current);
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
                    if (rules::fieldUsable(*item)) {
                        if (!description.empty()) description += "\n";
                        description += app.text("ui.menu.items.use_hint");
                    }
                }
            }
            if (focus_ == Focus::ItemTarget) {
                drawListPage(app, area, itemTargets_, true, app.text("ui.menu.items.target_hint"));
                break;
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
        case Page::Home:
            drawHome(app, area);
            break;
        case Page::Back:
            break;
        case Page::Party:
            if (focus_ == Focus::Gift) {
                drawListPage(app, area, gifts_, true, app.text("ui.menu.party.gift.hint"));
            } else {
                drawParty(app, area);
            }
            break;
        case Page::Roster:
            drawListPage(app, area, roster_, false, app.text("ui.menu.roster.hint"));
            break;
    }
}

void MenuScene::drawParty(Application& app, const engine::Rect& area) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    if (focus_ != Focus::PartyActions) {
        drawListPage(app, area, party_, focus_ == Focus::Party, app.text("ui.menu.party.hint"));
        return;
    }
    // 选了一位：上面的队伍停在她那一行，下面说明区换成两项动作。
    const engine::Rect listArea{area.x, area.y, area.w, std::max(0, area.h - kDescriptionH)};
    party_.render(e, listArea, theme);
    const int ruleY = area.y + area.h - kDescriptionH;
    ui::drawRuleH(e, static_cast<float>(area.x + theme.padding), static_cast<float>(ruleY),
                  static_cast<float>(area.w - theme.padding * 2), theme);
    partyActions_.render(e, engine::Rect{area.x, ruleY + 4, area.w, ui::listAreaHeight(2, theme)}, theme);
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
        const PortraitPlacement at =
            portraitPlacement(box, member.portraitFrame, member.portraitScale, member.portraitFootY);
        if (at.dst.w > 0.f && at.dst.h > 0.f) e.drawTexture(member.portrait, at.src, at.dst, engine::DrawOptions{});
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
        // 目标一句可能很长（第 7 章第 2 步 68 个字）：按页宽折行，地名接在最后一行下面
        //（2026-11-20 用户试玩：「记事里面描述没有换行，导致超出界面了」）。
        const std::string text = app.text(step->textKey);
        const int width = std::max(1, area.w - theme.padding * 2);
        const std::vector<engine::LayoutLine> lines = engine::layoutText(
            text, width, [&e, &theme](const std::string& s) { return e.measureText(s, theme.bodyFontSize).x; });
        for (const engine::LayoutLine& line : lines) {
            e.drawText(line.text, x, y, theme.bodyFontSize, theme.paper, theme.bodyStyle);
            y += theme.bodyFontSize + theme.lineSpacing;
        }
        const std::string key = core::mapDisplayNameKey(step->targetMap);
        e.drawText(key.empty() ? step->targetMap : app.text(key), x, y + 4, 18, theme.paperDim, theme.bodyStyle);
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

void MenuScene::drawHome(Application& app, const engine::Rect& area) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    ui::drawTextBlock(e, app.text("ui.menu.home.warning"),
                      engine::Rect{area.x, area.y, area.w, 120}, theme.bodyFontSize, theme);
    if (focus_ == Focus::Home) {
        homeChoices_.render(e, engine::Rect{area.x, area.y + 120, area.w,
                                           ui::listAreaHeight(2, theme)}, theme);
    }
}

}  // namespace fanren::game
