// loadTileMap 的实现：解析 Tiled JSON（.tmj）。规则见 docs/map_spec.md。
//
// 本函数只拿到一个 tmj 路径，做的是「单文件自洽性」校验：图层是否齐备、地图属性
// 是否齐备、对象矩形是否对齐网格、default spawn 是否恰好一个……
// map_spec 第 7 节里需要跨文件核对的规则（portal 双向可达、npc.role_id 是否在
// data/roles 里、脚本路径是否存在、旗标/文案 key 是否已登记、BFS 连通性、
// collision 与 building 的抽查告警）留给 tools/validate.py：那些检查要么需要
// GameData/全地图索引，要么只是告警不阻塞，加进来会让这个函数背上它签名之外的
// 依赖。
#include "io/DataLoader.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "io/JsonUtil.h"

namespace fanren::io {

namespace {

namespace fs = std::filesystem;
using nlohmann::json;
using fanren::core::MapObject;
using fanren::core::Point;
using fanren::core::TileMap;

// 全项目瓦片尺寸固定 32px（见 docs/interfaces.md engine::kTileSize 与
// map_spec.md 第 1 节）。io 不能链接 engine（依赖方向 io -> core），所以这里
// 独立存一份而不是 #include engine 的头；两边数值必须保持一致，一旦要改，两处
// 都要改。
constexpr int kTileSize = 32;

struct LayerSpec {
    const char* name;
    const char* type;
};

constexpr std::array<LayerSpec, 6> kExpectedLayers = {{
    {"ground", "tilelayer"},
    {"overlay", "tilelayer"},
    {"building", "tilelayer"},
    {"front", "tilelayer"},
    {"collision", "tilelayer"},
    {"objects", "objectgroup"},
}};

core::Result<TileMap> fail(const std::string& msg) { return core::Result<TileMap>::failure(msg); }

// Tiled 的自定义属性在 JSON 里是一个数组：[{"name":..,"type":..,"value":..}, ...]，
// 不是普通对象，取值前要先摊平成 name -> value 方便查。
core::Result<std::map<std::string, json>> readPropertiesArray(const json& owner,
                                                                const std::string& context) {
    std::map<std::string, json> result;
    if (!owner.contains("properties")) {
        return core::Result<std::map<std::string, json>>::success(std::move(result));
    }
    if (!owner["properties"].is_array()) {
        return core::Result<std::map<std::string, json>>::failure(context + " 的 properties 字段必须是数组");
    }
    for (const json& p : owner["properties"]) {
        if (!p.is_object() || !p.contains("name") || !p["name"].is_string() || !p.contains("value")) {
            return core::Result<std::map<std::string, json>>::failure(context +
                                                                        " 的 properties 数组里有格式不对的项");
        }
        result[p["name"].get<std::string>()] = p["value"];
    }
    return core::Result<std::map<std::string, json>>::success(std::move(result));
}

std::string propertyValueToString(const json& v) {
    if (v.is_boolean()) return v.get<bool>() ? "true" : "false";
    if (v.is_number_integer() || v.is_number_unsigned()) return std::to_string(v.get<long long>());
    if (v.is_number_float()) {
        std::ostringstream oss;
        oss << v.get<double>();
        return oss.str();
    }
    if (v.is_string()) return v.get<std::string>();
    // 兜底：颜色 / 文件 / 对象引用等少见属性类型，原样序列化，不当场判失败——
    // 这些类型 P1 用不到，报错反而会挡住合法但罕见的地图。
    return v.dump();
}

core::Result<std::string> requireStringProp(const std::map<std::string, json>& props, const std::string& key,
                                             const std::string& context) {
    const auto it = props.find(key);
    if (it == props.end()) return core::Result<std::string>::failure(context + " 缺少属性 " + key);
    if (!it->second.is_string()) return core::Result<std::string>::failure(context + " 的属性 " + key + " 类型应为 string");
    return core::Result<std::string>::success(it->second.get<std::string>());
}

core::Result<int> requireIntProp(const std::map<std::string, json>& props, const std::string& key,
                                  const std::string& context) {
    const auto it = props.find(key);
    if (it == props.end()) return core::Result<int>::failure(context + " 缺少属性 " + key);
    if (!it->second.is_number_integer() && !it->second.is_number_unsigned()) {
        return core::Result<int>::failure(context + " 的属性 " + key + " 类型应为 int");
    }
    return core::Result<int>::success(it->second.get<int>());
}

core::Result<bool> requireBoolProp(const std::map<std::string, json>& props, const std::string& key,
                                    const std::string& context) {
    const auto it = props.find(key);
    if (it == props.end()) return core::Result<bool>::failure(context + " 缺少属性 " + key);
    if (!it->second.is_boolean()) return core::Result<bool>::failure(context + " 的属性 " + key + " 类型应为 bool");
    return core::Result<bool>::success(it->second.get<bool>());
}

// 校验六个图层齐备、名称与顺序正确（map_spec 第 3 节）。
core::Result<bool> validateLayerStructure(const json& mapJson, const std::string& context) {
    if (!mapJson.contains("layers") || !mapJson["layers"].is_array()) {
        return core::Result<bool>::failure(context + " 缺少 layers 数组");
    }
    const json& layers = mapJson["layers"];

    std::vector<std::string> actualNames;
    for (const json& layer : layers) {
        actualNames.push_back(layer.value("name", std::string{}));
    }

    // 先把缺失的图层名全部找出来一次性报掉，比只说「顺序不对」更好定位。
    std::vector<std::string> missing;
    for (const LayerSpec& spec : kExpectedLayers) {
        if (std::find(actualNames.begin(), actualNames.end(), std::string(spec.name)) == actualNames.end()) {
            missing.push_back(spec.name);
        }
    }
    if (!missing.empty()) {
        std::string msg = context + " 缺少图层：";
        for (std::size_t i = 0; i < missing.size(); ++i) {
            if (i != 0) msg += "、";
            msg += missing[i];
        }
        return core::Result<bool>::failure(msg);
    }
    if (actualNames.size() != kExpectedLayers.size()) {
        return core::Result<bool>::failure(context + " 图层数量不对：应恰好 " +
                                            std::to_string(kExpectedLayers.size()) + " 个，实际 " +
                                            std::to_string(actualNames.size()) + " 个（多余图层是 map_spec 违规）");
    }
    for (std::size_t i = 0; i < kExpectedLayers.size(); ++i) {
        if (actualNames[i] != kExpectedLayers[i].name) {
            return core::Result<bool>::failure(context + " 图层顺序错误：第 " + std::to_string(i + 1) +
                                                " 层应为 " + kExpectedLayers[i].name + "，实际为 " + actualNames[i]);
        }
        const std::string actualType = layers[i].value("type", std::string{});
        if (actualType != kExpectedLayers[i].type) {
            return core::Result<bool>::failure(context + " 图层 " + actualNames[i] + " 类型应为 " +
                                                kExpectedLayers[i].type + "，实际为 " + actualType);
        }
    }
    return core::Result<bool>::success(true);
}

// 解析一个瓦片层的 data 数组。要求是 JSON 整数数组（CSV），不是 base64 字符串
// （map_spec 第 3 节要求 CSV，第 7 节规则 2）；长度必须等于 width*height。
core::Result<std::vector<int>> parseTileLayer(const json& layer, int width, int height,
                                               const std::string& context) {
    if (!layer.contains("data")) {
        return core::Result<std::vector<int>>::failure(context + " 缺少 data 字段");
    }
    if (!layer["data"].is_array()) {
        // 最常见的原因：Tiled 导出时选了 base64（可能还带 zlib/gzip 压缩），
        // map_spec 明确要求 CSV（JSON 整数数组）以保证 git 可读可 diff。
        return core::Result<std::vector<int>>::failure(context + " 的图块数据不是 JSON 整数数组"
                                                                   "（很可能用了 base64 编码，map_spec 只允许 CSV）");
    }
    const json& data = layer["data"];
    const std::size_t expected = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (data.size() != expected) {
        return core::Result<std::vector<int>>::failure(context + " 的图块数量应为 " + std::to_string(expected) +
                                                         "（" + std::to_string(width) + "x" + std::to_string(height) +
                                                         "），实际 " + std::to_string(data.size()));
    }
    std::vector<int> out;
    out.reserve(data.size());
    for (const json& cell : data) {
        if (!cell.is_number_integer() && !cell.is_number_unsigned()) {
            return core::Result<std::vector<int>>::failure(context + " 的图块数据里有非整数项");
        }
        out.push_back(cell.get<int>());
    }
    return core::Result<std::vector<int>>::success(std::move(out));
}

// tileset 必须是外部引用（"source" 指向 .tsj）且文件存在；不允许内嵌 tileset
// （map_spec 第 2 节，第 7 节规则 2）。这两项都不需要跨文件数据，loadTileMap
// 自己就能查，所以没有留给 validate.py。
core::Result<bool> validateTilesets(const json& mapJson, const fs::path& mapDir, const std::string& context) {
    if (!mapJson.contains("tilesets")) return core::Result<bool>::success(true);
    if (!mapJson["tilesets"].is_array()) {
        return core::Result<bool>::failure(context + " 的 tilesets 字段必须是数组");
    }
    for (const json& ts : mapJson["tilesets"]) {
        if (!ts.contains("source") || !ts["source"].is_string()) {
            return core::Result<bool>::failure(context + " 存在内嵌 tileset：必须用 \"source\" 引用外部 .tsj 文件");
        }
        const std::string source = ts["source"].get<std::string>();
        std::error_code ec;
        if (!fs::exists(mapDir / source, ec)) {
            return core::Result<bool>::failure(context + " 引用的 tileset 不存在: " + source);
        }
    }
    return core::Result<bool>::success(true);
}

// 地图属性八项齐备（map_spec 第 5 节）。map_id/display_name_key/region/bgm/
// chapter/outdoor/can_leave_edge 七项必填，parent_map 仅在 can_leave_edge 为真
// 时必填。
//
// 注意：can_leave_edge 与 parent_map 在这里只做存在性 + 类型校验，不写回
// TileMap —— core::TileMap（Types.h，本模块不得修改）目前没有对应字段。这是
// map_spec 与 Types.h 之间的一处缺口，已在交付报告里说明，未擅自改动契约。
core::Result<bool> validateAndFillMapAttributes(const json& mapJson, const fs::path& path, TileMap& map) {
    const std::string context = "地图 " + path.filename().string();
    core::Result<std::map<std::string, json>> propsResult = readPropertiesArray(mapJson, context);
    if (!propsResult) return core::Result<bool>::failure(propsResult.error);
    const std::map<std::string, json>& props = propsResult.value;

    core::Result<std::string> mapId = requireStringProp(props, "map_id", context);
    if (!mapId) return core::Result<bool>::failure(mapId.error);
    const std::string expectedId = path.stem().string();
    if (mapId.value != expectedId) {
        return core::Result<bool>::failure(context + " 的 map_id 属性 (\"" + mapId.value +
                                            "\") 与文件名 (\"" + expectedId + "\") 不一致");
    }

    core::Result<std::string> displayNameKey = requireStringProp(props, "display_name_key", context);
    if (!displayNameKey) return core::Result<bool>::failure(displayNameKey.error);
    core::Result<std::string> region = requireStringProp(props, "region", context);
    if (!region) return core::Result<bool>::failure(region.error);
    core::Result<std::string> bgm = requireStringProp(props, "bgm", context);
    if (!bgm) return core::Result<bool>::failure(bgm.error);
    core::Result<int> chapter = requireIntProp(props, "chapter", context);
    if (!chapter) return core::Result<bool>::failure(chapter.error);
    core::Result<bool> outdoor = requireBoolProp(props, "outdoor", context);
    if (!outdoor) return core::Result<bool>::failure(outdoor.error);
    core::Result<bool> canLeaveEdge = requireBoolProp(props, "can_leave_edge", context);
    if (!canLeaveEdge) return core::Result<bool>::failure(canLeaveEdge.error);
    if (canLeaveEdge.value) {
        core::Result<std::string> parentMap = requireStringProp(props, "parent_map", context);
        if (!parentMap) {
            return core::Result<bool>::failure(context +
                                                " 的 can_leave_edge 为 true 时必须提供 parent_map 属性");
        }
    }

    // chapter 属性应与文件名里的章号一致（map_spec 第 7 节规则 14）。通用地图
    // （common_ 前缀）不编码章号，跳过这项检查。
    const std::string stem = expectedId;
    if (stem.size() > 2 && stem[0] == 'c' && stem[1] == 'h' && std::isdigit(static_cast<unsigned char>(stem[2]))) {
        std::size_t i = 2;
        int fromName = 0;
        while (i < stem.size() && std::isdigit(static_cast<unsigned char>(stem[i]))) {
            fromName = fromName * 10 + (stem[i] - '0');
            ++i;
        }
        if (fromName != chapter.value) {
            return core::Result<bool>::failure(context + " 的 chapter 属性 (" + std::to_string(chapter.value) +
                                                ") 与文件名章号 (" + std::to_string(fromName) + ") 不一致");
        }
    }

    map.id = mapId.value;
    map.displayNameKey = displayNameKey.value;
    map.region = region.value;
    map.bgm = bgm.value;
    map.chapter = chapter.value;
    map.outdoor = outdoor.value;
    return core::Result<bool>::success(true);
}

// 解析 objects 图层：每个对象转成 MapObject，矩形按 32px 网格对齐（第 7 节规则
// 11），名字在图内唯一（同一条规则）。属性一律摊平成字符串存进
// MapObject::properties——具体每种 type 该有哪些必填属性（第 4 节表格）留给
// validate.py，这里只对 spawn 的 default/facing 做校验，因为 defaultSpawn()
// 这条规则（第 7 节规则 4）纯粹是单文件内部一致性，且 core::TileMap::
// defaultSpawn() 直接依赖它成立。
core::Result<std::vector<MapObject>> parseObjectsLayer(const json& layer, const std::string& context) {
    std::vector<MapObject> objects;
    if (!layer.contains("objects") || !layer["objects"].is_array()) {
        return core::Result<std::vector<MapObject>>::failure(context + " 的 objects 图层缺少 objects 数组");
    }

    std::vector<std::string> seenNames;
    int spawnCount = 0;
    int defaultSpawnCount = 0;

    for (const json& obj : layer["objects"]) {
        if (!obj.contains("name") || !obj["name"].is_string() || obj["name"].get<std::string>().empty()) {
            return core::Result<std::vector<MapObject>>::failure(context + " 存在没有 name 的对象");
        }
        std::string name = obj["name"].get<std::string>();
        if (std::find(seenNames.begin(), seenNames.end(), name) != seenNames.end()) {
            return core::Result<std::vector<MapObject>>::failure(context + " 的对象名重复: \"" + name + "\"");
        }

        if (!obj.contains("type") || !obj["type"].is_string() || obj["type"].get<std::string>().empty()) {
            return core::Result<std::vector<MapObject>>::failure(context + " 的对象 \"" + name +
                                                                   "\" 没有设置 Class/type 字段");
        }
        const std::string type = obj["type"].get<std::string>();

        if (!obj.contains("x") || !obj.contains("y") || !obj.contains("width") || !obj.contains("height") ||
            !obj["x"].is_number() || !obj["y"].is_number() || !obj["width"].is_number() ||
            !obj["height"].is_number()) {
            return core::Result<std::vector<MapObject>>::failure(context + " 的对象 \"" + name +
                                                                   "\" 缺少 x/y/width/height");
        }
        const double x = obj["x"].get<double>();
        const double y = obj["y"].get<double>();
        const double w = obj["width"].get<double>();
        const double h = obj["height"].get<double>();

        auto alignedToGrid = [](double v) {
            return v >= 0.0 && std::abs(v - std::round(v / kTileSize) * kTileSize) < 1e-6;
        };
        if (!alignedToGrid(x) || !alignedToGrid(y) || !alignedToGrid(w) || !alignedToGrid(h) || w < kTileSize ||
            h < kTileSize) {
            return core::Result<std::vector<MapObject>>::failure(context + " 的对象 \"" + name +
                                                                   "\" 矩形未对齐 32px 网格，或宽高小于一格");
        }

        MapObject object;
        object.name = name;
        object.type = type;
        object.position = Point{static_cast<int>(std::lround(x / kTileSize)),
                                 static_cast<int>(std::lround(y / kTileSize))};
        object.width = static_cast<int>(std::lround(w / kTileSize));
        object.height = static_cast<int>(std::lround(h / kTileSize));

        core::Result<std::map<std::string, json>> propsResult =
            readPropertiesArray(obj, context + " 的对象 \"" + name + "\"");
        if (!propsResult) return core::Result<std::vector<MapObject>>::failure(propsResult.error);
        for (const auto& [key, value] : propsResult.value) {
            object.properties[key] = propertyValueToString(value);
        }

        if (type == "spawn") {
            ++spawnCount;
            if (object.property("facing").empty()) {
                return core::Result<std::vector<MapObject>>::failure(context + " 的 spawn \"" + name +
                                                                       "\" 缺少 facing 属性");
            }
            const std::string facing = object.property("facing");
            if (facing != "up" && facing != "down" && facing != "left" && facing != "right") {
                return core::Result<std::vector<MapObject>>::failure(context + " 的 spawn \"" + name +
                                                                       "\" 的 facing 属性不合法: " + facing);
            }
            if (propsResult.value.find("default") == propsResult.value.end() ||
                !propsResult.value.at("default").is_boolean()) {
                return core::Result<std::vector<MapObject>>::failure(context + " 的 spawn \"" + name +
                                                                       "\" 缺少 default 属性（bool 类型）");
            }
            if (object.property("default") == "true") ++defaultSpawnCount;
        }

        seenNames.push_back(std::move(name));
        objects.push_back(std::move(object));
    }

    if (spawnCount == 0) {
        return core::Result<std::vector<MapObject>>::failure(context + " 没有任何 spawn 对象（至少需要一个）");
    }
    if (defaultSpawnCount == 0) {
        return core::Result<std::vector<MapObject>>::failure(context + " 没有 default=true 的 spawn");
    }
    if (defaultSpawnCount > 1) {
        return core::Result<std::vector<MapObject>>::failure(context + " 存在多个 default=true 的 spawn，应恰好一个");
    }

    return core::Result<std::vector<MapObject>>::success(std::move(objects));
}

// 实际解析逻辑。绝大多数 json 访问前都先做了 is_xxx()/contains() 检查，但
// nlohmann::json 的个别接口（如对非 object 调用 .value()）在极端畸形输入下仍可能
// 抛出——为免这类边角情况破坏「不跨模块边界抛异常」的约定，公开入口 loadTileMap
// 用 try/catch 兜底，把任何漏网的异常也转成 Result::failure。
core::Result<TileMap> loadTileMapImpl(const std::string& tmjPath) {
    const fs::path path(tmjPath);
    core::Result<json> parsed = detail::readJsonFile(path);
    if (!parsed) return fail(parsed.error);
    const json& mapJson = parsed.value;
    const std::string context = "地图 " + path.filename().string();

    if (!mapJson.is_object()) return fail(context + " 顶层不是 JSON 对象");

    core::Result<bool> layerCheck = validateLayerStructure(mapJson, context);
    if (!layerCheck) return fail(layerCheck.error);

    if (!mapJson.contains("width") || !mapJson["width"].is_number_integer() || mapJson["width"].get<int>() <= 0 ||
        !mapJson.contains("height") || !mapJson["height"].is_number_integer() || mapJson["height"].get<int>() <= 0) {
        return fail(context + " 缺少合法的 width/height");
    }
    const int width = mapJson["width"].get<int>();
    const int height = mapJson["height"].get<int>();

    if (mapJson.contains("tilewidth") && mapJson["tilewidth"].is_number_integer() &&
        mapJson["tilewidth"].get<int>() != kTileSize) {
        return fail(context + " 的 tilewidth 不是 " + std::to_string(kTileSize) + "px（全项目瓦片尺寸统一）");
    }
    if (mapJson.contains("tileheight") && mapJson["tileheight"].is_number_integer() &&
        mapJson["tileheight"].get<int>() != kTileSize) {
        return fail(context + " 的 tileheight 不是 " + std::to_string(kTileSize) + "px（全项目瓦片尺寸统一）");
    }

    core::Result<bool> tilesetCheck = validateTilesets(mapJson, path.parent_path(), context);
    if (!tilesetCheck) return fail(tilesetCheck.error);

    TileMap map;
    map.width = width;
    map.height = height;

    core::Result<bool> attrCheck = validateAndFillMapAttributes(mapJson, path, map);
    if (!attrCheck) return fail(attrCheck.error);

    const json& layers = mapJson["layers"];
    std::array<std::vector<int>*, 5> targets = {&map.ground, &map.overlay, &map.building, &map.front,
                                                 &map.collision};
    for (std::size_t i = 0; i < targets.size(); ++i) {
        const std::string layerContext = context + " 的图层 " + kExpectedLayers[i].name;
        core::Result<std::vector<int>> tiles = parseTileLayer(layers[i], width, height, layerContext);
        if (!tiles) return fail(tiles.error);
        *targets[i] = std::move(tiles.value);
    }

    core::Result<std::vector<MapObject>> objects = parseObjectsLayer(layers[5], context);
    if (!objects) return fail(objects.error);
    map.objects = std::move(objects.value);

    return core::Result<TileMap>::success(std::move(map));
}

}  // namespace

core::Result<TileMap> loadTileMap(const std::string& tmjPath) {
    try {
        return loadTileMapImpl(tmjPath);
    } catch (const std::exception& e) {
        return core::Result<TileMap>::failure("解析地图 " + tmjPath + " 时出现未预期的异常: " + e.what());
    }
}

}  // namespace fanren::io
