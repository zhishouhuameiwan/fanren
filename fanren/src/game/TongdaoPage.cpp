#include "game/TongdaoPage.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <utility>

#include "core/rules/Companions.h"
#include "core/rules/Realm.h"
#include "core/rules/Tongdao.h"
#include "engine/TextLayout.h"
#include "game/Application.h"
#include "game/Busts.h"
#include "game/DialogueScene.h"
#include "game/MenuScene.h"
#include "game/ShopScene.h"
#include "game/SpriteAtlas.h"
#include "io/VisualLoader.h"

namespace fanren::game {
namespace {

constexpr const char* kSfxCursor = "ui_cursor";
constexpr const char* kSfxConfirm = "ui_confirm";
constexpr const char* kSfxCancel = "ui_cancel";
constexpr const char* kSfxError = "ui_error";

// 人物表四向走路帧里「下」（面朝镜头）的下标。
constexpr std::size_t kFacingDown = 2;

// 剪影：把画乘成近黑，只留轮廓。
constexpr engine::Color kSilhouetteTint{14, 16, 24, 235};

// 卡片上那块头肩：从立绘整格里裁（按格子的比例算，840×1260 与测试用的小格一样裁）。
constexpr float kCardCropLeft = 0.15f;
constexpr float kCardCropTop = 0.03f;

// 详情右栏各块的 y（离正文区顶）。施工记录第 2 节的表。
constexpr int kNameY = 8;
constexpr int kIntroY = 46;
constexpr int kCellsY = 74;
constexpr int kCellH = 32;
constexpr int kCellGap = 6;
constexpr int kProgressY = 114;
constexpr int kBlocksY = 146;
constexpr int kBlockH = 50;      // 一块：标签 + 至多两行
constexpr int kLabelW = 92;
constexpr int kInfoFont = 18;
constexpr int kInfoLineH = 23;

engine::RectF toRectF(const engine::Rect& r) {
    return engine::RectF{static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.w),
                         static_cast<float>(r.h)};
}

const core::Companion* findCompanion(Application& app, const std::string& id) {
    const auto it = app.data().companions.find(id);
    return it == app.data().companions.end() ? nullptr : &it->second;
}

// 有这条文案就取它，没有就取缺省那一条（路 N 的 tongdao.<id>.hint / .intro 可能还没写）。
std::string textOr(Application& app, const std::string& key, const std::string& fallbackKey) {
    if (app.data().text.count(key) != 0) return app.text(key);
    return fallbackKey.empty() ? std::string{} : app.text(fallbackKey);
}

std::string roleName(Application& app, const std::string& roleId) {
    const core::RoleTemplate* role = roleId.empty() ? nullptr : app.data().findRole(roleId);
    return role != nullptr ? role->name : roleId;
}

std::string magicName(Application& app, const std::string& magicId) {
    const core::Magic* magic = app.data().findMagic(magicId);
    return magic != nullptr ? magic->name : magicId;
}

std::string joinList(const std::vector<std::string>& parts, const std::string& sep = "、") {
    std::string out;
    for (const std::string& part : parts) {
        if (part.empty()) continue;
        if (!out.empty()) out += sep;
        out += part;
    }
    return out;
}

std::string tierName(Application& app, const core::Companion& companion, int tier) {
    return app.text(rules::bondTierNameKey(companion.line, tier));
}

// 此刻用的那一段角色；没遇到（currentRole 也可能给出第一段）时取第一段。
std::string roleNow(Application& app, const core::Companion& companion) {
    std::string role = rules::currentRole(app.data().companions, companion.id, app.state());
    if (role.empty() && !companion.roles.empty()) role = companion.roles.front().roleId;
    return role;
}

int tierThreshold(int tier) {
    const int clamped = std::clamp(tier, 1, 5);
    return rules::kBondTierThresholds[clamped - 1];
}

// 一行字放不下就截断、加「…」。
std::string ellipsize(engine::Engine& e, const std::string& text, int width, int size) {
    if (e.measureText(text, size).x <= width) return text;
    std::vector<std::string> glyphs = engine::splitGraphemes(text);
    std::string out;
    for (const std::string& g : glyphs) {
        if (e.measureText(out + g + "…", size).x > width) break;
        out += g;
    }
    return out + "…";
}

// 按宽度折行，至多 maxLines 行（多的截掉、末行加「…」）；返回画了几行。
int drawWrapped(engine::Engine& e, const ui::Theme& theme, const std::string& text, int x, int y, int width, int size,
                int lineH, engine::Color color, int maxLines) {
    if (text.empty() || width <= 0) return 0;
    const std::vector<engine::LayoutLine> lines =
        engine::layoutText(text, width, [&e, size](const std::string& s) { return e.measureText(s, size).x; });
    int drawn = 0;
    for (std::size_t i = 0; i < lines.size() && drawn < maxLines; ++i) {
        std::string line = lines[i].text;
        if (drawn + 1 == maxLines && i + 1 < lines.size()) line = ellipsize(e, line + "…", width, size);
        e.drawText(line, x, y + drawn * lineH, size, color, theme.bodyStyle);
        ++drawn;
    }
    return drawn;
}

// 重拼列表之后把光标挪回原来那一行（那一行灰了就停在它之后最近的一行；挪不动就停下，不绕回顶上）。
void restoreSelection(ui::ListView& list, int keep) {
    for (int guard = 0; guard < list.count() && list.selection() >= 0 && list.selection() < keep; ++guard) {
        const int before = list.selection();
        list.moveDown();
        if (list.selection() <= before) break;
    }
}

void narrate(Application& app, const std::string& body) {
    app.pushDialogueLine(std::string{}, body);
    app.pushScene(std::make_unique<DialogueScene>(std::string{}, body));
}

const char* elementKey(int element) {
    switch (element) {
        case core::kElementMetal: return "ui.tongdao.element.metal";
        case core::kElementWood: return "ui.tongdao.element.wood";
        case core::kElementWater: return "ui.tongdao.element.water";
        case core::kElementFire: return "ui.tongdao.element.fire";
        case core::kElementEarth: return "ui.tongdao.element.earth";
        default: return "";
    }
}

const char* perkKey(core::PerkKind kind) {
    // 不加 default:：种类表多一种时让编译器提醒这里也要一句说明。
    switch (kind) {
        case core::PerkKind::None: return "";
        case core::PerkKind::ChasePower: return "ui.tongdao.perk.chase_power";
        case core::PerkKind::OpeningBp: return "ui.tongdao.perk.opening_bp";
        case core::PerkKind::VictoryHeal: return "ui.tongdao.perk.victory_heal";
        case core::PerkKind::AmbushBp: return "ui.tongdao.perk.ambush_bp";
        case core::PerkKind::ElementStagger: return "ui.tongdao.perk.element_stagger";
        case core::PerkKind::TalismanPower: return "ui.tongdao.perk.talisman_power";
    }
    return "";
}

}  // namespace

