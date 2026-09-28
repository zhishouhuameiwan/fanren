#include "script/ScriptHost.h"

// sol2 只出现在这里。ScriptHost.h 是 pimpl，外界永远看不到 sol 类型。
#include <sol/sol.hpp>

#include <cstdio>
#include <exception>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <utility>

#include "core/rules/Field.h"
#include "core/rules/Realm.h"

namespace fanren::script {
namespace {

// Lua 命令表的 kind 字段 → C++ 枚举。
// 用字符串而不是整数：脚本里写 kind = "talk" 一眼能读懂，而且 C++ 这边增删
// 枚举项时不会让脚本里的魔数静默错位成另一条命令。
const std::unordered_map<std::string, CommandKind>& kindTable() {
    static const std::unordered_map<std::string, CommandKind> table = {
        {"talk", CommandKind::Talk},
        {"choice", CommandKind::Choice},
        {"battle", CommandKind::Battle},
        {"teleport", CommandKind::Teleport},
        {"fade_out", CommandKind::FadeOut},
        {"fade_in", CommandKind::FadeIn},
        {"give_item", CommandKind::GiveItem},
        {"take_item", CommandKind::TakeItem},
        {"set_flag", CommandKind::SetFlag},
        {"shop", CommandKind::Shop},
        {"wait", CommandKind::Wait},
        {"play_sfx", CommandKind::PlaySfx},
        {"game_over", CommandKind::GameOver},
        {"ending", CommandKind::Ending},
        {"advance_days", CommandKind::AdvanceDays},
        {"bottle_grant", CommandKind::BottleGrant},
        {"bottle_unlock_mature", CommandKind::BottleUnlockMature},
        {"field_unlock", CommandKind::FieldUnlock},
        {"bottle_spend", CommandKind::BottleSpend},
        {"bottle_mature", CommandKind::BottleMature},
        {"party_add", CommandKind::PartyAdd},
        {"party_remove", CommandKind::PartyRemove},
        {"magic_learn", CommandKind::MagicLearn},
        {"magic_forget", CommandKind::MagicForget},
        {"realm_advance", CommandKind::RealmAdvance},
        {"realm_cap", CommandKind::RealmCap},
        {"play_bgm", CommandKind::PlayBgm},
    };
    return table;
}

// 统计一块（或全部）灵田的槽位。fieldId 为空表示合计所有田：第 2 章只有一块，
// 第 7 章的百药园会有多块，口径先按多块定死，免得到时改签名。
// 查不到的田返回 0 而不是报错 —— 剧情闸门问「种了几株」，答案在田还没开的时候
// 本来就该是零，让脚本为此多写一次 if 只会让闸门条件更容易写错。
int countSlots(const core::GameState* state, const std::string& fieldId,
               bool (*accept)(const fanren::rules::FieldSlot&)) {
    if (state == nullptr) {
        return 0;
    }
    int total = 0;
    for (const fanren::rules::SpiritField& field : state->fields) {
        if (!fieldId.empty() && field.id != fieldId) {
            continue;
        }
        for (const fanren::rules::FieldSlot& slot : field.slots) {
            if (accept(slot)) {
                ++total;
            }
        }
    }
    return total;
}

// scriptPath 迟早不会只是硬编码常量：地图触发器、存档里的事件 id 都会喂进来。
// joinPath 是拼串，拦不住 ".."，所以在拼之前就把能逃出 scriptRoot 的形状否掉。
bool isSafeRelativePath(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    if (path.front() == '/' || path.front() == '\\') {
        return false;
    }
    if (path.size() >= 2 && path[1] == ':') {   // 盘符
        return false;
    }
    std::size_t begin = 0;
    while (true) {
        const std::size_t end = path.find_first_of("/\\", begin);
        const std::string segment =
            path.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
        if (segment.empty() || segment == "..") {
            return false;
        }
        if (end == std::string::npos) {
            return true;
        }
        begin = end + 1;
    }
}

std::string joinPath(const std::string& root, const std::string& relative) {
    if (root.empty()) {
        return relative;
    }
    const char tail = root.back();
    if (tail == '/' || tail == '\\') {
        return root + relative;
    }
    return root + "/" + relative;
}

// 自己读文件而不是用 lua.load_file：一来「文件不存在」能给出人话错误，二来
// chunkname 可以用相对路径，Lua 的报错前缀就成了 "ch01/sanshu.lua:12:"，
// 而不是一长串绝对路径。
bool readTextFile(const std::string& path, std::string& out, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "脚本文件不存在或无法读取: " + path;
        return false;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    if (in.bad()) {
        error = "读取脚本失败: " + path;
        return false;
    }
    out = buffer.str();
    // UTF-8 BOM 会被 Lua 当成语法错误，静默吃掉比让人对着 "unexpected symbol"
    // 发呆强。项目自身的脚本一律无 BOM，这里只是防外部工具存坏。
    if (out.size() >= 3 && static_cast<unsigned char>(out[0]) == 0xEF &&
        static_cast<unsigned char>(out[1]) == 0xBB && static_cast<unsigned char>(out[2]) == 0xBF) {
        out.erase(0, 3);
    }
    return true;
}

bool toCommand(const sol::object& value, Command& out, std::string& error) {
    if (value.get_type() != sol::type::table) {
        error = "yield 出来的不是命令表；事件必须通过 scripts/common/api.lua 的 API 挂起";
        return false;
    }
    const sol::table table = value.as<sol::table>();
    const sol::optional<std::string> kind = table["kind"];
    if (!kind) {
        error = "命令表缺少 kind 字段";
        return false;
    }
    const auto it = kindTable().find(*kind);
    if (it == kindTable().end()) {
        error = "未知的命令 kind = " + *kind;
        return false;
    }

    out = Command{};
    out.kind = it->second;
    out.a = table.get_or("a", std::string{});
    out.b = table.get_or("b", std::string{});
    out.x = table.get_or("x", 0);
    out.y = table.get_or("y", 0);
    const sol::optional<sol::table> options = table["options"];
    if (options) {
        const std::size_t count = options->size();
        out.options.reserve(count);
        for (std::size_t i = 1; i <= count; ++i) {
            out.options.push_back(options->get_or(i, std::string{}));
        }
    }
    return true;
}

}  // namespace

// ---------------------------------------------------------------------------

struct ScriptHost::Impl {
    // lua 必须最先声明：thread 与 co 都是它的引用，成员按声明逆序析构，
    // 这样 lua_State 才会最后一个走。
    sol::state lua;

