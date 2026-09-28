#pragma once

// 测试用临时路径的唯一命名。
//
// 从前每个测试文件各写各的，而且名字多半是写死的常量（%TEMP%\fanren_trigger_root、
// fanren_slice_save.json 之类）。两个 fanren_tests.exe 同时跑——并行的 ctest、两个
// 代理各自 build——就会互相 remove_all 掉对方正在读的 data/，或是抢同一个 json 文件
// 句柄。症状是偶发红灯加一句「数据目录不存在」「复制 data/ 失败」「文件被另一进程
// 占用」，而重跑一次又是绿的，于是没人查得下去。
//
// 名字由四段拼成，前三段缺一段都不够：
//   prefix —— 给人看的，在 %TEMP% 里一眼认出是谁留下的。
//   pid    —— 分开同时在跑的进程。活着的进程之间 pid 必不相同，这是唯一一段给出
//             保证而非概率的。只靠时间戳并不够：两个被同一个脚本同时拉起的进程，
//             走到这里的时刻相差可能不到一个 QPC tick（典型 100ns），撞得上。
//   stamp  —— 分开先后两次跑。pid 会被系统回收重用，隔一会儿再跑可能撞上。
//   计数   —— 分开同一进程内的多个夹具。进程内自增，和上面两段正交。
//
// 新的测试文件需要临时目录/临时文件时 include 本文件，不要再自己拼名字。
//
// 尚未接进来的两处（2026-09-21）：ScriptApiTests.cpp 与 PanelTests.cpp 各有一份
// 自己手搓的「时间戳 + 进程内计数」，没有 pid 那一段。它们比写死常量好得多，
// 实测一百多个并发进程没撞过，但按上面的道理，缺了 pid 就只剩概率、没有保证。
// 当时这两个文件正被别的工作线改着，没去动；哪条线落地了，顺手接到
// uniqueTempPath 上来，这段注释一并删掉。

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

namespace fanren::test {

inline std::uint64_t currentProcessId() {
#if defined(_WIN32)
    return static_cast<std::uint64_t>(::_getpid());
#else
    return static_cast<std::uint64_t>(::getpid());
#endif
}

// 拼一个临时路径出来。本函数不碰磁盘，建目录还是建文件由调用方决定，也不去
// exists() 核对——唯一性由下面三段的组合给出，其中 pid 那段是确定性的。
// suffix 给需要扩展名的场合用（uniqueTempPath("fanren_slice_save", ".json")）。
//
// 这个函数必须一直是「外部链接的 inline」：下面那个 counter 之所以全进程只有
// 一份，靠的正是 inline 函数在各 TU 间被折叠成同一个实体。谁要是把它改成
// static inline、或挪进匿名命名空间，counter 就会变成每个 TU 一份，编译器不会
// 有半句提醒，而本文件要修的那种偶发撞车会原样回来。
inline std::filesystem::path uniqueTempPath(const std::string& prefix, const std::string& suffix = {}) {
    static std::atomic<std::uint64_t> counter{0};
    const auto stamp = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return std::filesystem::temp_directory_path() /
           (prefix + "_" + std::to_string(currentProcessId()) + "_" + std::to_string(stamp) + "_" +
            std::to_string(counter.fetch_add(1)) + suffix);
}

// 建好即用、析构即删的临时目录。
class TempDir {
public:
    explicit TempDir(const std::string& prefix = "fanren_test") : path_(uniqueTempPath(prefix)) {
        std::filesystem::create_directories(path_);
    }
    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }
    // 五个特殊成员一次写全（声明了析构就得表态）。移动也一并禁掉：目录的
    // 生命周期就该绑在这个局部变量上，要搬走它多半是设计走歪了；显式 delete
    // 比让编译器隐式抑制移动更好读，报错也说得清。
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    TempDir(TempDir&&) = delete;
    TempDir& operator=(TempDir&&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

}  // namespace fanren::test