// ---------------------------------------------------------------------------
// 纯查询
// ---------------------------------------------------------------------------

std::string tongdaoXuhuaScript(const std::string& companionId, int tier) {
    return "ch07/tongdao_" + companionId + "_t" + std::to_string(tier) + ".lua";
}

std::string tongdaoQingjiaoScript(const std::string& companionId) {
    return "ch07/tongdao_" + companionId + "_qingjiao.lua";
}

bool tongdaoScriptExists(Application& app, const std::string& relative) {
    std::error_code ignored;
    return std::filesystem::is_regular_file(std::filesystem::path(app.assetRoot()) / "scripts" / relative, ignored);
}

std::string tongdaoFill(std::string text, const std::string& slot, const std::string& value) {
    for (std::size_t at = text.find(slot); at != std::string::npos; at = text.find(slot, at + value.size())) {
        text.replace(at, slot.size(), value);
    }
    return text;
}

std::string perkText(const core::GameData& data, const core::CompanionPerk& perk, int value) {
    const char* key = perkKey(perk.kind);
    if (key[0] == '\0') return {};
    std::string text = tongdaoFill(data.lookupText(key), "{v}", std::to_string(value));
    if (const char* element = elementKey(perk.element); element[0] != '\0') {
        text = tongdaoFill(text, "{element}", data.lookupText(element));
    }
    return text;
}

std::vector<std::string> tierGains(Application& app, const core::Companion& companion, int tier) {
    std::vector<std::string> out;
    const core::GameData& data = app.data();
    const bool active = rules::rosterEffectsActive(app.state());
    const std::string role = roleNow(app, companion);
    const core::CompanionRole* entry = rules::companionRoleEntry(companion, role);
    const auto gainMagic = [&](const std::string& id) {
        out.push_back(tongdaoFill(app.text("ui.tongdao.gain.magic"), "{name}", magicName(app, id)));
    };
    if (tier <= 1) {
        out.push_back(app.text("ui.tongdao.gain.join"));
        if (const core::RoleTemplate* tmpl = data.findRole(role)) {
            for (const std::string& id : tmpl->magics) gainMagic(id);
        }
        return out;
    }
    if (tier == 2) {
        if (companion.chase) out.push_back(app.text("ui.tongdao.gain.chase"));
        if (!companion.sparBattle.empty()) out.push_back(app.text("ui.tongdao.gain.spar"));
        out.push_back(app.text("ui.tongdao.gain.likes"));
    }
    // 按档的法术、被动、合招只在第 7 章生效（rules::rosterEffectsActive）：判据不成立时不写，免得说了不作数的话。
    if (active && entry != nullptr) {
        for (const core::TierMagic& magic : entry->tierMagics) {
            if (magic.tier == tier) gainMagic(magic.magicId);
        }
        if (entry->passive.kind != core::PerkKind::None) {
            if (tier == rules::kPassiveMinTier) {
                out.push_back(tongdaoFill(app.text("ui.tongdao.gain.passive"), "{desc}",
                                          perkText(data, entry->passive, rules::passiveValueAtTier(entry->passive, tier))));
            } else if (tier == rules::kPassiveStrongTier &&
                       rules::passiveValueAtTier(entry->passive, tier) >
                           rules::passiveValueAtTier(entry->passive, rules::kPassiveMinTier)) {
                out.push_back(tongdaoFill(app.text("ui.tongdao.gain.passive_up"), "{desc}",
                                          perkText(data, entry->passive, rules::passiveValueAtTier(entry->passive, tier))));
            }
        }
    }
    if (tier == 4) {
        const std::string duo = (active && entry != nullptr && !entry->duo.empty()) ? entry->duo : companion.duo;
        if (!duo.empty()) out.push_back(tongdaoFill(app.text("ui.tongdao.gain.duo"), "{name}", magicName(app, duo)));
    }
    if (tier == rules::kQingjiaoMinTier && companion.kind == core::CompanionKind::Person) {
        out.push_back(app.text("ui.tongdao.gain.qingjiao"));
    }
    if (companion.kind == core::CompanionKind::Person && tongdaoScriptExists(app, tongdaoXuhuaScript(companion.id, tier))) {
        out.push_back(app.text("ui.tongdao.gain.xuhua"));
    }
    return out;
}

std::vector<TongdaoEntry> tongdaoEntries(Application& app) {
    std::vector<TongdaoEntry> out;
    const core::GameState& state = app.state();
    for (const auto& [id, companion] : app.data().companions) {
        TongdaoEntry entry;
        entry.id = id;
        entry.met = rules::inRoster(companion, state);
        entry.person = companion.kind == core::CompanionKind::Person;
        entry.roleId = roleNow(app, companion);
        entry.bond = state.bondOf(id);
        entry.tier = rules::bondTier(entry.bond);
        entry.capTier = rules::bondTierCap(companion, state);
        entry.tierName = tierName(app, companion, entry.tier);
        if (entry.met) {
            entry.name = roleName(app, entry.roleId);
            const std::string intro = "tongdao." + id + ".intro";
            entry.introIsBond = app.data().text.count(intro) == 0;
            entry.intro = !entry.introIsBond
                              ? app.text(intro)
                              : tongdaoFill(app.text("ui.tongdao.bond"), "{value}", std::to_string(entry.bond));
            const int pending = entry.person ? rules::xuhuaPending(app.data().companions, id, state) : 0;
            entry.xuhuaReady = pending > 0 && tongdaoScriptExists(app, tongdaoXuhuaScript(id, pending));
        } else {
            entry.name = app.text("ui.tongdao.unknown");
            entry.intro = textOr(app, "tongdao." + id + ".hint", "ui.tongdao.hint.default");
        }
        out.push_back(std::move(entry));
    }
    return out;
}