    std::string scriptRoot;
    core::GameState* state = nullptr;
    bool initialised = false;

    sol::thread thread;
    sol::coroutine co;
    bool running = false;
    bool hasCommand = false;
    Command pending;

    std::string currentScript;
    std::string lastError;
    ScriptHost::LogSink log;

    void write(const std::string& message) const {
        if (log) {
            log(message);
            return;
        }
        std::fputs(message.c_str(), stderr);
        std::fputc('\n', stderr);
    }

    // 标志位层面的「协程结束」。不碰 co / thread。
    void endCoroutine() {
        running = false;
        hasCommand = false;
        pending = Command{};
    }

    // 放掉协程引用。调用点必须在 protected_function_result 析构之后：那个结果
    // 对象析构时还要弹协程的栈，若此刻 thread 已经没人引用、又恰好撞上一次 GC，
    // 弹的就是一块已回收的 lua_State。
    void releaseCoroutine() {
        co = sol::coroutine{};
        thread = sol::thread{};
    }

    // 二者合一。只在手上没有活着的结果对象时用。
    void clearCoroutine() {
        endCoroutine();
        releaseCoroutine();
    }

    void bindQueries() {
        // 只读查询。它们立即返回，绝不挂起 —— 这正是「yield 不跨 C 边界」得以
        // 成立的前提：C++ 注册的函数一个都不 yield。写操作一律走命令队列。
        sol::table host = lua.create_named_table("__host");
        Impl* self = this;
        host.set_function("flag_get", [self](const std::string& name) -> int {
            return self->state != nullptr ? self->state->flag(name) : 0;
        });
        host.set_function("item_count", [self](const std::string& id) -> int {
            return self->state != nullptr ? self->state->itemCount(id) : 0;
        });
        host.set_function("realm_at_least", [self](int value) -> bool {
            if (self->state == nullptr) {
                return false;
            }
            // 非法编号一律判 false：脚本传错数字不该意外解锁高境界内容。
            if (!fanren::rules::isValid(fanren::rules::fromValue(value))) {
                return false;
            }
            return fanren::rules::toValue(self->state->realm) >= value;
        });
        // 当前境界的编号。`realm_at_least` 只答是非，而剧情要写「他现在是第几层」
        // 就得有个读数——第 4 章那句「他数了数自己剩的……」正是这一类。
        //
        // 返回的是**境界编号**，与 api.lua 里 `realm.QI_REFINING_7 = 7` 那组常量
        // 同一套坐标，也就是 `realm_at_least` 收的那个数。炼气期的编号恰好就是
        // 层数（1-13），这正是 enum 当初这样编号的原因；筑基以上是 21/22/23、
        // 31/32/33，脚本要分档请配 `realm_at_least`，不要拿它当「第几层」去算。
        //
        // 刻意**不另开一个只数层数的查询**：那个数在炼气期是 1-13、在筑基期是
        // 1-3，同一个变量在不同大境界里意思不同，写闸门时几乎必然出错。
        host.set_function("realm_value", [self]() -> int {
            return self->state != nullptr ? fanren::rules::toValue(self->state->realm) : 0;
        });

        // ---- P3 第 2 章增补：时间、掌天瓶、灵田的只读查询 ----
        // 全是纯读。剧情闸门（「攒够一滴绿液没有」「种下过至少一株没有」）要在
        // 对话中途求值，带副作用的查询会让同一句台词说两遍就改一次存档。
        host.set_function("day", [self]() -> int {
            return self->state != nullptr ? self->state->day : 0;
        });
        host.set_function("bottle_owned", [self]() -> bool {
            return self->state != nullptr && self->state->bottle.owned;
        });
        host.set_function("bottle_mature_known", [self]() -> bool {
            return self->state != nullptr && self->state->bottle.matureKnown;
        });
        host.set_function("bottle_drops", [self]() -> int {
            return self->state != nullptr ? self->state->bottle.drops : 0;
        });
        host.set_function("field_planted", [self](const std::string& fieldId) -> int {
            return countSlots(self->state, fieldId,
                              [](const fanren::rules::FieldSlot& slot) {
                                  return !slot.seedId.empty();
                              });
        });
        host.set_function("field_ripe", [self](const std::string& fieldId) -> int {
            return countSlots(self->state, fieldId,
                              [](const fanren::rules::FieldSlot& slot) { return slot.ripe; });
        });

        // ---- P3 第 3 章增补：队伍的只读查询（契约第 1.3 节）----
        // 写走命令队列（PartyAdd / PartyRemove），读走这里，与既有分界一致。
        host.set_function("party_has", [self](const std::string& roleId) -> bool {
            return self->state != nullptr && self->state->partyHas(roleId);
        });
        host.set_function("party_size", [self]() -> int {
            return self->state != nullptr ? static_cast<int>(self->state->party.size()) : 0;
        });

        // ---- P3 第 4 章增补：已习得法术的只读查询（契约第 1.3 节）----
        // 写走命令队列（MagicLearn / MagicForget），读走这里，与既有分界一致。
        //
        // **刻意不在这里判「data 里有没有这门法术」**：__host 拿不到 GameData，
        // 而且 knows() 问的是「他会不会」，不是「这个 id 合不合法」。
        // 后者由 MagicLearn 在 game 层回绝（code = "no_magic"）。
        host.set_function("magic_knows", [self](const std::string& magicId) -> bool {
            return self->state != nullptr && self->state->knowsMagic(magicId);
        });
        host.set_function("magic_count", [self]() -> int {
            return self->state != nullptr ? static_cast<int>(self->state->learnedMagics.size()) : 0;
        });
    }

