#pragma once
#include "arcade_custom_score.h"
#include "Extension/Skater/score_hud.h"
#include "Extension/UI/hud_corner.h"
#include "Extension/UI/score_line_hud.h"

namespace dingosdk::arcade_mode {
// The original native widget owns visibility and animations. Its Arcade numbers
// come from performed native/custom actions, never its fractional display value.
class CustomScoreDisplay {
public:
    void reset() noexcept { *this = {}; }
    const auto& rows() const noexcept { return rows_; }
    std::optional<hud_corner::ScorePresentation> update(const Model& model,const CustomTrickScoring& scoring,
        const std::optional<tricks::ScoreHud>& hud,std::uint64_t now,bool verified) {
        if(!hud || !hud->native_scoring) { reset();return std::nullopt; }
        const auto& native=*hud->native_scoring;
        const auto& state=model.state();const auto& terminal=model.last_settlement();
        const bool same_owner=last_ && last_->manager==native.manager && last_->hud==native.hud && last_->scoring==native.scoring;
        if(!same_owner) {last_.reset();rows_.clear();until_=0;proven_=false;}
        if(terminal.sequence!=terminal_) {
            if(last_ && proven_ && terminal.kind==SettlementKind::bank && terminal.actions && terminal.points) {
                last_->total=terminal.points;last_->multiplier=terminal.actions;
                last_->points=terminal.points/terminal.actions;until_=now+3000;
            } else {last_.reset();rows_.clear();until_=0;}
            terminal_=terminal.sequence;
        }
        hud_corner::ScorePresentation view{native.manager,native.hud,native.scoring,0,0,0};
        if(state.combo_active) {
            until_=0;rows_.clear();
            if(verified) {
                const auto maximum=std::numeric_limits<std::uint64_t>::max();
                view.points=state.raw_points+std::min(state.custom_points,maximum-state.raw_points);
                view.multiplier=state.multiplier;
                view.total=view.multiplier && view.points>maximum/view.multiplier ? maximum : view.points*view.multiplier;
            }
            if(state.custom_points)for(unsigned i=0;i<scoring.row_count();++i) {
                const auto& row=scoring.rows()[i];
                rows_.push_back({row.occurrence,std::string(trick_definition(row.trick)->name),terminal.sequence});
            }
            last_=view;proven_=verified;return view;
        }
        if(last_ && until_ && now<until_)return last_;
        last_.reset();rows_.clear();proven_=false;
        // No fractional native multiplier leaks through while Arcade owns this
        // HUD. Zero hides the ring until there is a performed action to count.
        return view;
    }
private:
    std::optional<hud_corner::ScorePresentation> last_;
    std::vector<score_line::ExtraTrick> rows_;
    std::uint64_t terminal_{},until_{};
    bool proven_{};
};
}
