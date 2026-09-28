#include "io/VisualLoader.h"

#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "io/StrictJson.h"

namespace fanren::io {
namespace {

using detail::toInt;
using nlohmann::json;

// 对象里名为 key 的成员；不是对象或没有这个键时为 nullptr。
const json* member(const json& owner, const char* key) {
    if (!owner.is_object()) return nullptr;
    const auto it = owner.find(key);
    return it == owner.end() ? nullptr : &*it;
}

bool nonEmptyString(const json* v) {
    return v != nullptr && v->is_string() && !v->get_ref<const std::string&>().empty();
}

// 两份文件共用的出错口径与取值法：第一处错就停，报「哪个字段、错在哪」。
struct Reader {
    const char* fileName;   // "index.json" / "meta.json"，报错的开头
    std::string error;

    bool fail(const std::string& where, const std::string& what) {
        if (error.empty()) error = std::string(fileName) + " 的 " + where + "：" + what;
        return false;
    }

    // 可选的整数字段：没有就保持 out 原值；有但不是整数就报错。
    bool optionalInt(const json& owner, const char* key, const std::string& where, int& out) {
        const json* v = member(owner, key);
        if (v == nullptr) return true;
        if (!toInt(*v, out)) return fail(where + "." + key, "应当是整数");
        return true;
    }

    bool optionalString(const json& owner, const char* key, const std::string& where, std::string& out) {
        const json* v = member(owner, key);
        if (v == nullptr) return true;
        if (!v->is_string()) return fail(where + "." + key, "应当是字符串");
        out = v->get<std::string>();
        return true;
    }

    bool intList(const json& v, const std::string& where, std::vector<int>& out) {
        if (!v.is_array()) return fail(where, "应当是整数数组");
        out.clear();
        for (const json& item : v) {
            int n = 0;
            if (!toInt(item, n) || n < 0) return fail(where, "应当是非负整数数组");
            out.push_back(n);
        }
        return true;
    }

    bool stringMap(const json* v, const std::string& where, std::map<std::string, std::string>& out) {
        if (v == nullptr) return true;
        if (!v->is_object()) return fail(where, "应当是对象");
        for (const auto& item : v->items()) {
            if (!item.value().is_string()) return fail(where + "." + item.key(), "应当是字符串");
            out[item.key()] = item.value().get<std::string>();
        }
        return true;
    }

    // 可选的非负数：没有就保持原值。
    bool amount(const json& owner, const char* key, const std::string& where, float& out) {
        const json* v = member(owner, key);
        if (v == nullptr) return true;
        if (!v->is_number() || v->get<double>() < 0.0) return fail(where + "." + key, "应当是非负数");
        out = static_cast<float>(v->get<double>());
        return true;
    }
};

// ---------------------------------------------------------------------------
// index.json
// ---------------------------------------------------------------------------

// 顶层给的帧宽高：人物表、敌人图没写自己的就取它。
struct DefaultFrame {
    int w = 16;
    int h = 24;
};

struct SpriteReader : Reader {
    SpriteReader() : Reader{"index.json", {}} {}

    // 人物表与敌人图共有的格子：file 必填；帧宽高缺了取顶层，列数缺省 3；foot_y 缺省是最后一行
    // （人物表的脚底描边压在最后一行，docs/art-sprites.md 第 4 节），head_y 缺省 0。
    template <typename Sheet>
    bool readGrid(const json& v, const std::string& where, DefaultFrame frame, const char* noFile, Sheet& out) {
        if (!v.is_object()) return fail(where, "应当是对象");
        const json* file = member(v, "file");
        if (!nonEmptyString(file)) return fail(where + ".file", noFile);
        out.file = file->get<std::string>();
        out.frameW = frame.w;
        out.frameH = frame.h;
        if (!optionalInt(v, "frame_w", where, out.frameW) || !optionalInt(v, "frame_h", where, out.frameH) ||
            !optionalInt(v, "cols", where, out.cols)) {
            return false;
        }
        if (out.frameW <= 0 || out.frameH <= 0 || out.cols <= 0) {
            return fail(where, "帧宽、帧高、列数都必须是正数");
        }
        out.footY = out.frameH - 1;
        out.headY = 0;
        return optionalInt(v, "foot_y", where, out.footY) && optionalInt(v, "head_y", where, out.headY);
    }

