#include "game/Application.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iterator>
#include <limits>
#include <optional>
#include <string_view>
#include <system_error>
#include <utility>

#include "core/rules/Bottle.h"
#include "core/rules/Calendar.h"
#include "core/rules/Companions.h"
#include "core/rules/Field.h"
#include "core/rules/Lilian.h"
#include "core/rules/Lore.h"
#include "core/rules/Realm.h"
#include "core/rules/Tutorials.h"
#include "game/AlchemyScene.h"
#include "game/BattleScene.h"
#include "game/BoardScene.h"
#include "game/CultivationScene.h"
#include "game/DialogueScene.h"
#include "game/FadeScene.h"
#include "game/FieldScene.h"
#include "game/KeyConfigScene.h"
#include "game/LilianPartyScene.h"
#include "game/MenuScene.h"
#include "game/PathActionScene.h"
#include "game/SettingsScene.h"
#include "game/ShopScene.h"
#include "game/TitleScene.h"
#include "game/TutorialScene.h"
#include "game/WorldScene.h"
#include "io/BattleLoader.h"
#include "io/DataLoader.h"
#include "io/EncounterLoader.h"
#include "io/ExplorationLoader.h"
#include "io/LilianLoader.h"
#include "io/RecipeLoader.h"
#include "io/SaveFile.h"
#include "io/ShopLoader.h"
#include "io/VisualLoader.h"

namespace fanren::game {

// 扫描码的上限两边各有一份（io 不依赖 engine）：设置文件的形状检查与引擎的键位表必须是同一个数。
static_assert(engine::kScanCodeLimit == io::kScancodeLimit, "engine::kScanCodeLimit 与 io::kScancodeLimit 必须相同");

namespace {

// 无头模式下每帧的固定步长。真机跑的是真实 deltaTime，bot 跑固定步长才能复现。
constexpr double kHeadlessStep = 1.0 / 60.0;

// 灵田的生长速度：多少天长一年。谷中药圃按标准历算，第 7 章的百药园将来会
// 用更小的值（灵脉更足），届时由地图属性覆盖。
constexpr int kFieldDaysPerYear = rules::kDaysPerYear;

// 地图属性里的整数。写错或缺失时退回缺省值，不让一条写坏的属性把面板打不开。
[[nodiscard]] int propertyInt(const std::string& raw, int fallback) {
    if (raw.empty()) return fallback;
    char* end = nullptr;
    const long value = std::strtol(raw.c_str(), &end, 10);
    // 整串都得是数字才认。只查「有没有读到数字」的话，"6 格" 会被悄悄读成 6，
    // 而这正是「写错了要退回缺省值」想拦的那一类手误。
    if (end == raw.c_str() || *end != '\0') return fallback;
    return static_cast<int>(std::clamp<long>(value, 0, 1000));
}

// 这一味药自己的年份上限。与 FieldScene 里那个同名函数是同一套口径：
// 传全局的 kMaxHerbAge 等于没有上限（绿液翻倍 + 售价平方曲线，十一滴就能把
// 一株一阶药推到一万年），查不到这味药时退到最低那一档而不是退到全局上限
// ——一个拼错的 id 不该换来一株能刷到万年的药。
//
// 没有做成公共函数是因为改动面：FieldScene.* 眼下有别人在改。两处都只是
// 「查 data，查不到退最低档」，口径一旦要动，这条注释是第二处的路标。
[[nodiscard]] int herbMaxAgeOf(const core::GameData& data, const std::string& itemId) {
    if (const core::Item* item = data.findItem(itemId); item != nullptr && item->maxAge > 0) {
        return item->maxAge;
    }
    return rules::herbMaxAgeForGrade(0);
}

// 催熟失败原因 → 回填给脚本的机器码。
//
// 脚本侧的取值表写在 docs/interfaces-p3-script.md 第 6 节，改一边要改两边；
// 这里刻意用 switch 而不是查表，新增一个枚举值时 /W4 会因为漏掉的分支报警。
[[nodiscard]] const char* matureFailureCode(rules::MatureFailure failure) {
    switch (failure) {
        case rules::MatureFailure::NoBottle:      return "no_bottle";
        case rules::MatureFailure::MatureUnknown: return "mature_unknown";
        case rules::MatureFailure::NoDrops:       return "no_drops";
        case rules::MatureFailure::NotRipe:       return "not_ripe";
        case rules::MatureFailure::AtMaxAge:      return "at_max_age";
        case rules::MatureFailure::None:          break;
    }
    return "";
}

// 世界层的两个路径行动钩子（WorldScene::setPathActionHooks）。钩子是函数指针，
// 接不了成员函数，所以在这里各转一手。
[[nodiscard]] PathBubble pathBubbleHook(Application& app, const core::MapObject& npc) {
    return pathBubbleFor(app.pathActionsFor(npc.name));
}

bool pathActionHook(Application& app, const core::MapObject& npc) {
    return app.openPathActions(npc.name);
}

// 把文案里每一处 {<前缀><动作 id>}（前缀 "{key." 或 "{pad."）换成 label(那个动作)。认不出的 id 原样留着：画面上一眼看得出。
template <typename Label>
[[nodiscard]] std::string expandActionPlaceholders(std::string line, std::string_view open, const Label& label) {
    std::size_t from = 0;
    while ((from = line.find(open, from)) != std::string::npos) {
        const std::size_t close = line.find('}', from + open.size());
        if (close == std::string::npos) break;
        const std::optional<engine::Engine::Key> action =
            engine::Engine::keyFromId(std::string_view(line).substr(from + open.size(), close - from - open.size()));
        if (!action) {
            from = close + 1;
            continue;
        }
        const std::string text = label(*action);
        line.replace(from, close + 1 - from, text);
        from += text.size();
    }
    return line;
}

}  // namespace

int bottleChargeDays(rules::Realm realm) noexcept {
    switch (rules::tierOf(realm)) {
        case rules::RealmTier::Core:       return 3;
        case rules::RealmTier::Foundation: return 5;
        case rules::RealmTier::QiRefining:
        case rules::RealmTier::Mortal:
            break;
    }
    // 凡人与炼气期都用基准值：拾瓶当日起算，第 8 天得到第一滴，
    // 对上原著「第八日瓶盖方开，内有一滴绿液」。
    return rules::kBaseChargeDays;
}

int battleMenace(const core::BattleSetup& battle, const core::GameData& data) {
    long long menace = 0;
    for (const core::BattleUnitSpec& unit : battle.units) {
        if (unit.ally) continue;
        // 模板查不到的单位 BattleScene 会跳过不建，这里同样不算。
        if (const core::RoleTemplate* role = data.findRole(unit.roleId)) {
            menace += static_cast<long long>(role->maxHp) * role->attack;
        }
    }
    return static_cast<int>(std::min<long long>(menace, std::numeric_limits<int>::max()));
}

int menaceStars(int menace) {
    // 每多一颗，凶一倍（见 Application.h 那张表）。
    int stars = 1;
    for (long long threshold = 2LL * kDangerUnit; menace >= threshold && stars < hud::PlaceBanner::kMaxStars;
         threshold *= 2) {
        ++stars;
    }
    return stars;
}

int encounterDangerStars(const rules::EncounterTable& table, rules::Realm realm,
                         const std::map<std::string, core::BattleSetup>& battles,
                         const core::GameData& data) {
    int worst = -1;
    for (const rules::EncounterEntry& entry : table.entries) {
        // 与 rules::step 同一道境界过滤：这个境界遇不上的那几条不算进「这里有多凶」。
        if (rules::toValue(realm) < rules::toValue(entry.minRealm) ||
            rules::toValue(realm) > rules::toValue(entry.maxRealm)) {
            continue;
        }
        const auto it = battles.find(entry.battleId);
        if (it == battles.end()) continue;   // 加载器已对过账，走不到；与 battleSetup 查不到同一个态度
        worst = std::max(worst, battleMenace(it->second, data));
    }
    return worst < 0 ? 0 : menaceStars(worst);
}

Application::Application() = default;
Application::~Application() = default;

core::Result<bool> Application::init(const std::string& assetRoot, bool headless) {
    assetRoot_ = assetRoot;
    portraits_.setRoot(assetRoot_);
    headless_ = headless;

    engine_ = std::make_unique<engine::Engine>();
    auto engineReady = engine_->init("凡人修仙传", headless);
    if (!engineReady) return core::Result<bool>::failure(engineReady.error);

    auto data = io::loadGameData(assetRoot_ + "/data");
    if (!data) return core::Result<bool>::failure(data.error);
    data_ = std::move(data.value);

    // 章节表。**排在 data_ 之后**：章号与章名是文案 key，要拿 data_ 查成正文。
    // 没有这份文件是合法的（不排卡片），写坏了才算失败，见 loadChapterTable。
    auto chapters = loadChapterTable(assetRoot_ + "/data/chapters.json", data_);
    if (!chapters) return core::Result<bool>::failure(chapters.error);
    chapters_ = std::move(chapters.value);

    auto battles = io::loadBattles(assetRoot_ + "/data/battles");
    if (!battles) return core::Result<bool>::failure(battles.error);
    battles_ = std::move(battles.value);

    // 战斗背景指派表。与 data/、maps/ **同一个资产根**：从前是战斗画面拿引擎的资产根（<仓库>/assets）
    // 去找 data/visual/，那里没有这张表，于是夜袭一律画成白天（终审 HIGH-1）。
    // 没有这份文件是合法的（全部按所在地图取背景）；写坏了只打警告不拦启动——背景取错是画面问题，
    // 不该让人进不了游戏，但得在日志里说清楚，那是「夜战变白天」查不出的来处。
    if (const std::string path = assetRoot_ + "/data/visual/battles.json"; std::filesystem::exists(path)) {
        auto backdrops = io::loadBattleBackdrops(path);
        if (backdrops) {
            battleBackdrops_ = std::move(backdrops.value);
        } else {
            std::fprintf(stderr, "[battle] 战斗背景指派表读不了，按所在地图取背景：%s\n", backdrops.error.c_str());
        }
    }

    // 遭遇表。**排在编成之后**：battleId 在加载时就对账（io/EncounterLoader.h）。
    auto encounters = io::loadEncounterTables(assetRoot_ + "/data/encounters", battles_);
    if (!encounters) return core::Result<bool>::failure(encounters.error);
    encounterTables_ = std::move(encounters.value);

    auto shops = io::loadShops(assetRoot_ + "/data/shops");
    if (!shops) return core::Result<bool>::failure(shops.error);
    shops_ = std::move(shops.value);
    initialShops_ = shops_;

    // 配方。**必须排在 data_ 之后**：引用校验（材料 id / 产出 id 在不在物品册上）
    // 要拿 data_ 去查，见 io/RecipeLoader.h。
    auto recipes = io::loadRecipes(assetRoot_ + "/data/recipes", data_);
    if (!recipes) return core::Result<bool>::failure(recipes.error);
    recipes_ = std::move(recipes.value);

    // 历练地（docs/interfaces-companions.md 3.12）。存在这里、不进 GameData（契约第 6 节）。
    // 没有这个目录是合法的（一处历练也没有）；写坏了拦启动，与编成、遭遇表同一个态度。
    // 名字的文案 key 在这里对一遍：组队面板的标题查不到正文就会把 key 本身画出来。
    auto sites = io::loadLilianSites(assetRoot_ + "/data/lilian");
    if (!sites) return core::Result<bool>::failure(sites.error);
    for (const auto& [id, site] : sites.value) {
        if (data_.text.count(site.nameKey) == 0) {
            return core::Result<bool>::failure("历练地 " + id + " 的 name_key（" + site.nameKey + "）查不到文案");
        }
    }
    lilianSites_ = std::move(sites.value);

    // 集齐奖励表（docs/ch07-playtest-ui.md 第 4 节）。与历练地同一个态度：没有目录是合法的（一张也没配），
    // 写坏了拦启动。物品 id 在这里对一遍：发奖那一刻才发现查不到，玩家拿到的就是一张写着 id 的卡片。
    auto rewards = io::loadExplorationRewards(assetRoot_ + "/data/exploration");
    if (!rewards) return core::Result<bool>::failure(rewards.error);
    for (const auto& [mapId, reward] : rewards.value) {
        for (const core::BagEntry& entry : reward.items) {
            if (data_.findItem(entry.itemId) == nullptr) {
                return core::Result<bool>::failure("集齐奖励 " + mapId + " 的物品 " + entry.itemId + " 不在 data/items");
            }
        }
    }
    explorationRewards_ = std::move(rewards.value);

    scripts_ = std::make_unique<script::ScriptHost>();
    // 脚本出错不该静默：错误直接汇进引擎日志，开发期一眼看见。
    scripts_->setLogSink([](const std::string& message) {
        // 暂时写到标准错误；接入正式日志系统后换掉这一处即可。
        std::fputs(message.c_str(), stderr);
        std::fputc('\n', stderr);
    });
    auto scriptsReady = scripts_->init(assetRoot_ + "/scripts", &state_);
    if (!scriptsReady) return core::Result<bool>::failure(scriptsReady.error);

    // 路径行动的数据（data_.pathActions）此刻已在：接上世界层的气泡与 E 键。
    WorldScene::setPathActionHooks(&pathBubbleHook, &pathActionHook);
    // 地名横幅拿不到 Application，危险度那一行的界面词在这里交给它（与上面那两个钩子同一个做法）。
    hud::PlaceBanner::setDangerLabel(text("ui.world.danger"));

    // init 之前就 setSettings 过的（引擎那时还没有）：这里推给引擎。缺省值与引擎自己的缺省一致，推了等于没推。
    applySettings();

    return core::Result<bool>::success(true);
}

void Application::shutdown() {
    // 面板里改了设置、没关面板就点了窗口的 X / Alt+F4：run 退出后走到这里，这时把改动写下去，不丢。
    // 没 enable 过（测试、截图口）不写；坏文件而又没人改过时 settings_ 与 savedSettings_ 相等，也不写。
    if (!settingsPath_.empty() && settings_ != savedSettings_) {
        static_cast<void>(saveSettings());   // 写不进的原因 saveSettings 已经打到标准错误
    }
    scenes_.clear();
    pendingPush_.clear();
    scripts_.reset();
    if (engine_) engine_->shutdown();
    engine_.reset();
}

void Application::pushScene(ScenePtr scene) {
    // 延迟到帧末再改栈：场景的 update 里压栈会让正在遍历的容器失效。
    pendingPush_.push_back(std::move(scene));
}

void Application::popScene() {
    ++pendingPops_;
}

void Application::replaceScene(ScenePtr scene) {
    pendingPops_ = static_cast<int>(scenes_.size());
    pendingPush_.push_back(std::move(scene));
    // 整个栈换掉：以回调开的那一仗（若还在栈上）跟着没了，登记在它身上的回调也作废——
    // 否则下一场脚本开的仗收场时，会被认成那一场、战果送错了人。
    battleOutcome_ = nullptr;
}

Scene* Application::topScene() {
    return scenes_.empty() ? nullptr : scenes_.back().get();
}

bool Application::canSave() const {
    // 协程挂起期间禁止存档：Lua 栈无法序列化（方案 3.5 规则 4）。
    return scripts_ != nullptr && scripts_->canSave() && !commandPending_;
}

core::Result<bool> Application::loadMap(const std::string& mapId, const std::string& spawnId) {
    if (mapId.empty()) return core::Result<bool>::failure("地图 id 为空");

    auto loaded = io::loadTileMap(assetRoot_ + "/maps/" + mapId + ".tmj");
    if (!loaded) return core::Result<bool>::failure(loaded.error);

    map_ = std::make_unique<core::TileMap>(std::move(loaded.value));
    state_.mapId = map_->id;

    // 指定入口优先；找不到就退到 default spawn，再找不到就放在原点并报错，
    // 免得直接把玩家丢在不可通行格里。
    const core::MapObject* spawn = nullptr;
    if (!spawnId.empty()) {
        for (const core::MapObject& object : map_->objects) {
            if (object.type == "spawn" && object.property("id") == spawnId) {
                spawn = &object;
                break;
            }
        }
    }
    if (spawn == nullptr) spawn = map_->defaultSpawn();
    if (spawn == nullptr) {
        state_.position = core::Point{0, 0};
        return core::Result<bool>::failure("地图 " + mapId + " 没有任何 spawn");
    }

    state_.position = spawn->position;
    const std::string facing = spawn->property("facing");
    if (facing == "up") state_.facing = 0;
    else if (facing == "right") state_.facing = 1;
    else if (facing == "left") state_.facing = 3;
    else state_.facing = 2;

    // 脚本点播的曲子换图时仍然作数（worldBgm）：夜探翻墙进了墨府，还是那一首。
    const std::string bgm = worldBgm();
    if (engine_ && !bgm.empty() && bgm != "none") engine_->playBgm(bgm);
    return core::Result<bool>::success(true);
}

std::vector<rules::PortalLink> loadPortalLinks(const std::string& mapsRoot,
                                               std::vector<std::string>& problems) {
    namespace fs = std::filesystem;
    const fs::path root(mapsRoot);
    std::vector<fs::path> files;

    // **手动步进，每一步都带 error_code**（与 io/DataLoader.cpp 的 listJsonFilesSorted
    // 同一个写法）。range-for 的 `++it` 调的是会抛异常的那个重载：构造时带了 ec 只护得住
    // 第一步，扫到一半文件系统出错就会抛出一个没人接的 filesystem_error，整个进程崩掉。
    // 循环条件先看 ec 再解引用：increment 失败之后迭代器的状态不归我们假设。
    std::error_code ec;
    fs::directory_iterator it(root, ec);
    for (; !ec && it != fs::directory_iterator(); it.increment(ec)) {
        std::error_code typeEc;
        if (it->is_regular_file(typeEc) && it->path().extension() == ".tmj") {
            files.push_back(it->path());
        }
    }
    if (ec) {
        // 目录读到一半出错：**整个指路停用**，不拿半张图去算。缺了几张图的「最短路」
        // 会绕远、甚至指向另一道门——一道亮错的门比一道不亮的门更糟
        //（rules::nextPortalToward 的口径：宁可不指，也不瞎指）。
        problems.push_back("读不了地图目录，跨图指路停用：" + root.string() + "（" + ec.message() +
                           "）");
        return {};
    }
    // 按文件名排序：directory_iterator 的次序由文件系统决定，而 nextPortalToward 在
    // 平手时取链表里靠前的那一条——次序不定，指的门就不定。
    std::sort(files.begin(), files.end());

    std::vector<rules::PortalLink> all;
    for (const fs::path& file : files) {
        auto loaded = io::loadTileMap(file.string());
        if (!loaded) {
            // 一张坏图只跳过它自己：它照样会在门禁与 loadMap 那里响亮地失败，
            // 这里记一笔，免得一张坏图把全部指路一起弄瞎。
            problems.push_back("跨图指路跳过一张读不进来的图：" + loaded.error);
            continue;
        }
        std::vector<rules::PortalLink> links = rules::portalLinksOf(loaded.value);
        all.insert(all.end(), std::make_move_iterator(links.begin()),
                   std::make_move_iterator(links.end()));
    }
    return all;
}

const std::vector<rules::PortalLink>& Application::portalLinks() {
    if (portalLinksLoaded_) return portalLinks_;
    portalLinksLoaded_ = true;

    std::vector<std::string> problems;
    portalLinks_ = loadPortalLinks(assetRoot_ + "/maps", problems);
    // 不静默：每一条都落到标准错误，与脚本日志同一个出口。
    for (const std::string& problem : problems) {
        std::fputs(("[world] " + problem + "\n").c_str(), stderr);
    }
    return portalLinks_;
}

core::Result<std::string> Application::quickSave() {
    namespace fs = std::filesystem;
    // **先过 canSave()。** 脚本协程挂起期间 Lua 栈无法序列化（方案 3.5 规则 4），
    // 这时候存下去，读回来会是一个对话演到一半却没有对话的局面。
    // 世界层本来就在脚本运行时不受理输入，所以这一条正常走不到；
    // 写它是因为「正常走不到」不是「走不到」——而静默存一份坏档最难查。
    if (!canSave()) {
        return core::Result<std::string>::failure("这会儿存不了：剧情正演到一半");
    }
    // 历练里不能存档（docs/interfaces-companions.md 3.12）。三个存档入口（存档点、F5、主菜单「存盘」）
    // 都走这一个函数，闸只收在这里：存下去的会是换过的队伍与历练图，读回来原队伍就没了。
    if (inLilian()) {
        return core::Result<std::string>::failure("历练途中存不了，出去之后再记");
    }
    const std::string path = quickSavePath();
    const fs::path dir = fs::path(path).parent_path();
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        return core::Result<std::string>::failure("建不了存档目录：" + dir.string());
    }
    auto wrote = io::saveGame(state_, path);
    if (!wrote) {
        return core::Result<std::string>::failure(wrote.error);
    }
    return core::Result<std::string>::success(path);
}

