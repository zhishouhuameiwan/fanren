#include "game/DialogueScene.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

#include "game/Application.h"

namespace fanren::game {
namespace {

// 名签：压在框上沿、离框左边 kTagInset，高 kTagHeight，一半多一点在框外（kTagRise）。
// 露在框里的那一截（kTagHeight − kTagRise = 18）小于正文的上内边距 24，名签不会压到第一行字。
constexpr int kTagInset = 32;
constexpr int kTagHeight = 36;
constexpr int kTagRise = 18;
constexpr int kTagPadX = 20;
constexpr int kTagFontSize = 22;

// 按键提示那块小牌（框上沿右侧）。字号沿用改造前的 14。
constexpr int kHintFontSize = 14;
constexpr int kHintHeight = 22;
constexpr int kHintPadX = 12;

// 「▼」：宽 14、高 9，一秒不到起伏一次，幅度 3 像素——慢到不催人，又看得出它在等你。
constexpr float kMarkHalfWidth = 7.f;
constexpr float kMarkHeight = 9.f;
constexpr float kMarkBob = 3.f;
constexpr float kMarkHertz = 0.9f;

// 对话框贴着屏幕下方，左右各留一个安全边距。
constexpr int kBoxMargin = 48;
constexpr int kBaseBoxHeight = 200;

// 每秒显示的字数。太快读不清，太慢玩家会一直按跳过。
//
// 45 → 60 是这一批调的：一句 44 字（中位数）从一秒整变成七成秒。判据是按键
// 节奏——一个事件平均 13 句、最长 39 句，逐字动画每句多花的那点时间要乘以 13。
// 60 是「标准」档（缺省）；系统设置里另有慢 30、快 120、瞬显（docs/settings.md 第 1 节）。
constexpr double kGraphemesPerSecond = 60.0;
constexpr double kSlowGraphemesPerSecond = 30.0;
constexpr double kFastGraphemesPerSecond = 120.0;

// 长按 Skip（Ctrl）时，每个对话框停多久自动翻页。
//
// 不设成 0：一口气冲到底的话，玩家想停在某一句上就停不住，而且屏幕上什么也
// 看不清。0.10 秒的节奏是「每句都闪过一下、看得见自己冲到哪儿了」，
// 一场 39 句的戏约四秒走完。
constexpr double kFastForwardDwell = 0.10;

// 回看面板。铺满大半个屏幕：一行对白 44 字，窄面板会把每一行折成三行。
constexpr int kLogPanelMargin = 60;

}  // namespace

DialogueScene::DialogueScene(std::string speaker, std::string body)
    : speaker_(std::move(speaker)), body_(std::move(body)) {}

void DialogueScene::setChoices(std::vector<std::string> options) {
    std::vector<ui::ListItem> items;
    items.reserve(options.size());
    for (std::string& option : options) {
        ui::ListItem item;
        item.label = std::move(option);
        items.push_back(std::move(item));
    }
    hasChoices_ = !items.empty();
    // 页大小在 onEnter 里按皮肤设：这里还拿不到 Theme，而页大小必须与
    // optionBlockHeight 夹出来的那个上限是同一个数，猜不得。
    choices_.setItems(std::move(items));
}

void DialogueScene::onEnter(Application& app) {
    // 页大小取「选项区最多给到几行」而不是选项数：装得下时两者相等，玩家看到
    // 的仍是完整一页；装不下时才有差别，而那时页大小小于条目数正是翻页键还能
    // 动的前提（ListView 认为 count() <= pageSize 就是整表一页，翻页毫无动静）。
    choices_.setPageSize(maxOptionRows(app.theme()));

    graphemes_ = engine::splitGraphemes(body_);
    revealed_ = 0;
    revealAccumulator_ = 0.0;
    done_ = false;
    time_ = 0.0;
    // 无头模式没人看动画，直接全显，免得 bot 要空跑几十帧等文字。
    // 文字速度选了瞬显的，第一帧就是整句（不等第一次 update）。
    if (app.headless() || revealAtOnce_ || !revealRate(app)) revealAll();
}

std::optional<double> DialogueScene::graphemesPerSecond(io::TextSpeed speed) {
    switch (speed) {
        case io::TextSpeed::Slow: return kSlowGraphemesPerSecond;
        case io::TextSpeed::Normal: return kGraphemesPerSecond;
        case io::TextSpeed::Fast: return kFastGraphemesPerSecond;
        case io::TextSpeed::Instant: return std::nullopt;
    }
    return kGraphemesPerSecond;
}

std::optional<double> DialogueScene::revealRate(const Application& app) {
    return graphemesPerSecond(app.settings().textSpeed);
}

engine::Rect DialogueScene::nameTagArea(const engine::Rect& box, int textWidth) {
    return engine::Rect{box.x + kTagInset, box.y - kTagRise, std::max(0, textWidth) + kTagPadX * 2,
                        kTagHeight};
}

void DialogueScene::revealAll() {
    revealed_ = graphemes_.size();
}

bool DialogueScene::fullyRevealed() const {
    return revealed_ >= graphemes_.size();
}

std::string DialogueScene::revealedText() const {
    std::string visible;
    visible.reserve(body_.size());
    for (std::size_t i = 0; i < revealed_ && i < graphemes_.size(); ++i) {
        visible += graphemes_[i];
    }
    return visible;
}

void DialogueScene::finish(Application& app, int choiceIndex) {
    if (done_) return;
    done_ = true;
    script::CommandResult result;
    result.ok = true;
    result.choiceIndex = choiceIndex;
    app.completeCommand(result);
}

int DialogueScene::maxOptionRows(const ui::Theme& theme) {
    // 框底钉在 kLogicalHeight - kBoxMargin，框顶不许越过上边的同一条安全边距，
    // 正文那块 kBaseBoxHeight 又是雷打不动的，剩下的才归选项。
    const int budget = engine::kLogicalHeight - kBoxMargin * 2 - kBaseBoxHeight;
    // 至少留一行：内边距大得离谱的皮肤会把 budget 压到 0，那时返回 0 的后果是
    // 一份「有选项却一个也画不出来」的对话——玩家会卡死在这里，比框顶出屏严重。
    return std::max(1, ui::listRowsThatFit(budget, theme));
}

int DialogueScene::optionBlockHeight(int rows, const ui::Theme& theme) {
    // 走 ui::listAreaHeight 而不是 rows * 行高：ListView 画之前还要让出上下各
    // 一个 padding，按裸行高摆位的话差出来的正好是一到两行——三个选项只画得出
    // 一个，而玩家看到的是一份「只有一个选项」的对话，右下角那个 1/3 他多半
    // 当成装饰。对话框整体因此高 48px，正文可用区一点没变（它减的就是这个数）。
    //
    // 行数在这里夹一次：超过上限的选项不再把框撑高，改为在列表里滚动。
    // 夹的是「框有多高」而不是「有几个选项」——选项一个都没少，只是要滚。
    return ui::listAreaHeight(std::min(rows, maxOptionRows(theme)), theme);
}

engine::Rect DialogueScene::boxAreaFor(int rows, const ui::Theme& theme) {
    const int height = kBaseBoxHeight + optionBlockHeight(rows, theme);
    return engine::Rect{kBoxMargin, engine::kLogicalHeight - height - kBoxMargin,
                        engine::kLogicalWidth - kBoxMargin * 2, height};
}

engine::Rect DialogueScene::boxArea(const ui::Theme& theme) const {
    return boxAreaFor(hasChoices_ ? choices_.count() : 0, theme);
}

engine::Rect DialogueScene::logPanelArea() {
    return engine::Rect{kLogPanelMargin, kLogPanelMargin,
                        engine::kLogicalWidth - kLogPanelMargin * 2,
                        engine::kLogicalHeight - kLogPanelMargin * 2};
}

int DialogueScene::logPageRows(const ui::Theme& theme) {
    const int contentH = logPanelArea().h - ui::panelTitleBandHeight(/*hasTitle=*/true, theme);
    return std::max(1, ui::listRowsThatFit(contentH, theme));
}

void DialogueScene::openLog(Application& app) {
    std::vector<ui::ListItem> items;
    const std::vector<Application::SpokenLine>& log = app.dialogueLog();
    // 倒序：刚说过的那一句在最上面。翻记录的人找的永远是最近那几句。
    for (auto it = log.rbegin(); it != log.rend(); ++it) {
        ui::ListItem row;
        row.label = it->speaker.empty() ? it->body : (it->speaker + "：" + it->body);
        items.push_back(std::move(row));
    }
    logView_.reset();
    logView_.setEmptyHint("（还没有说过话）");
    logView_.setPageSize(logPageRows(app.theme()));
    logView_.setItems(std::move(items));
    log_ = true;
}

bool DialogueScene::update(Application& app, double deltaSeconds) {
    if (done_) return false;
    engine::Engine& eng = app.engine();
    time_ += deltaSeconds;

    // 回看面板开着时，这一层把所有输入都吃掉：翻记录的时候不该把没读的
    // 那几句按掉。菜单键与取消键都关掉它。
    if (log_) {
        if (eng.keyPressed(engine::Engine::Key::Menu)) {
            log_ = false;
            return true;
        }
        // 列表的确认键在这里没有「选中」的语义，按下即关；取消同理。
        if (logView_.update(eng) || logView_.cancelled()) log_ = false;
        return true;
    }
    if (eng.keyPressed(engine::Engine::Key::Menu)) {
        openLog(app);
        return true;
    }

    // 长按 Skip（Ctrl）一路快进：先把这一句全显出来。
    // 按住期间每个框停 kFastForwardDwell 就自动翻过去，**但有选项时绝不自动选**
    // ——替玩家做选择是这套快进唯一不可接受的后果。
    const bool fastForward = eng.keyDown(engine::Engine::Key::Skip);
    if (fastForward) revealAll();

    // 文字还在滚时，确认键的含义是「立刻显示完」而不是「翻页」，否则手快的
    // 玩家会一路跳过没读到的句子。
    //
    // 这一支必须排在选项处理之前：同一次按键若既全显又确认，玩家等于在没看见
    // 选项的情况下替自己选了第一项。
    if (!fullyRevealed()) {
        // 速率每帧问一次设置（docs/settings.md 第 1 节 #7）；瞬显没有速率，当场全显。
        if (const std::optional<double> rate = revealRate(app)) {
            revealAccumulator_ += deltaSeconds * *rate;
            const auto step = static_cast<std::size_t>(revealAccumulator_);
            if (step > 0) {
                revealed_ = std::min(graphemes_.size(), revealed_ + step);
                revealAccumulator_ -= static_cast<double>(step);
            }
        } else {
            revealAll();
        }
        if (eng.keyPressed(engine::Engine::Key::Confirm)) revealAll();
        return true;
    }

    if (!hasChoices_) {
        if (eng.keyPressed(engine::Engine::Key::Confirm)) {
            finish(app, -1);
            return false;
        }
        if (fastForward) {
            holdSeconds_ += deltaSeconds;
            if (holdSeconds_ >= kFastForwardDwell) {
                finish(app, -1);
                return false;
            }
        }
        return true;
    }

    if (choices_.update(eng)) {
        finish(app, choices_.selection());
        return false;
    }
    // 取消这条路 P1 时是死的：脚本侧约定 choice 取消返回 nil、choiceIndex 为 -1，
    // 但对话框根本没接 Cancel 键。接上控件顺带把它接通。
    if (choices_.cancelled()) {
        finish(app, -1);
        return false;
    }
    return true;
}

void DialogueScene::renderLog(Application& app) const {
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();
    const engine::Rect area = logPanelArea();
    const std::string title = "回看";
    ui::drawPanel(eng, area, title, theme);
    logView_.render(eng, ui::panelContentArea(area, title, theme), theme);
}

void DialogueScene::render(Application& app) {
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();

    if (log_) {
        renderLog(app);
        return;
    }

    // 镜头贴着地图下沿时，贴底的框会盖住说话的两个人（终审 LOW-6）：那时整个框上移让开，名签、键位牌跟着框走。
    const engine::Rect box = keepHeroInSight(boxArea(theme), app.heroScreenRect(), kBoxMargin);
    // 说话人不再当面板标题，改成框上沿左侧一块独立的名签（旁白不画）：
    // 框里整块都留给正文，名字也不必与正文挤在同一个平面上。
    ui::drawPanel(eng, box, std::string{}, theme);
    renderNameTag(app, box);
    renderKeyHint(app, box);

    const engine::Rect content = ui::panelContentArea(box, std::string{}, theme);
    const int optionRows = hasChoices_ ? choices_.count() : 0;
    const int optionHeight = optionBlockHeight(optionRows, theme);

    // 正文只排已显示的部分：逐字显示时断行要跟着增长的文本走，
    // 否则最后一行会提前占位、看起来像文字在跳。
    const engine::Rect textArea{content.x, content.y, content.w,
                                std::max(0, content.h - optionHeight)};
    ui::drawTextBlock(eng, revealedText(), textArea, theme.bodyFontSize, theme);

    if (!fullyRevealed()) return;
    if (!hasChoices_) {
        renderContinueMark(app, box);
        return;
    }

    const engine::Rect optionArea{content.x, content.y + content.h - optionHeight, content.w,
                                  optionHeight};
    // 正文与选项之间一道两头渐隐的暗金线：选项是另一件事，不是正文的最后几行。
    ui::drawRuleH(eng, static_cast<float>(optionArea.x + theme.padding),
                  static_cast<float>(optionArea.y),
                  static_cast<float>(optionArea.w - theme.padding * 2), theme);
    choices_.render(eng, optionArea, theme);
}

void DialogueScene::renderNameTag(Application& app, const engine::Rect& box) const {
    if (speaker_.empty()) return;   // 旁白没有名签
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();
    const engine::Point size = eng.measureText(speaker_, kTagFontSize);
    const engine::Rect tag = nameTagArea(box, size.x);
    ui::drawPlate(eng, tag, theme);
    eng.drawText(speaker_, tag.x + kTagPadX, tag.y + (tag.h - kTagFontSize) / 2 - 1, kTagFontSize,
                 theme.goldBright, theme.bodyStyle);
}

void DialogueScene::renderKeyHint(Application& app, const engine::Rect& box) const {
    // 两个键的提示钉在框的右上角。**没有这一行，这两个键等于不存在**——
    // 玩家不会去猜一个没写在任何地方的快捷键，而「对白太多」这件事一半是
    // 按键太多，长按快进正是那一半的解。
    // 改造后它骑在框的上沿（与左边的名签对称），不再占正文第一行的右半截。
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();
    // 键名跟着键位走（docs/settings.md 第 6 节）：文案里是 {key.menu} / {key.skip}，Application::text 展开；
    // 默认键位下展开出来就是改造前写死的那句「Tab 回看　长按 Ctrl 快进」。
    const std::string hint = app.text("ui.dialogue.keys");
    const engine::Point size = eng.measureText(hint, kHintFontSize);
    if (size.x <= 0) return;
    const int w = size.x + kHintPadX * 2;
    const engine::Rect plate{box.x + box.w - kTagInset - w, box.y - kHintHeight / 2, w,
                             kHintHeight};
    const engine::RectF plateF{static_cast<float>(plate.x), static_cast<float>(plate.y),
                               static_cast<float>(plate.w), static_cast<float>(plate.h)};
    eng.fillRect(plateF, theme.inkDeep, engine::BlendMode::Alpha);
    ui::drawFrame(eng, plateF, theme.goldDim);
    eng.drawText(hint, plate.x + kHintPadX, plate.y + (kHintHeight - kHintFontSize) / 2 - 1,
                 kHintFontSize, theme.paperDim);
}

void DialogueScene::renderContinueMark(Application& app, const engine::Rect& box) const {
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();
    const float phase = static_cast<float>(time_) * kMarkHertz * 2.f * std::numbers::pi_v<float>;
    const float bob = kMarkBob * std::sin(phase);
    const float cx = static_cast<float>(box.x + box.w - theme.padding) - kMarkHalfWidth;
    const float top = static_cast<float>(box.y + box.h - theme.padding) - kMarkHeight - 4.f + bob;
    const auto triangle = [&](float dx, float dy, const engine::Color& color) {
        const std::vector<engine::Vertex> vertices{
            {cx - kMarkHalfWidth + dx, top + dy, color, 0.f, 0.f},
            {cx + kMarkHalfWidth + dx, top + dy, color, 0.f, 0.f},
            {cx + dx, top + kMarkHeight + dy, color, 0.f, 0.f},
        };
        eng.drawGeometry(engine::kInvalidTexture, vertices, {}, engine::BlendMode::Alpha);
    };
    triangle(1.f, 2.f, theme.inkDeep);   // 投影：压在浅色的地图上也认得出
    triangle(0.f, 0.f, theme.goldBright);
}

}  // namespace fanren::game