    // 战斗帧：可选；写了就得是「动作 → 非空的非负整数表」。动作名不设白名单——
    // 战斗画面加一种动作不该连带改读取器，认不认得那个名字是用的人的事。
    bool readBattleFrames(const json& owner, const std::string& where, std::map<std::string, std::vector<int>>& out) {
        const json* battle = member(owner, "battle");
        if (battle == nullptr) return true;
        if (!battle->is_object()) return fail(where + ".battle", "应当是对象");
        for (const auto& item : battle->items()) {
            const std::string at = where + ".battle." + item.key();
            std::vector<int> frames;
            if (!intList(item.value(), at, frames)) return false;
            if (frames.empty()) return fail(at, "战斗帧不能是空表");
            out[item.key()] = std::move(frames);
        }
        return true;
    }

    // 朝向只有三种写法；写错了（"Left"、"west"）就是翻不翻转没人说得清，报错。
    bool readFacing(const json& owner, const std::string& where, core::BattleFacing& out) {
        const json* v = member(owner, "battle_facing");
        if (v == nullptr) return true;
        const std::string name = v->is_string() ? v->get<std::string>() : std::string{};
        if (name == "left") {
            out = core::BattleFacing::Left;
        } else if (name == "right") {
            out = core::BattleFacing::Right;
        } else if (name == "none") {
            out = core::BattleFacing::None;
        } else {
            return fail(where + ".battle_facing", "只认 left / right / none");
        }
        return true;
    }

    bool readSheet(const json& v, const std::string& where, DefaultFrame frame, core::CharacterSheet& out) {
        if (!readGrid(v, where, frame, "缺少人物表的文件名", out)) return false;
        const json* walk = member(v, "walk");
        if (walk == nullptr || !walk->is_object()) return fail(where + ".walk", "缺少四向走路帧");
        // 键名 → GameState::facing 的下标。
        constexpr std::array<const char*, 4> kDirections{"up", "right", "down", "left"};
        for (std::size_t d = 0; d < kDirections.size(); ++d) {
            const json* frames = member(*walk, kDirections[d]);
            const std::string at = where + ".walk." + kDirections[d];
            if (frames == nullptr) return fail(at, "缺少这一向的走路帧");
            if (!intList(*frames, at, out.walk[d])) return false;
            if (out.walk[d].empty()) return fail(at, "走路帧不能是空表");
        }
        return readBattleFrames(v, where, out.battle) && readFacing(v, where, out.battleFacing) &&
               optionalString(v, "weapon", where, out.weapon);
    }

    // 非人形敌人只有战斗帧：没有战斗帧的敌人图什么也画不了，当生成器坏了报。
    bool readEnemy(const json& v, const std::string& where, DefaultFrame frame, core::EnemySheet& out) {
        if (!readGrid(v, where, frame, "缺少敌人图的文件名", out)) return false;
        if (!optionalString(v, "kind", where, out.kind) || !optionalString(v, "alias_of", where, out.aliasOf) ||
            !readBattleFrames(v, where, out.battle) || !readFacing(v, where, out.battleFacing)) {
            return false;
        }
        if (out.battle.empty()) return fail(where + ".battle", "缺少战斗帧");
        if (const json* floating = member(v, "float")) {
            if (!floating->is_boolean()) return fail(where + ".float", "应当是 true / false");
            out.floating = floating->get<bool>();
        }
        return true;
    }

    bool readRect(const json& v, const std::string& where, core::PixelRect& out) {
        std::vector<int> n;
        if (!intList(v, where, n) || n.size() != 4 || n[2] <= 0 || n[3] <= 0) {
            return fail(where, "应当是 [x, y, w, h]，w、h 为正");
        }
        out = core::PixelRect{n[0], n[1], n[2], n[3]};
        return true;
    }