std::string Application::quickSavePath() const {
    return (std::filesystem::path(assetRoot_) / "saves" / "quick.sav").string();
}

// ---------------------------------------------------------------------------
// 系统设置（docs/settings.md 第 2、3 节）
// ---------------------------------------------------------------------------

void Application::setSettings(const io::Settings& settings) {
    settings_ = settings;
    settings_.bgmVolume = std::clamp(settings_.bgmVolume, 0, io::kVolumeLevels);
    settings_.sfxVolume = std::clamp(settings_.sfxVolume, 0, io::kVolumeLevels);
    applySettings();
}

void Application::applySettings() {
    if (!engine_) return;
    engine::Engine& e = *engine_;
    e.setBgmVolume(io::volumeGain(settings_.bgmVolume));
    e.setSfxVolume(io::volumeGain(settings_.sfxVolume));
    e.setFullscreen(settings_.fullscreen);
    e.setIntegerScale(settings_.scale == io::DisplayScale::Integer);
    e.setVSync(settings_.vsync);
    e.setEffectsLevel(settings_.effects == io::EffectsLevel::Lite ? engine::EffectsLevel::Lite
                                                                  : engine::EffectsLevel::Full);
    e.setRumbleEnabled(settings_.padRumble);   // 手柄震动（docs/gamepad.md 第 8 节）
    // 键位（docs/settings.md 第 6 节）：默认表上叠 keys 里列出的动作，整张交给引擎校验。不过就整张回默认——
    // 只回滚一个动作会造出别的冲突，整张回滚的结果才可预期。
    auto keys = e.setCustomKeys(keyTableOf(settings_));
    if (!keys) {
        std::fputs(("[settings] 键位表整张回默认：" + keys.error + "\n").c_str(), stderr);
        settings_.keys.clear();
        static_cast<void>(e.setCustomKeys(engine::Engine::defaultKeyTable()));
    }
}

std::string Application::text(const std::string& key) const {
    // 提示跟设备走（docs/gamepad.md 第 6 节）：最后一下按的是手柄、且文案表里有 key + ".pad"，就取那一条。
    // 键盘模式下不取 .pad：展开结果与改造前逐字节相同。
    const bool pad = engine_ && engine_->lastInputDevice() == engine::Engine::InputDevice::Gamepad;
    const auto padText = pad ? data_.text.find(key + ".pad") : data_.text.end();
    std::string line = padText != data_.text.end() ? padText->second : data_.lookupText(key);
    // {key.<id>}：那个动作眼下第一个自定义键；没有自定义键就取第一个固定键（不变式保证两样至少有一样）。
    line = expandActionPlaceholders(std::move(line), "{key.", [this](engine::Engine::Key action) {
        const engine::Engine::KeySlots slots =
            engine_ ? engine_->customKeys(action) : engine::Engine::defaultCustomKeys(action);
        engine::ScanCode code = slots[0] != 0 ? slots[0] : slots[1];
        if (code == 0) {
            const std::vector<engine::ScanCode> fixed = engine::Engine::fixedKeys(action);
            if (!fixed.empty()) code = fixed.front();
        }
        return engine::Engine::keyLabel(code);
    });
    // {pad.<id>}：那个动作第一个手柄键的显示名（手柄键位固定，docs/gamepad.md 第 2 节那张表的第一个）。
    return expandActionPlaceholders(std::move(line), "{pad.", [](engine::Engine::Key action) {
        const std::vector<engine::Engine::PadButton> buttons = engine::Engine::padButtons(action);
        return buttons.empty() ? std::string{} : engine::Engine::padLabel(buttons.front());
    });
}

