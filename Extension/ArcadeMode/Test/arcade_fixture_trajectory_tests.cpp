#include "Extension/ArcadeMode/arcade_fixture_trajectory.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <type_traits>
using namespace dingosdk::arcade_mode;
unsigned checks{};
void check(bool v, const char* m) { ++checks; if (!v) { std::cerr << m << '\n'; std::exit(1); } }
TrajectoryContext context() { return {{1, 2, 3, 4, 5}, 1, 2, 3, true}; }
TrajectorySample sample(std::uint64_t now = 1000) { return {now, {10, 20, 30}, 0, 0, 100, true}; }
int main() {
    static_assert(std::is_trivially_copyable_v<FixtureTrajectoryEvidence>);
    const auto origin = sample().position;
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
