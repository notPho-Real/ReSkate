#include "Extension/ArcadeMode/arcade_fixture_trajectory.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numbers>
#include <type_traits>
using namespace dingosdk::arcade_mode;
unsigned checks{};
void check(bool v, const char* m) { ++checks; if (!v) { std::cerr << m << '\n'; std::exit(1); } }
TrajectoryContext context() { return {{1, 2, 3, 4, 5}, 1, 2, 3, true}; }
TrajectorySample sample(std::uint64_t now = 1000) { return {now, {10, 20, 30}, 0, 0, 100, true}; }
int main() {
    static_assert(std::is_trivially_copyable_v<FixtureTrajectoryEvidence>);
    const auto origin = sample().position;
    {
        float heading = std::atan2(-1.e-8f, 1.f) * 180.f / std::numbers::pi_v<float>;
        check(heading < 0, "near-zero native forward vector yields negative atan2 heading");
        heading += 360.f;
        check(heading == 360.f, "existing float producer arithmetic rounds negative near-zero heading to360");
        auto s = sample(); s.heading = heading;
        FixtureTrajectory rejected, fixed;
        check(!rejected.arm(context(), s, origin, 1000), "unnormalized360 remains rejected by existing range guard");
        s.heading = trajectory_heading(s.heading);
        check(s.heading == 0 && fixed.arm(context(), s, origin, 1000), "adapter canonicalizes only equivalent360 endpoint");
        const auto row = fixed.tick(context(), sample(1050), 1050, true);
        check(row && row->heading_mdeg == 0 && row->reason == TrajectoryReason::none,
              "normalized endpoint produces valid zero heading with no failure reason");
        for (float invalid : {-1.f, 360.01f, std::numeric_limits<float>::infinity(),
                              std::numeric_limits<float>::quiet_NaN()}) {
            FixtureTrajectory t; auto bad = sample(); bad.heading = trajectory_heading(invalid);
            check(!t.arm(context(), bad, origin, 1000), "normalization does not admit out-of-range or nonfinite heading");
        }
    }
    for (unsigned fault = 0; fault < 12; ++fault) {
        FixtureTrajectory t; auto c = context(); auto s = sample();
        if (fault == 0) c.valid = false;
        if (fault == 1) c.owner[2] = 0;
        if (fault == 2) c.generation = 0;
        if (fault == 3) s.sampled = 849;
        if (fault == 4) s.sampled = 1001;
        if (fault == 5) s.speed = .11f;
        if (fault == 6) s.heading = 4;
        if (fault == 7) s.state = 200;
        if (fault == 8) s.position[0] += .51f;
        if (fault == 9) s.position[1] = std::numeric_limits<float>::quiet_NaN();
        if (fault == 10) s.valid = false;
        if (fault == 11) s.sampled = 0;
        check(!t.arm(c, s, origin, 1000), "invalid arming baseline refused");
    }
    {
        FixtureTrajectory t; check(!t.tick(context(), sample(), 1000, true), "default inactive");
        check(t.arm(context(), sample(), origin, 1000), "arm stopped verified baseline");
        check(!t.arm(context(), sample(), origin, 1000), "cannot rearm active capture");
        check(!t.tick(context(), sample(1050), 1050, false), "console handoff emits no position");
        auto s = sample(1100); s.position = {11.234f, 19.5f, 33}; s.speed = 2.5f; s.heading = 359.5f; s.state = 201;
        const auto row = t.tick(context(), s, 1100, true);
        check(row && row->status == TrajectoryStatus::sample && row->sampled == 1100 && row->ms == 1100 &&
              row->x_mm == 1234 && row->y_mm == -500 && row->z_mm == 3000 && row->speed_mm_s == 2500 &&
              row->heading_mdeg == 359500 && row->state == 201, "relative bounded native sample");
        check(!t.tick(context(), s, 1110, true), "no duplicate timestamp or unbounded tick sampling");
        const auto end = t.cancel(1120);
        check(end && end->sequence == 2 && !end->sampled && !end->state && !end->x_mm &&
              end->status == TrajectoryStatus::cancelled, "terminal contains no fabricated position");
    }
    for (unsigned fault = 0; fault < 10; ++fault) {
        FixtureTrajectory t; t.arm(context(), sample(), origin, 1000);
        t.tick(context(), sample(1050), 1050, true);
        auto c = context(); auto s = sample(1100); std::uint64_t now = 1100; bool gameplay = true;
        if (fault == 0) c.owner[0]++;
        if (fault == 1) c.revision++;
        if (fault == 2) c.generation++;
        if (fault == 3) c.object++;
        if (fault == 4) s.position[2] += 101;
        if (fault == 5) s.position[1] += 21;
        if (fault == 6) s.speed = 51;
        if (fault == 7) gameplay = false;
        if (fault == 8) now = 1049;
        if (fault == 9) { now = 1201; s.sampled = now; }
        auto r = t.tick(c, s, now, gameplay);
        check(r && !r->sampled && !t.active() && r->status == (fault >= 8 ? TrajectoryStatus::gap : TrajectoryStatus::invalid),
              "owner geometry menu bounds and tick gaps stop without partial samples");
        const auto reason = fault <= 3 ? TrajectoryReason::context_changed : fault <= 5 ? TrajectoryReason::position_invalid :
            fault == 6 ? TrajectoryReason::speed_invalid : fault == 7 ? TrajectoryReason::gameplay_ended : TrajectoryReason::tick_time;
        check(r->reason == reason, "terminal reports exact first failing guard without raw invalid data");
    }
    for (unsigned fault = 0; fault < 7; ++fault) {
        FixtureTrajectory t; t.arm(context(), sample(), origin, 1000);
        t.tick(context(), sample(1050), 1050, true);
        auto c = context(); auto s = sample(1100);
        if (fault == 0) c.valid = false;
        if (fault == 1) s.valid = false;
        if (fault == 2) s.heading = 360.f;
        if (fault == 3) s.state = 10001;
        if (fault == 4) s.sampled = 1049;
        if (fault == 5) s.sampled = 949;
        if (fault == 6) c.owner[0] = 0;
        const auto row = t.tick(c, s, 1100, true);
        constexpr std::array reasons{TrajectoryReason::context_invalid, TrajectoryReason::sample_invalid,
            TrajectoryReason::heading_invalid, TrajectoryReason::state_invalid, TrajectoryReason::sample_reordered,
            TrajectoryReason::sample_time, TrajectoryReason::context_invalid};
        check(row && row->reason == reasons[fault] && !row->sampled && !row->state && !row->heading_mdeg && !t.active(),
              "distinct context sample heading state reordering and freshness reasons preserve failclosed stops");
    }
    {
        FixtureTrajectory t; t.arm(context(), sample(), origin, 1000);
        check(!t.tick(context(), sample(), 1050, true), "cached producer snapshot is not repeated");
        check(!t.tick(context(), sample(), 1100, true), "fresh age alone does not create a new sample");
        check(!t.tick(context(), sample(), 1150, true), "freshness boundary inclusive");
        const auto stale = t.tick(context(), sample(), 1151, true);
        check(stale && stale->status == TrajectoryStatus::gap && !t.count, "stalled producer invalidates even while client keeps ticking");
    }
    {
        FixtureTrajectory t; t.arm(context(), sample(), origin, 1000);
        std::optional<FixtureTrajectoryEvidence> end;
        for (std::uint64_t now = 1050; now <= 25050; now += 50) end = t.tick(context(), sample(now), now, true);
        check(end && end->status == TrajectoryStatus::complete && t.count == 480 && !t.active(), "twenty-four second bound");
        check(!t.tick(context(), sample(25100), 25100, true), "no sampling after completion");
        check(t.arm(context(), sample(25200), origin, 25200) && t.session == 2, "new session distinct and opt in");
        t.delivery_failed(); check(t.dropped == 1 && t.status == TrajectoryStatus::dropped && !t.active(), "queue loss halts capture and retained in status");
    }
    {
        FixtureTrajectory t; t.arm(context(), sample(), origin, 1000);
        std::optional<FixtureTrajectoryEvidence> end;
        for (std::uint64_t now = 1050; now <= 6050; now += 50) end = t.tick(context(), sample(now), now, false);
        check(end && end->status == TrajectoryStatus::timeout && !t.count, "bounded console handoff");
    }
    std::cout << checks << " fixture trajectory checks passed\n";
}
