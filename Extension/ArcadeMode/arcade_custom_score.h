#pragma once
#include "arcade_model.h"
#include "arcade_native_input_state.h"
#include "arcade_trick_registry.h"
#include <array>

namespace dingosdk::arcade_mode {
// Identities belong to the Arcade registry, never the engine's flip enum.
struct CustomTrickExecution {
    NativeInputOwner owner{};
    std::uint64_t generation{}, episode{}, occurrence{}, step{}, at{};
    ArcadeTrick trick{};
    bool performed{};
    float held_seconds{};
};
struct CustomExecutionBatch {
    std::array<CustomTrickExecution,64> rows{};
    unsigned count{};
    bool overflow{};
};
struct CustomScoreRow {
    ArcadeTrick trick{};
    std::uint64_t occurrence{};
};
// Adds authored trick points to the existing combo. Native score receipts keep
// their separate proof boundary; a HUD number cannot mint either kind of points.
class CustomTrickScoring {
public:
    static constexpr std::uint32_t entry_points=250, flip_points=500;
    static constexpr double hold_points_per_second=100;
    const auto& rows() const noexcept {return rows_;}
    unsigned row_count() const noexcept {return count_;}
    void clear() noexcept {rows_={};count_=0;episode_=0;fraction_=0;}
    void synchronize(const Model& model) noexcept {
        if(settlement_!=model.last_settlement().sequence) {
            clear();settlement_=model.last_settlement().sequence;
        }
    }
    bool observe(Model& model,const CustomTrickExecution& e,const NativeInputOwner& owner,
                 std::uint64_t generation,std::uint64_t now) noexcept {
        synchronize(model);
        if(!model.state().combo_active || model.state().balance_kind!=BalanceKind::manual ||
            !owner.valid() || !generation || e.owner!=owner || e.generation!=generation || !e.episode || !e.occurrence || !e.step ||
            !e.at || now<e.at || now-e.at>150 || !std::isfinite(e.held_seconds) ||
            e.held_seconds<0 || e.held_seconds>.1f ||
            (e.trick!=ArcadeTrick::casper && e.trick!=ArcadeTrick::casper_flip))return false;
        if(owner!=owner_ || generation!=generation_) {
            clear();owner_=owner;generation_=generation;last_occurrence_=last_step_=0;
        }
        if(e.step<=last_step_ || e.occurrence<last_occurrence_)return false;
        if(e.performed) {
            if(e.occurrence==last_occurrence_)return false;
            if(e.trick==ArcadeTrick::casper_flip && episode_!=e.episode)return false;
            if(!model.custom_trick(e.trick==ArcadeTrick::casper?entry_points:flip_points))return false;
            if(count_==rows_.size()) {
                for(unsigned i=1;i<count_;++i)rows_[i-1]=rows_[i];
                --count_;
            }
            rows_[count_++]={e.trick,e.occurrence};
            last_occurrence_=e.occurrence;
            episode_=e.trick==ArcadeTrick::casper?e.episode:0;
            fraction_=0;
        } else if(e.trick!=ArcadeTrick::casper || episode_!=e.episode ||
                  e.occurrence!=last_occurrence_)return false;
        if(e.trick==ArcadeTrick::casper && e.held_seconds>0) {
            fraction_+=static_cast<double>(e.held_seconds)*hold_points_per_second;
            const auto points=static_cast<std::uint32_t>(fraction_+1e-5);
            model.credit_custom_points(points);fraction_-=points;
        }
        last_step_=e.step;return true;
    }
private:
    NativeInputOwner owner_{};
    std::uint64_t generation_{},episode_{},last_occurrence_{},last_step_{},settlement_{};
    std::array<CustomScoreRow,5> rows_{};
    unsigned count_{};
    double fraction_{};
};
}
