#include "game/DialogueScene.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <utility>

#include "game/Application.h"
#include "game/Busts.h"
#include "game/DialogueExtras.h"
#include "game/Portraits.h"

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

// 头像外那圈细框离头像几像素（框画在头像外面，不压头像本身的像素）。
constexpr float kPortraitFrameOut = 3.f;

}  // namespace

DialogueScene::DialogueScene(std::string speaker, std::string body)
    : speaker_(std::move(speaker)), body_(std::move(body)) {
    static std::uint64_t next = 0;
    serial_ = ++next;
}

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
    autoSeconds_ = 0.0;
    // 无头模式没人看动画，直接全显，免得 bot 要空跑几十帧等文字。
    // 文字速度选了瞬显的，第一帧就是整句（不等第一次 update）。
    if (app.headless() || revealAtOnce_ || !revealRate(app)) revealAll();
}

void DialogueScene::enterBust(Application& app) {
    // 立绘（3.1）：这一句说话人的那一格交给跨句的 bustStage——同一个人接着说不重新淡入，换人淡出。
    // 无头模式 frameFor 不读表、不警告，一律没有。
    std::optional<BustShot> bust;
    // 同道录 2a·路 U：事件图铺着时不交立绘（图上已经画着她）；名牌、对话框、小头像照常。
    if (bustAllowed_ && !speakerRole_.empty() && app.currentCg().empty()) {
        const ChapterDone chapterDone = [&app](int n) { return app.chapterTable().chapterDone(app.state(), n); };
        if (const std::optional<PortraitFrame> frame =
                app.busts().frameFor(app.engine(), app.headless(), speakerRole_, face_, chapterDone)) {
            bust = BustShot{speakerRole_, frame->texture, frame->src};
        }
    }
    hasBust_ = bust.has_value();
    app.bustStage().speak(bust);
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

    // ---- 地基包路 D：整段跳过的确认框（docs/ch07-foundation.md 4.3）----
    // 开着时吃掉别的输入：确认「跳过」就记一笔、把这一句收掉，后面的 talk 由 Application 直接记进回看；
    // 「接着看」或取消键关掉确认框，这一句照旧。
    if (askSkip_) {
        askSkipSeconds_ += deltaSeconds;
        // 防误触：弹出后 askSkipGuard_ 秒内不收确认键（上下移、取消照收）。
        if (askSkipSeconds_ < askSkipGuard_) {
            if (eng.keyPressed(engine::Engine::Key::Up)) skipView_.moveUp();
            if (eng.keyPressed(engine::Engine::Key::Down)) skipView_.moveDown();
            if (eng.keyPressed(engine::Engine::Key::Cancel)) askSkip_ = false;
            return true;
        }
        if (skipView_.update(eng)) {
            askSkip_ = false;
            if (skipView_.selection() == 0) {
                app.beginDialogueSkip();
                finish(app, -1);
                return false;
            }
            return true;
        }
        if (skipView_.cancelled()) askSkip_ = false;
        return true;
    }
    // 路径行动键：自动播放开 / 关（4.2），立刻写进设置。选项框里也能按，只是选项永远不会被自动选。
    if (eng.keyPressed(engine::Engine::Key::Action)) {
        app.toggleAutoAdvance();
        autoSeconds_ = 0.0;
    }
    // 取消键：整段跳过（只在脚本的对白里；有选项时不行——那一下归选项自己的取消）。
    if (!hasChoices_ && eng.keyPressed(engine::Engine::Key::Cancel) && app.canSkipDialogue()) {
        openSkipAsk(app);
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
        // 自动播放（4.2）：全显之后停 autoAdvanceDelay 就翻过去。**只在没有选项的框里**——
        // 有选项的框走下面那一支，那里没有任何计时，选择永远等玩家自己按。
        if (app.settings().autoAdvance) {
            autoSeconds_ += deltaSeconds;
            if (autoSeconds_ >= autoAdvanceDelay(graphemes_.size())) {
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
    // 立绘在框下面一层（3.1）：先画，框、名签、键位牌、「自动」小标、跳过确认框都压在它上面。
    // 栈里叠着几层对话框时（路径行动一次压好几句）只有最上面那一层画。
    if (app.bustOwner() == serial_) renderBusts(app, box);
    // 说话人不再当面板标题，改成框上沿左侧一块独立的名签（旁白不画）：
    // 框里整块都留给正文，名字也不必与正文挤在同一个平面上。
    ui::drawPanel(eng, box, std::string{}, theme);
    renderNameTag(app, box);
    const int hintLeft = renderKeyHint(app, box);
    // 地基包路 D：第二块键位牌（自动 / 跳过）与「自动」小标，依次排在第一块的左边；确认框压在最上面。
    const int hint2Left = renderKeyHint2(app, box, hintLeft);
    renderAutoBadge(app, box, hint2Left);
    if (askSkip_) renderSkipAsk(app, box);

    const engine::Rect content = ui::panelContentArea(box, std::string{}, theme);
    const int optionRows = hasChoices_ ? choices_.count() : 0;
    const int optionHeight = optionBlockHeight(optionRows, theme);

    // 正文只排已显示的部分：逐字显示时断行要跟着增长的文本走，
    // 否则最后一行会提前占位、看起来像文字在跳。
    engine::Rect textArea{content.x, content.y, content.w, std::max(0, content.h - optionHeight)};
    // 说话人在头像表里有头像就画在正文左侧、正文让开（伙伴改版 3.9）；没登记的角色与旁白一笔不多画。
    // 头像按章生效（韩立的青年头像只在第 7 章）：「演完没有」与取外观同一个判据，问章节表的 done_flag。
    const ChapterDone chapterDone = [&app](int n) { return app.chapterTable().chapterDone(app.state(), n); };
    // 这一句有立绘就不画小头像（3.1），正文也不让位。
    if (const std::optional<PortraitFrame> portrait =
            hasBust_ ? std::nullopt : app.portraits().frameFor(eng, app.headless(), speakerRole_, face_, chapterDone)) {
        const PortraitLayout layout = portraitLayout(textArea, theme.padding, static_cast<int>(portrait->src.w),
                                                     static_cast<int>(portrait->src.h));
        renderPortrait(app, *portrait, layout.portrait);
        textArea = layout.text;
    }
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

void DialogueScene::renderBusts(Application& app, const engine::Rect& box) const {
    const std::vector<BustStage::Layer> layers = app.bustStage().layers();
    if (layers.empty()) return;
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();
    const engine::Rect bottomBox = boxArea(theme);
    const engine::Rect right = bustDialogueArea(box, bottomBox);
    // 框是半透明的：压在框后面的那一截会隐约透出来，读字时像底下糊着一块。只画到框顶那道金线为止（bustVisibleRows），
    // 切口正好叫金线压住——看上去就是人站在框后面。
    const int rows = bustVisibleRows(right, box);
    // 主角站在右边、叫立绘挡住时改站左侧（施工图 3.5.2）。
    const engine::Rect area = bustAvoidHero(right, rows, app.heroScreenRect());
    if (rows <= 0) return;
    const engine::RectF dst{static_cast<float>(area.x), static_cast<float>(area.y), static_cast<float>(area.w),
                            static_cast<float>(rows)};
    const float keep = static_cast<float>(rows) / static_cast<float>(area.h);
    for (const BustStage::Layer& layer : layers) {
        engine::DrawOptions options;
        options.tint.a = static_cast<std::uint8_t>(std::lround(std::clamp(layer.alpha, 0.f, 1.f) * 255.f));
        engine::RectF src = layer.shot.src;
        src.h *= keep;
        eng.drawTexture(layer.shot.texture, src, dst, options);
    }
}

void DialogueScene::renderPortrait(Application& app, const PortraitFrame& portrait, const engine::Rect& area) const {
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();
    const engine::RectF dst{static_cast<float>(area.x), static_cast<float>(area.y), static_cast<float>(area.w),
                            static_cast<float>(area.h)};
    // 垫一块深底、外面一圈暗金细框：头像图透明的地方不透出底下的地图，也与正文分得开。
    eng.fillRect(dst, theme.inkDeep, engine::BlendMode::Alpha);
    eng.drawTexture(portrait.texture, portrait.src, dst, engine::DrawOptions{});
    ui::drawFrame(eng, engine::RectF{dst.x - kPortraitFrameOut, dst.y - kPortraitFrameOut,
                                     dst.w + kPortraitFrameOut * 2.f, dst.h + kPortraitFrameOut * 2.f},
                  theme.goldDim);
}

int DialogueScene::renderKeyHint(Application& app, const engine::Rect& box) const {
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
    if (size.x <= 0) return box.x + box.w - kTagInset;
    const int w = size.x + kHintPadX * 2;
    const engine::Rect plate{box.x + box.w - kTagInset - w, box.y - kHintHeight / 2, w,
                             kHintHeight};
    const engine::RectF plateF{static_cast<float>(plate.x), static_cast<float>(plate.y),
                               static_cast<float>(plate.w), static_cast<float>(plate.h)};
    eng.fillRect(plateF, theme.inkDeep, engine::BlendMode::Alpha);
    ui::drawFrame(eng, plateF, theme.goldDim);
    eng.drawText(hint, plate.x + kHintPadX, plate.y + (kHintHeight - kHintFontSize) / 2 - 1,
                 kHintFontSize, theme.paperDim);
    return plate.x;
}

// ---------------------------------------------------------------------------
// 地基包路 D：自动播放与整段跳过的画法（docs/ch07-foundation.md 4.2、4.3）
// ---------------------------------------------------------------------------

namespace {
constexpr int kPlateGap = 8;          // 框上沿几块小牌之间的空
constexpr int kSkipAskWidth = 420;
}  // namespace

int DialogueScene::renderKeyHint2(Application& app, const engine::Rect& box, int right) const {
    // 键名跟着键位与设备走（Application::text 展开 {key.action} / {pad.*}）。
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();
    const std::string hint = app.text(keyHint2Key(hasChoices_, app.canSkipDialogue()));
    const engine::Point size = eng.measureText(hint, kHintFontSize);
    if (size.x <= 0) return right;
    const int w = size.x + kHintPadX * 2;
    const engine::Rect plate{right - kPlateGap - w, box.y - kHintHeight / 2, w, kHintHeight};
    const engine::RectF plateF{static_cast<float>(plate.x), static_cast<float>(plate.y),
                               static_cast<float>(plate.w), static_cast<float>(plate.h)};
    eng.fillRect(plateF, theme.inkDeep, engine::BlendMode::Alpha);
    ui::drawFrame(eng, plateF, theme.goldDim);
    eng.drawText(hint, plate.x + kHintPadX, plate.y + (kHintHeight - kHintFontSize) / 2 - 1, kHintFontSize,
                 theme.paperDim);
    return plate.x;
}

void DialogueScene::renderAutoBadge(Application& app, const engine::Rect& box, int right) const {
    if (!app.settings().autoAdvance) return;
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();
    const std::string label = app.text("ui.dialogue.auto");
    const engine::Point size = eng.measureText(label, kHintFontSize);
    const int w = size.x + kHintPadX * 2;
    const engine::Rect plate{right - kPlateGap - w, box.y - kHintHeight / 2, w, kHintHeight};
    const engine::RectF plateF{static_cast<float>(plate.x), static_cast<float>(plate.y),
                               static_cast<float>(plate.w), static_cast<float>(plate.h)};
    // 金底墨字：与旁边两块暗牌一眼分得开——它说的是「现在正自动着」，不是一条键位提示。
    eng.fillRect(plateF, theme.gold, engine::BlendMode::Alpha);
    ui::drawFrame(eng, plateF, theme.goldBright);
    eng.drawText(label, plate.x + kHintPadX, plate.y + (kHintHeight - kHintFontSize) / 2 - 1, kHintFontSize,
                 theme.inkDeep);
}

engine::Rect DialogueScene::skipAskArea(const engine::Rect& box, const ui::Theme& theme) {
    const int h = ui::panelTitleBandHeight(/*hasTitle=*/true, theme) + ui::listAreaHeight(2, theme);
    const int x = (engine::kLogicalWidth - kSkipAskWidth) / 2;
    const int above = box.y - h - kTagHeight;
    if (above >= kBoxMargin / 2) return engine::Rect{x, above, kSkipAskWidth, h};
    return engine::Rect{x, box.y + box.h + kTagHeight / 2, kSkipAskWidth, h};
}

void DialogueScene::openSkipAsk(Application& app) {
    std::vector<ui::ListItem> items(2);
    items[0].label = app.text("ui.dialogue.skip.yes");
    items[1].label = app.text("ui.dialogue.skip.no");
    skipView_.reset();
    skipView_.setPageSize(2);
    skipView_.setItems(std::move(items));
    askSkip_ = true;
    askSkipSeconds_ = 0.0;
    askSkipGuard_ = skipAskGuardOverride_ ? *skipAskGuardOverride_ : skipAskGuardSeconds(app.headless());
}

void DialogueScene::renderSkipAsk(Application& app, const engine::Rect& box) const {
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();
    const engine::Rect area = skipAskArea(box, theme);
    // 垫一层实底：面板本身半透明，压在地图上字会透。
    eng.fillRect(engine::RectF{static_cast<float>(area.x), static_cast<float>(area.y), static_cast<float>(area.w),
                               static_cast<float>(area.h)},
                 theme.inkDeep, engine::BlendMode::Alpha);
    const std::string title = app.text("ui.dialogue.skip.ask");
    ui::drawPanel(eng, area, title, theme);
    skipView_.render(eng, ui::panelContentArea(area, title, theme), theme);
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