core::Result<bool> Application::enableSettingsFile(const std::string& path) {
    settingsPath_ = path;
    io::SettingsRead read = io::loadSettings(path);
    // 读进来了但有几项没照文件办：逐条说出来，不静默（与跨图指路那几句同一个出口）。
    for (const std::string& warning : read.warnings) {
        std::fputs(("[settings] " + path + "：" + warning + "\n").c_str(), stderr);
    }
    setSettings(read.settings);
    // 此刻生效的这一份就算「已落盘」：坏文件按默认生效、没人改过时两者相等，退出时也就不去覆盖那个坏文件。
    savedSettings_ = settings_;
    if (!read.error.empty()) return core::Result<bool>::failure(read.error);
    return core::Result<bool>::success(read.found);
}

core::Result<bool> Application::saveSettings() {
    if (settingsPath_.empty()) return core::Result<bool>::success(false);
    auto wrote = io::saveSettings(settings_, settingsPath_);
    if (!wrote) {
        // 带整条路径的完整原因打到标准错误；返回给调用方的是一句放得进面板说明行的短话。
        std::fputs(("[settings] " + wrote.error + "\n").c_str(), stderr);
        return core::Result<bool>::failure("写不进 " + std::filesystem::path(settingsPath_).filename().string());
    }
    savedSettings_ = settings_;
    return core::Result<bool>::success(true);
}

std::string Application::defaultSettingsPath() const {
    return (std::filesystem::path(assetRoot_) / "saves" / "settings.json").string();
}

void Application::syncFullscreenFromEngine() {
    if (!engine_) return;
    const bool actual = engine_->fullscreen();
    if (actual == settings_.fullscreen) return;
    settings_.fullscreen = actual;
    auto saved = saveSettings();
    // 写失败：完整原因 saveSettings 已经打到标准错误；设置面板开着就再写在它的说明行上——不限栈顶：
    // 改键面板压在它上面时也写给它，改键面板一收就看得见（面板不在时只有日志，这一局照样按全屏跑，
    // 退出时再试着写一次）。
    if (!saved) {
        for (auto it = scenes_.rbegin(); it != scenes_.rend(); ++it) {
            if (auto* panel = dynamic_cast<SettingsScene*>(it->get())) {
                panel->showSaveFailure(*this, saved.error);
                break;
            }
        }
    }
}

void Application::openMainMenu() {
    pushScene(std::make_unique<MenuScene>());
}

void Application::returnToTitle() {
    scripts_->abortEvent("返回首页");
    commandPending_ = false;
    pendingPush_.clear();
    pendingCards_.clear();
    dialogueLog_.clear();
    spokenKeys_.clear();
    cardLog_.clear();
    screenFade_ = 0.f;
    bgmOverride_.clear();
    itemsGiven_ = false;
    itemToasts_.clear();
    resetGuideWatch();
    shops_ = initialShops_;
    // 历练里返回首页：那一趟就此作罢（与读档、新开局同一个口径），不然 inLilian() 还为真，
    // 回到首页再「继续旅程」之前，存档闸、worldObjects() 都还指着那一趟。
    lilian_.reset();
    replaceScene(std::make_unique<TitleScene>());
}

core::Result<bool> Application::startNewJourney() {
    // 全新的一局：上一局（标题画面之前若有）留下的一切都不带。对话记录与台词本一并清掉，
    // 回看面板里不该翻出上一局的话。
    state_ = core::GameState{};
    lilian_.reset();
    dialogueLog_.clear();
    spokenKeys_.clear();
    pendingCards_.clear();
    itemToasts_.clear();
    resetGuideWatch();
    screenFade_ = 0.f;
    auto loaded = loadMap(kNewGameMap, std::string{});
    if (!loaded) return loaded;
    replaceScene(std::make_unique<WorldScene>());
    // 开篇卡压在世界层上面：卡片退场时底下就是韩家村。
    presentCards(openingCards(chapters_));
    return core::Result<bool>::success(true);
}

core::Result<bool> Application::continueJourney(const std::string& savePath) {
    auto save = io::loadGame(savePath);
    if (!save) return core::Result<bool>::failure(save.error);
    state_ = std::move(save.value);
    lilian_.reset();   // 档里没有历练状态：读档就是回到进去之前那一份
    // 存档里带着地图 id 与坐标：载图会把人摆到出生点，所以坐标要在载完之后放回去。
    // 载图失败时不压任何场景：标题画面还开着，把错说出来，玩家可以改选「新的旅程」
    // （那一条会把状态整个换掉，这里读进来的半截不会带过去）。
    const core::Point at = state_.position;
    // 朝向也一样：loadMap 按出生点的 facing 改写它。从前只放回了坐标，读档后人总朝着出生点那一向，
    // 存下来的朝向形同虚设（2026-11-20 第 7 章章首存档摆在包的正下方、面朝它，读回来却朝右对着蒲团）。
    const int facing = state_.facing;
    const std::string mapId = state_.mapId;   // 拷一份：loadMap 会改写 state_.mapId
    auto loaded = loadMap(mapId, std::string{});
    if (!loaded) return loaded;
    state_.position = at;
    state_.facing = facing;
    pendingCards_.clear();
    itemToasts_.clear();   // 读档前那一局拿到的东西，不该在读回来的这一局里弹出来
    resetGuideWatch();
    screenFade_ = 0.f;
    replaceScene(std::make_unique<WorldScene>());
    return core::Result<bool>::success(true);
}

void Application::presentCards(const std::vector<CardRequest>& cards) {
    if (cards.empty()) return;
    cardLog_.insert(cardLog_.end(), cards.begin(), cards.end());
    // 无头模式没人看：卡片不停留，也就不必压场景（ChapterCardScene 自己在无头下同样第一帧退场，
    // 那是给截图口之类直接压它的路子留的第二道）。
    if (headless_) return;
    std::vector<Card> resolved;
    resolved.reserve(cards.size());
    for (const CardRequest& request : cards) resolved.push_back(resolveCard(chapters_, request));
    pushScene(std::make_unique<ChapterCardScene>(std::move(resolved)));
}

void Application::flushChapterCards() {
    if (pendingCards_.empty()) return;
    // 三道闸，缺一道都会把卡片插在戏的半中间：
    //   · 脚本还在跑 / 还有命令等着回填——下一句台词、下一场仗还在后头；
    //   · 这一帧栈上还有要压、要弹的场景——等它定下来再看顶上是谁；
    //   · 顶上不是世界层——对话框、面板、战斗都还开着。
    if (scripts_ == nullptr || scripts_->isRunning() || commandPending_) return;
    if (pendingPops_ > 0 || !pendingPush_.empty()) return;
    if (scenes_.empty() || dynamic_cast<WorldScene*>(scenes_.back().get()) == nullptr) return;
    std::vector<CardRequest> cards = std::move(pendingCards_);
    pendingCards_.clear();
    presentCards(cards);
}

bool Application::systemSceneActive() const {
    return std::any_of(scenes_.begin(), scenes_.end(), [](const ScenePtr& scene) {
        return dynamic_cast<const MenuScene*>(scene.get()) != nullptr ||
               dynamic_cast<const TitleScene*>(scene.get()) != nullptr ||
               dynamic_cast<const SettingsScene*>(scene.get()) != nullptr ||
               dynamic_cast<const KeyConfigScene*>(scene.get()) != nullptr;
    });
}

void Application::setScreenFade(float level) {
    screenFade_ = std::clamp(level, 0.f, 1.f);
}

void Application::advanceDays(int days) {
    if (days <= 0) return;

    // 日历本身也拒绝倒退时间（负数会把所有「上次结算日」推到未来，凝液与
    // 灵田会集体停摆到追平为止），这里只信它返回的实际跨度。
    rules::Calendar calendar{state_.day};
    const int crossed = rules::advanceDays(calendar, days);
    if (crossed <= 0) return;
    state_.day = calendar.day;

    // 按天走的系统在这里逐个结算。新增一项（商店补货、遭遇计数跨天重置）
    // 就往下面加一行——这正是这个函数存在的全部理由。
    for (rules::SpiritField& field : state_.fields) {
        rules::growField(field, state_.day, kFieldDaysPerYear);
    }
    rules::refill(state_.bottle, state_.day, bottleChargeDays(state_.realm));
    restFor(crossed);
}

// 静养：日子过去了，伤也就养回来了。
//
// **在这一条出现之前，这个游戏没有任何一处把气血加回去。** 战斗里能吃药，
// 战斗之外一处也没有：打坐面板不回血（突破失败时还会砍半）、`advance_days`
// 一点也不补、没有客栈也没有休息点。于是韩立从第 1 章带的伤会一路带到第 14 章。
// 第 4 章独立校对（MEDIUM-3）点名的正是这件事：本章三场仗是一条单调下降的血线，
// 而它们中间隔着的日子，剧情上是三天备战。
//
// 口径取**每过去一天回一成**（按上限算，至少 1 点），不取「立刻回满」：
//   · 三天的空档只回三成——短间隔的消耗感留住了，`jinguang.lua` 末尾那三天备战
//     （二次整改前在 `kaizhan.lua`）正该是「缓过来一些」而不是「满血重来」：
//     曲魂从坊门那一仗剩的 1 点缓到 16 点。韩立在那一仗只掉了不到一成（120 剩 110），三天就回满，
//     这是按口径算出来的，不是口径失守；
//   · 一百五十天的空档自然回满——「过了半年伤还没好」那种荒唐没有了；
//   · 十天回满，对一个吃灵药长大的人是说得过去的数。
//
// 放在 `advanceDays` 里而不是另开一个「休息」入口，理由就是这个函数自己
// 注释里那一句：**按天走的系统在这里逐个结算**。日子过去了这件事只有一个出口。
//
// **`CultivationScene` 也走这个函数**，所以打坐兼作静养。这是有意的：
// 玩家打坐一个月回来满血，符合直觉，也不必再造一套休息机制。
// 它不会把战斗的消耗抹平——打坐要花掉真实的日子，而本章三场仗之间只有三天。
void Application::restFor(int days) {
    if (days <= 0) return;
    const auto heal = [days](int current, int cap) {
        if (cap <= 0 || current >= cap) return current;
        // 按百分数算（kRestPercentPerDay 的注释写了为什么不再当除数用）。
        // 先乘后除、在 long long 里算：上限一大，int 里先乘会溢出。
        const long long perDay =
            std::max<long long>(1, static_cast<long long>(cap) * kRestPercentPerDay / 100);
        const long long gained = current + perDay * days;
        return static_cast<int>(std::min<long long>(gained, cap));
    };
    state_.hp = heal(state_.hp, state_.maxHp);
    state_.mp = heal(state_.mp, state_.maxMp);
    // 同伴一样养伤。hp < 0 是「还没打过仗，按模板满血」的记号（PartyMember），
    // 不要把它当成一个受了重伤的人抬上来。
    for (core::PartyMember& member : state_.party) {
        if (member.hp < 0) continue;
        const core::RoleTemplate* role = data_.findRole(member.roleId);
        if (role == nullptr) continue;
        member.hp = heal(member.hp, role->maxHp);
    }
}

