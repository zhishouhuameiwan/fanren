#include "io/GameRoot.h"

#include <system_error>
#include <utility>

namespace fanren::io {

namespace {

namespace fs = std::filesystem;

bool looksLikeGameRoot(const fs::path& dir) {
    std::error_code ec;
    return fs::is_directory(dir / "data", ec) && fs::is_directory(dir / "maps", ec);
}

}  // namespace

fs::path locateGameRoot(const std::vector<fs::path>& starts) {
    for (const fs::path& start : starts) {
        if (start.empty()) continue;
        std::error_code ec;
        fs::path dir = fs::absolute(start, ec);
        if (ec) continue;
        // 「H:\a\build\」的 parent_path 是「H:\a\build」本身，不先剥掉尾分隔符就会少走一层。
        if (!dir.has_filename()) dir = dir.parent_path();
        for (int level = 0; level <= kGameRootSearchDepth; ++level) {
            if (looksLikeGameRoot(dir)) return dir;
            fs::path parent = dir.parent_path();
            if (parent == dir) break;  // 到盘符根了
            dir = std::move(parent);
        }
    }
    return {};
}

}  // namespace fanren::io