    bool readObject(const json& v, const std::string& where, core::ObjectSprite& out) {
        if (!v.is_object()) return fail(where, "应当是对象");
        const json* frames = member(v, "frames");
        if (frames == nullptr || !frames->is_array() || frames->empty()) {
            return fail(where + ".frames", "缺少物件的帧");
        }
        for (std::size_t i = 0; i < frames->size(); ++i) {
            core::PixelRect r;
            if (!readRect((*frames)[i], where + ".frames[" + std::to_string(i) + "]", r)) return false;
            out.frames.push_back(r);
        }
        // 锚点缺省：第 0 帧的底边中点（平铺在格子上的标记就是这么对的）。
        out.anchorX = static_cast<float>(out.frames.front().w) / 2.f;
        out.anchorY = static_cast<float>(out.frames.front().h);
        if (const json* anchor = member(v, "anchor")) {
            std::vector<int> a;
            if (!intList(*anchor, where + ".anchor", a) || a.size() != 2) return fail(where + ".anchor", "应当是 [x, y]");
            out.anchorX = static_cast<float>(a[0]);
            out.anchorY = static_cast<float>(a[1]);
        }
        if (const json* fps = member(v, "fps")) {
            if (!fps->is_number() || fps->get<double>() < 0.0) return fail(where + ".fps", "应当是非负数");
            out.fps = static_cast<float>(fps->get<double>());
        }
        if (const json* blend = member(v, "blend")) {
            const std::string mode = blend->is_string() ? blend->get<std::string>() : std::string{};
            if (mode != "add" && mode != "alpha") return fail(where + ".blend", "只认 \"add\" 与 \"alpha\"");
            out.additive = mode == "add";
        }
        return true;
    }

    bool readVariants(const json* v, std::map<std::string, std::vector<core::LookVariant>>& out) {
        if (v == nullptr) return true;
        if (!v->is_object()) return fail("role_variants", "应当是对象");
        for (const auto& entry : v->items()) {
            const std::string where = "role_variants." + entry.key();
            if (!entry.value().is_array()) return fail(where, "应当是数组");
            std::vector<core::LookVariant> parsed;
            for (const json& item : entry.value()) {
                if (!item.is_object()) return fail(where, "每一条都应当是对象");
                const json* look = member(item, "look");
                if (!nonEmptyString(look)) return fail(where, "缺少 look");
                core::LookVariant variant;
                variant.look = look->get<std::string>();
                if (!optionalInt(item, "min_chapter", where, variant.minChapter) ||
                    !optionalInt(item, "max_chapter", where, variant.maxChapter)) {
                    return false;
                }
                parsed.push_back(std::move(variant));
            }
            out[entry.key()] = std::move(parsed);
        }
        return true;
    }

    // 顶层的帧宽高、播放序与播放速度。
    bool readHeader(const json& root, DefaultFrame& frame, core::SpriteIndex& index) {
        if (!optionalInt(root, "frame_w", "顶层", frame.w) || !optionalInt(root, "frame_h", "顶层", frame.h)) {
            return false;
        }
        if (const json* cycle = member(root, "walk_cycle")) {
            if (!intList(*cycle, "walk_cycle", index.walkCycle)) return false;
            if (index.walkCycle.empty()) return fail("walk_cycle", "不能是空表");
        }
        if (const json* fps = member(root, "walk_fps")) {
            if (!fps->is_number() || fps->get<double>() <= 0.0) return fail("walk_fps", "应当是正数");
            index.walkFps = static_cast<float>(fps->get<double>());
        }
        return true;
    }

    bool readSheets(const json& root, DefaultFrame frame, core::SpriteIndex& index) {
        const json* sheets = member(root, "sheets");
        if (sheets == nullptr || !sheets->is_object()) return fail("sheets", "缺少人物表");
        const int maxCycle = *std::max_element(index.walkCycle.begin(), index.walkCycle.end());
        for (const auto& item : sheets->items()) {
            const std::string where = "sheets." + item.key();
            core::CharacterSheet sheet;
            if (!readSheet(item.value(), where, frame, sheet)) return false;
            // walk_cycle 是每一向帧表的下标：表比它短，播到那一格就越界了。在这里一次查清，
            // 渲染时就不必每帧再去夹。
            for (const std::vector<int>& frames : sheet.walk) {
                if (static_cast<int>(frames.size()) <= maxCycle) {
                    return fail(where + ".walk", "每一向的帧数必须多于 walk_cycle 里最大的下标");
                }
            }
            index.sheets.emplace(item.key(), std::move(sheet));
        }
        return true;
    }

    bool readEnemies(const json& root, DefaultFrame frame, core::SpriteIndex& index) {
        const json* enemies = member(root, "enemies");
        if (enemies == nullptr) return true;
        if (!enemies->is_object()) return fail("enemies", "应当是对象");
        for (const auto& item : enemies->items()) {
            core::EnemySheet sheet;
            if (!readEnemy(item.value(), "enemies." + item.key(), frame, sheet)) return false;
            index.enemies.emplace(item.key(), std::move(sheet));
        }
        return true;
    }

