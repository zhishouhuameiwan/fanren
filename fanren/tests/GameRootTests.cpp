// 游戏根目录的自动定位（io/GameRoot.h）：双击 build\fanren.exe 不带 --assets 也要找得到 data/ maps/。
//
// 判据抄头文件的约定：同时有 data/ 与 maps/ 才算根；起点本身之外往上找 kGameRootSearchDepth 层；
// 起点依次试、先到先得；带尾分隔符的起点（SDL_GetBasePath 的样子）不少走一层；找不到返回空路径。
// 一律在临时目录里摆布局（tests/TempDir.h），不碰工作目录。
#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include "TempDir.h"
#include "io/GameRoot.h"

namespace {

namespace fs = std::filesystem;

using fanren::io::kGameRootSearchDepth;
using fanren::io::locateGameRoot;

// 在 dir 底下摆出一个游戏根的最小样子。
void makeGameRoot(const fs::path& dir) {
    fs::create_directories(dir / "data");
    fs::create_directories(dir / "maps");
}

// 往上找会走出起点所在的目录。「找不到」的用例要是一路走进 %TEMP% 本身，碰巧那里有 data/ 与 maps/
// 就会假红；所以这类布局都摆在临时目录往下 kGameRootSearchDepth 层的沙箱里，走满层数也出不了临时目录。
fs::path sandbox(const fanren::test::TempDir& tmp) {
    fs::path dir = tmp.path();
    for (int level = 0; level < kGameRootSearchDepth; ++level) dir /= "s" + std::to_string(level);
    fs::create_directories(dir);
    return dir;
}

// 比较前两边都规范化，免得「H:/x」与「H:\x」、大小写之外的写法差异误报。
fs::path normal(const fs::path& p) { return fs::weakly_canonical(p); }

}  // namespace

// 双击的样子：起点是 <根>/build-<槽>/（带尾分隔符，SDL_GetBasePath 返回的就是这样），往上一层就是根。
TEST(GameRoot, FindsRootAboveExeDirectory) {
    fanren::test::TempDir tmp("fanren_gameroot");
    makeGameRoot(tmp.path());
    const fs::path buildDir = tmp.path() / "build-slot";
    fs::create_directories(buildDir);

    const fs::path found = locateGameRoot({buildDir.string() + "\\"});
    ASSERT_FALSE(found.empty());
    EXPECT_EQ(normal(found), normal(tmp.path()));
}

// 命令行从工程根启动的样子：起点本身就是根，不许往上走过头。
TEST(GameRoot, StartItselfIsRoot) {
    fanren::test::TempDir tmp("fanren_gameroot");
    makeGameRoot(tmp.path());

    const fs::path found = locateGameRoot({tmp.path()});
    ASSERT_FALSE(found.empty());
    EXPECT_EQ(normal(found), normal(tmp.path()));
}

// 只有 maps/ 的目录不算（tests/ 底下就有 maps/）；只有 data/ 的也不算。
TEST(GameRoot, NeedsBothDataAndMaps) {
    fanren::test::TempDir tmp("fanren_gameroot");
    const fs::path box = sandbox(tmp);
    fs::create_directories(box / "only_maps" / "maps");
    fs::create_directories(box / "only_data" / "data");

    EXPECT_TRUE(locateGameRoot({box / "only_maps"}).empty());
    EXPECT_TRUE(locateGameRoot({box / "only_data"}).empty());
}

// 起点依次试、先到先得：第一个起点（工作目录）往上找不到，才轮到第二个（exe 目录）。
TEST(GameRoot, FallsBackToLaterStart) {
    fanren::test::TempDir tmp("fanren_gameroot");
    const fs::path elsewhere = sandbox(tmp) / "elsewhere";
    const fs::path root = tmp.path() / "game";
    fs::create_directories(elsewhere);
    makeGameRoot(root);
    fs::create_directories(root / "build");

    const fs::path found = locateGameRoot({fs::path{}, elsewhere, root / "build"});
    ASSERT_FALSE(found.empty());
    EXPECT_EQ(normal(found), normal(root));
}

// 两个起点都能找到时取第一个的：命令行 `cd` 到哪份工程就用哪份，不被 exe 所在的那份抢走。
TEST(GameRoot, EarlierStartWins) {
    fanren::test::TempDir tmp("fanren_gameroot");
    const fs::path first = tmp.path() / "first";
    const fs::path second = tmp.path() / "second";
    makeGameRoot(first);
    makeGameRoot(second);

    const fs::path found = locateGameRoot({first, second});
    EXPECT_EQ(normal(found), normal(first));
}

// 层数有上限：根在起点往上 kGameRootSearchDepth 层找得到，再多一层就找不到。
TEST(GameRoot, SearchDepthIsBounded) {
    fanren::test::TempDir tmp("fanren_gameroot");
    makeGameRoot(tmp.path());
    fs::path atLimit = tmp.path();
    for (int level = 0; level < kGameRootSearchDepth; ++level) atLimit /= "d" + std::to_string(level);
    const fs::path beyond = atLimit / "one_more";
    fs::create_directories(beyond);

    EXPECT_EQ(normal(locateGameRoot({atLimit})), normal(tmp.path()));
    EXPECT_TRUE(locateGameRoot({beyond}).empty());
}

// 哪里都找不到：如实返回空路径，不猜一个缺省值出来。
TEST(GameRoot, NothingFoundIsEmpty) {
    fanren::test::TempDir tmp("fanren_gameroot");
    const fs::path deep = sandbox(tmp) / "deep";
    fs::create_directories(deep);

    EXPECT_TRUE(locateGameRoot({deep}).empty());
    EXPECT_TRUE(locateGameRoot({}).empty());
}