namespace {

// 相处那四样共用的前两道闸：地点、人物。返回空串 = 都过了。
std::string placeOrKind(Application& app, const core::Companion& companion) {
    if (app.state().mapId != kTongdaoMap) return app.text("ui.tongdao.why.place");
    if (companion.kind != core::CompanionKind::Person) return app.text("ui.tongdao.why.beast");
    return {};
}

std::string checkReason(Application& app, const core::Companion& companion, rules::TongdaoCheck check, int needTier) {
    switch (check) {
        case rules::TongdaoCheck::Ok: return {};
        case rules::TongdaoCheck::TierLow:
            return tongdaoFill(app.text("ui.tongdao.why.tier"), "{tier}", tierName(app, companion, needTier));
        case rules::TongdaoCheck::DoneToday: return app.text("ui.tongdao.why.today");
        case rules::TongdaoCheck::AlreadyDone: return app.text("ui.tongdao.why.qingjiao_done");
        case rules::TongdaoCheck::NoSpar: return app.text("ui.tongdao.why.no_spar");
        case rules::TongdaoCheck::NotPerson: return app.text("ui.tongdao.why.beast");
        case rules::TongdaoCheck::NoCompanion:
        case rules::TongdaoCheck::NotMet: return app.text("ui.tongdao.unknown");
    }
    return {};
}

// 送得出手的东西：背包里除了钱、剧情物以外的每一堆（与队伍页的赠与同一道过滤）。
bool anyGiftable(Application& app) {
    for (const core::BagEntry& entry : app.state().bag) {
        if (entry.itemId == kSpiritStoneItemId || entry.count <= 0) continue;
        const core::Item* item = app.data().findItem(entry.itemId);
        if (item != nullptr && item->kind != core::ItemKind::Story) return true;
    }
    return false;
}

}  // namespace

std::vector<ui::ListItem> tongdaoActionRows(Application& app, const std::string& companionId) {
    std::vector<ui::ListItem> rows(kTongdaoActionCount);
    rows[0].label = app.text("ui.tongdao.act.gift");
    rows[1].label = app.text("ui.tongdao.act.spar");
    rows[2].label = app.text("ui.tongdao.act.qingjiao");
    rows[3].label = app.text("ui.tongdao.act.xuhua");
    rows[4].label = app.text("ui.tongdao.act.bonus");
    const core::Companion* companion = findCompanion(app, companionId);
    const core::GameState& state = app.state();
    const auto refuse = [](ui::ListItem& row, std::string why) {
        if (why.empty()) return;
        row.enabled = false;
        row.disabledReason = std::move(why);
    };
    if (companion == nullptr || !rules::inRoster(*companion, state)) {
        for (ui::ListItem& row : rows) refuse(row, app.text("ui.tongdao.unknown"));
        return rows;
    }
    const auto& companions = app.data().companions;
    const std::string gate = placeOrKind(app, *companion);

    // 赠礼：地点、人物；今天送过了；身上没有送得出手的。
    if (!gate.empty()) {
        refuse(rows[0], gate);
    } else if (const auto bond = state.bonds.find(companionId); bond != state.bonds.end() && bond->second.giftDay == state.day) {
        refuse(rows[0], app.text("ui.tongdao.why.gift_today"));
    } else if (!anyGiftable(app)) {
        refuse(rows[0], app.text("ui.tongdao.why.no_gift"));
    }
    // 切磋：问规则层。
    if (!gate.empty()) {
        refuse(rows[1], gate);
    } else {
        refuse(rows[1], checkReason(app, *companion, rules::canSpar(companions, companionId, state), rules::kSparMinTier));
    }
    // 请教：规则层；脚本不在写「（内容未到）」。
    if (!gate.empty()) {
        refuse(rows[2], gate);
    } else if (const rules::TongdaoCheck check = rules::canQingjiao(companions, companionId, state);
               check != rules::TongdaoCheck::Ok) {
        refuse(rows[2], checkReason(app, *companion, check, rules::kQingjiaoMinTier));
    } else if (!tongdaoScriptExists(app, tongdaoQingjiaoScript(companionId))) {
        refuse(rows[2], app.text("ui.tongdao.why.no_content"));
    }
    // 叙话：有待演的那一档、那一档的脚本在。
    if (!gate.empty()) {
        refuse(rows[3], gate);
    } else if (rules::bondTier(state.bondOf(companionId)) < rules::kXuhuaMinTier) {
        refuse(rows[3], tongdaoFill(app.text("ui.tongdao.why.tier"), "{tier}",
                                    tierName(app, *companion, rules::kXuhuaMinTier)));
    } else if (const int pending = rules::xuhuaPending(companions, companionId, state); pending == 0) {
        refuse(rows[3], app.text("ui.tongdao.why.xuhua_none"));
    } else if (!tongdaoScriptExists(app, tongdaoXuhuaScript(companionId, pending))) {
        refuse(rows[3], app.text("ui.tongdao.why.no_content"));
    }
    // 带上在册加成：哪里都能勾（不是相处，是排兵），解锁了才能勾。
    rows[4].detail = app.text(state.flag(rules::bonusOnFlag(companionId)) != 0 ? "ui.tongdao.act.bonus.on"
                                                                                : "ui.tongdao.act.bonus.off");
    if (companion->rosterBonus.kind == core::PerkKind::None) {
        refuse(rows[4], app.text("ui.tongdao.why.no_bonus"));
    } else if (!rules::rosterBonusUnlocked(*companion, state)) {
        refuse(rows[4], app.text("ui.tongdao.why.bonus_locked"));
    }
    return rows;
}

// ---------------------------------------------------------------------------
// 版面
// ---------------------------------------------------------------------------

engine::Rect TongdaoPage::cardRect(const engine::Rect& area, int slot) {
    const int col = slot % kCols;
    const int row = (slot / kCols) % kRows;
    const int gridW = kCols * kCardW + (kCols - 1) * kCardGap;
    const int left = area.x + (area.w - gridW) / 2;
    return engine::Rect{left + col * (kCardW + kCardGap), area.y + kCardTop + row * (kCardH + kCardGap), kCardW,
                        kCardH};
}

engine::Rect TongdaoPage::detailArtRect(const engine::Rect& area) {
    return engine::Rect{area.x, area.y, kDetailArtW, area.h};
}

engine::Rect TongdaoPage::detailInfoRect(const engine::Rect& area) {
    const int x = area.x + kDetailArtW + 12;
    return engine::Rect{x, area.y, area.x + area.w - 12 - x, area.h};
}

engine::Rect TongdaoPage::actionListRect(const engine::Rect& area, const ui::Theme& theme) {
    const engine::Rect info = detailInfoRect(area);
    const int h = ui::listAreaHeight(kActionRows, theme);
    return engine::Rect{info.x - theme.padding / 2, area.y + area.h - h, info.w + theme.padding / 2, h};
}