void Application::openFacility(const core::MapObject& facility) {
    const std::string kind = facility.property("kind");

    // map_spec 4.6 把设施的 require_flag 定成「解锁旗标」，引擎从前一直没判：第 6 章的制符桌
    // （require ch06.zhifu）在 9a 之前就开得出面板，玩家能先把 9a 要用的符纸丹砂画掉；药田
    //（require ch06.done）在 18b 之前按下去就地建出了灵田。旗标还是 0 就不开面板、不建灵田，
    // 只说一句为什么——与闸门拦人同一种提示（WorldScene::tryStep 的 deny_text_key），默不作声
    // 会让玩家以为按键坏了。只判这一处：走到这里的每一种 kind 都先过这道闸。
    if (const std::string need = facility.property("require_flag");
        !need.empty() && state_.flag(need) == 0) {
        showMessage("ui.facility.locked");
        return;
    }
    // 教学卡（第一期地基包 2.3）：第一次打开灵田、打坐、炼丹……压在面板上面讲一句。
    tutorialEvent("facility." + kind);

    if (kind == "meditate") {
        // 此地的打坐效率（map_spec 4.6 的 effectiveness，百分比；契约 docs/interfaces-p3-ch08.md 1.4）：
        // 灵眼之泉这类洞府灵脉写它，没写就是普通蒲团。非正数当缺省——门禁只放 100–300，这里只是不崩。
        const int sitePercent = propertyInt(facility.property("effectiveness"), kDefaultSitePercent);
        pushScene(std::make_unique<CultivationScene>(sitePercent > 0 ? sitePercent : kDefaultSitePercent));
        return;
    }
    if (kind == "field" && !facility.property("ref_id").empty()) {
        // ref_id 为空的灵田是地图写错了。不在这里 return：默不作声的按钮
        // 与「功用尚未开启」看起来一模一样，而后者至少能让人去查地图。
        const std::string id = facility.property("ref_id");
        if (state_.findField(id) == nullptr) {
            // 灵田的定义在地图上（ref_id + slots）。存档里还没有就地建出来：
            // 让玩家走到田边才发现「这块田不存在」是最差的结果，而把建田的
            // 责任推给剧情脚本，等于每加一块田都要记得补一段脚本。
            rules::SpiritField field;
            field.id = id;
            const int slots =
                std::max(1, propertyInt(facility.property("slots"), kDefaultFieldSlots));
            field.slots.resize(static_cast<std::size_t>(slots));
            state_.fields.push_back(std::move(field));
        }
        pushScene(std::make_unique<FieldScene>(id));
        return;
    }
    // ---- 炼制四艺（P3 第 4 章，契约 docs/interfaces-p3-ch04.md 第 3.3 节）----
    //
    // map_spec.md §4.6 早就把这四个 kind 留好了。四种走同一块面板：
    // 判定、成功率、失败策略全在 rules::Crafting 里按 CraftKind 分流，
    // game 层再照着分一次就成了两处真源。**本章只有 alchemy 真的有内容**
    //（data/recipes/alchemy 下六张方子），其余三类的方子也读得到、也列得出来，
    // 只是本章的地图上没有那三种炉子。
    //
    // 炉鼎品阶取 facility 的 grade 属性。**这条属性 map_spec.md §4.6 的属性表里
    // 还没有**——规范归主控与地图方，实现方不单方面去改别人的规范（G-6 的教训），
    // 所以要加的那一行连同理由写在 docs/interfaces-p3-ch04.md 第 3.3 节与交付报告里。
    // 在它落地之前，缺字段按 kDefaultCraftToolGrade（1 品，一口最普通的炉子）收，
    // 老地图一个字不改也能开炉。
    if (rules::CraftKind craftKind{};
        io::craftKindFromString(kind, craftKind)) {
        const int grade = propertyInt(facility.property("grade"), kDefaultCraftToolGrade);
        pushScene(std::make_unique<AlchemyScene>(craftKind, grade));
        return;
    }

    if (kind == "shop" && !facility.property("ref_id").empty()) {
        // 走到柜台前直接开店，与脚本里的 shop("店id") 进的是同一个场景。
        // ref_id 为空的店是地图写错了，落到下面的占位文案上——同灵田，
        // 默不作声与「功用尚未开启」看起来一模一样，而后者至少能让人去查地图。
        pushScene(std::make_unique<ShopScene>(facility.property("ref_id")));
        return;
    }

    if (kind == "board") {
        // 告示板 = 记事：眼下该做什么、走到这儿做过什么。
        pushScene(std::make_unique<BoardScene>());
        return;
    }

    if (kind == "lilian") {
        openLilianFacility(facility);
        return;
    }

    if (kind == "save") {
        // 存档点走的是与 F5 完全相同的那一个入口，不另开一套——两套存盘迟早
        // 一套带着某个字段、另一套不带。成败都如实说给玩家听。
        auto saved = quickSave();
        pushScene(std::make_unique<DialogueScene>(
            std::string{},
            saved ? "已将此刻记下：saves/quick.sav（重开时加 --load 读回）"
                  : ("记不下来：" + saved.error)));
        return;
    }

    // 剩下的 kind 尚未实现。默不作声会让玩家以为按键坏了，
    // 所以宁可明说「还没开」——占位文案至少是可证伪的。
    pushScene(std::make_unique<DialogueScene>(std::string{}, "此处的功用尚未开启。"));
}

std::vector<const core::PathAction*> Application::pathActionsFor(const std::string& npcName) const {
    // 地图 id 取 state_.mapId：loadMap 与读档都把它与 map_ 对齐，于是不必先问 map_ 在不在。
    return rules::pathActionsAt(data_.pathActions, state_, state_.mapId, npcName);
}

bool Application::openPathActions(const std::string& npcName) {
    if (pathActionsFor(npcName).empty()) return false;
    // 说话人取那个 npc 对象的 role_id（契约 5.4）。条目的（地图，对象名）加载器已对过账；
    // 对象上没写 role_id 的话说话人就空着，对话框按旁白画——显眼，一眼看得出地图漏了什么。
    std::string role;
    for (const core::MapObject& object : map_->objects) {
        if (object.type == "npc" && object.name == npcName) role = object.property("role_id");
    }
    pushScene(std::make_unique<PathActionScene>(npcName, std::move(role)));
    return true;
}

bool Application::applyPathEffects(const core::PathAction& action,
                                   const std::vector<rules::PathEffect>& effects,
                                   const std::string& speakerRole) {
    const std::size_t firstPush = pendingPush_.size();
    bool applied = true;
    for (const rules::PathEffect& effect : effects) {
        applied = applyPathEffect(action, effect, speakerRole);
        if (!applied) break;
    }
    // 这一批压的几层按清单次序演：栈是后进先出，不倒过来的话最后压的那层（那一仗、
    // 「记下了破绽」那句旁白）会抢在头一句话前面上屏。
    std::reverse(pendingPush_.begin() + static_cast<std::ptrdiff_t>(firstPush), pendingPush_.end());
    return applied;
}

bool Application::applyPathEffect(const core::PathAction& action, const rules::PathEffect& effect,
                                  const std::string& speakerRole) {
    using Kind = rules::PathEffect::Kind;
    // **不要加 default:**：规则层多一种效果时让编译器告警，而不是悄悄跳过。
    switch (effect.kind) {
        case Kind::Say:
            sayAs(speakerRole, effect.textKey);
            return true;
        case Kind::RevealWeakness: {
            // 契约 5.4 的两条路里取「此刻就写」：玩家打探完回头就去打，开战那一刻才并进去的话，
            // 这中间看状态、看存档都查不到这一笔。categoryFromName 认不出的名字门禁与加载器已挡掉。
            state_.learnWeaknesses(effect.reveal.roleId, core::categoryFromName(effect.reveal.category));
            // 让玩家知道记下了：一句旁白，不经 sayAs——这是界面在说话，不是那个 NPC。
            const std::string body = text("ui.path.reveal.lead") + speakerName(effect.reveal.roleId) +
                                     text("ui.path.reveal.tail") + effect.reveal.category;
            pushDialogueLine(std::string{}, body);
            pushScene(std::make_unique<DialogueScene>(std::string{}, body));
            return true;
        }
        case Kind::GiveItem:
            state_.addItem(effect.item.itemId, effect.item.count, effect.item.herbAge);
            announceItem(effect.item.itemId, effect.item.count);
            return true;
        case Kind::TakeItem:
            return state_.removeItem(effect.item.itemId, effect.item.count);
        case Kind::SetFlag:
            state_.setFlag(effect.flag);
            return true;
        case Kind::StartBattle:
            // 收场按战果施加 challengeResultEffects（逃也算负：won 只认 BattlePhase::Won）。
            // 输了不 game over：编成 defeat_is_fatal 假，BattleScene 已按气血至少 1 放回来，
            // 而这里根本没有脚本去读 "lost" 走 game_over。
            startBattle(effect.battleId, [this, &action, speakerRole](const script::CommandResult& result) {
                applyPathEffects(action, rules::challengeResultEffects(action, result.battleWon), speakerRole);
            });
            return true;
        case Kind::GainCultivation: {
            // 与 grantBattleReward 同口径：只加修为、不动境界，饱和加法。
            const long long sum = static_cast<long long>(state_.cultivation) + effect.amount;
            state_.cultivation =
                static_cast<int>(std::min<long long>(sum, std::numeric_limits<int>::max()));
            return true;
        }
    }
    return true;
}

namespace {

// 站在这一格上、此刻开着的遭遇区；没有返回 nullptr。require_flag 是本作给 map_spec 4.5 补的
// 可选属性（docs/interfaces-octo-encounters.md 第 3 节）：谷外要等教学那一仗打完才有野兽。
[[nodiscard]] bool zoneOpen(const core::MapObject& zone, const core::GameState& state) {
    const std::string need = zone.property("require_flag");
    return need.empty() || state.flag(need) != 0;
}

[[nodiscard]] const core::MapObject* openZoneAt(const core::TileMap& map, const core::GameState& state,
                                                core::Point cell) {
    const core::MapObject* zone = map.objectAt(cell, "encounter");
    return zone != nullptr && zoneOpen(*zone, state) ? zone : nullptr;
}

// 规则层要调用方给种子（同种子 + 同输入必得同结果）。拿「哪一天、今天第几场、这一段走了
// 几步、站在哪一格」揉一个：同一份存档走同一条路必遇同一场，换一条路、换一天就不同。
[[nodiscard]] std::uint32_t encounterSeed(const core::GameState& state) {
    const auto u = [](int v) { return static_cast<std::uint32_t>(v); };
    return u(state.day) * 2654435761u ^ u(state.encounter.triggeredToday) * 40503u ^
           u(state.encounter.stepsSinceLast) * 2246822519u ^ u(state.position.x) * 73856093u ^
           u(state.position.y) * 19349663u;
}

}  // namespace

bool Application::stepEncounters() {
    if (!encountersEnabled_ || map_ == nullptr) return false;
    const core::MapObject* zone = openZoneAt(*map_, state_, state_.position);
    if (zone == nullptr) return false;
    const auto it = encounterTables_.find(zone->property("table_id"));
    if (it == encounterTables_.end()) return false;   // 门禁对过账（validate.py 规则 27），走不到

    // 区（地图对象）定疏密，表定遇上谁：步数区间与每日上限按区上写的，区没写 daily_cap 才用表的。
    rules::EncounterTable table = it->second;
    table.stepsMin = std::stoi(zone->property("steps_min"));
    table.stepsMax = std::stoi(zone->property("steps_max"));
    if (!zone->property("daily_cap").empty()) table.dailyCap = std::stoi(zone->property("daily_cap"));

    const std::string battleId =
        rules::step(table, state_.encounter, state_.realm, state_.day, encounterSeed(state_));
    if (battleId.empty()) return false;
    if (engine_) engine_->playSfx("encounter");
    startBattle(battleId, [this](const script::CommandResult& result) { finishEncounter(result); });
    return true;
}

void Application::finishEncounter(const script::CommandResult& result) {
    // 胜：编成奖励 BattleScene::finish 已发；逃、对方跑了：什么也不发、什么也不说。
    // 负：不 game over。气血已按至少 1 放回来，人就地不动（不挪回出生点：挪过去的那一格可能
    // 正压在一个 enter 触发器上，替玩家踩响一段剧情）；计数器刚清零，离下一场至少还有
    // steps_min 步，够他走出这片野地或者歇口气。
    if (result.code == "lost") showMessage("ui.encounter.lost");
}

int Application::dangerStars() const {
    if (map_ == nullptr) return 0;
    int stars = 0;
    for (const core::MapObject& object : map_->objects) {
        if (object.type != "encounter" || !zoneOpen(object, state_)) continue;
        const auto it = encounterTables_.find(object.property("table_id"));
        if (it == encounterTables_.end()) continue;
        stars = std::max(stars, encounterDangerStars(it->second, state_.realm, battles_, data_));
    }
    return stars;
}