    bool readOverrides(const json& root, core::SpriteIndex& index) {
        const json* overrides = member(root, "battle_overrides");
        if (overrides == nullptr) return true;
        if (!overrides->is_object()) return fail("battle_overrides", "应当是对象");
        for (const auto& item : overrides->items()) {
            std::map<std::string, std::string> looks;
            if (!stringMap(&item.value(), "battle_overrides." + item.key(), looks)) return false;
            index.battleOverrides[item.key()] = std::move(looks);
        }
        return true;
    }

    bool readObjects(const json& root, core::SpriteIndex& index) {
        const json* objects = member(root, "objects");
        if (objects == nullptr) return true;
        if (!objects->is_object()) return fail("objects", "应当是对象");
        const json* file = member(*objects, "file");
        if (!nonEmptyString(file)) return fail("objects.file", "缺少物件图集的文件名");
        index.objectsFile = file->get<std::string>();
        if (!optionalString(*objects, "emissive", "objects", index.emissiveFile)) return false;
        const json* items = member(*objects, "items");
        if (items == nullptr || !items->is_object()) return fail("objects.items", "缺少物件表");
        for (const auto& item : items->items()) {
            core::ObjectSprite sprite;
            if (!readObject(item.value(), "objects.items." + item.key(), sprite)) return false;
            index.objects.emplace(item.key(), std::move(sprite));
        }
        return true;
    }
};

core::Result<core::SpriteIndex> readSpriteIndex(const json& root) {
    using R = core::Result<core::SpriteIndex>;
    if (!root.is_object()) return R::failure("index.json 顶层应当是对象");
    SpriteReader r;
    core::SpriteIndex index;
    DefaultFrame frame;
    const bool ok = r.readHeader(root, frame, index) && r.readSheets(root, frame, index) &&
                    r.stringMap(member(root, "roles"), "roles", index.roles) &&
                    r.readVariants(member(root, "role_variants"), index.variants) &&
                    r.stringMap(member(root, "facilities"), "facilities", index.facilities) &&
                    r.readObjects(root, index) && r.readEnemies(root, frame, index) && r.readOverrides(root, index);
    if (!ok) return R::failure(r.error);
    return R::success(std::move(index));
}

// ---------------------------------------------------------------------------
// meta.json
// ---------------------------------------------------------------------------

bool timeOfDayFromName(const std::string& name, core::TimeOfDay& out) {
    using core::TimeOfDay;
    if (name == "day") out = TimeOfDay::Day;
    else if (name == "dusk") out = TimeOfDay::Dusk;
    else if (name == "night") out = TimeOfDay::Night;
    else if (name == "indoor") out = TimeOfDay::Indoor;
    else return false;
    return true;
}

// 烘焙器（maplights.py）会写的粒子种类。A1 若新增一种，先在这里与 core::AmbientParticle 加上，
// 再在 game/MapVisual.cpp 接到引擎的粒子上——漏了哪一处，编译或这里的报错都会响。
bool particleFromName(const std::string& name, core::AmbientParticle& out) {
    using core::AmbientParticle;
    if (name == "dust") out = AmbientParticle::Dust;
    else if (name == "firefly") out = AmbientParticle::Firefly;
    else if (name == "petal") out = AmbientParticle::Petal;
    else if (name == "leaf") out = AmbientParticle::Leaf;
    else if (name == "rain") out = AmbientParticle::Rain;
    else if (name == "snow") out = AmbientParticle::Snow;
    else if (name == "ember") out = AmbientParticle::Ember;
    else if (name == "mist") out = AmbientParticle::Mist;
    else return false;
    return true;
}

// 室内三种主题：它们在烘焙器里一律取 indoor 时辰。
bool indoorTheme(const std::string& theme) {
    return theme == "indoor_wood" || theme == "indoor_stone" || theme == "cave";
}

struct MetaReader : Reader {
    MetaReader() : Reader{"meta.json", {}} {}

    bool readColor(const json& v, const std::string& where, core::Rgb& out) {
        if (!v.is_array() || v.size() != 3) return fail(where, "应当是 [r, g, b]");
        std::array<int, 3> c{};
        for (std::size_t i = 0; i < 3; ++i) {
            if (!toInt(v[i], c[i]) || c[i] < 0 || c[i] > 255) return fail(where, "颜色分量应当是 0–255 的整数");
        }
        out = core::Rgb{static_cast<std::uint8_t>(c[0]), static_cast<std::uint8_t>(c[1]),
                        static_cast<std::uint8_t>(c[2])};
        return true;
    }