engine::Rect TongdaoPage::giftListRect(const engine::Rect& area) {
    const engine::Rect info = detailInfoRect(area);
    const int top = area.y + kBlocksY + 28;
    return engine::Rect{info.x - 12, top, info.w + 12, area.y + area.h - top};
}

int TongdaoPage::pageCount() const {
    const int n = static_cast<int>(entries_.size());
    return std::max(1, (n + kPerPage - 1) / kPerPage);
}

std::string TongdaoPage::selectedId() const {
    if (cursor_ < 0 || cursor_ >= static_cast<int>(entries_.size())) return {};
    return entries_[static_cast<std::size_t>(cursor_)].id;
}

std::vector<std::string> TongdaoPage::strings(const core::GameData& data) {
    std::vector<std::string> out;
    for (const auto& [key, value] : data.text) {
        if (key.rfind("ui.tongdao.", 0) == 0) out.push_back(value);
    }
    return out;
}

// ---------------------------------------------------------------------------
// 状态
// ---------------------------------------------------------------------------

void TongdaoPage::loadArt(Application& app) {
    engine::Engine& e = app.engine();
    art_.assign(entries_.size(), Art{});
    if (app.headless()) return;   // 无头：不读立绘表、不载图
    const ChapterDone chapterDone = [&app](int n) { return app.chapterTable().chapterDone(app.state(), n); };
    SpriteIndex sprites;
    const std::vector<std::string> found = e.findAssets("art/sprites", "index.json");
    if (!found.empty()) {
        core::Result<SpriteIndex> index = io::loadSpriteIndex(found.front());
        if (index) sprites = std::move(index.value);
    }
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        Art& art = art_[i];
        const std::string& role = entries_[i].roleId;
        if (const std::optional<PortraitFrame> bust = app.busts().frameFor(e, false, role, "normal", chapterDone)) {
            art.bust = bust->texture;
            art.bustSrc = bust->src;
        }
        if (const CharacterSheet* sheet = sheetForRole(sprites, role, chapterDone)) {
            art.sprite = e.loadTexture("art/sprites/" + sheet->file);
            const engine::RectF frame = sheetFrameRect(*sheet, sheet->walk[kFacingDown].front());
            art.spriteFrame = engine::Rect{static_cast<int>(frame.x), static_cast<int>(frame.y),
                                           static_cast<int>(frame.w), static_cast<int>(frame.h)};
            art.spriteScale = MenuScene::menuSpriteScale(sheet->scale);
            art.spriteFootY = sheet->footY;
        }
    }
}

void TongdaoPage::rebuild(Application& app) {
    const std::string keep = selectedId();
    entries_ = tongdaoEntries(app);
    loadArt(app);
    cursor_ = std::clamp(cursor_, 0, std::max(0, static_cast<int>(entries_.size()) - 1));
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        if (entries_[i].id == keep) cursor_ = static_cast<int>(i);
    }
    rebuildActions(app);
    rebuildGifts(app);
}

void TongdaoPage::enter(Application& app) {
    rebuild(app);
    focus_ = Focus::Cards;
    cursor_ = 0;
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        if (entries_[i].met) {
            cursor_ = static_cast<int>(i);
            break;
        }
    }
}

bool TongdaoPage::openDetail(Application& app, const std::string& companionId) {
    rebuild(app);
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        if (entries_[i].id != companionId) continue;
        if (!entries_[i].met) return false;
        cursor_ = static_cast<int>(i);
        rebuildActions(app);
        focus_ = Focus::Detail;
        return true;
    }
    return false;
}

bool TongdaoPage::openGift(Application& app, const std::string& companionId) {
    if (!openDetail(app, companionId)) return false;
    rebuildGifts(app);
    focus_ = Focus::Gift;
    return true;
}

void TongdaoPage::rebuildActions(Application& app) {
    const int keep = actions_.selection();
    actions_.reset();
    actions_.setPageSize(kActionRows);
    actions_.setItems(tongdaoActionRows(app, selectedId()));
    restoreSelection(actions_, keep);
}

void TongdaoPage::rebuildGifts(Application& app) {
    const core::GameData& data = app.data();
    const core::GameState& state = app.state();
    const core::Companion* companion = findCompanion(app, selectedId());
    // 二档起行尾直接标合意 / 不喜（施工图 3.4）；一档只写数量。
    const bool showTaste = companion != nullptr && rules::bondTier(state.bondOf(companion->id)) >= 2;
    std::vector<ui::ListItem> rows;
    giftChoices_.clear();
    const std::vector<ui::ListItem> bagRows = MenuScene::itemRows(data, state);
    std::size_t bagRow = 0;
    for (const core::BagEntry& entry : state.bag) {
        if (entry.itemId == kSpiritStoneItemId || entry.count <= 0) continue;
        ui::ListItem row = bagRows[bagRow++];   // itemRows 与这里同一道过滤、同一个次序
        const core::Item* item = data.findItem(entry.itemId);
        if (item == nullptr || item->kind == core::ItemKind::Story) continue;
        if (showTaste) {
            const rules::GiftTaste taste = rules::giftTaste(*companion, entry.itemId);
            if (taste == rules::GiftTaste::Liked) row.detail = app.text("ui.tongdao.gift.liked") + "  " + row.detail;
            if (taste == rules::GiftTaste::Disliked) {
                row.detail = app.text("ui.tongdao.gift.disliked") + "  " + row.detail;
            }
        }
        rows.push_back(std::move(row));
        giftChoices_.push_back(GiftChoice{entry.itemId, entry.herbAge});
    }
    const int keep = gifts_.selection();
    gifts_.reset();
    gifts_.setEmptyHint(app.text("ui.tongdao.why.no_gift"));
    gifts_.setPageSize(std::max(1, ui::listRowsThatFit(giftListRect(engine::Rect{0, 0, 860, 584}).h, app.theme())));
    gifts_.setItems(std::move(rows));
    restoreSelection(gifts_, keep);
}

// ---------------------------------------------------------------------------
// 按键
// ---------------------------------------------------------------------------

void TongdaoPage::moveCursor(Application& app, int dx, int dy) {
    const int n = static_cast<int>(entries_.size());
    if (n == 0) return;
    int next = cursor_ + dx + dy * kCols;
    if (dy != 0 && (next < 0 || next >= n)) return;   // 上下越界不动
    next = std::clamp(next, 0, n - 1);
    if (next != cursor_) {
        cursor_ = next;
        app.engine().playSfx(kSfxCursor);
    }
}

