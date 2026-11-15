#pragma once
#include "arcade_custom_score.h"
#include "Extension/Skater/score_hud.h"
#include "Extension/UI/score_line.h"

namespace dingosdk::arcade_mode {
// Combines native presentation with the separately earned Arcade bonus. Native
// UI amounts remain presentation only: no inferred native score enters Model.
class CustomScoreDisplay {
public:
    void reset() noexcept {*this={};}
    std::optional<score_line::Line> update(const Model& model,const CustomTrickScoring& scoring,
        const std::optional<tricks::ScoreHud>& hud,std::uint64_t now) {
        const auto& state=model.state();
        const auto& terminal=model.last_settlement();
        if(state.combo_active && (!active_ || terminal.sequence!=terminal_)) {
            names_.clear();last_custom_=0;closed_until_=0;banked_=0;
        }
        if(terminal.sequence!=terminal_) {
            terminal_=terminal.sequence;
            banked_=terminal.kind==SettlementKind::bank?terminal.custom_points:0;
            closed_until_=banked_?now+2000:0;
        }
        active_=state.combo_active;
        if(hud && hud->rows_read) {
            for(auto& prior:seen_)if(prior.handle && std::none_of(hud->rows.begin(),hud->rows.end(),
                [&](const auto& row){return row.handle==prior.handle;}))prior={};
            for(auto it=hud->rows.rbegin();it!=hud->rows.rend();++it) {
                if(!it->handle || it->name.empty())continue;
                // The native HUD recycles its row models. A handle identifies
                // storage, not a permanent trick occurrence.
                auto prior=std::find_if(seen_.begin(),seen_.end(),
                    [&](const auto& row){return row.handle==it->handle;});
                if(prior!=seen_.end() && prior->name==it->name)continue;
                if(prior==seen_.end())prior=seen_.begin()+(seen_next_++ % seen_.size());
                *prior=*it;
                if(active_)append(it->name);
            }
        }
        if(active_ && state.custom_points)for(unsigned i=0;i<scoring.row_count();++i) {
            const auto& row=scoring.rows()[i];
            if(row.occurrence<=last_custom_)continue;
            append(std::string(trick_definition(row.trick)->name));last_custom_=row.occurrence;
        }
        if(!hud || !hud->rows_read || (active_?!state.custom_points:(!closed_until_ || now>=closed_until_)))return std::nullopt;
        score_line::Line line;
        line.tricks=names_;
        line.closed=!active_;
        const double bonus=active_?static_cast<double>(state.custom_points)*state.multiplier:static_cast<double>(banked_);
        const auto amount=[](double value) {return std::isfinite(value)?std::clamp(value,0.0,score_line::most_points):0.0;};
        line.points=amount(static_cast<double>(hud->line_score)+bonus);
        line.line=amount(static_cast<double>(hud->line_shown)+bonus);
        line.multiplier=static_cast<float>(std::clamp(state.multiplier,1u,99u));
        line.timer=state.landing_grace?std::clamp(state.link_grace_s/.45f,0.f,1.f):1.f;
        return line;
    }
private:
    void append(std::string name) {
        if(name.empty())return;
        if(names_.size()==score_line::most_tricks)names_.erase(names_.begin());
        names_.push_back(std::move(name));
    }
    std::vector<std::string> names_;
    std::array<tricks::ScoreHudRow,64> seen_{};
    unsigned seen_next_{};
    std::uint64_t last_custom_{},terminal_{},banked_{},closed_until_{};
    bool active_{};
};
}