    // 顶层的强度：写了才有值，没写留给时辰缺省。
    bool readStrength(const json& root, const char* key, std::optional<float>& out) {
        if (member(root, key) == nullptr) return true;
        float value = 0.f;
        if (!amount(root, key, "顶层", value)) return false;
        out = value;
        return true;
    }

    bool readCoordinate(const json& owner, const char* key, const std::string& where, float& out) {
        const json* v = member(owner, key);
        if (v == nullptr || !v->is_number()) return fail(where + "." + key, "缺少坐标");
        out = static_cast<float>(v->get<double>());
        return true;
    }

    bool readKind(const json& owner, const std::string& where, core::AmbientParticle& out) {
        const json* v = member(owner, "kind");
        if (v == nullptr || !v->is_string()) return fail(where + ".kind", "缺少粒子种类");
        if (!particleFromName(v->get<std::string>(), out)) {
            return fail(where + ".kind", "认不出的粒子种类「" + v->get<std::string>() + "」");
        }
        return true;
    }

    // 可选的数组：没有是空表，有却不是数组就报错。
    bool readList(const json& root, const char* key, const json*& out) {
        out = member(root, key);
        if (out != nullptr && !out->is_array()) return fail(key, "应当是数组");
        return true;
    }

    // 主题、时辰（没写按主题推）、环境光与三样强度。
    bool readHeader(const json& root, core::MapMeta& out) {
        if (const json* theme = member(root, "theme")) {
            if (!theme->is_string()) return fail("theme", "应当是字符串");
            out.theme = theme->get<std::string>();
        }
        out.time = indoorTheme(out.theme) ? core::TimeOfDay::Indoor : core::TimeOfDay::Day;
        if (const json* time = member(root, "time")) {
            if (!time->is_string() || !timeOfDayFromName(time->get<std::string>(), out.time)) {
                return fail("time", "只认 day / dusk / night / indoor");
            }
        }
        if (const json* ambient = member(root, "ambient")) {
            core::Rgb rgb;
            if (!readColor(*ambient, "ambient", rgb)) return false;
            out.ambient = rgb;
        }
        return readStrength(root, "dof", out.dof) && readStrength(root, "bloom", out.bloom) &&
               readStrength(root, "vignette", out.vignette);
    }

    bool readParticles(const json* list, core::MapMeta& out) {
        if (list == nullptr) return true;
        for (std::size_t i = 0; i < list->size(); ++i) {
            const json& item = (*list)[i];
            const std::string where = "particles[" + std::to_string(i) + "]";
            core::MetaParticleLayer layer;
            if (!item.is_object() || !readKind(item, where, layer.kind) ||
                !amount(item, "rate", where, layer.density)) {
                return fail(where, "应当是对象");   // 前面已报过的，fail 不会覆盖
            }
            out.particles.push_back(layer);
        }
        return true;
    }

    bool readLights(const json* list, core::MapMeta& out) {
        if (list == nullptr) return true;
        for (std::size_t i = 0; i < list->size(); ++i) {
            const json& item = (*list)[i];
            const std::string where = "lights[" + std::to_string(i) + "]";
            core::MetaLight light;
            if (!item.is_object()) return fail(where, "应当是对象");
            if (!readCoordinate(item, "x", where, light.x) || !readCoordinate(item, "y", where, light.y) ||
                !amount(item, "r", where, light.radius) || !amount(item, "intensity", where, light.intensity) ||
                !amount(item, "flicker", where, light.flicker)) {
                return false;
            }
            if (const json* c = member(item, "color")) {
                if (!readColor(*c, where + ".color", light.color)) return false;
            }
            if (!optionalString(item, "kind", where, light.kind)) return false;
            out.lights.push_back(std::move(light));
        }
        return true;
    }

    bool readWater(const json* list, core::MapMeta& out) {
        if (list == nullptr) return true;
        for (std::size_t i = 0; i < list->size(); ++i) {
            const json& cell = (*list)[i];
            core::Point p;
            if (!cell.is_array() || cell.size() != 2 || !toInt(cell[0], p.x) || !toInt(cell[1], p.y)) {
                return fail("water[" + std::to_string(i) + "]", "应当是 [x, y] 两个整数");
            }
            out.water.push_back(p);
        }
        return true;
    }