void TongdaoPage::stepPerson(Application& app, int direction) {
    const int n = static_cast<int>(entries_.size());
    for (int step = 1; step < n; ++step) {
        const int at = ((cursor_ + direction * step) % n + n) % n;
        if (!entries_[static_cast<std::size_t>(at)].met) continue;
        cursor_ = at;
        actions_.reset();
        rebuildActions(app);
        app.engine().playSfx(kSfxCursor);
        return;
    }
}

TongdaoPage::Outcome TongdaoPage::update(Application& app) {
    engine::Engine& e = app.engine();
    using Key = engine::Engine::Key;
    if (focus_ == Focus::Cards) {
        if (e.keyPressed(Key::Cancel)) {
            e.playSfx(kSfxCancel);
            return Outcome::Back;
        }
        if (e.keyPressed(Key::Left)) moveCursor(app, -1, 0);
        if (e.keyPressed(Key::Right)) moveCursor(app, 1, 0);
        if (e.keyPressed(Key::Up)) moveCursor(app, 0, -1);
        if (e.keyPressed(Key::Down)) moveCursor(app, 0, 1);
        if (e.keyPressed(Key::Confirm)) {
            const std::string id = selectedId();
            if (id.empty() || !entries_[static_cast<std::size_t>(cursor_)].met) {
                e.playSfx(kSfxError);   // 没遇到的人没有详情
                return Outcome::Stay;
            }
            e.playSfx(kSfxConfirm);
            actions_.reset();
            rebuildActions(app);
            focus_ = Focus::Detail;
            // 教学卡「相处」：头一回在百药园看一位的详情。
            if (app.state().mapId == kTongdaoMap) app.tongdaoTutorial("tongdao.together");
        }
        return Outcome::Stay;
    }
    if (focus_ == Focus::Detail) {
        if (e.keyPressed(Key::Left)) stepPerson(app, -1);
        if (e.keyPressed(Key::Right)) stepPerson(app, 1);
        const int before = actions_.selection();
        const bool confirmed = actions_.update(e);
        if (actions_.selection() != before) e.playSfx(kSfxCursor);
        if (actions_.cancelled()) {
            actions_.reset();
            focus_ = Focus::Cards;
            e.playSfx(kSfxCancel);
            return Outcome::Stay;
        }
        return confirmed ? activate(app) : Outcome::Stay;
    }
    // 赠礼列表
    const int before = gifts_.selection();
    const bool confirmed = gifts_.update(e);
    if (gifts_.selection() != before) e.playSfx(kSfxCursor);
    if (gifts_.cancelled()) {
        gifts_.reset();
        focus_ = Focus::Detail;
        e.playSfx(kSfxCancel);
    } else if (confirmed) {
        giveSelected(app);
    }
    return Outcome::Stay;
}

TongdaoPage::Outcome TongdaoPage::activate(Application& app) {
    engine::Engine& e = app.engine();
    const std::string id = selectedId();
    const int row = actions_.selection();
    if (id.empty() || row < 0 || row >= kTongdaoActionCount) {
        e.playSfx(kSfxError);
        return Outcome::Stay;
    }
    // 列表是此刻拼的，再问一遍：别处（脚本、别的面板）动过状态时不照旧表办事。
    const std::vector<ui::ListItem> rows = tongdaoActionRows(app, id);
    if (!rows[static_cast<std::size_t>(row)].enabled) {
        e.playSfx(kSfxError);
        rebuildActions(app);
        return Outcome::Stay;
    }
    switch (static_cast<TongdaoAction>(row)) {
        case TongdaoAction::Gift:
            e.playSfx(kSfxConfirm);
            gifts_.reset();
            rebuildGifts(app);
            focus_ = Focus::Gift;
            return Outcome::Stay;
        case TongdaoAction::Spar: {
            const std::string name = entries_[static_cast<std::size_t>(cursor_)].name;
            Application* host = &app;
            const rules::TongdaoCheck check = app.startSpar(
                id, [host, name](const rules::SparOutcome& outcome, const script::CommandResult& result) {
                    std::string line = host->text(result.battleWon ? "ui.tongdao.spar.won" : "ui.tongdao.spar.lost");
                    line = tongdaoFill(line, "{name}", name);
                    line = tongdaoFill(line, "{gain}", std::to_string(outcome.gained));
                    if (!outcome.prize.empty()) {
                        std::vector<std::string> items;
                        for (const core::BagEntry& entry : outcome.prize) {
                            const core::Item* item = host->data().findItem(entry.itemId);
                            std::string label = item != nullptr ? item->name : entry.itemId;
                            if (entry.count > 1) label += " ×" + std::to_string(entry.count);
                            items.push_back(std::move(label));
                        }
                        line += tongdaoFill(host->text("ui.tongdao.spar.prize"), "{items}", joinList(items));
                    }
                    narrate(*host, line);
                });
            if (check != rules::TongdaoCheck::Ok) {
                e.playSfx(kSfxError);
                rebuildActions(app);
                return Outcome::Stay;
            }
            e.playSfx(kSfxConfirm);
            return Outcome::CloseMenu;
        }
        case TongdaoAction::Qingjiao:
        case TongdaoAction::Xuhua: {
            const bool qingjiao = static_cast<TongdaoAction>(row) == TongdaoAction::Qingjiao;
            const std::string script =
                qingjiao ? tongdaoQingjiaoScript(id)
                         : tongdaoXuhuaScript(id, rules::xuhuaPending(app.data().companions, id, app.state()));
            if (auto started = app.startEvent(script); !started) {
                std::fprintf(stderr, "[tongdao] 事件脚本 %s 起不来：%s\n", script.c_str(), started.error.c_str());
                e.playSfx(kSfxError);
                return Outcome::Stay;
            }
            e.playSfx(kSfxConfirm);
            return Outcome::CloseMenu;
        }
        case TongdaoAction::Bonus: {
            const std::string flag = rules::bonusOnFlag(id);
            app.state().setFlag(flag, app.state().flag(flag) != 0 ? 0 : 1);
            e.playSfx(kSfxConfirm);
            rebuildActions(app);
            return Outcome::Stay;
        }
    }
    return Outcome::Stay;
}

