#pragma once
// 美术产物的描述文件读进来之后的样子：精灵索引 assets/art/sprites/index.json（schema 见
// docs/art-sprites.md 第 5 节）与烘焙地图的 assets/art/maps/<id>/meta.json（字段见 game/MapVisual.h 文件头）。
//
// 纯数据，无行为、无依赖。读在 io 层（io/VisualLoader.h，严格：缺必填项、类型错、认不出的写法一律报错），
// 怎么画在 game 层（game/SpriteAtlas.h 挑外观算帧、game/MapVisual.h 换成引擎的颜色与粒子种类）。
// 放在 core 而不放 io 的头文件里，与 core::TileMap、core::BattleSetup 同一个习惯：io 只管「文件 → 结构体」，
// 结构体是世界画面、主菜单、战斗画面几路共用的词汇，谁都不该为了拿到它去碰 JSON。
#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "core/model/Types.h"

namespace fanren::core {

// ---------------------------------------------------------------------------
// 精灵索引
// ---------------------------------------------------------------------------

// 图集里的一块像素矩形（物件帧直接给矩形，不走帧号）。
struct PixelRect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

// 战斗帧朝哪边。人物表一律面向左（我方站在右侧），敌方人形单位运行时水平翻转；
// 非人形敌人面向右、不翻转；光球不分朝向。
enum class BattleFacing { Left, Right, None };

// 一张人物表（一个外观）：每帧 frameW×frameH（常人 16×24、巨汉 24×32），cols 列一行。
struct CharacterSheet {
    std::string file;   // 相对 art/sprites/ 的路径
    int frameW = 16;
    int frameH = 24;
    int cols = 3;
    // 四向走路帧，下标与 GameState::facing 同一套：0 上 / 1 右 / 2 下 / 3 左。
    // 每一向的第 0 帧是站立帧，其余是迈步帧；walk_cycle 里的数就是这张小表的下标。
    std::array<std::vector<int>, 4> walk;
    // 脚底：帧内的 (frameW/2, footY+1) 对准人物所站那一格的底边中点。
    int footY = 23;
    // 最高的不透明行：名牌、头顶气泡往这条线上面放。
    int headY = 0;
    // 战斗帧：动作（idle / ready / attack / cast / hurt / down / victory / guard）→ 帧号。
    std::map<std::string, std::vector<int>> battle;
    BattleFacing battleFacing = BattleFacing::Left;
    std::string weapon;   // sword / dao / staff / knife / fist / none …（缺 = 空串）
};

// 一个非人形敌人（兽、光球）：只有战斗帧，没有走路帧。帧号 → 像素与人物表同一条公式。
struct EnemySheet {
    std::string file;
    std::string kind;     // beast / orb …（缺 = 空串）
    int frameW = 16;
    int frameH = 24;
    int cols = 3;
    int footY = 23;       // 非人形的帧下面可能留空，落地必须用它，不能拿帧底边代替
    int headY = 0;
    std::map<std::string, std::vector<int>> battle;
    BattleFacing battleFacing = BattleFacing::Right;
    bool floating = false;   // index.json 的 "float"：悬空（光球）
    std::string aliasOf;     // 与哪个敌人共用一张图（只作记录：本条自己的字段是全的）
};

// 一种地图物件。帧是图集里的像素矩形，锚点是帧内的「落地点」。
struct ObjectSprite {
    std::vector<PixelRect> frames;
    float anchorX = 0.f;
    float anchorY = 0.f;
    float fps = 0.f;        // 0 = 静止，只用第 0 帧
    bool additive = false;  // blend 写的是 "add"：纯光（传送箭头、门光、闪光），加色混合
};

// role_variants 的一条（0 = 这一端不限）：maxChapter 那一章还没演完、且 minChapter 的前一章已经演完，就用 look。
struct LookVariant {
    std::string look;
    int minChapter = 0;
    int maxChapter = 0;
};

struct SpriteIndex {
    // 行走播放序（站、迈甲、站、迈乙）与播放速度。
    std::vector<int> walkCycle{0, 1, 0, 2};
    float walkFps = 8.f;
    std::map<std::string, CharacterSheet> sheets;                  // 外观 id → 人物表
    std::map<std::string, std::string> roles;                      // 角色 id → 外观 id
    std::map<std::string, std::vector<LookVariant>> variants;      // 角色 id → 按章的变体
    std::map<std::string, EnemySheet> enemies;                     // 敌人 id → 战斗图
    // 战斗 id → 角色 id → 外观 id（识海之战里韩立是光球）。取人物时先看它，再看变体，再看 roles。
    std::map<std::string, std::map<std::string, std::string>> battleOverrides;
    std::string objectsFile;                                       // 相对 art/sprites/
    std::string emissiveFile;                                      // 同尺寸、只留发光像素的那一张
    std::map<std::string, ObjectSprite> objects;                   // 物件 id → 帧
    std::map<std::string, std::string> facilities;                 // 设施 kind → 物件 id
};

// ---------------------------------------------------------------------------
// 烘焙地图的 meta.json
// ---------------------------------------------------------------------------

enum class TimeOfDay { Day, Dusk, Night, Indoor };

// meta 里认得的粒子种类。与 engine::ParticleKind 一一对应（game/MapVisual.cpp 换过去）；
// core 不依赖引擎，所以这里另立一份，名字 ↔ 枚举只在 io 层认。
enum class AmbientParticle { Dust, Firefly, Petal, Leaf, Rain, Snow, Ember, Mist };

struct Rgb {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
};

struct MetaLight {
    float x = 0.f;              // 格坐标（格中心 = x + 0.5）
    float y = 0.f;
    float radius = 3.f;         // 光照半径（格）
    Rgb color{255, 214, 160};
    float intensity = 1.f;
    float flicker = 0.f;        // 0 = 稳定；0.4 = 火把那样跳
    std::string kind;           // lantern / window / furnace / torch / candle / sconce / save …
};

struct MetaParticleLayer {
    AmbientParticle kind = AmbientParticle::Dust;
    float density = 0.f;        // meta 里的 rate：0..1 的浓淡
};

struct MetaEmitter {
    AmbientParticle kind = AmbientParticle::Ember;
    float x = 0.f;              // 格坐标
    float y = 0.f;
};

// 文件里写了什么就是什么；没写的留空，由 game::mapVisualFrom 按时辰补缺省——那张缺省表
// 与烘焙器 maplights.py 的 TIME_DEFAULTS 逐项相同，只在 game/MapVisual.cpp 里有一份。
struct MapMeta {
    std::string theme;
    TimeOfDay time = TimeOfDay::Day;   // 已定好：写了按写的，没写按主题推（室内三种主题取 indoor）
    std::optional<Rgb> ambient;
    std::optional<float> dof;
    std::optional<float> bloom;
    std::optional<float> vignette;
    std::vector<MetaParticleLayer> particles;   // 没写 = 空表，不替地图编粒子
    std::vector<MetaLight> lights;
    std::vector<Point> water;                   // 水面格
    std::vector<MetaEmitter> emitters;
    std::string backdrop;   // 这张图上开打时的战斗背景：assets/art/battle/<backdrop>/（缺 = 空串）
};

}  // namespace fanren::core