    bool readEmitters(const json* list, core::MapMeta& out) {
        if (list == nullptr) return true;
        for (std::size_t i = 0; i < list->size(); ++i) {
            const json& item = (*list)[i];
            const std::string where = "emitters[" + std::to_string(i) + "]";
            core::MetaEmitter emitter;
            if (!item.is_object()) return fail(where, "应当是对象");
            if (!readKind(item, where, emitter.kind) || !readCoordinate(item, "x", where, emitter.x) ||
                !readCoordinate(item, "y", where, emitter.y)) {
                return false;
            }
            out.emitters.push_back(emitter);
        }
        return true;
    }
};

core::Result<core::MapMeta> readMapMeta(const json& root) {
    using R = core::Result<core::MapMeta>;
    if (!root.is_object()) return R::failure("meta.json 顶层应当是对象");
    MetaReader r;
    core::MapMeta meta;
    const json* particles = nullptr;
    const json* lights = nullptr;
    const json* water = nullptr;
    const json* emitters = nullptr;
    const bool ok = r.readHeader(root, meta) && r.readList(root, "particles", particles) &&
                    r.readList(root, "lights", lights) && r.readList(root, "water", water) &&
                    r.readList(root, "emitters", emitters) &&
                    r.readParticles(particles, meta) && r.readLights(lights, meta) && r.readWater(water, meta) &&
                    r.readEmitters(emitters, meta) && r.optionalString(root, "backdrop", "顶层", meta.backdrop);
    if (!ok) return R::failure(r.error);
    return R::success(std::move(meta));
}

}  // namespace

core::Result<core::SpriteIndex> parseSpriteIndex(std::string_view json) {
    auto parsed = detail::parseStrictJson(json);
    if (!parsed) return core::Result<core::SpriteIndex>::failure("index.json " + parsed.error);
    return readSpriteIndex(parsed.value);
}

core::Result<core::SpriteIndex> loadSpriteIndex(const std::string& path) {
    auto text = detail::readTextFile(path);
    if (!text) return core::Result<core::SpriteIndex>::failure(text.error);
    auto parsed = detail::parseStrictJson(text.value);
    if (!parsed) return core::Result<core::SpriteIndex>::failure(path + " " + parsed.error);
    return readSpriteIndex(parsed.value);
}

core::Result<core::MapMeta> parseMapMeta(std::string_view json) {
    auto parsed = detail::parseStrictJson(json);
    if (!parsed) return core::Result<core::MapMeta>::failure("meta.json " + parsed.error);
    return readMapMeta(parsed.value);
}

core::Result<core::MapMeta> loadMapMeta(const std::string& path) {
    auto text = detail::readTextFile(path);
    if (!text) return core::Result<core::MapMeta>::failure(text.error);
    auto parsed = detail::parseStrictJson(text.value);
    if (!parsed) return core::Result<core::MapMeta>::failure(path + " " + parsed.error);
    auto meta = readMapMeta(parsed.value);
    if (!meta) return core::Result<core::MapMeta>::failure(path + "：" + meta.error);
    return meta;
}

namespace {

core::Result<std::map<std::string, std::string>> readBattleBackdrops(const json& root) {
    using Out = core::Result<std::map<std::string, std::string>>;
    if (!root.is_object()) return Out::failure("battles.json：顶层应当是对象");
    const auto battles = root.find("battles");
    if (battles == root.end() || !battles->is_object()) {
        return Out::failure("battles.json 的 battles：应当是对象");
    }
    std::map<std::string, std::string> out;
    for (const auto& [battleId, entry] : battles->items()) {
        const auto backdrop = entry.is_object() ? entry.find("backdrop") : entry.end();
        if (!entry.is_object() || backdrop == entry.end() || !backdrop->is_string() ||
            backdrop->get<std::string>().empty()) {
            return Out::failure("battles.json 的 battles." + battleId + ".backdrop：应当是非空字符串");
        }
        out.emplace(battleId, backdrop->get<std::string>());
    }
    return Out::success(std::move(out));
}

}  // namespace

core::Result<std::map<std::string, std::string>> parseBattleBackdrops(std::string_view text) {
    auto parsed = detail::parseStrictJson(text);
    if (!parsed) {
        return core::Result<std::map<std::string, std::string>>::failure("battles.json " + parsed.error);
    }
    return readBattleBackdrops(parsed.value);
}

core::Result<std::map<std::string, std::string>> loadBattleBackdrops(const std::string& path) {
    using Out = core::Result<std::map<std::string, std::string>>;
    auto text = detail::readTextFile(path);
    if (!text) return Out::failure(text.error);
    auto table = parseBattleBackdrops(text.value);
    if (!table) return Out::failure(path + "：" + table.error);
    return table;
}

}  // namespace fanren::io