void TongdaoPage::giveSelected(Application& app) {
    engine::Engine& e = app.engine();
    const std::string id = selectedId();
    const int index = gifts_.selection();
    if (id.empty() || index < 0 || index >= static_cast<int>(giftChoices_.size())) {
        e.playSfx(kSfxError);
        return;
    }
    const GiftChoice choice = giftChoices_[static_cast<std::size_t>(index)];
    core::GameState& state = app.state();
    // 先核背包、再记账、再扣（与队伍页的赠与同一个次序，不会记了账却没扣成）。
    if (state.itemCountOfAge(choice.itemId, choice.herbAge) < 1) {
        e.playSfx(kSfxError);
        rebuildGifts(app);
        return;
    }
    const rules::GiftResult result = rules::giveGiftTo(app.data().companions, id, choice.itemId, state);
    if (result.check != rules::TongdaoCheck::Ok || !result.outcome.given) {
        e.playSfx(kSfxError);
        if (result.check == rules::TongdaoCheck::Ok) narrate(app, app.text("ui.tongdao.why.gift_today"));
        focus_ = Focus::Detail;
        rebuild(app);
        return;
    }
    (void)state.removeItemOfAge(choice.itemId, 1, choice.herbAge);   // 上面核过数量，扣得成
    e.playSfx(kSfxConfirm);
    const char* key = result.outcome.taste == rules::GiftTaste::Liked    ? "ui.tongdao.gift.done.liked"
                      : result.outcome.taste == rules::GiftTaste::Plain ? "ui.tongdao.gift.done.plain"
                                                                         : "ui.tongdao.gift.done.disliked";
    std::string line = tongdaoFill(app.text(key), "{name}", entries_[static_cast<std::size_t>(cursor_)].name);
    line = tongdaoFill(line, "{gain}", std::to_string(result.outcome.gained));
    narrate(app, line);
    focus_ = Focus::Detail;
    rebuild(app);
}

// ---------------------------------------------------------------------------
// 画
// ---------------------------------------------------------------------------

void TongdaoPage::render(Application& app, const engine::Rect& area, bool focused) const {
    drawWaizhuan(app, area);
    if (!focused || focus_ == Focus::Cards) {
        drawCards(app, area, focused);
        return;
    }
    drawDetail(app, area);
}

void TongdaoPage::drawWaizhuan(Application& app, const engine::Rect& area) const {
    // 「外传」压在右栏标题带的右端（施工图 2.1：相处、切磋、同修都是外传，界面上标出来）。
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const std::string tag = app.text("ui.tongdao.waizhuan");
    const engine::Point size = e.measureText(tag, 18);
    const int x = area.x + area.w - theme.padding - size.x;
    const int y = area.y - ui::panelTitleBandHeight(true, theme) + theme.lineSpacing + 3;
    ui::drawFrame(e, engine::RectF{static_cast<float>(x - 8), static_cast<float>(y - 3), static_cast<float>(size.x + 16),
                                   static_cast<float>(size.y + 6)},
                  theme.cinnabar);
    e.drawText(tag, x, y, 18, theme.cinnabar, theme.bodyStyle);
}

void TongdaoPage::drawCards(Application& app, const engine::Rect& area, bool focused) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const int x = area.x + theme.padding;
    e.drawText(app.text("ui.tongdao.cards.hint"), x, area.y + 6, 18, theme.paperDim, theme.bodyStyle);
    const int pages = pageCount();
    const int current = std::clamp(page(), 0, pages - 1);
    if (pages > 1) {
        std::string label = tongdaoFill(app.text("ui.tongdao.page"), "{n}", std::to_string(current + 1));
        label = tongdaoFill(label, "{total}", std::to_string(pages));
        const engine::Point size = e.measureText(label, 18);
        e.drawText(label, area.x + area.w - theme.padding - size.x, area.y + 6, 18, theme.gold, theme.bodyStyle);
    }
    for (int slot = 0; slot < kPerPage; ++slot) {
        const std::size_t index = static_cast<std::size_t>(current * kPerPage + slot);
        if (index >= entries_.size()) break;
        drawCard(app, cardRect(area, slot), index, focused && static_cast<int>(index) == cursor_);
    }
    // 底下一行：在册加成用了几条名额。
    const auto& companions = app.data().companions;
    const int used = static_cast<int>(rules::activeRosterBonuses(companions, app.state()).size());
    std::string slots = tongdaoFill(app.text("ui.tongdao.bonus.slots"), "{used}", std::to_string(used));
    slots = tongdaoFill(slots, "{slots}", std::to_string(rules::rosterBonusSlots(app.state().realm)));
    const std::string line = app.text("ui.tongdao.bonus") + "　" + slots;
    const int footY = cardRect(area, kCols).y + kCardH + 10;
    e.drawText(line, x, footY, 17, theme.paperDim, theme.bodyStyle);
}

void TongdaoPage::drawArtInto(Application& app, const engine::Rect& box, std::size_t index, bool full,
                              bool silhouette) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    if (index >= art_.size()) return;
    const Art& art = art_[index];
    engine::DrawOptions options;
    if (silhouette) options.tint = kSilhouetteTint;
    if (art.bust != engine::kInvalidTexture) {
        if (full) {
            const engine::RectF src = bustRosterSource(art.bustSrc);
            const engine::RectF dst = bustRosterArea(box, src);
            if (dst.w > 0.f) e.drawTexture(art.bust, src, dst, options);
        } else {
            // 头肩：格子左右各裁 15%，从头顶线上一点往下，按框的比例取一块。
            const engine::RectF& cell = art.bustSrc;
            const float w = cell.w * (1.f - 2.f * kCardCropLeft);
            const float h = w * static_cast<float>(box.h) / static_cast<float>(std::max(1, box.w));
            const engine::RectF src{cell.x + cell.w * kCardCropLeft, cell.y + cell.h * kCardCropTop, w,
                                    std::min(h, cell.h * (1.f - kCardCropTop))};
            e.drawTexture(art.bust, src, toRectF(box), options);
        }
        return;
    }
    if (art.sprite != engine::kInvalidTexture) {
        const int scale = art.spriteScale * (full ? 2 : 1);
        engine::Rect figureBox = box;
        if (full) {
            figureBox = engine::Rect{box.x + (box.w - MenuScene::kPartyFigureW) / 2,
                                     box.y + box.h - MenuScene::kPartyFigureH - MenuScene::kPartyFigureBottom,
                                     MenuScene::kPartyFigureW, MenuScene::kPartyFigureH};
        }
        const MenuScene::PortraitPlacement at =
            MenuScene::portraitPlacement(figureBox, art.spriteFrame, scale, art.spriteFootY);
        if (at.dst.w > 0.f && at.dst.h > 0.f) e.drawTexture(art.sprite, at.src, at.dst, options);
        return;
    }
    // 没有画（无头、美术没到）：遇到过的画姓——与状态页的印章同一种画法；剪影什么也不画（卡片上另写「？？？」）。
    if (silhouette || index >= entries_.size() || entries_[index].name.empty()) return;
    const std::string glyph = engine::splitGraphemes(entries_[index].name).front();
    const engine::Point size = e.measureText(glyph, 44);
    e.drawText(glyph, box.x + (box.w - size.x) / 2, box.y + (box.h - size.y) / 2, 44,
               theme.gold, theme.titleStyle);
}