    // 处理一次 resume 的结果：出错 / 正常结束 / 挂起并带回一条命令。
    // 只动标志位，不碰 co / thread —— 见 releaseCoroutine 的注释。调用方在
    // result 析构之后按 running 决定要不要 releaseCoroutine。
    core::Result<bool> pump(sol::protected_function_result& result) {
        hasCommand = false;

        if (!result.valid()) {
            const sol::error err = result;
            // Lua 的错误串已经是 "<chunkname>:<line>: <message>"，直接用。
            lastError = err.what();
            write("[script] " + currentScript + " 运行出错: " + lastError);
            endCoroutine();
            return core::Result<bool>::failure(lastError);
        }

        if (result.status() != sol::call_status::yielded) {
            endCoroutine();
            return core::Result<bool>::success(true);
        }

        Command command;
        std::string why;
        if (result.return_count() < 1) {
            // 光秃秃的 coroutine.yield()。不先挡一道的话 get<sol::object>(0) 会去
            // 摸一个根本不存在的栈位。
            lastError = currentScript + ": coroutine.yield() 没有带命令表";
            write("[script] " + lastError);
            endCoroutine();
            return core::Result<bool>::failure(lastError);
        }
        if (!toCommand(result.get<sol::object>(0), command, why)) {
            lastError = currentScript + ": " + why;
            write("[script] " + lastError);
            endCoroutine();
            return core::Result<bool>::failure(lastError);
        }

        pending = std::move(command);
        hasCommand = true;
        running = true;
        return core::Result<bool>::success(true);
    }
};

// ---------------------------------------------------------------------------

ScriptHost::ScriptHost() : impl_(std::make_unique<Impl>()) {}

ScriptHost::~ScriptHost() {
    // 方案 3.5 规则 6：协程未正常结束就被丢弃，必须留下痕迹。
    if (impl_->running) {
        impl_->write("[script] ScriptHost 析构时协程 " + impl_->currentScript + " 仍未结束，已丢弃");
    }
}

core::Result<bool> ScriptHost::init(const std::string& scriptRoot, core::GameState* state) {
    using R = core::Result<bool>;
    if (state == nullptr) {
        return R::failure("ScriptHost::init: GameState 指针不能为空");
    }

    try {
        // 方案 3.5 规则 6：重新 init 会连同 lua_State 一起换掉，在跑的协程就没了。
        // 和 startEvent / abortEvent 一样，丢弃前必须留痕。
        if (impl_->running) {
            impl_->write("[script] 协程 " + impl_->currentScript + " 未正常结束就被 init 丢弃");
        }
        impl_->clearCoroutine();
        impl_->initialised = false;
        impl_->scriptRoot = scriptRoot;
        impl_->state = state;
        impl_->lastError.clear();
        impl_->currentScript.clear();

        // 只开脚本真正用得上的库。io / os / package 一概不开：事件脚本没有读写
        // 文件或调用进程环境的正当理由，少开一个库就少一处可被脚本误伤的面。
        impl_->lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::table,
                                  sol::lib::math, sol::lib::coroutine);
        // base 库里还夹着两个能读盘的函数，一并摘掉。
        impl_->lua["dofile"] = sol::lua_nil;
        impl_->lua["loadfile"] = sol::lua_nil;

        impl_->bindQueries();

        const std::string apiPath = joinPath(scriptRoot, "common/api.lua");
        std::string source;
        std::string ioError;
        if (!readTextFile(apiPath, source, ioError)) {
            impl_->lastError = ioError;
            return R::failure(ioError);
        }

        sol::load_result chunk = impl_->lua.load(source, "@common/api.lua", sol::load_mode::text);
        if (!chunk.valid()) {
            const sol::error err = chunk;
            impl_->lastError = err.what();
            return R::failure("common/api.lua 语法错误: " + impl_->lastError);
        }
        sol::protected_function api = chunk;
        sol::protected_function_result ran = api();
        if (!ran.valid()) {
            const sol::error err = ran;
            impl_->lastError = err.what();
            return R::failure("common/api.lua 执行失败: " + impl_->lastError);
        }

        impl_->initialised = true;
        return R::success(true);
    } catch (const std::exception& e) {
        // 不跨模块边界抛异常：sol2 内部的异常（含 panic）在这里就地收口。
        impl_->lastError = e.what();
        return R::failure(std::string("ScriptHost::init 异常: ") + e.what());
    } catch (...) {
        impl_->lastError = "未知异常";
        return R::failure("ScriptHost::init 未知异常");
    }
}

