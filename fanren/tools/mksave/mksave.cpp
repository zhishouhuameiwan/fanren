// 生成手测用的存档检查点。
//
// ---------------------------------------------------------------------------
// 这个工具是干什么的
// ---------------------------------------------------------------------------
// 游戏现在能读档了（`fanren --load <存档>`，见 src/main.cpp），但**没有任何一处
// 写档**——`io::saveGame` 除了测试之外没有调用方。于是「想手测第 4 章」就意味着
// 先把前三章打一遍，那是几个钟头。
//
// 这个工具把几个有用的时点直接写成存档文件，人拿着就能从那里开始玩。
// 它**不是给玩家的功能**，是给开发与验收用的夹具；所以放在 tools/ 而不是 src/。
//
// ---------------------------------------------------------------------------
// 一条要紧的纪律：这里造的状态必须与测试夹具**同源**
// ---------------------------------------------------------------------------
// 第 3 章章末那一份**不在这里手搭**：它读的是 tests/fixtures/ch03-end-first.sav，
// 即第 3 章通关测试走到终点时写出、并且每次都逐字段比着的那一份交接存档
//（tests/ChapterFixture.h）。第 4 章的通关测试读的也是它，与它不同的字段两边都只有
// 一处——位置（从前还有 ch01.muqin_bie，2026-09-27 交接存档带齐第 1、2 章旗标后删了）——
// 理由写在 tests/Ch04SliceTests.cpp 的 startFromChapterThreeEnding() 上，这里照做。
// **两边如果漂了，手测看到的就不是测试验过的那一章**；第 4 章独立校对的 CRITICAL-1、
// 复验判据 1 都是「起点对不上账」。重生成交接存档之后要重跑本工具
//（tests/NpcPresenceTests.cpp 有一条看着 saves/ch04-*.sav 与交接存档的旗标对不对得上）。
//
// 用法：
//   mksave <资产根目录> <输出目录>
// 生成：
//   ch04-start.sav   第 3 章章末交过来的那一份（炼气三层，站在韩家村）
//   ch04-siege.sav   节点 6 开战之前（炼气八层、两门法术、峰上炼下的药，站在演武场）
//   ch06-start.sav   第 5 章章末交过来的那一份（tests/fixtures/ch05-end-first.sav，炼气八层，站在南城东门里）
#include <cstdio>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "core/rules/Realm.h"
#include "io/DataLoader.h"
#include "io/SaveFile.h"

