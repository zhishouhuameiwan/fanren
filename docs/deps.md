# 依赖清单与锁定策略

状态：本文档基于 `H:\Work\Kys\probe\` 探针工程的**真实构建与运行结果**编写（2026-09-20，Windows，MSVC v145 / VS "18" 2026，Ninja）。凡标注"验证通过"的条目均已实际跑通；未验证的条目会明确写出原因，不做未经验证的断言。

## 一、依赖矩阵

| 库 | 版本 | 获取方式 | URL | SHA256 | 验证状态 |
|---|---|---|---|---|---|
| SDL3 | 3.4.16 | 预编译 VC 包（bootstrap.py，复用自 fanren-sdl3/vendor） | https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-devel-3.4.16-VC.zip | `1a784cb2a5c64d56fe7a62090fe9d242d9865f235e4ea9678f1a6ba4e693e7de` | 验证通过 |
| SDL3_ttf | 3.2.2 | 预编译 VC 包（bootstrap.py，复用自 fanren-sdl3/vendor） | https://github.com/libsdl-org/SDL_ttf/releases/download/release-3.2.2/SDL3_ttf-devel-3.2.2-VC.zip | `67805c5babfc49ca0c56882dc9b8cabbcdd1e6f9edde10ddac91ddb38f3afb8c` | 验证通过 |
| SDL3_image | 3.4.6 | 预编译 VC 包（bootstrap.py 新下载） | https://github.com/libsdl-org/SDL_image/releases/download/release-3.4.6/SDL3_image-devel-3.4.6-VC.zip | `03c6b313623edadf707a7c187e2036a5be5f12e693025c0697833379970bb4c0` | 验证通过 |
| SDL3_mixer | 3.2.4 | 预编译 VC 包（bootstrap.py 新下载） | https://github.com/libsdl-org/SDL_mixer/releases/download/release-3.2.4/SDL3_mixer-devel-3.2.4-VC.zip | `f4263ed5082fb7018059d64952017534e26821e9e878ce6b8c924b77cb17c4fb` | 验证通过 |
| Lua | 5.4.9（5.4 系列最终版，官方声明"不再有后续 5.4 发布"） | 官方源码 tar.gz（bootstrap.py 下载，本地编译为静态库） | https://www.lua.org/ftp/lua-5.4.9.tar.gz | `2335b6c582a52654f94612bf10d2f4672805d05329aa6568b1d8cd9e5c6fb8e6` | 验证通过 |
| sol2 | v3.3.0（GitHub 最后一个正式 Release；见下方"版本选择说明"） | 源码 zip（bootstrap.py 下载，仅取 include/ 头文件） | https://github.com/ThePhD/sol2/archive/refs/tags/v3.3.0.zip | `a7489629c596c8a67108ad3603cb6a90073ba6647e50441c8c55492254190d67` | 验证通过 |
| nlohmann-json | v3.12.0 | 官方发布的单头文件（bootstrap.py 下载） | https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp | `aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63` | 验证通过 |
| GoogleTest | 1.15.2 | **vendor 源码包 + bootstrap.py SHA256 校验**（P1 改用，不再用 FetchContent） | https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz | `7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926` | P1 已接入，见第 2.3 节 |

### SDL3 系列四库可得性结论（明确回答任务问题）

与任务描述中"卫星库历史上落后主库"的顾虑不同，**当前时间点（2026-09-20）四个 SDL3 系列库均已有正式 release 且都带 Windows VC 预编译包**：

- SDL3：`release-3.4.16`，发布于 2026-09-02。
- SDL3_ttf：`release-3.2.2`，发布于 2025-03-31（对四库中最旧，但仍是 SDL3_ttf 目前的最新正式版）。
- SDL3_image：`release-3.4.6`，发布于 2026-09-02，资产列表中含 `SDL3_image-devel-3.4.6-VC.zip`。
- SDL3_mixer：`release-3.2.4`，发布于 2026-06-03，资产列表中含 `SDL3_mixer-devel-3.2.4-VC.zip`。

四者的 VC 包解压后目录结构一致（`cmake/`、`include/`、`lib/{x86,x64,arm64}/`），均可用 `find_package(<Name> CONFIG REQUIRED)` 直接消费，**不需要替代方案**。

### sol2 版本选择说明

sol2 在 GitHub 上只有 `v3.3.0`（2022-06-25）被标记为正式 Release；仓库里还有更新的 tag（`v3.5.0`、`v4.0.0-alpha`），但这些 tag 对应的 `CMakeLists.txt` 已经把内部 `project(sol2 VERSION 4.0.0 ...)`，默认 `SOL2_BUILD_LUA=TRUE` 会自己用 FetchContent 拉取并编译一份 Lua——这与本工程"自带 vendored Lua 5.4.9"的策略冲突，且 4.0 线仍标注 alpha。因此选用最后一个正式 Release `v3.3.0`：它对 Lua 5.1–5.4 都有官方支持，且本探针**只取用它的 `include/` 头文件目录，完全不执行 sol2 自带的 CMakeLists.txt**（避免上述冲突）。P1 若想跟进更新的 sol2 tag，需要同样只取头文件、绕开其 CMakeLists。

## 二、获取策略

### 2.1 预编译包（SDL3 / SDL3_ttf / SDL3_image / SDL3_mixer）

- 通过 `probe/bootstrap.py` 下载官方 `*-devel-*-VC.zip`，下载后立即计算 SHA256 并与脚本里写死的值比对，不一致则 `RuntimeError` 中止。
- 解压时对每个 zip entry 做路径穿越检查（`target.is_relative_to(vendor)`），防止恶意 zip 条目写到 vendor 目录之外（zip-slip）。
- SDL3 / SDL3_ttf 两个包版本与 `fanren-sdl3` 现有工程完全一致，`bootstrap.py` 会优先从 `..\fanren-sdl3\vendor\` 复制过来，避免重复下载（并且两边校验值也已核对一致）。
- **锁定手段**：把具体版本号写进目录名（如 `vendor/SDL3-3.4.16`）+ CMakeLists 里 `find_package(SDL3 3.4.16 CONFIG REQUIRED)` 做双重锁定——升级版本必须同时改文件名、SHA256 和 CMake 里的版本号，任何一处漏改都会在 configure 阶段直接报错，不会静默用错版本。

### 2.2 源码型依赖（Lua / sol2 / nlohmann-json）

三者都选择"vendor 源码 + SHA256 锁定"而不是构建期 FetchContent 联网拉取，原因：

1. 构建环境（尤其 CI／离线开发机）不一定能连到 GitHub / lua.org；把下载动作集中在 `bootstrap.py`（一次性、有重试友好的错误提示）比让 `cmake configure` 阶段联网更可控。
2. `bootstrap.py` 已经统一做 SHA256 校验，和预编译包用的是同一套机制，文档和心智模型不用分叉。
3. sol2 的官方 CMakeLists.txt 有前面提到的"默认自己编译 Lua"问题，用 FetchContent 直接 `add_subdirectory` 反而更麻烦；不如只取头文件。

- **Lua 5.4.9**：下载官方源码 tar.gz，`bootstrap.py` 解压到 `vendor/lua-5.4.9/`；`CMakeLists.txt` 用 `file(GLOB ... src/*.c)` 收集全部 `.c`，剔除 `lua.c`（独立解释器 `lua.exe` 的 `main()`）和 `luac.c`（编译器 `luac.exe` 的 `main()`），其余文件编译成静态库 `lua54`。
- **sol2 v3.3.0**：下载 GitHub 源码 zip，`bootstrap.py` 解压到 `vendor/sol2-3.3.0/`；`CMakeLists.txt` 只把 `vendor/sol2-3.3.0/include` 加为 INTERFACE 头文件目录并链接 `lua54`，不 `add_subdirectory` 它自己的 `CMakeLists.txt`。
- **nlohmann-json v3.12.0**：直接下载官方发布的独立单头文件 `json.hpp`（release 资产里专门提供，不需要整个仓库），`CMakeLists.txt` 用 `file(COPY)` 把它复制到 `<build>/nlohmann_json_include/nlohmann/json.hpp`，这样业务代码可以按官方文档写法 `#include <nlohmann/json.hpp>`，而不是裸的 `#include "json.hpp"`。

### 2.3 GoogleTest（P1 已接入，获取方式已变更）

**P1 决策：改用 vendor 源码包 + `add_subdirectory`，不用 FetchContent。**

P0 原本建议 `FetchContent_Declare(... GIT_TAG v1.15.2)`。P1 首次接入时实测踩到两个问题，故改方案：

1. **git clone 会在构建被中断后留下锁文件。** 一次配置超时被中止后，`_deps/googletest-subbuild` 下的
   ninja 文件与 `googletest-src/.git` 的 pack 临时文件被残留的 `cmake` / `ninja` / `git` 进程占住，
   下一次 configure 直接失败：`ninja: error: failed recompaction: Permission denied`。
   必须手工杀进程 + 删 `build/` 才能恢复——这在 CI 和多人协作里是反复踩的坑。
2. **每次干净构建都要联网。** 与本项目"vendor + SHA256 锁定、可离线重复构建"的策略不一致。

现行做法与其他依赖完全一致：`bootstrap.py` 下载 tarball 并校验 SHA256 到 `vendor/googletest-1.15.2`，
CMake 用 `add_subdirectory(... EXCLUDE_FROM_ALL)` 接入。`gtest_force_shared_crt ON` 仍然必须设置（原因见第三节）。

## 三、MSVC 构建注意事项

以下每一条都是本次探针构建过程中实际会踩、或已经验证过的问题，不是泛泛而谈：

- **`/bigobj`**：sol2 的模板展开确实很重。本探针的 `main.cpp`（一个 `sol::state`、一次基础脚本执行、一段协程 yield/resume）**实测在去掉 `/bigobj` 的情况下也能编译通过**——也就是说对这么小的用法不是硬性必需。但真实项目一旦开始注册多个 `usertype`（游戏里大概率会有十几个到几十个 C++ 类型暴露给 Lua），obj 文件的 section 数很容易超过默认上限触发 `C1128`。因此 CMakeLists 里默认保留 `/bigobj`（成本几乎为零，只是让 obj 支持更多 section），当作防御性默认值而非"探针证明它必需"。
- **`/W3` 而非 `/W4 /permissive-`**：探针刻意把警告级别降到 `/W3`，因为 `main.cpp` 会直接 `#include` 我们不拥有的第三方头（Lua C 头、sol2、nlohmann-json），用 `/W4 /permissive-` 会把这些头文件里的告警也刷出来，淹没真正需要关注的项目自身代码告警。**真实游戏项目自己的源码仍应该用 `/W4 /permissive-`**（`fanren-sdl3/CMakeLists.txt` 就是这么做的），只有对不可控的第三方 vendor 代码才降级或用 `SYSTEM` include。CMake 3.25+ 可以用 `target_include_directories(... SYSTEM ...)` 更精确地只对第三方头降噪，而不是全局降警告级别——P1 实现真实工程时建议采用这个更精细的做法，而不是照抄探针的"全局 /W3"。
- **Lua 必须按 C 编译，不能当 C++**：Lua 的 `.c` 源文件必须由 C 编译器编译并保持 C 链接（no name mangling），因为 sol2 通过 `lua.hpp`（`extern "C" { #include "lua.h" ... }`）以 C 链接方式声明这些符号。本工程通过 `project(... LANGUAGES C CXX)` 声明双语言，且 Lua 源文件保持 `.c` 扩展名，CMake 会自动用 C 编译器编译它们，探针里额外用 `set_source_files_properties(${LUA_SOURCES} PROPERTIES LANGUAGE C)` 显式声明以防止未来有人把源文件列表改错。**如果误把 Lua 源码当 C++ 编译（比如改扩展名为 .cpp，或用错误的语言覆盖），符号名会被 C++ 名字修饰改变，链接 sol2 时会得到一堆 `unresolved external symbol lua_xxx` 之类的错误**——这是任务描述里点名要写进文档的坑，本次确认了原因链路但探针本身按正确方式配置，因此没有实际触发这个错误。
- **`gtest_force_shared_crt`（P1 需要，本探针未包含 GoogleTest，故未实测）**：这是 GoogleTest 官方文档记载的已知问题，不是本探针实测出来的——GoogleTest 默认用静态 CRT（`/MT` / `/MTd`）编译，而 CMake 在 MSVC 下的默认运行时库是动态的（`/MD` / `/MDd`）。如果项目其余部分（包括本探针里的 SDL3 系列预编译包，它们本身就是用 `/MD` 编译发布的）用 `/MD`，直接把默认配置的 GoogleTest 链接进来会在链接期报 `LNK2038`（`RuntimeLibrary` 值不一致）。设置 `gtest_force_shared_crt ON` 让 GoogleTest 也用 `/MD`，与其余目标保持一致。P1 引入 GoogleTest 时必须验证这一点，不能假设"应该没问题"。
- **运行时库一致性（本探针已验证无冲突）**：探针最终构建（`CMAKE_BUILD_TYPE=Release`，MSVC 默认 `/MD`）中，SDL3 系列四个预编译包（官方发布物，均为 `/MD` 动态运行时）+ 本地编译的 `lua54` 静态库（同样默认 `/MD`）+ probe.exe 自身，**链接期没有出现任何 `LNK4098`（运行时库不匹配）告警**，说明当前这套组合运行时库是一致的。这一条只保证了"探针验证过的这套依赖版本组合"没问题；升级任何一个预编译包版本后应重新确认。
- **`vcvars64.bat` 的 `vswhere.exe` 告警**：每次运行 `vcvars64.bat` 都会打印一行 `'vswhere.exe' is not recognized as an internal or external command`。这是当前机器 `PATH` 里没有 `vswhere.exe`（通常在 `%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\`）导致的良性告警——`vcvarsall.bat` 内部本来会尝试用它做一些可选检测，找不到时会跳过但仍继续正确设置好编译器环境（探针的 configure/build 日志证明了这点：C/CXX 编译器都被正确识别为 `MSVC 19.51.36256.0`）。**不影响构建结果，可以忽略，但如果想根治可以把 `vswhere.exe` 所在目录加进 `PATH`。**
- **中文字符串字面量**：`main.cpp` 里直接写了含中文的 `nlohmann::json` 源码字符串字面量（`R"({"角色":"韩立",...})"`），配合 `/utf-8` 编译选项（同时指定源文件编码和执行字符集为 UTF-8），实测控制台输出中文正常，没有乱码——这验证了 `fanren-sdl3/CMakeLists.txt` 里已经在用的 `/utf-8` 选项在新库组合下依然有效。
- **DLL 部署**：`add_custom_command(... $<TARGET_RUNTIME_DLLS:probe> ...)` 会自动把 `SDL3.dll`、`SDL3_ttf.dll`、`SDL3_image.dll`、`SDL3_mixer.dll` 拷贝到 `probe.exe` 旁边（探针构建后实测存在且 `probe.exe` 可独立运行，不依赖额外 `PATH` 设置）。但 `SDL3_image` / `SDL3_mixer` 的 `lib/x64/optional/` 目录下还有一批可选编解码器 DLL（如 `libavif-16.dll`、`libpng16-16.dll`、`libwebp-*.dll`、`libogg-0.dll`、`libopus-0.dll` 等）——这些是运行期按需 `dlopen` 的可选插件，`$<TARGET_RUNTIME_DLLS>` 不会自动拷贝它们，因为它们不是 probe.exe 的直接链接依赖。探针不加载任何真实图片/音频文件，所以用不到；**真实游戏项目一旦要读 PNG/WebP/OGG 等格式，需要额外把用到的 optional DLL 手动拷贝到输出目录**，这是 P1 要处理的部署细节，已记入第五节风险清单。

## 四、探针工程验证结果

探针位置：`H:\Work\Kys\probe\`（`bootstrap.py` / `CMakeLists.txt` / `src\main.cpp` / `build.bat`）。

逐库验证状态：

| 检查项 | 状态 | 说明 |
|---|---|---|
| SDL3 核心（`SDL_Init` / `SDL_GetVersion` / `SDL_Quit`） | 验证通过 | |
| SDL3_ttf（`TTF_Init` / `TTF_Version` / `TTF_Quit`） | 验证通过 | |
| SDL3_image（`IMG_Version`） | 验证通过（API 差异见下） | 该版本 SDL3_image **已不存在 `IMG_Init`/`IMG_Quit`**（对比 SDL2_image 的旧 API），格式解码器改为按需懒初始化，探针只调用仍然存在的 `IMG_Version()` 来证明确实链接到了真实的 `SDL3_image.dll`。 |
| SDL3_mixer（`MIX_Init` / `MIX_Version` / `MIX_Quit`） | 验证通过（API 差异见下） | 该版本 SDL3_mixer **把整套公开 API 从旧的 `Mix_*`（基于 channel）重命名/重构为 `MIX_*`**（基于 `MIX_Mixer`/`MIX_Track`/`MIX_Audio` 对象模型）。`MIX_Init()` 只加载编解码器插件、不打开音频设备，因此在没有物理声卡的环境下也应能成功（本机实测环境下 `SDL_Init(SDL_INIT_AUDIO)` 与 `MIX_Init()` 均返回成功）。 |
| nlohmann-json（含中文 key/value 的解析） | 验证通过 | |
| sol2 + Lua 基础脚本执行 | 验证通过 | |
| sol2 + Lua **协程 yield/resume**（项目关键依赖） | 验证通过 | 三次 `coroutine.yield`、状态在 `sol::call_status::yielded` 与 `sol::call_status::ok`（dead）之间正确转换、最终返回值正确取回。 |
| GoogleTest | P1 已接入并实跑（vendor 方式） | 获取方式变更原因见第 2.3 节；P0 探针范围内确实未验证。 |

`probe.exe` 的真实控制台输出（`H:\Work\Kys\probe\build\probe.exe`，Release 构建，退出码 `0`）：

```
[ OK ] SDL3 core: SDL_GetVersion=3.4.16 SDL_Init(AUDIO)=ok
[ OK ] SDL3_ttf: TTF_Version=3.2.2 TTF_Init=ok
[ OK ] SDL3_image: IMG_Version=3.4.6 (no IMG_Init/IMG_Quit in this API version)
[ OK ] SDL3_mixer: MIX_Version=3.2.4 MIX_Init=ok
[ OK ] nlohmann-json: parsed 角色=韩立 境界=筑基期 灵石=12345
[ OK ] sol2/Lua basic: 6 * 7 == 42
[ OK ] sol2/Lua coroutine: yields=1,2,3 totals=1,3,6 final="coroutine-finished" status=ok(dead)

---- summary ----
SDL3 core        PASS
SDL3_ttf         PASS
SDL3_image       PASS
SDL3_mixer       PASS
nlohmann-json    PASS
sol2/Lua basic   PASS
sol2/Lua coroutine PASS
PROBE_OK
```

上述结果通过完整流程复现过两次：一次是首次构建（`vendor/` 从空开始，`bootstrap.py` 实际下载 SDL3_image / SDL3_mixer / Lua / sol2 / json.hpp 并校验 SHA256），一次是删除 `build/` 目录后的干净重新构建（`bootstrap.py` 命中缓存，直接走 `cmake configure` → `ninja build` → 运行 `probe.exe`）。两次的 36 步 Ninja 构建均无编译错误、无编译警告，`probe.exe` 均以退出码 `0` 打印 `PROBE_OK`。

## 五、已知风险与待办

1. **SDL3_image / SDL3_mixer 的 API 相比 SDL2 系列有破坏性变化**，是本次调研中最值得 P1 团队提前知道的一点：
   - `SDL3_image` 去掉了 `IMG_Init(IMG_INIT_PNG|...)` / `IMG_Quit()`，改为懒加载；如果团队里有人参考 SDL2_image 教程写代码，会找不到这两个函数。
   - `SDL3_mixer` 把整个 API 从 `Mix_*`（`Mix_OpenAudio`/`Mix_PlayChannel`/...）重构为 `MIX_*`（`MIX_CreateMixerDevice`/`MIX_Track`/`MIX_Audio`/...），是一次概念级的重新设计（channel 模型 → mixer/track/audio 对象模型），不是简单改名。P1 做音频系统设计时必须按新 API 的对象模型来，直接照搬 SDL2_mixer 时代的示例代码会完全不可用。
2. **`SDL3_image`/`SDL3_mixer` 的可选编解码器 DLL 未纳入自动部署**（`lib/x64/optional/` 下的 `libavif`/`libpng`/`libwebp`/`libogg`/`libopus`/`libgme`/`libxmp` 等）。P1 需要根据美术资源实际用到的格式（大概率至少要 PNG，音频可能要 OGG/MP3），在 CMake 里显式把对应 optional DLL 拷贝到输出目录，并在 deps 矩阵里补充这些间接依赖的版本/许可证信息（它们各自还有自己的 LICENSE 文件，需要一并核查开源许可证合规性，尤其 `libgme`/`libxmp` 这类模块音乐库的许可证条款可能与主项目授权方式不完全兼容，需要单独确认）。
3. **GoogleTest 完全未在 P0 验证**，包括 `gtest_force_shared_crt`、与本探针里 SDL3/Lua 静态库的运行时库一致性、以及 `enable_testing()`/`gtest_discover_tests()` 在本项目 CMake 结构下的接入方式。这是 P1 启动 TDD 工作流前必须先补的一块，风险等级中等（技术方案成熟、文档充分，但"未跑过"就是"未跑过"）。
4. **sol2 版本选择偏保守（v3.3.0，2022 年发布）**，比仓库里最新 tag 落后约 3 年。目前功能验证（含协程）完全够用，但如果 P1 需要 v3.3.0 之后修的某个具体 bug 或新增 API，需要重新评估是否切到更新的 tag，并重新走一遍"只取 include/、绕开其 CMakeLists"的集成方式（因为新版本 tag 的 CMakeLists.txt 结构改动更大，`SOL2_BUILD_LUA` 默认行为的冲突可能不止一处）。
5. **Lua 独立静态库与 sol2 的版本耦合**：`SOL2_LUA_VERSION` 在 sol2 官方 CMakeLists 里默认写的是 `5.4.4`，本工程实际 vendor 的是 `5.4.9`。因为本工程绕开了 sol2 自己的 CMakeLists（只用它的头文件），这个默认值没有被实际用到，不构成风险；但如果 P1 未来决定改用 sol2 官方的 CMakeLists 全量集成方式，需要显式覆盖 `SOL2_LUA_VERSION` 为 `5.4.9`（或改用系统检测模式），否则可能触发它自带的 FetchContent 逻辑再拉一份不同版本的 Lua。
6. **探针未覆盖真实窗口/渲染路径**：本探针是"无窗口冒烟测试"，只验证了库能否初始化和链接，没有验证 `SDL_CreateWindow`/`SDL_CreateRenderer`/实际渲染一帧/实际播放一段音频这些更贴近真实使用场景的路径。这些放在 P1（真正建 `fanren` 工程时）用真实渲染循环验证更合适，P0 阶段不做是有意的范围控制，但不应被误读为"渲染路径已验证"。
7. **`vcvars64.bat` 路径硬编码为 VS "18" (2026) Community 版**（`C:\Program Files\Microsoft Visual Studio\18\Community\...`），与 `fanren-sdl3/build.bat` 保持一致。如果 CI 机器或其他开发者机器上 VS 安装路径/版本不同（比如 Professional 版，或版本号不是 "18"），`build.bat` 会在这一步直接失败，需要相应机器上手动改路径或改造成用 `vswhere.exe` 动态探测（这也顺带能解决上面提到的 `vswhere.exe` 告警）。