core::Result<bool> ScriptHost::startEvent(const std::string& scriptPath) {
    using R = core::Result<bool>;
    if (!impl_->initialised) {
        return R::failure("ScriptHost 尚未 init");
    }
    // 先验参再动状态：路径不合法不该连累正在跑的协程。
    if (!isSafeRelativePath(scriptPath)) {
        impl_->lastError = "非法的脚本路径（必须是 scriptRoot 下的相对路径）: " + scriptPath;
        impl_->write("[script] " + impl_->lastError);
        return R::failure(impl_->lastError);
    }
    if (impl_->running) {
        // 方案 3.5 规则 6：上一个协程没跑完就被顶掉，属于 game 层的场景管理错误。
        // 这里只记日志不断言 —— 断言会让 Release 与 Debug 行为分叉，日志才是
        // 两边都看得见的证据。
        impl_->write("[script] 协程 " + impl_->currentScript + " 未正常结束就被新事件顶掉: " +
                     scriptPath);
        impl_->clearCoroutine();
    }

    impl_->lastError.clear();
    impl_->currentScript = scriptPath;

    std::string source;
    std::string ioError;
    if (!readTextFile(joinPath(impl_->scriptRoot, scriptPath), source, ioError)) {
        impl_->lastError = ioError;
        impl_->write("[script] " + ioError);
        return R::failure(ioError);
    }

    try {
        // load_mode::text：只收源码，不收预编译字节码。畸形字节码能直接打穿
        // Lua 虚拟机，而我们从没有加载字节码的需求。
        sol::load_result chunk = impl_->lua.load(source, "@" + scriptPath, sol::load_mode::text);
        if (!chunk.valid()) {
            const sol::error err = chunk;
            impl_->lastError = err.what();
            impl_->write("[script] " + scriptPath + " 语法错误: " + impl_->lastError);
            impl_->clearCoroutine();
            return R::failure(impl_->lastError);
        }

        // 协程要跑在自己的 lua_State 上，而 sol::coroutine 必须从那个 state 的
        // 视角取到入口函数，故先把 chunk 挂到全局过一道手，取完立刻摘掉。
        sol::protected_function entry = chunk;
        impl_->lua["__fanren_event_entry"] = entry;
        impl_->thread = sol::thread::create(impl_->lua.lua_state());
        sol::state_view threadView = impl_->thread.state();
        sol::coroutine created = threadView["__fanren_event_entry"];
        impl_->co = created;
        impl_->lua["__fanren_event_entry"] = sol::lua_nil;

        impl_->running = true;
        core::Result<bool> outcome;
        {
            sol::protected_function_result first = impl_->co();
            outcome = impl_->pump(first);
        }  // first 先析构，之后才轮得到协程引用
        if (!impl_->running) {
            impl_->releaseCoroutine();
        }
        return outcome;
    } catch (const std::exception& e) {
        impl_->lastError = e.what();
        impl_->write("[script] " + scriptPath + " 异常: " + impl_->lastError);
        impl_->clearCoroutine();
        return R::failure(impl_->lastError);
    } catch (...) {
        impl_->lastError = "未知异常";
        impl_->clearCoroutine();
        return R::failure("ScriptHost::startEvent 未知异常: " + scriptPath);
    }
}