std::string Application::worldBgm() const {
    if (!bgmOverride_.empty()) return bgmOverride_;
    return map_ == nullptr ? std::string{} : map_->bgm;
}

void Application::sayAs(const std::string& roleId, const std::string& textKey, const std::string& face) {
    // 先记下 key 再建场景：建场景会把 key 换成正文，之后就查不到它了。
    if (spokenKeys_.size() >= kSpokenLogCap) spokenKeys_.erase(spokenKeys_.begin());
    spokenKeys_.push_back(textKey);
    const std::string speaker = roleId.empty() ? std::string{} : speakerName(roleId);
    const std::string body = text(textKey);
    // 同一行同时进两份记录：key 那份给测试，这份给玩家回看。
    pushDialogueLine(speaker, body, roleId, face);
    auto scene = std::make_unique<DialogueScene>(speaker, body);
    // 头像要的是角色 id 与表情，不是名字（伙伴改版 3.9）；画不画由对话框自己定。
    scene->setPortrait(roleId, face);
    pushScene(std::move(scene));
}

core::Result<bool> Application::startEvent(const std::string& scriptPath) {
    if (scriptPath.empty()) return core::Result<bool>::failure("脚本路径为空");
    if (scripts_ == nullptr) return core::Result<bool>::failure("脚本宿主未初始化");
    return scripts_->startEvent(scriptPath);
}

void Application::startBattle(const std::string& battleId, BattleOutcome onFinish) {
    startBattle(BattleStart{battleId}, std::move(onFinish));
}

void Application::startBattle(const BattleStart& start, BattleOutcome onFinish) {
    BattleStart actual = start;
    // 历练里按韩立的境界拼档的后缀（契约 3.12「难度」）。只在这里拼：世界逻辑递进来的是 foe.battle 原样。
    if (lilian_) {
        const core::LilianSite* site = lilianSite(lilian_->siteId);
        const int tier = site == nullptr ? 0 : rules::lilianTier(*site, state_.realm);
        actual.battleId = rules::tieredBattleId(start.battleId, tier);
        // 拼出来的那一场不存在：**不开仗、不调回调**。压下去的话 BattleScene 查不到编成会退回内置的
        // 探针杂兵战，打赢了世界逻辑的回调还会把明雷记成已清。进门时已按这一档把每个 foe 的编成核过
        // （enterLilian），走到这里只能是境界在里面变了、或别的路递进来一个没核过的 id。
        if (battleSetup(actual.battleId) == nullptr) {
            std::fprintf(stderr, "[lilian] 历练 %s 里要开的编成 %s 不存在，这一仗不开\n",
                         lilian_->siteId.c_str(), actual.battleId.c_str());
            return;
        }
    }
    battleOutcome_ = std::move(onFinish);
    battleStart_ = actual;
    pushScene(std::make_unique<BattleScene>(actual.battleId));
}

const core::LilianSite* Application::lilianSite(const std::string& siteId) const {
    const auto it = lilianSites_.find(siteId);
    return it == lilianSites_.end() ? nullptr : &it->second;
}

namespace {

// 进一趟历练之前读这处历练地的每一张图，做两件事，都只读文件、不动任何状态：
//   · chance 抽签（契约 3.12「每趟不同」）：没抽中的明雷记成已清、宝箱记成已开，记进这一趟的账；
//   · 核对这一档每个 foe 的编成（foe.battle 拼上 tier 的后缀）都在——缺一场就进不去，
//     免得进去之后撞上它才发现（startBattle 那时只能不开这一仗）。
// 任何一张图读不进来、缺任何一场编成，整个失败：不拿半张账进去。
[[nodiscard]] core::Result<core::WorldObjectState> rollLilianObjects(
    const std::string& mapsRoot, const std::vector<std::string>& maps, const std::string& siteId, int day,
    int tier, const std::function<bool(const std::string&)>& battleExists) {
    using Out = core::Result<core::WorldObjectState>;
    core::WorldObjectState objects;
    const std::uint32_t seed = rules::lilianSeed(day, siteId);
    for (const std::string& mapId : maps) {
        auto loaded = io::loadTileMap(mapsRoot + "/" + mapId + ".tmj");
        if (!loaded) return Out::failure(loaded.error);
        for (const core::MapObject& object : loaded.value.objects) {
            if (object.type != "foe" && object.type != "chest") continue;
            const std::string id = object.property("id");
            if (object.type == "foe" && !object.property("battle").empty()) {
                const std::string battle = rules::tieredBattleId(object.property("battle"), tier);
                if (!battleExists(battle)) {
                    return Out::failure("缺编成 " + battle + "（" + mapId + " 上的 " + object.name + "）");
                }
            }
            const std::string raw = object.property("chance");
            if (raw.empty() || id.empty()) continue;   // 没写 chance 的每趟都在；没 id 的门禁报
            // chance 是 1–99 的整数（出现的百分比）。写坏了不吞：打一行警告，按「一定出现」办——
            // 少一只明雷玩家看不出来，多一只看得出来。
            const int chance = propertyInt(raw, -1);
            if (chance < 1 || chance > 99) {
                std::fprintf(stderr, "[lilian] %s 上 %s 的 chance（%s）不是 1–99 的整数，按一定出现办\n",
                             mapId.c_str(), id.c_str(), raw.c_str());
                continue;
            }
            if (rules::lilianChanceShows(seed, id, chance)) continue;
            if (object.type == "foe") {
                objects.clearedFoes[id] = day;
            } else {
                objects.openedChests.insert(id);
            }
        }
    }
    return Out::success(std::move(objects));
}

}  // namespace

core::Result<bool> Application::enterLilian(const LilianEntry& entry) {
    if (lilian_) return core::Result<bool>::failure("已经在历练里了");
    // 先抽签、核编成：只读文件，失败了什么都还没动。没登记的历练地只读入口图、不分档。
    const core::LilianSite* site = lilianSite(entry.siteId);
    const std::vector<std::string> maps = site != nullptr ? site->maps : std::vector<std::string>{entry.mapId};
    const int tier = site == nullptr ? 0 : rules::lilianTier(*site, state_.realm);
    auto rolled = rollLilianObjects(assetRoot_ + "/maps", maps, entry.siteId, state_.day, tier,
                                    [this](const std::string& id) { return battleSetup(id) != nullptr; });
    if (!rolled) return core::Result<bool>::failure(rolled.error);

    LilianTrip trip{entry.siteId, state_.mapId, state_.position, state_.facing, state_.party,
                    std::move(rolled.value)};
    std::vector<core::PartyMember> members;
    for (const std::string& roleId : entry.party) members.push_back(core::PartyMember{roleId, -1, true});
    // 原图留在内存里：入口图万一载不进来（抽签刚读过它，只有文件在这一刹那变了才会），直接换回去，
    // 不从磁盘重读——于是没有「原图也读不回来」这一支，也不会出现人不在历练里、却站在历练图上的局面。
    std::unique_ptr<core::TileMap> previous = std::move(map_);
    state_.party = std::move(members);
    auto loaded = loadMap(entry.mapId, entry.spawn);
    if (!loaded) {
        map_ = std::move(previous);
        state_.mapId = trip.mapId;
        state_.position = trip.position;
        state_.facing = trip.facing;
        state_.party = std::move(trip.party);
        const std::string bgm = worldBgm();
        if (engine_ && !bgm.empty() && bgm != "none") engine_->playBgm(bgm);
        return loaded;
    }
    lilian_ = std::move(trip);
    return core::Result<bool>::success(true);
}

void Application::openLilianFacility(const core::MapObject& facility) {
    // 历练里：这就是出口（契约 3.12「出来：走到出口」）。不另设一种对象，世界逻辑不必认识历练。
    if (lilian_) {
        auto left = leaveLilian();
        if (!left) {
            const std::string said = "回不去：" + left.error;
            pushDialogueLine(std::string{}, said);
            pushScene(std::make_unique<DialogueScene>(std::string{}, said));
            return;
        }
        showMessage("ui.lilian.back");
        return;
    }
    const std::string siteId = facility.property("site");
    const core::LilianSite* site = lilianSite(siteId);
    if (site == nullptr) {
        // 地图写错了 site（门禁查得到，这里只是不崩）：与「功用尚未开启」同一个态度，再留一行日志。
        std::fprintf(stderr, "[lilian] 设施 %s 的 site（%s）不在 data/lilian 里\n", facility.name.c_str(),
                     siteId.c_str());
        showMessage("ui.lilian.unknown");
        return;
    }
    if (state_.flag(site->unlockFlag) == 0) {
        showMessage("ui.lilian.locked");
        return;
    }
    pushScene(std::make_unique<LilianPartyScene>(site->id));
}

core::Result<bool> Application::leaveLilian() {
    if (!lilian_) return core::Result<bool>::failure("不在历练里");
    // 先回原图：回不去就仍算在历练里（队伍、存档闸都不动），而不是队伍换回来了人却留在历练图上。
    auto loaded = loadMap(lilian_->mapId, std::string{});
    if (!loaded) return loaded;
    LilianTrip trip = std::move(*lilian_);
    lilian_.reset();
    state_.party = std::move(trip.party);
    state_.position = trip.position;
    state_.facing = trip.facing;
    advanceDays(1);
    return core::Result<bool>::success(true);
}

bool Application::inLilian() const { return lilian_.has_value(); }

core::WorldObjectState& Application::worldObjects() { return lilian_ ? lilian_->objects : state_.worldObjects; }

void Application::completeCommand(const script::CommandResult& result) {
    // 以回调开的那一仗在收场（startBattle）：改走登记的回调，脚本那边没有人在等。
    // 认「栈顶是不是一场仗」而不是「有没有登记」：压在那一仗上面的邀战对话框收起时也会
    // 调到这里，那一下不是战果。
    if (battleOutcome_ && dynamic_cast<BattleScene*>(topScene()) != nullptr) {
        const BattleOutcome outcome = std::move(battleOutcome_);
        battleOutcome_ = nullptr;
        outcome(result);
        return;
    }
    if (!commandPending_) return;
    commandPending_ = false;
    scripts_->resumeWith(result);
}

void Application::pushDialogueLine(std::string speaker, std::string body, std::string roleId, std::string face) {
    if (body.empty()) return;
    if (dialogueLog_.size() >= kDialogueLogCap) dialogueLog_.erase(dialogueLog_.begin());
    dialogueLog_.push_back(SpokenLine{std::move(speaker), std::move(body), std::move(roleId), std::move(face)});
}

void Application::showMessage(const std::string& textKey) {
    if (textKey.empty()) return;
    // 空说话人 = 旁白，对话框不画名字框。
    const std::string body = text(textKey);
    pushDialogueLine(std::string{}, body);
    pushScene(std::make_unique<DialogueScene>(std::string{}, body));
}

