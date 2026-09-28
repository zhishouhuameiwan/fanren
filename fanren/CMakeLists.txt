cmake_minimum_required(VERSION 3.25)
project(Fanren VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

set(VENDOR "${CMAKE_CURRENT_SOURCE_DIR}/vendor")

# ---------------------------------------------------------------------------
# Compile options
#
# Every target we write builds with /W4 /permissive- and must stay warning
# free. Noise from third party headers we do not maintain (sol2, Lua,
# nlohmann-json) is silenced on the dependency target itself: its include
# directory is declared SYSTEM, so CMake passes it as "/external:I <dir>
# /external:W0" and those headers compile at warning level 0. Our own sources
# keep /W4 regardless of which dependency they pull in.
#
# Do NOT go back to lowering the warning level per consuming target. There used
# to be a fanren_thirdparty_warnings() that added /W3 /bigobj; CMake appends
# both helpers' flags, so cl saw ".. /W4 /permissive- /W3 /bigobj", answered
# with "command line warning D9025: overriding /W4 with /W3" on every io and
# script TU, and -- the part that actually mattered -- compiled our own io and
# script sources at /W3 instead of the /W4 this comment claimed.
# ---------------------------------------------------------------------------
if(MSVC)
    add_compile_options(/utf-8 /EHsc)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
endif()

function(fanren_own_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endfunction()

# ---------------------------------------------------------------------------
# Third-party dependencies
#
# All vendored and SHA256-locked by bootstrap.py. Run it before configuring.
# Versions here are the exact ones validated by the P0 link probe.
# ---------------------------------------------------------------------------
list(PREPEND CMAKE_PREFIX_PATH
    "${VENDOR}/SDL3-3.4.16"
    "${VENDOR}/SDL3_ttf-3.2.2"
    "${VENDOR}/SDL3_image-3.4.6"
    "${VENDOR}/SDL3_mixer-3.2.4")

find_package(SDL3 3.4.16 CONFIG REQUIRED)
find_package(SDL3_ttf 3.2.2 CONFIG REQUIRED)
find_package(SDL3_image 3.4.6 CONFIG REQUIRED)
find_package(SDL3_mixer 3.2.4 CONFIG REQUIRED)

# nlohmann-json: single header, re-homed so <nlohmann/json.hpp> works.
set(NLOHMANN_JSON_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/nlohmann_json_include")
file(COPY "${VENDOR}/json.hpp" DESTINATION "${NLOHMANN_JSON_INCLUDE_DIR}/nlohmann")
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json SYSTEM INTERFACE "${NLOHMANN_JSON_INCLUDE_DIR}")

# Lua 5.4.9. MUST be compiled as C: sol2 assumes C linkage via lua.hpp's
# extern "C". Sources keep their .c extension and the project declares
# LANGUAGES C CXX, so CMake picks the C compiler automatically.
set(LUA_SRC_DIR "${VENDOR}/lua-5.4.9/src")
file(GLOB LUA_SOURCES CONFIGURE_DEPENDS "${LUA_SRC_DIR}/*.c")
list(REMOVE_ITEM LUA_SOURCES "${LUA_SRC_DIR}/lua.c" "${LUA_SRC_DIR}/luac.c")
add_library(lua54 STATIC ${LUA_SOURCES})
target_include_directories(lua54 SYSTEM PUBLIC "${LUA_SRC_DIR}")
set_source_files_properties(${LUA_SOURCES} PROPERTIES LANGUAGE C)

# sol2 v3.3.0, headers only. Its own CMakeLists is deliberately not used: it
# would FetchContent its own Lua and fight the vendored lua54 target.
add_library(sol2 INTERFACE)
target_include_directories(sol2 SYSTEM INTERFACE "${VENDOR}/sol2-3.3.0/include")
target_link_libraries(sol2 INTERFACE lua54)

# ---------------------------------------------------------------------------
# Layer: core -- pure rules.
#
# ARCHITECTURAL CONSTRAINT: core links NOTHING. No SDL, no JSON, no Lua, no
# file IO. It operates on already-parsed structs so every rule is unit
# testable without a window, a filesystem or a script runtime. Adding a
# target_link_libraries() entry here is a design change, not a convenience.
# ---------------------------------------------------------------------------
file(GLOB_RECURSE FANREN_CORE_SOURCES CONFIGURE_DEPENDS "src/core/*.cpp")
add_library(fanren_core STATIC ${FANREN_CORE_SOURCES})
target_include_directories(fanren_core PUBLIC src)
fanren_own_warnings(fanren_core)

# ---------------------------------------------------------------------------
# Layer: io -- JSON loading, save serialisation, Tiled parsing. No SDL.
# ---------------------------------------------------------------------------
file(GLOB_RECURSE FANREN_IO_SOURCES CONFIGURE_DEPENDS "src/io/*.cpp")
if(FANREN_IO_SOURCES)
    add_library(fanren_io STATIC ${FANREN_IO_SOURCES})
    target_include_directories(fanren_io PUBLIC src)
    target_link_libraries(fanren_io PUBLIC fanren_core PRIVATE nlohmann_json)
    fanren_own_warnings(fanren_io)
endif()

# ---------------------------------------------------------------------------
# Layer: script -- Lua bindings and the coroutine scheduler.
#
# Depends on core only. Event API calls push command objects onto a queue;
# the game layer consumes them and resumes the coroutine with the result.
# script must NOT link engine or game (see plan doc 3.2 / 3.5).
# ---------------------------------------------------------------------------
file(GLOB_RECURSE FANREN_SCRIPT_SOURCES CONFIGURE_DEPENDS "src/script/*.cpp")
if(FANREN_SCRIPT_SOURCES)
    add_library(fanren_script STATIC ${FANREN_SCRIPT_SOURCES})
    target_include_directories(fanren_script PUBLIC src)
    target_link_libraries(fanren_script PUBLIC fanren_core PRIVATE sol2)
    fanren_own_warnings(fanren_script)
    if(MSVC)
        # /bigobj: sol2's template instantiations can overflow the default
        # section count in a single .obj. Not strictly required at the current
        # binding volume (the P0 probe built fine without it) but kept as a
        # cheap defensive default; usertype-heavy bindings are known to need it.
        # It is a codegen flag, not a warning flag, so it does not fight /W4.
        target_compile_options(fanren_script PRIVATE /bigobj)
    endif()
endif()

# ---------------------------------------------------------------------------
# Layer: engine -- SDL3 platform layer.
#
# Window, renderer, atlases, fonts and CJK text layout, input, audio,
# resolution handling. Note SDL3_image has no IMG_Init and SDL3_mixer uses the
# MIX_* object model, not SDL2's channel-based Mix_*; wrap both here so the
# rest of the codebase never touches those APIs directly.
# ---------------------------------------------------------------------------
file(GLOB_RECURSE FANREN_ENGINE_SOURCES CONFIGURE_DEPENDS "src/engine/*.cpp")
if(FANREN_ENGINE_SOURCES)
    add_library(fanren_engine STATIC ${FANREN_ENGINE_SOURCES})
    target_include_directories(fanren_engine PUBLIC src)
    target_link_libraries(fanren_engine
        PUBLIC SDL3::SDL3 SDL3_ttf::SDL3_ttf
        PRIVATE SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer)
    fanren_own_warnings(fanren_engine)
endif()

# ---------------------------------------------------------------------------
# Layer: ui -- shared widgets (text box, list, choices, grid, panel).
# Built once here so dialogue / shop / codex / menus do not each grow their own.
# ---------------------------------------------------------------------------
file(GLOB_RECURSE FANREN_UI_SOURCES CONFIGURE_DEPENDS "src/ui/*.cpp")
if(FANREN_UI_SOURCES)
    add_library(fanren_ui STATIC ${FANREN_UI_SOURCES})
    target_include_directories(fanren_ui PUBLIC src)
    target_link_libraries(fanren_ui PUBLIC fanren_engine)
    fanren_own_warnings(fanren_ui)
endif()

# ---------------------------------------------------------------------------
# Layer: game -- scene state machine and script command dispatch.
#
# Built as a library rather than compiled straight into the executable so the
# headless slice test can drive the very same code the player runs. main.cpp
# stays a thin entry point with no game logic in it.
# ---------------------------------------------------------------------------
file(GLOB_RECURSE FANREN_GAME_SOURCES CONFIGURE_DEPENDS "src/game/*.cpp")
if(FANREN_GAME_SOURCES)
    add_library(fanren_game STATIC ${FANREN_GAME_SOURCES})
    target_include_directories(fanren_game PUBLIC src)
    target_link_libraries(fanren_game PUBLIC fanren_core fanren_engine)
    foreach(layer fanren_io fanren_script fanren_ui)
        if(TARGET ${layer})
            target_link_libraries(fanren_game PUBLIC ${layer})
        endif()
    endforeach()
    fanren_own_warnings(fanren_game)
endif()

# ---------------------------------------------------------------------------
# Executable: thin entry point.
# ---------------------------------------------------------------------------
if(TARGET fanren_game AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    add_executable(fanren WIN32 src/main.cpp)
    target_link_libraries(fanren PRIVATE fanren_game)
    fanren_own_warnings(fanren)
    if(WIN32)
        add_custom_command(TARGET fanren POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:fanren> $<TARGET_FILE_DIR:fanren>
            COMMAND_EXPAND_LISTS)
    endif()
endif()
# ---------------------------------------------------------------------------
# mksave -- 手测用的存档检查点生成器（不是游戏的一部分）。
#
# 只链 io + core，不碰 SDL：它不开窗口、不跑游戏，就是把几份 GameState 写成
# 存档文件，好让人不必把前三章重打一遍。理由与用法写在 tools/mksave/mksave.cpp 首部。
# ---------------------------------------------------------------------------
if(TARGET fanren_io AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/mksave/mksave.cpp")
    add_executable(mksave tools/mksave/mksave.cpp)
    target_link_libraries(mksave PRIVATE fanren_io fanren_core)
    fanren_own_warnings(mksave)
endif()

# ---------------------------------------------------------------------------
# Tests -- GoogleTest via FetchContent.
#
# gtest_force_shared_crt is REQUIRED on MSVC: without it gtest builds against
# the static CRT while our targets use the dynamic one, and linking fails with
# duplicate/mismatched runtime symbols.
# ---------------------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
    # GoogleTest is vendored and SHA256-locked by bootstrap.py, same as every
    # other dependency. It is deliberately NOT pulled with FetchContent+git:
    # a git clone leaves lock files behind if a build is interrupted, which
    # then fails the next configure with "ninja: failed recompaction:
    # Permission denied", and it needs network access on every clean build.
    #
    # gtest_force_shared_crt is REQUIRED on MSVC: without it gtest builds
    # against the static CRT while our targets use the dynamic one, and
    # linking fails on mismatched runtime symbols.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
    add_subdirectory("${VENDOR}/googletest-1.15.2" "${CMAKE_BINARY_DIR}/googletest-build" EXCLUDE_FROM_ALL)

    file(GLOB_RECURSE FANREN_TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")
    if(FANREN_TEST_SOURCES)
        add_executable(fanren_tests ${FANREN_TEST_SOURCES})
        target_link_libraries(fanren_tests PRIVATE fanren_core GTest::gtest_main)
        foreach(layer fanren_io fanren_script fanren_engine fanren_game)
            if(TARGET ${layer})
                target_link_libraries(fanren_tests PRIVATE ${layer})
            endif()
        endforeach()
        fanren_own_warnings(fanren_tests)

        # The engine tests exercise the real SDL3 platform layer in headless
        # mode, so fanren_tests.exe needs SDL3/SDL3_ttf/SDL3_image/SDL3_mixer
        # beside it. This must be registered BEFORE gtest_discover_tests():
        # POST_BUILD commands run in registration order and test discovery
        # launches the executable, which fails if the DLLs are not there yet.
        if(WIN32 AND TARGET fanren_engine)
            add_custom_command(TARGET fanren_tests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:fanren_tests> $<TARGET_FILE_DIR:fanren_tests>
                COMMAND_EXPAND_LISTS)
        endif()

        include(GoogleTest)
        gtest_discover_tests(fanren_tests)
    endif()
endif()