bool ScriptHost::isRunning() const {
    return impl_->running;
}

bool ScriptHost::pollCommand(Command& out) {
    if (!impl_->running || !impl_->hasCommand) {
        return false;
    }
    out = impl_->pending;
    return true;
}

void ScriptHost::resumeWith(const CommandResult& result) {
    if (!impl_->running || !impl_->hasCommand) {
        impl_->write("[script] resumeWith 在没有待执行命令时被调用，已忽略");
        return;
    }

    try {
        // 结果表建在协程自己的 state 上，省掉一次跨 state 搬运。
        sol::state_view threadView = impl_->thread.state();
        sol::table payload = threadView.create_table();
        // ok 现在有人读了：take() 报「东西不够」、bottle.spend() 报「绿液不够」、
        // bottle.mature() 报四种催熟失败，全靠这一位。
        payload["ok"] = result.ok;
        // 数值与失败原因码。恒定回填（而不是只在失败时塞进去）：缺字段与
        // 「字段是空的」在 Lua 里都是 nil，脚本分不出「这条命令没这个回填」
        // 和「它回填了个空」，那种分不出来的差别迟早被当成偶发 bug。
        payload["value"] = result.value;
        payload["code"] = result.code;

        // index 保持 0 起算、取消为 -1；转成 Lua 习惯的 1 起算由 api.lua 负责，
        // C++ 这边不掺和序号语义。但越界要在这里挡：game 层报了个不存在的选项，
        // 脚本会悄悄落进 else 分支，把 UI 的 bug 伪装成玩家的选择。
        int index = result.choiceIndex;
        if (impl_->pending.kind == CommandKind::Choice &&
            index >= static_cast<int>(impl_->pending.options.size())) {
            impl_->write("[script] " + impl_->currentScript + " 收到越界的选项序号 " +
                         std::to_string(index) + "，按取消处理");
            index = -1;
        }
        payload["index"] = index;
        payload["won"] = result.battleWon;

        {
            sol::protected_function_result next = impl_->co(payload);
            impl_->pump(next);
        }  // next 先析构，之后才轮得到协程引用
        if (!impl_->running) {
            impl_->releaseCoroutine();
        }
    } catch (const std::exception& e) {
        impl_->lastError = e.what();
        impl_->write("[script] " + impl_->currentScript + " resume 异常: " + impl_->lastError);
        impl_->clearCoroutine();
    } catch (...) {
        impl_->lastError = "未知异常";
        impl_->write("[script] " + impl_->currentScript + " resume 未知异常");
        impl_->clearCoroutine();
    }
}

void ScriptHost::abortEvent(const std::string& reason) {
    if (!impl_->running) {
        return;
    }
    impl_->write("[script] 协程 " + impl_->currentScript + " 未正常结束就被丢弃: " + reason);
    impl_->clearCoroutine();
}

const std::string& ScriptHost::lastError() const {
    return impl_->lastError;
}

void ScriptHost::setLogSink(LogSink sink) {
    impl_->log = std::move(sink);
}

}  // namespace fanren::script