bool Application::dispatch(const script::Command& command, script::CommandResult& outcome) {
    using script::CommandKind;

    switch (command.kind) {
        case CommandKind::Talk:
            // 与路径行动的话同一个入口（sayAs），口径只有一处定义。表情没给就是缺省那一个。
            sayAs(command.b, command.a, command.c.empty() ? std::string(script::kDefaultFace) : command.c);
            return false;
        case CommandKind::Choice: {
            std::vector<std::string> options;
            options.reserve(command.options.size());
            for (const std::string& key : command.options) options.push_back(text(key));
            // 脚本的 choice{} 只给选项、不给问句（api.lua），框里于是一直是空的，玩家对着一片
            // 空白做选择。现在把刚说完的那一句连同说话人的名签留在框里：问的是什么一眼看得见。
            // 那一句刚才已经逐字读过，这回直接全显，不再滚一遍。
            std::string speaker;
            std::string prompt = text(command.a);
            SpokenLine portrait;
            if (command.a.empty() && !dialogueLog_.empty()) {
                speaker = dialogueLog_.back().speaker;
                prompt = dialogueLog_.back().body;
                portrait = dialogueLog_.back();
            }
            auto scene = std::make_unique<DialogueScene>(speaker, prompt);
            scene->setPortrait(portrait.roleId, portrait.face);
            scene->setChoices(std::move(options));
            scene->revealAtOnce();
            pushScene(std::move(scene));
            return false;
        }
        case CommandKind::Battle: {
            battleStart_ = BattleStart{command.a};
            pushScene(std::make_unique<BattleScene>(command.a));
            return false;
        }
        case CommandKind::Teleport:
            loadMap(command.a, std::string{});
            if (command.x != 0 || command.y != 0) {
                state_.position = core::Point{command.x, command.y};
            }
            return true;
        case CommandKind::SetFlag: {
            // x 就是值，不要在这里替脚本兜底。省略参数时的默认值由 Lua 侧的
            // api.lua 补好，命令到这里时已经是确定的数值；在这里把 0 改写成 1,
            // 只会让脚本永远无法显式清掉一个旗标。
            const int before = state_.flag(command.a);
            state_.setFlag(command.a, command.x);
            // 章节卡：某一章的完成旗标刚从 0 变成非 0，就记下「第 N 章　终」接下一章的开篇。
            // 只记不播——这一场脚本多半还有话没说完，等它演完、回到行走时再播
            // （flushChapterCards）。剧情脚本因此一句不用改。
            const std::vector<CardRequest> cards =
                cardsForFlagChange(chapters_, command.a, before, command.x);
            pendingCards_.insert(pendingCards_.end(), cards.begin(), cards.end());
            return true;
        }
        case CommandKind::GiveItem:
            state_.addItem(command.a, command.x <= 0 ? 1 : command.x, command.y);
            // 主线里给的东西同样弹一张得物卡片（docs/ch07-playtest-ui.md 第 2 节）：卡片不挡脚本，对白照常往下走。
            announceItem(command.a, command.x <= 0 ? 1 : command.x);
            return true;
        case CommandKind::TakeItem: {
            // 东西不够就如实报失败，让脚本能走「你没有这个」的分支。
            const int count = command.x <= 0 ? 1 : command.x;
            // y 是灵草年份，与 GiveItem 同一个槽位。给了年份就必须精确扣那一堆：
            // removeItem 明写「先扣年份低的，把高年份留给玩家」，于是章末那场
            // take(黄精, 1, 44) 扣走的是背包里最便宜的一株种苗，四十四年那株
            // 原封不动留着——钱照拿、药还在，商店落地后可以再卖一次。
            //
            // y <= 0 仍走 removeItem，这不是兜底而是两种不同的请求：
            //   * take(id, n)       —— 「扣 n 件，哪一堆都行」。api.lua 把省略
            //     的年份补成 0，非灵草也一律传 0，二者在命令里长得一模一样，
            //     无从分辨，所以 0 只能解释成「没指定」。zhitong.lua 的
            //     take("herb_huangjing_cao", 1) 要的正是这个：交一株黄精抵人情，
            //     哪一株都算数；解释成「正好 0 年那一株」会让手上只剩足年药的
            //     玩家在段四卡死。
            //   * take(id, n, age) —— 「扣那一堆里的 n 件」，年份对不上就失败。
            // 而「确实只想扣 0 年那一株」并不会因此写不出来：removeItem 本就先
            // 扣年份最低的，有 0 年那堆时结果与精确扣完全一致。
            outcome.ok = command.y > 0
                             ? state_.removeItemOfAge(command.a, count, command.y)
                             : state_.removeItem(command.a, count);
            return true;
        }
        case CommandKind::TakeItemAged: {
            // take_aged（契约 docs/interfaces-p3-ch07.md 4.3）：从年份不低于 y 的堆里扣 x 件，
            // 够格里年份低的先扣；不够就一件不扣、如实报失败。三种扣法各管一样：TakeItem 不给年份
            // 是「哪一堆都行」、给了是「恰好这一堆」，这一条是「够格的里挑」——马师伯收「四十四年以上」
            // 的黄精，前两种一个会把刚种下的苗交上去，一个对不上浇到 1536 年的千年药。
            const int count = command.x <= 0 ? 1 : command.x;
            outcome.ok = state_.removeItemAtLeastAge(command.a, count, command.y);
            return true;
        }
        case CommandKind::PlaySfx:
            if (engine_) engine_->playSfx(command.a);
            return true;
        case CommandKind::PlayBgm: {
            // bgm("map") 撤掉点播、放回地图曲；别的 id 点播那一首，换图也不撤（worldBgm）。
            // 撤掉之后地图属性写着 none 的话就是静音——与 loadMap 同一个口径。
            bgmOverride_ = command.a == script::kMapBgm ? std::string{} : command.a;
            const std::string bgm = worldBgm();
            if (engine_) {
                if (bgm.empty() || bgm == "none") {
                    engine_->stopBgm();
                } else {
                    engine_->playBgm(bgm);
                }
            }
            return true;
        }
        case CommandKind::GameOver:
            requestQuit();
            return true;
        case CommandKind::Ending:
            // 结局卡：与章节卡同一种黑底墨金的卡，标题 + 正文，等玩家按确认后淡出、回填。
            pushScene(ChapterCardScene::ending(text(command.a), text(command.b)));
            return false;
        case CommandKind::Shop:
            // 开店。店 id 查不到时照样压场景：面板会开着、只是什么也买不到，
            // 并把失败原样回填给脚本（见 ShopScene::leave）。在这里直接
            // return true 吞掉，玩家看到的就是「跟药商说完话什么也没发生」。
            pushScene(std::make_unique<ShopScene>(command.a));
            return false;
        case CommandKind::FadeOut:
        case CommandKind::FadeIn:
        case CommandKind::Wait: {
            // 从前这三条是空操作。现在做成真的：黑幕浓度记在这里，FadeScene 按命令给的
            // 毫秒把它推过去，推完回填（黑幕为什么不归那个场景画，见 FadeScene.h）。
            const FadeScene::Kind kind = command.kind == CommandKind::FadeOut ? FadeScene::Kind::Out
                                         : command.kind == CommandKind::FadeIn
                                             ? FadeScene::Kind::In
                                             : FadeScene::Kind::Wait;
            if (headless_) {
                // 无头没人看，当场到终值、当场回填：与改造前一样一帧也不多等，
                // 测试与无头 bot 的节奏不受影响。
                setScreenFade(FadeScene::levelAt(kind, screenFade_, 0.0, 0.0));
                return true;
            }
            pushScene(std::make_unique<FadeScene>(kind, command.x));
            return false;
        }

        // ---- P3 第 2 章增补 ----
        case CommandKind::AdvanceDays:
            // 转调唯一的时间入口，不在这里另写一份结算：漏掉其中一项（比如
            // 灵田长了而绿液没凝）在存档里是看不出来的，要到几小时后才暴露。
            // x <= 0 由 advanceDays 自己吞掉，脚本传 0 不算错误。
            advanceDays(std::min(command.x, kMaxScriptAdvanceDays));
            return true;
        case CommandKind::BottleGrant:
            // lastChargeDay 必须拨到当日。它默认 0，不拨的话拾瓶那一刻就按
            // 「已过 state_.day 天」一次性补满绿液，原著「第八日方得一滴」
            // 当场作废——这是本章最容易写错的一处。
            //
            // 已经有瓶子时整条跳过而不是只跳过 owned：重置计时会把玩家攒了
            // 好几天的零头抹掉，而重复调用 grant 在剧情回放里是正常的。
            if (!state_.bottle.owned) {
                state_.bottle.owned = true;
                state_.bottle.lastChargeDay = state_.day;
            }
            return true;
        case CommandKind::BottleUnlockMature:
            // 幂等，且刻意不隐含 owned：没瓶子却会催熟是个无意义状态，
            // 在这里替脚本兜底只会把写错的调用顺序藏起来。
            state_.bottle.matureKnown = true;
            return true;
        case CommandKind::FieldUnlock: {
            if (command.a.empty() || command.x <= 0) {
                // 如实报失败，让脚本能分支，也让写错的参数在测试里看得见。
                outcome.ok = false;
                return true;
            }
            const int slots = std::min(command.x, kMaxFieldSlots);
            if (rules::SpiritField* existing = state_.findField(command.a)) {
                // 只补足，绝不重建 slots：重建会把玩家种了几年的药一次清空，
                // 而这条命令在剧情里完全可能被重复走到（回忆、读档后重放）。
                if (static_cast<int>(existing->slots.size()) < slots) {
                    existing->slots.resize(static_cast<std::size_t>(slots));
                }
                return true;
            }
            rules::SpiritField field;
            field.id = command.a;
            field.slots.resize(static_cast<std::size_t>(slots));
            state_.fields.push_back(std::move(field));
            return true;
        }

        // ---- P3 事后追加：把绿液用掉的两条（契约第 6 节）----
        case CommandKind::BottleSpend: {
            // 倒出绿液本身，不涉及年份：第 2 章试药那一碗就是这么没的
            // （倒进碗里稀释了喂兔子）。这一步发生时催熟之能还没被发现，
            // 走 matureHerb 只会拿到「尚不知瓶中绿液有催熟之能」而一滴不扣。
            //
            // 减法仍在 rules::spendDrops 里，不在这里写第二份。
            const int count = command.x <= 0 ? 1 : command.x;
            outcome.ok = rules::spendDrops(state_.bottle, count);
            if (!outcome.ok) outcome.code = matureFailureCode(rules::MatureFailure::NoDrops);
            outcome.value = state_.bottle.drops;
            return true;
        }
        case CommandKind::BottleMature: {
            // 用一滴绿液催熟背包里的一株灵草。年份跃升、上限、四种失败理由
            // 全部由 rules::matureHerb 给，这里一个数也不自己算——脚本里写死
            // 「浇三滴得四十四年」的那一刻，规则与文案就开始各走各的。
            const int age = std::max(command.y, 0);
            outcome.value = age;
            if (command.a.empty() || state_.itemCountOfAge(command.a, age) < 1) {
                // 手上没有这一堆。这条不是 matureHerb 的失败原因（它不认识
                // 背包），但脚本同样要分得清「浇不动」和「根本没这株药」。
                outcome.ok = false;
                outcome.code = "no_item";
                return true;
            }

            const rules::MatureResult result =
                rules::matureHerb(state_.bottle, age, herbMaxAgeOf(data_, command.a));
            outcome.ok = result.ok;
            outcome.value = result.newAge;
            if (!result.ok) {
                outcome.code = matureFailureCode(result.failure);
                return true;
            }

            // 先扣那一堆再放回新年份的一株。这里同样不能走 removeItem：
            // 它会扣掉年份最低的那一堆，于是玩家浇的是这一株、变年份的是
            // 另一株（与 TakeItem 那个坑是同一个坑）。
            //
            // 扣不掉只可能是上面那次清点与这里对不上，真发生了就把绿液退回去，
            // 绝不留下「液也没了、药也没变」的账。
            //
            // 这一支当前不可达（清点与扣除之间只隔着 matureHerb，而它只碰
            // bottle、不碰 bag），留着是防将来有人在中间插一脚：那时漏掉的
            // 不会是编译错误，而是玩家的一滴绿液。
            if (!state_.removeItemOfAge(command.a, 1, age)) {
                ++state_.bottle.drops;
                outcome.ok = false;
                outcome.code = "no_item";
                outcome.value = age;
                return true;
            }
            state_.addItem(command.a, 1, result.newAge);
            return true;
        }

        // ---- P3 第 3 章增补：队伍（契约 docs/interfaces-p3-ch03.md 第 1.3 节）----
        case CommandKind::PartyAdd: {
            // 角色 id 必须在 data/roles 里查得到，查不到如实回填失败。
            //
            // 不静默收下的理由：一个拼错的 id 进了队，到战斗里会变成
            // BattleScene 找不到模板而被跳过的一个空位——玩家看到的是
            // 「同伴没上场」，而那时已经很难追回是哪一句脚本写错了。
            // 这一道判定只能在 game 层做：core 的 GameState 拿不到 GameData。
            if (command.a.empty() || data_.findRole(command.a) == nullptr) {
                outcome.ok = false;
                outcome.code = "no_role";
                return true;
            }
            // 幂等：已在队里时 partyAdd 返回 false，但对脚本来说这不是失败
            //（「让他入队」这件事的结果已经成立），所以 ok 保持 true。
            (void)state_.partyAdd(command.a);
            outcome.ok = true;
            return true;
        }
        case CommandKind::PartyRemove:
            // 与 take() 同一个口径：没扣掉东西就如实报 false，让脚本能分辨
            // 「他走了」与「他本来就不在队里」。
            outcome.ok = state_.partyRemove(command.a);
            if (!outcome.ok) outcome.code = "not_in_party";
            return true;

        // ---- P3 第 4 章增补：已习得法术（契约 docs/interfaces-p3-ch04.md 第 1.3 节）----
        case CommandKind::MagicLearn: {
            // 法术 id 必须在 data/magics 里查得到，查不到如实回填失败。
            //
            // 这是契约点名要的一条：**拼错的法术 id 不许静默收下**。收下之后
            // 它会一路存进档、再进战斗单位的 magics，然后在 buildMagicItems 里
            // 被 data.findMagic 悄悄跳过——玩家看到的是一个空菜单，
            // 而那时已经很难追回是哪一句脚本写错了。
            // 这道判定只能在 game 层做：core 的 GameState 拿不到 GameData，
            // script 的 __host 也拿不到（与 PartyAdd 同一个道理）。
            if (command.a.empty() || data_.findMagic(command.a) == nullptr) {
                outcome.ok = false;
                outcome.code = "no_magic";
                return true;
            }
            // 幂等：已经会了时 learnMagic 返回 false，但对脚本来说这不是失败
            //（「让他学会」这件事的结果已经成立），所以 ok 保持 true。
            // 与 PartyAdd 一字不差的口径。
            (void)state_.learnMagic(command.a);
            outcome.ok = true;
            return true;
        }
        case CommandKind::MagicForget:
            // 与 PartyRemove 同一个口径：没忘掉就如实报 false，让脚本能分辨
            // 「他忘了」与「他本来就不会」。
            //
            // **不查 data**：忘掉一门 data 里已经被删掉的法术仍然该成功——
            // 那正是把存档里的悬空 id 清出去的唯一办法。
            outcome.ok = state_.forgetMagic(command.a);
            if (!outcome.ok) outcome.code = "not_learned";
            return true;

        // ---- P3 第 4 章增补：提升境界（契约 docs/interfaces-p3-ch04.md 第 6 节）----
        case CommandKind::RealmAdvance: {
            const rules::Realm target = rules::fromValue(command.x);
            // 非法编号一律回绝。口径与 __host.realm_at_least 一致：脚本传错数字
            // 不该意外把主角送进一个 enum 空段里——那种存档读回来 isValid 判 false，
            // 气血法力全退回凡人档，而现场看不出是哪一句脚本干的。
            if (!rules::isValid(target)) {
                outcome.ok = false;
                outcome.code = "no_realm";
                return true;
            }

            const int current = rules::toValue(state_.realm);
            // **只许升不许降。** 跌落走 RealmDemote，按目标境界重算上限与修为。
            //
            // 收口选的是「ok 回答的是**这条命令要的那个结果成不成立**」，
            // 与 PartyAdd 逐条对齐：
            //   * 目标 == 当前：他已经在那个境界上了，「让他到炼气七层」这件事
            //     的结果**已经成立** → ok = true，什么也不做。与 PartyAdd 的
            //     「已在队里 → partyAdd 返回 false，但 ok 保持 true」一字不差，
            //     剧情回放、读档重跑同一段都不会因此走进失败分支。
            //   * 目标 < 当前：结果**不成立**，而这条命令也不打算让它成立
            //     （不许降），于是如实回绝 → ok = false，code = "not_higher"。
            //     这里不跟着 PartyAdd 判 true：那会让「脚本把境界写小了」
            //     与「他本来就更高」在脚本侧长得一模一样，而前者是个 bug。
            if (command.x < current) {
                outcome.ok = false;
                outcome.code = "not_higher";
                return true;
            }
            // 剧情升境**顺带把上限抬到目标层**（契约 docs/interfaces-p3-script.md 第 7 节）。
            // 升境本身不受上限约束——它就是剧情在说话；而说完之后上限若还停在原处，
            // 面板上就会出现「他已经是五层，上限却是三层」这种自相矛盾的存档。
            // 等于当前时同样抬：「他就在这一层」这件事成立，上限至少到这里。
            raiseRealmCap(target);
            if (command.x == current) {
                outcome.ok = true;
                return true;
            }

            state_.realm = target;
            // 升上去之后上限必须跟着长，否则这条命令只改了一个编号，
            // 而「修为只是一道剧情闸门，不是变强」正是 G-10 与 CultivationScene
            // 那一段注释要终结的东西。
            //
            // 当前值按**差额上抬**（rules::liftToFloor），不补满。三条理由：
            //   1. 突破这件事在打坐面板里已经有一套口径了
            //      （CultivationScene::applyRealmAttributes 走的就是 liftToFloor），
            //      同一件事经脚本发生与经面板发生必须是同一个结果；界面各算一套
            //      是数值失控最常见的来源。
            //   2. 补满会让这条命令顺带变成一次免费的全恢复。脚本在大战前写一句
            //      「他修为涨了」，玩家就白得一次满血满蓝——而 BattleScene::finish
            //      会把气血写回存档，这个差别是玩家看得见的。
            //   3. 玩家那句直觉「突破之后是满的」照样成立：突破的常态是打坐时，
            //      那时本来就是满的，而 liftToFloor 把当前值抬同样的量，满血进满血出。
            //      只有带伤突破的人会带着那道伤出来——突破不是疗伤。
            const rules::Vitals hp =
                rules::liftToFloor({state_.hp, state_.maxHp}, rules::realmMaxHp(target));
            const rules::Vitals mp =
                rules::liftToFloor({state_.mp, state_.maxMp}, rules::realmMaxMp(target));
            state_.hp = hp.current;
            state_.maxHp = hp.max;
            state_.mp = mp.current;
            state_.maxMp = mp.max;
            // 瓶子容量的下限跟着大境界走（契约 docs/interfaces-p3-ch07.md 3.3 第 1 处；另两处是
            // CultivationScene 的 applyRealmAttributes 与 SaveFile 读档，同一个函数）。只补不削，不送液。
            state_.bottle.capacity =
                std::max(state_.bottle.capacity, rules::bottleCapacityFloor(rules::tierOf(target)));
            // 只在真升上去的这一支响（终审 M-3）：原地不动、往回拨都已在上面返回，不该有动静。
            engine_->playSfx("realm_up");
            outcome.ok = true;
            return true;
        }

        // ---- 第一期地基包：教学卡（docs/ch07-foundation.md 2.3）----
        case CommandKind::Tutorial: {
            // 不等卡片弹出：卡片等这段戏演完才弹（flushTutorials），脚本照常往下走。
            outcome.code = requestTutorial(command.a);
            outcome.ok = outcome.code.empty();
            return true;
        }

        // ---- 伙伴改版：默契加分（docs/interfaces-companions.md 2.2、3.8）----
        case CommandKind::BondAdd: {
            // 伙伴 id 必须在 data/companions 里查得到：拼错的 id 记下一笔谁也读不到的账，
            // 到要出追击、合招时才发现「怎么一直不熟」。
            if (data_.companions.count(command.a) == 0) {
                outcome.ok = false;
                outcome.code = "no_companion";
                return true;
            }
            // 只增不减（契约 2.2）。先判再记账：负数不该在存档里留下一条全 0 的空账。
            if (command.x < 0) {
                outcome.ok = false;
                outcome.code = "negative";
                return true;
            }
            // 脚本自己保证只演一次，这里不设每天的上限。
            outcome.ok = rules::addBond(state_.bonds[command.a], command.x);
            return true;
        }

        // ---- P3 第 9 章：跌落到目标境界（契约 1.6）----
        case CommandKind::RealmDemote: {
            const rules::Realm target = rules::fromValue(command.x);
            switch (rules::checkDemote(state_.realm, target)) {
                case rules::DemoteCheck::NoRealm:
                    outcome.ok = false;
                    outcome.code = "no_realm";
                    return true;
                case rules::DemoteCheck::NotLower:
                    outcome.ok = false;
                    outcome.code = "not_lower";
                    return true;
                case rules::DemoteCheck::AlreadyThere:
                    outcome.ok = true;
                    return true;
                case rules::DemoteCheck::Ok:
                    break;
            }
            state_.formerRealm = std::max(state_.formerRealm, state_.realm);
            state_.realm = target;
            state_.realmCap = target;
            state_.maxHp = rules::realmMaxHp(target);
            state_.hp = std::min(state_.hp, state_.maxHp);
            state_.maxMp = rules::realmMaxMp(target);
            state_.mp = 0;
            state_.cultivation = 0;
            state_.cultivationRemainder = 0;
            outcome.ok = true;
            return true;
        }

        // ---- 第 4 章二次整改（技术债 G-14）：抬剧情给的境界上限 ----
        //
        // 契约 docs/interfaces-p3-script.md 第 7 节。语义是「上限**至少**到这一层」：
        //   * 非法编号 → ok = false，code = "no_realm"（与 RealmAdvance 同一口径）；
        //   * 低于或等于现有上限 → ok = true，什么也不做。**与 RealmAdvance 的
        //     not_higher 故意不同**：境界往回拨一定是脚本写错了，而上限「已经更高」
        //     是正常的——读档重跑同一段、老档迁移上来的上限本就高于这一节，
        //     剧情要的那个结果（至少到这一层）已经成立；
        //   * 高于 → 抬上去。**只升不降**：往下压上限等于替第 9 章的跌落做决定，
        //     这条命令决定不了那件事。
        case CommandKind::RealmCap: {
            const rules::Realm target = rules::fromValue(command.x);
            if (!rules::isValid(target)) {
                outcome.ok = false;
                outcome.code = "no_realm";
                return true;
            }
            raiseRealmCap(target);
            outcome.ok = true;
            return true;
        }
    }
    return true;
}