void TongdaoPage::drawCard(Application& app, const engine::Rect& box, std::size_t index, bool selected) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const TongdaoEntry& entry = entries_[index];
    const engine::RectF frame = toRectF(box);
    e.drawGradient(frame, theme.inkDeep, ui::withAlpha(theme.ink, 200), engine::BlendMode::Alpha);

    const engine::Rect artBox{box.x + 4, box.y + 4, box.w - 8, kCardArtH};
    e.drawGradient(toRectF(artBox), ui::withAlpha(theme.ink, 120), theme.inkDeep, engine::BlendMode::Alpha);
    drawArtInto(app, artBox, index, false, !entry.met);
    if (!entry.met) {
        const std::string mark = app.text("ui.tongdao.unknown");
        const engine::Point size = e.measureText(mark, 26);
        e.drawText(mark, artBox.x + (artBox.w - size.x) / 2, artBox.y + (artBox.h - size.y) / 2, 26, theme.paperDim,
                   theme.titleStyle);
    }
    ui::drawRuleH(e, static_cast<float>(box.x + 12), static_cast<float>(artBox.y + artBox.h + 2),
                  static_cast<float>(box.w - 24), theme);

    const int textX = box.x + 12;
    const int textW = box.w - 24;
    const int nameY = artBox.y + artBox.h + 8;
    if (entry.met) {
        const engine::Point tierSize = e.measureText(entry.tierName, 18);
        const std::string name = ellipsize(e, entry.name, textW - tierSize.x - 10, 22);
        e.drawText(name, textX, nameY, 22, theme.goldBright, theme.titleStyle);
        e.drawText(entry.tierName, box.x + box.w - 12 - tierSize.x, nameY + 3, 18, theme.gold, theme.bodyStyle);
        const int lo = tierThreshold(entry.tier);
        const int hi = entry.tier >= 5 ? rules::kBondMax : tierThreshold(entry.tier + 1);
        ui::drawGauge(e, engine::Rect{textX, nameY + 32, textW, 6}, entry.bond - lo, std::max(1, hi - lo), theme.gold,
                      theme);
    } else {
        e.drawText(entry.name, textX, nameY, 22, theme.paperDim, theme.titleStyle);
    }
    e.drawText(ellipsize(e, entry.intro, textW, 16), textX, nameY + 44, 16, theme.paperDim, theme.bodyStyle);

    if (entry.xuhuaReady) {
        // 角标「可叙话」：画面右上角一块小牌子。
        const std::string tag = app.text("ui.tongdao.can_xuhua");
        const engine::Point size = e.measureText(tag, 16);
        const engine::Rect plate{box.x + box.w - size.x - 26, box.y + 10, size.x + 16, size.y + 8};
        ui::drawPlate(e, plate, theme);
        e.drawText(tag, plate.x + 8, plate.y + 4, 16, theme.goldBright, theme.bodyStyle);
    }
    ui::drawFrame(e, frame, selected ? theme.goldBright : theme.goldDim);
    if (selected) {
        ui::drawFrame(e, engine::RectF{frame.x + 2.f, frame.y + 2.f, frame.w - 4.f, frame.h - 4.f}, theme.gold);
    }
}

void TongdaoPage::drawDetail(Application& app, const engine::Rect& area) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    if (cursor_ < 0 || cursor_ >= static_cast<int>(entries_.size())) return;
    const std::size_t index = static_cast<std::size_t>(cursor_);
    const engine::Rect art = detailArtRect(area);
    drawArtInto(app, engine::Rect{art.x, art.y, art.w, art.h - 6}, index, true, false);
    ui::drawRuleV(e, static_cast<float>(art.x + art.w + 4), static_cast<float>(area.y + theme.padding / 2),
                  static_cast<float>(area.h - theme.padding), theme);
    drawInfo(app, detailInfoRect(area), entries_[index]);
    if (focus_ == Focus::Gift) {
        const engine::Rect list = giftListRect(area);
        const core::Companion* companion = findCompanion(app, entries_[index].id);
        std::string title = tongdaoFill(app.text("ui.tongdao.gift.title"), "{name}", entries_[index].name);
        e.drawText(title, list.x + 12, list.y - 26, kInfoFont, theme.goldBright, theme.bodyStyle);
        (void)companion;
        gifts_.render(e, list, theme);
        return;
    }
    actions_.render(e, actionListRect(area, theme), theme);
}

