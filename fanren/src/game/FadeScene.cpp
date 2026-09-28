#include "game/FadeScene.h"

#include <algorithm>

#include "game/Application.h"
#include "script/Command.h"

namespace fanren::game {

FadeScene::FadeScene(Kind kind, int milliseconds)
    : kind_(kind), duration_(static_cast<double>(std::max(0, milliseconds)) / 1000.0) {}

float FadeScene::levelAt(Kind kind, float from, double t, double duration) {
    // **不要加 default:**：新增一种命令时让编译器告警，而不是悄悄当成等待。
    float to = from;
    switch (kind) {
        case Kind::Out: to = 1.f; break;
        case Kind::In: to = 0.f; break;
        case Kind::Wait: break;
    }
    if (duration <= 0.0 || t >= duration) return to;
    const float k = static_cast<float>(std::max(0.0, t) / duration);
    return from + (to - from) * k;
}

void FadeScene::onEnter(Application& app) {
    // 从「现在有多黑」接着推：连着两次 fade.out，第二次不该先亮一下再黑。
    from_ = app.screenFade();
    elapsed_ = 0.0;
}

bool FadeScene::update(Application& app, double deltaSeconds) {
    elapsed_ += deltaSeconds;
    app.setScreenFade(levelAt(kind_, from_, elapsed_, duration_));
    if (elapsed_ < duration_) return true;
    script::CommandResult result;
    result.ok = true;
    app.completeCommand(result);
    return false;
}

}  // namespace fanren::game