void Application::raiseRealmCap(rules::Realm target) {
    if (rules::toValue(target) > rules::toValue(state_.realmCap)) state_.realmCap = target;
}

const core::BattleSetup* Application::battleSetup(const std::string& id) const {
    const auto it = battles_.find(id);
    return it == battles_.end() ? nullptr : &it->second;
}

std::string Application::battleBackdrop(const std::string& battleId) const {
    const auto it = battleBackdrops_.find(battleId);
    return it == battleBackdrops_.end() ? std::string{} : it->second;
}

engine::Rect Application::heroScreenRect() const {
    // 从栈顶往下找第一层不透明的：那一层就是玩家此刻眼里的底图。是世界层才有「屏幕上的主角」。
    const auto shown = std::find_if(scenes_.rbegin(), scenes_.rend(), [](const ScenePtr& s) { return s->opaque(); });
    if (shown == scenes_.rend() || dynamic_cast<const WorldScene*>(shown->get()) == nullptr || map_ == nullptr) {
        return {};
    }
    const Vec2f focus{static_cast<float>(state_.position.x) + 0.5f, static_cast<float>(state_.position.y) + 0.5f};
    const Vec2f camera = cameraTarget(map_->width, map_->height, focus);
    const int x = static_cast<int>(std::lround(static_cast<float>(state_.position.x * kWorldTilePx) - camera.x));
    const int y = static_cast<int>(std::lround(static_cast<float>(state_.position.y * kWorldTilePx) - camera.y));
    // 连同四周一格：说话、打探的对象（NPC）一定站在他相邻的一格上，头也高出脚下那一格一截。
    return engine::Rect{x - kWorldTilePx, y - kWorldTilePx, kWorldTilePx * 3, kWorldTilePx * 3};
}

engine::Rect keepHeroInSight(const engine::Rect& bottomPanel, const engine::Rect& hero, int topGap) {
    const bool covers = hero.w > 0 && hero.h > 0 && hero.x < bottomPanel.x + bottomPanel.w &&
                        bottomPanel.x < hero.x + hero.w && hero.y < bottomPanel.y + bottomPanel.h &&
                        bottomPanel.y < hero.y + hero.h;
    if (!covers) return bottomPanel;
    // 只抬必要的那一截，抬到主角那一圈的上沿之上；不直接翻到顶边——顶边左上角是地名与目标提示（WorldHud），
    // 半透明的框压上去，两层字叠在一起读不清。框比他头顶以上的地方还高时才贴到顶边。
    return engine::Rect{bottomPanel.x, std::max(topGap, hero.y - bottomPanel.h), bottomPanel.w, bottomPanel.h};
}