namespace {

using fanren::core::GameState;
using fanren::core::MapObject;
using fanren::core::Point;
using fanren::core::TileMap;
using fanren::rules::Realm;

// 把主角摆在某个地图对象**旁边一格能站人的地方**。
//
// 不摆在对象本身那一格：踏入型触发会在读档后的第一帧就响，玩家还没看清自己在哪
// 就被拖进剧情；而交互型触发要的恰恰是「站在旁边、面朝它按确认」。
[[nodiscard]] bool placeBeside(const TileMap& map, const std::string& objectName, Point& out) {
    const MapObject* target = nullptr;
    for (const MapObject& o : map.objects) {
        if (o.name == objectName) {
            target = &o;
            break;
        }
    }
    if (target == nullptr) return false;
    const Point base = target->position;
    const Point around[] = {{base.x - 1, base.y}, {base.x + 1, base.y},
                            {base.x, base.y - 1}, {base.x, base.y + 1},
                            {base.x - 2, base.y}, {base.x, base.y - 2}};
    for (const Point& p : around) {
        if (map.walkable(p)) {
            out = p;
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool spawnOf(const TileMap& map, Point& out) {
    const MapObject* spawn = map.defaultSpawn();
    if (spawn == nullptr) return false;
    out = spawn->position;
    return true;
}

// 第 3 章章末交到第 4 章手里的那一份：第 3 章通关测试写出的交接存档，原样读进来
// （位置由 main 摆到韩家村出生点）。与 tests/Ch04SliceTests.cpp 的
// startFromChapterThreeEnding() 一一对应。
// 从前这里还单独补了 ch01.muqin_bie：交接存档缺第 1 章旗标（docs/tech-debt.md G-23）。
// 2026-09-27 起那份存档带齐第 1、2 章目标链的完成旗标，这一行随之删掉——
// 缺那一批旗标时，第 1 章按它们撤场的人物会在这两份检查点里又站出来。
bool chapterThreeEnding(const std::string& assets, GameState& out) {
    const std::string fixture = assets + "/tests/fixtures/ch03-end-first.sav";
    auto handedOver = fanren::io::loadGame(fixture);
    if (!handedOver) {
        std::fprintf(stderr, "读不进第 3 章的交接存档 %s：%s\n", fixture.c_str(),
                     handedOver.error.c_str());
        return false;
    }
    out = handedOver.value;
    return true;
}

// 第 5 章章末交到第 6 章手里的那一份：第 5 章第一侧通关测试写出的交接存档，原样读进来
//（位置由 main 摆到南城东门里）。与 tests/Ch06AcceptanceTests.cpp 的 startFromChapterFiveEnding() 读的是同一份；
// 那边一个字段也不改，这里只改位置——第 5 章在墨府收场，第 6 章的入口是南城东门（docs/ch06-design.md 第 4 节），
// 墨府到南城那道门不设闸，走过去只改位置。
bool chapterFiveEnding(const std::string& assets, GameState& out) {
    const std::string fixture = assets + "/tests/fixtures/ch05-end-first.sav";
    auto handedOver = fanren::io::loadGame(fixture);
    if (!handedOver) {
        std::fprintf(stderr, "读不进第 5 章的交接存档 %s：%s\n", fixture.c_str(), handedOver.error.c_str());
        return false;
    }
    out = handedOver.value;
    return true;
}

// 节点 6「野狼帮来犯」开战之前。前五个节点的成果直接给上，省掉一年多的过场。
// 数目对着 tests/Ch04SliceTests.cpp 那一趟走到节点 6 跟前的实测（照价、每处取第一项）。
GameState beforeTheSiege(const GameState& start) {
    GameState s = start;
    // 节点 1-5 各自的完成旗标（取值 1 = 每一处二选一都取了第一项）。
    s.setFlag("ch04.liuxia", 1);
    s.setFlag("ch04.huodan_xue");
    s.setFlag("ch04.kailu", 1);
    s.setFlag("ch04.yufeng_xue");
    s.setFlag("ch04.fawu_bingyong", 1);
    // 本章的境界终点：炼气八层（设计文档 1.2 节，依据原著 ch75）。
    s.realm = Realm::QiRefining8;
    // 节点 4 的 realm.advance(八层) 顺带把上限抬到八层（技术债 G-14）。
    s.realmCap = Realm::QiRefining8;
    s.hp = s.maxHp = fanren::rules::realmMaxHp(s.realm);
    s.mp = s.maxMp = fanren::rules::realmMaxMp(s.realm);
    // 两门法术是节点 2 与节点 4 学的。
    s.learnMagic("magic_huodan_shu");
    s.learnMagic("magic_yufeng_jue");
    // 峰上那一年炼下的药：本章分次发的俸禄（照价那一条 45 炉的料）全部炼掉，
    // 通关测试那一趟炼出 23 瓶、熟练度炼到 100（docs/ch04-design.md 3.3）。
    // 料炼光了，背包里只剩第 3 章带来的零年黄精与紫参（方子用不上它们）。
    s.addItem("pill_yangjing_dan", 23, 0);
    s.alchemyProficiency = 100;
    s.cultivation += 40;           // 节点 5 切磋打赢的修为（b04_qiecuo_feiyu 的 rewards）
    s.day += 815;                  // 节点 1–4 走掉的日子：180 ＋ 270 ＋ 2 ＋ 363（节点 5 不走日子）
    return s;
}

[[nodiscard]] bool write(const GameState& state, const std::string& dir, const char* name) {
    const std::string path = dir + "/" + name;
    auto ok = fanren::io::saveGame(state, path);
    if (!ok) {
        std::fprintf(stderr, "写存档失败 %s: %s\n", path.c_str(), ok.error.c_str());
        return false;
    }
    std::printf("%s\n", path.c_str());
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "用法: mksave <资产根目录> <输出目录>\n");
        return 2;
    }
    const std::string assets = argv[1];
    const std::string out = argv[2];

    auto village = fanren::io::loadTileMap(assets + "/maps/ch01_hanjiacun.tmj");
    auto field = fanren::io::loadTileMap(assets + "/maps/ch04_yanwuchang.tmj");
    if (!village) {
        std::fprintf(stderr, "载入韩家村失败：%s\n", village.error.c_str());
        return 1;
    }
    if (!field) {
        std::fprintf(stderr, "载入演武场失败：%s\n", field.error.c_str());
        return 1;
    }

    GameState start;
    if (!chapterThreeEnding(assets, start)) return 1;
    // 位置：第 3 章在谷外收场，本章入口在韩家村东；中间六张图、六道门都不设闸，
    // 走过去只改位置（理由同 Ch04SliceTests::startFromChapterThreeEnding）。
    start.mapId = village.value.id;
    if (!spawnOf(village.value, start.position)) {
        std::fprintf(stderr, "韩家村没有出生点\n");
        return 1;
    }

    GameState siege = beforeTheSiege(start);
    siege.mapId = field.value.id;
    // 摆在开战那个踏入型触发的**旁边**：读档后往它走一步，戏就开了。
    if (!placeBeside(field.value, "trigger_kaizhan", siege.position)) {
        std::fprintf(stderr, "演武场上找不到 trigger_kaizhan，或它四周没有能站的格子\n");
        return 1;
    }

    // 第 6 章章首：第 5 章的交接存档，摆在南城东门（portal_to_tainan_cun）旁边一格、面朝东——往东一步出城。
    auto nancheng = fanren::io::loadTileMap(assets + "/maps/ch05_nancheng.tmj");
    if (!nancheng) {
        std::fprintf(stderr, "载入南城失败：%s\n", nancheng.error.c_str());
        return 1;
    }
    GameState chapterSix;
    if (!chapterFiveEnding(assets, chapterSix)) return 1;
    chapterSix.mapId = nancheng.value.id;
    if (!placeBeside(nancheng.value, "portal_to_tainan_cun", chapterSix.position)) {
        std::fprintf(stderr, "南城上找不到 portal_to_tainan_cun，或它四周没有能站的格子\n");
        return 1;
    }
    chapterSix.facing = 1;

    if (!write(start, out, "ch04-start.sav")) return 1;
    if (!write(siege, out, "ch04-siege.sav")) return 1;
    if (!write(chapterSix, out, "ch06-start.sav")) return 1;
    return 0;
}
