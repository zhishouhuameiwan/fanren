#pragma once
// 历练的组队面板（伙伴改版，docs/interfaces-companions.md 3.12「组队」）。
//
// 面朝 kind = lilian 的设施按确认（Application::openFacility）时压上来：列出名册上的伙伴
// （rules::roster），每位用 rules::currentRole 给的角色 id；确认一行选上 / 取消，至多 3 位；
// 最后一行「出发」→ Application::enterLilian。名册是空的也开得出来，只剩「出发」一行（独自前往）。
// Esc 作罢，什么也不动。面板只管挑人，进去、回滚、抽签都在 Application。
#include <string>
#include <vector>

#include "game/Scene.h"
#include "ui/Widgets.h"

namespace fanren::game {

class LilianPartyScene : public Scene {
public:
    // 至多带几位（契约 3.12）。
    static constexpr int kMaxCompanions = 3;

    explicit LilianPartyScene(std::string siteId);

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;

    [[nodiscard]] bool opaque() const override { return false; }
    [[nodiscard]] std::string name() const override { return "LilianParty"; }

    // 此刻选上的角色 id，按名册次序（测试读它）。
    [[nodiscard]] std::vector<std::string> chosenRoles() const;
    // 列表一页几行（测试读它：不超过面板画得下的行数）。
    [[nodiscard]] int pageSize() const;

private:
    struct Candidate {
        std::string companionId;
        std::string roleId;   // currentRole 给的
        std::string name;
        bool chosen = false;
    };

    void rebuildRows(Application& app);
    // 列表区（标题与两行说明之下）。onEnter 定页大小、render 画列表都用它。
    [[nodiscard]] engine::Rect listArea(Application& app) const;
    [[nodiscard]] int chosenCount() const;
    // 确认了「出发」：返回 false = 面板就此收起。
    bool depart(Application& app);

    std::string siteId_;
    std::string title_;
    std::string hint_;   // 说明行：挑人的规矩，或「只能独自前往」，或「至多三位」
    std::vector<Candidate> candidates_;
    ui::ListView list_;
};

}  // namespace fanren::game