rules::Shop* Application::shop(const std::string& id) {
    const auto it = shops_.find(id);
    return it == shops_.end() ? nullptr : &it->second;
}

std::string Application::speakerName(const std::string& roleId) const {
    if (const core::RoleTemplate* role = data_.findRole(roleId)) return role->name;
    // 查不到就回显 id：画面上一眼看出是哪个角色没进 data。
    return roleId;
}

void Application::pumpScriptCommands() {
    if (scripts_ == nullptr) return;

    // 一帧里可能连续完成多条就地命令（给物品、置旗标），循环直到遇到需要
    // 玩家参与的命令或脚本结束。上限防止脚本写出死循环把主循环挂住。
    constexpr int kMaxImmediatePerFrame = 64;
    for (int guard = 0; guard < kMaxImmediatePerFrame; ++guard) {
        if (commandPending_) return;

        script::Command command;
        if (!scripts_->pollCommand(command)) return;

        commandPending_ = true;
        script::CommandResult result;
        if (dispatch(command, result)) {
            // 就地完成：立刻回填并继续取下一条。
            completeCommand(result);
            continue;
        }
        return;  // 已压入场景，等场景回填
    }
}

void Application::tick(double deltaSeconds) {
    if (engine_) {
        engine_->pollEvents();
        if (engine_->shouldQuit()) requestQuit();
        // 排在场景 update 之前：这一帧按了 Alt+Enter，开着的设置面板读到的就已经是新状态。
        syncFullscreenFromEngine();
    }

    pumpScriptCommands();

    if (!scenes_.empty()) {
        Scene& top = *scenes_.back();
        if (!top.update(*this, deltaSeconds)) popScene();
    }

    // 章节卡排在改栈之前：这一帧要压的卡与别的压栈一起在下面落地，下一帧就在最上面。
    flushChapterCards();
    // 教学卡与见闻提示（第一期地基包）：同样排在改栈之前。
    watchGuide();
    flushTutorials();

    // 帧末统一改栈，避免在 update 中途让容器失效。
    while (pendingPops_ > 0 && !scenes_.empty()) {
        scenes_.back()->onExit(*this);
        scenes_.pop_back();
        --pendingPops_;
    }
    pendingPops_ = 0;
    for (ScenePtr& scene : pendingPush_) {
        scene->onEnter(*this);
        scenes_.push_back(std::move(scene));
    }
    pendingPush_.clear();

    // 这一帧发过东西：一声就够。一段脚本连给三样、一条路径行动给两样，都是同一次发放。
    if (itemsGiven_) {
        itemsGiven_ = false;
        engine_->playSfx("item_get");
    }

    // 得物卡片的时钟：只在世界层露着时走——给完东西紧接着开战、开菜单，卡片停在那里，回到地图上再放完。
    if (worldShown()) itemToasts_.advance(static_cast<float>(deltaSeconds));
    if (worldShown()) loreNotices_.advance(static_cast<float>(deltaSeconds));

    // 玩法计时器。系统菜单（主菜单、标题画面）的时间另记，见方案 2.2 的防刷口径：
    // 「首次游玩 ≥ 10 小时」数的是玩的时间，不是开着菜单发呆的时间。
    if (systemSceneActive()) {
        state_.playSecondsSystem += deltaSeconds;
    } else {
        state_.playSecondsGameplay += deltaSeconds;
    }
}

void Application::drawScenes() {
    // 从最上面的不透明场景开始画，它下面的被完全遮住不必重绘。
    std::size_t first = 0;
    for (std::size_t i = scenes_.size(); i > 0; --i) {
        if (scenes_[i - 1]->opaque()) {
            first = i - 1;
            break;
        }
    }
    for (std::size_t i = first; i < scenes_.size(); ++i) {
        scenes_[i]->render(*this);
        // 脚本 fade.out 的黑幕：紧贴着世界层画，压住地图、人物、HUD，却压不住它上面的
        // 对话框——黑场里的一句旁白是演出，不是事故。世界层被不透明的场景（战斗、卡片）
        // 盖住时它也跟着不画。
        if (screenFade_ > 0.f && dynamic_cast<WorldScene*>(scenes_[i].get()) != nullptr) {
            const auto alpha = static_cast<std::uint8_t>(std::lround(screenFade_ * 255.f));
            engine_->fillRect(engine::RectF{0.f, 0.f, static_cast<float>(engine::kLogicalWidth),
                                            static_cast<float>(engine::kLogicalHeight)},
                              ui::withAlpha(theme_.inkDeep, alpha), engine::BlendMode::Alpha);
        }
        // 得物卡片：紧贴着世界层（与黑幕之上）画，对话框压在它上面——一边读对白一边看得见刚拿到了什么。
        if (itemToasts_.active() && dynamic_cast<WorldScene*>(scenes_[i].get()) != nullptr) {
            if (!toastIconsTried_) {
                toastIconsTried_ = true;
                toastIcons_ = engine_->loadTexture("art/ui/icons.png");
            }
            drawItemToasts(*engine_, theme_, toastGeometry_, toastIcons_, itemToasts_, heroScreenRect());
        }
        // 「见闻：XXX」：同样紧贴着世界层画，对话框压在它上面。
        if (loreNotices_.active() && dynamic_cast<WorldScene*>(scenes_[i].get()) != nullptr) {
            drawLoreNotice(*engine_, theme_, loreGeometry_, loreNotices_);
        }
    }
}

void Application::announceItem(const std::string& itemId, int count) {
    itemToasts_.push(makeItemToast(data_, state_, itemId, count));
    noteItemsGiven();
}

const core::ExplorationReward* Application::explorationReward(const std::string& mapId) const {
    const auto it = explorationRewards_.find(mapId);
    return it == explorationRewards_.end() ? nullptr : &it->second;
}

bool Application::worldShown() const {
    const auto shown = std::find_if(scenes_.rbegin(), scenes_.rend(), [](const ScenePtr& s) { return s->opaque(); });
    return shown != scenes_.rend() && dynamic_cast<const WorldScene*>(shown->get()) != nullptr;
}

int Application::run() {
    while (!quitRequested_) {
        const double delta = headless_ ? kHeadlessStep : engine_->deltaSeconds();
        tick(delta);

        if (engine_ && !headless_) {
            engine_->beginFrame();
            drawScenes();
            engine_->endFrame();
        }

        if (scenes_.empty() && !scripts_->isRunning()) break;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 教学卡与见闻提示（第一期地基包，docs/ch07-foundation.md 2.3、2.5）
// ---------------------------------------------------------------------------

void Application::tutorialEvent(const std::string& event) {
    if (!tutorialsEnabled_ || event.empty()) return;
    const core::TutorialCard* card = rules::tutorialForEvent(data_.tutorials, event, state_, chapterDoneFn());
    if (card == nullptr) return;
    // 已经排着就不再排：同一个事件每帧都可能来一遍（走在宝箱旁边、每回合开菜单）。
    if (std::find(pendingTutorials_.begin(), pendingTutorials_.end(), card->id) != pendingTutorials_.end()) return;
    pendingTutorials_.push_back(card->id);
}

std::string Application::requestTutorial(const std::string& cardId) {
    const core::TutorialCard* card = rules::findTutorial(data_.tutorials, cardId);
    if (card == nullptr) return "no_card";
    if (!rules::tutorialEligible(*card, state_, chapterDoneFn())) return "skipped";
    // 关着（无头测试的缺省）：脚本照常走，不排卡、不记旗标——与 tutorialEvent 同一个闸。
    if (!tutorialsEnabled_) return "skipped";
    if (std::find(pendingTutorials_.begin(), pendingTutorials_.end(), card->id) == pendingTutorials_.end()) {
        pendingTutorials_.push_back(card->id);
    }
    return {};
}

rules::ChapterDoneFn Application::chapterDoneFn() const {
    return [this](int number) { return chapters_.chapterDone(state_, number); };
}

void Application::noteBattleStart() {
    battleStartSeen_.clear();
    for (const core::TutorialCard* card : rules::seenTutorials(data_.tutorials, state_)) battleStartSeen_.push_back(card->id);
}

void Application::replayTutorial(const std::string& cardId) {
    if (rules::findTutorial(data_.tutorials, cardId) == nullptr) return;
    pushScene(std::make_unique<TutorialScene>(cardId));
}

void Application::flushTutorials() {
    if (pendingTutorials_.empty()) return;
    if (pendingPops_ > 0 || !pendingPush_.empty() || scenes_.empty()) return;
    Scene* top = scenes_.back().get();
    // 两种时候弹：
    //   · 战斗里轮到我方（BattleScene 开菜单时才发事件；卡片压上去，战斗只更新最上面那一层，于是输入暂停）；
    //   · 不在战斗里：脚本演完、没有命令等着回填，顶上是走图或设施面板。对白、选项、章节卡、菜单开着都等。
    const bool inBattle = dynamic_cast<BattleScene*>(top) != nullptr;
    if (!inBattle) {
        if (scripts_ == nullptr || scripts_->isRunning() || commandPending_) return;
        const bool calm = dynamic_cast<WorldScene*>(top) != nullptr || dynamic_cast<FieldScene*>(top) != nullptr ||
                          dynamic_cast<CultivationScene*>(top) != nullptr ||
                          dynamic_cast<AlchemyScene*>(top) != nullptr;
        if (!calm) return;
    }
    const std::string id = pendingTutorials_.front();
    pendingTutorials_.erase(pendingTutorials_.begin());
    const core::TutorialCard* card = rules::findTutorial(data_.tutorials, id);
    if (card == nullptr || rules::tutorialSeen(*card, state_)) return;
    // 看过一次就记下；设置里关了教学提示也照样记（判据 3：关掉后不弹，但记事「指南」页里翻得到）。
    state_.flags[rules::tutorialSeenFlag(id)] = 1;
    tutorialLog_.push_back(id);
    if (settings_.tutorialCards) pushScene(std::make_unique<TutorialScene>(id));
}

void Application::resetGuideWatch() {
    guideBaselined_ = false;
    loreKnown_.clear();
    guideObjective_.clear();
    pendingTutorials_.clear();
    loreNotices_.clear();
}

void Application::watchGuide() {
    // 见闻：前后两份解锁表一比，多出来的报一句「见闻：XXX」。头一回（新开局、读档）只记不报。
    std::set<std::string> known = rules::unlockedLoreIds(data_.lore, state_);
    if (guideBaselined_) {
        for (const core::LoreEntry* entry : rules::newlyUnlockedLore(data_.lore, loreKnown_, known)) {
            loreNotices_.push(data_.lookupText("ui.guide.lore.unlocked") + data_.lookupText(entry->titleKey));
        }
    }
    loreKnown_ = std::move(known);

    // 主线目标：换了一步就发「objective.<id>」（教学卡可以挂在某一步上，例如交药那一步讲年份）。
    // 头一回也发：读第 7 章章首存档时，那一步挂着的卡同样该弹。
    const core::Objective* step = rules::currentObjective(data_.objectives, state_);
    const std::string stepId = step != nullptr ? step->id : std::string{};
    if (!stepId.empty() && (!guideBaselined_ || stepId != guideObjective_)) tutorialEvent("objective." + stepId);
    guideObjective_ = stepId;
    guideBaselined_ = true;

    if (!tutorialsEnabled_ || scenes_.empty() || dynamic_cast<WorldScene*>(scenes_.back().get()) == nullptr) return;
    // 下面几样只在走图时看（打完仗回到地图上才说「战斗外也能吃药」）。
    tutorialEvent("world.idle");
    bool anyMet = false;
    bool anyBond = false;
    for (const auto& [id, companion] : data_.companions) {
        if (!companion.metFlag.empty() && state_.flag(companion.metFlag) != 0) anyMet = true;
        if (state_.bondOf(id) > 0) anyBond = true;
    }
    if (anyMet) tutorialEvent("roster.joined");
    if (anyBond) tutorialEvent("bond.gained");
    if (state_.maxHp > 0 && state_.hp * 2 < state_.maxHp) tutorialEvent("world.hurt");
}

}  // namespace fanren::game