void TongdaoPage::drawInfo(Application& app, const engine::Rect& info, const TongdaoEntry& entry) const {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();
    const core::Companion* companion = findCompanion(app, entry.id);
    if (companion == nullptr) return;
    const int x = info.x;

    // 名字 + 档名
    const std::string name = ellipsize(e, entry.name, info.w - 120, 28);
    e.drawText(name, x, info.y + kNameY, 28, theme.goldBright, theme.titleStyle);
    e.drawText(entry.tierName, x + e.measureText(name, 28).x + 16, info.y + kNameY + 7, 20, theme.gold,
               theme.bodyStyle);
    if (!entry.introIsBond) {
        e.drawText(ellipsize(e, entry.intro, info.w, 17), x, info.y + kIntroY, 17, theme.paperDim, theme.bodyStyle);
    }

    // 五格档位：够到的填暗金，此刻那一格亮框；封顶以上的写「机缘未到」。
    const int cellW = (info.w - 4 * kCellGap) / 5;
    for (int i = 0; i < 5; ++i) {
        const int tier = i + 1;
        const engine::RectF cell{static_cast<float>(x + i * (cellW + kCellGap)), static_cast<float>(info.y + kCellsY),
                                 static_cast<float>(cellW), static_cast<float>(kCellH)};
        const bool reached = tier <= entry.tier;
        const bool capped = tier > entry.capTier;
        if (reached) e.fillRect(cell, ui::withAlpha(theme.gold, 70), engine::BlendMode::Alpha);
        ui::drawFrame(e, cell, tier == entry.tier ? theme.goldBright : theme.goldDim);
        const std::string label = capped ? app.text("ui.tongdao.capped") : tierName(app, *companion, tier);
        const int size = capped ? 15 : 18;
        const engine::Point m = e.measureText(label, size);
        e.drawText(label, static_cast<int>(cell.x) + (cellW - m.x) / 2, static_cast<int>(cell.y) + (kCellH - m.y) / 2,
                   size, capped ? theme.paperDim : (reached ? theme.goldBright : theme.paper), theme.bodyStyle);
    }

    // 默契进度：离下一档还差多少；最高一档 / 封顶时另写。
    std::string progress = tongdaoFill(app.text("ui.tongdao.bond"), "{value}", std::to_string(entry.bond));
    int gaugeNow = 1;
    int gaugeMax = 1;
    if (entry.tier >= 5) {
        progress += "　" + app.text(entry.bond >= rules::kBondMax ? "ui.tongdao.full" : "ui.tongdao.top");
    } else if (entry.tier >= entry.capTier) {
        progress += "　" + app.text("ui.tongdao.capped");
    } else {
        std::string next = tongdaoFill(app.text("ui.tongdao.to_next"), "{tier}", tierName(app, *companion, entry.tier + 1));
        next = tongdaoFill(next, "{gap}", std::to_string(tierThreshold(entry.tier + 1) - entry.bond));
        progress += "　" + next;
        gaugeNow = entry.bond - tierThreshold(entry.tier);
        gaugeMax = std::max(1, tierThreshold(entry.tier + 1) - tierThreshold(entry.tier));
    }
    const int progressW = e.measureText(progress, kInfoFont).x;
    e.drawText(progress, x, info.y + kProgressY, kInfoFont, theme.paper, theme.bodyStyle);
    const int gaugeX = x + progressW + 16;
    if (gaugeX + 40 < info.x + info.w) {
        ui::drawGauge(e, engine::Rect{gaugeX, info.y + kProgressY + 7, info.x + info.w - gaugeX, 8}, gaugeNow, gaugeMax,
                      theme.gold, theme);
    }
    if (focus_ == Focus::Gift) return;   // 赠礼列表占下半截

    // 下面四块：标签在左，正文折行至多两行。
    const int textX = x + kLabelW;
    const int textW = info.w - kLabelW;
    int y = info.y + kBlocksY;
    const auto block = [&](const std::string& label, const std::string& text, engine::Color color) {
        e.drawText(label, x, y, kInfoFont, theme.gold, theme.bodyStyle);
        drawWrapped(e, theme, text, textX, y, textW, kInfoFont, kInfoLineH, color, 2);
        y += kBlockH;
    };
    std::vector<std::string> has;
    for (int tier = 1; tier <= entry.tier; ++tier) {
        for (const std::string& gain : tierGains(app, *companion, tier)) {
            // 「已有」只列本事（法术、追击、被动、合招），不列「可切磋」这类相处的门槛。
            if (gain == app.text("ui.tongdao.gain.join") || gain == app.text("ui.tongdao.gain.spar") ||
                gain == app.text("ui.tongdao.gain.likes") || gain == app.text("ui.tongdao.gain.qingjiao") ||
                gain == app.text("ui.tongdao.gain.xuhua")) {
                continue;
            }
            has.push_back(gain);
        }
    }
    block(app.text("ui.tongdao.has"), has.empty() ? app.text("ui.tongdao.none") : joinList(has), theme.paper);
    std::string next;
    if (entry.tier >= 5) {
        next = app.text("ui.tongdao.next.top");
    } else if (entry.tier >= entry.capTier) {
        next = app.text("ui.tongdao.next.capped");
    } else {
        const std::vector<std::string> gains = tierGains(app, *companion, entry.tier + 1);
        next = "「" + tierName(app, *companion, entry.tier + 1) + "」：" +
               (gains.empty() ? app.text("ui.tongdao.none") : joinList(gains));
    }
    block(app.text("ui.tongdao.next"), next, theme.paper);

    std::string likes;
    if (entry.tier < 2) {
        likes = app.text("ui.tongdao.likes.hidden");
    } else {
        const auto names = [&app](const std::vector<std::string>& ids) {
            std::vector<std::string> out;
            for (const std::string& id : ids) {
                const core::Item* item = app.data().findItem(id);
                out.push_back(item != nullptr ? item->name : id);
            }
            return out.empty() ? app.text("ui.tongdao.none") : joinList(out);
        };
        likes = tongdaoFill(app.text("ui.tongdao.likes.line"), "{like}", names(companion->giftsLike));
        likes = tongdaoFill(likes, "{dislike}", names(companion->giftsDislike));
    }
    block(app.text("ui.tongdao.likes"), likes, entry.tier < 2 ? theme.paperDim : theme.paper);

    // 在册加成：说明 + 状态 + 名额。
    std::string bonus;
    engine::Color bonusColor = theme.paper;
    if (companion->rosterBonus.kind == core::PerkKind::None) {
        bonus = app.text("ui.tongdao.bonus.none");
        bonusColor = theme.paperDim;
    } else {
        const auto& companions = app.data().companions;
        const std::vector<std::string> active = rules::activeRosterBonuses(companions, app.state());
        std::string status;
        if (!rules::rosterBonusUnlocked(*companion, app.state())) {
            status = app.text(entry.person ? "ui.tongdao.bonus.locked.person" : "ui.tongdao.bonus.locked.beast");
            bonusColor = theme.paperDim;
        } else if (std::find(active.begin(), active.end(), entry.id) != active.end()) {
            status = app.text("ui.tongdao.bonus.on");
            bonusColor = theme.jade;
        } else {
            status = app.text("ui.tongdao.bonus.off");
            bonusColor = theme.paperDim;
        }
        std::string slots = tongdaoFill(app.text("ui.tongdao.bonus.slots"), "{used}", std::to_string(active.size()));
        slots = tongdaoFill(slots, "{slots}", std::to_string(rules::rosterBonusSlots(app.state().realm)));
        bonus = perkText(app.data(), companion->rosterBonus, companion->rosterBonus.value) + "（" + status + "，" +
                slots + "）";
    }
    block(app.text("ui.tongdao.bonus"), bonus, bonusColor);
}

}  // namespace fanren::game
