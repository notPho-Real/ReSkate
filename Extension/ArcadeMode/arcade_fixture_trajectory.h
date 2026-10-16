#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>

namespace dingosdk::arcade_mode {
enum class TrajectoryStatus : unsigned { idle, armed, sample, complete, invalid, gap, timeout, cancelled, dropped };
struct TrajectoryContext {
    std::array<std::uintptr_t, 5> owner{};
    std::uint64_t generation{}, revision{}, object{};
    bool valid{};
    bool operator==(const TrajectoryContext&) const = default;
};
struct TrajectorySample {
    std::uint64_t sampled{};
    std::array<float, 3> position{};
    float speed{}, heading{};
    unsigned state{};
    bool valid{};
};
// Relative coordinates only; native identities never enter the worker queue.
struct FixtureTrajectoryEvidence {
    std::uint64_t session{}, sequence{}, ms{}, sampled{}, generation{}, revision{}, dropped{};
    TrajectoryStatus status{TrajectoryStatus::idle};
    int x_mm{}, y_mm{}, z_mm{};
    unsigned speed_mm_s{}, heading_mdeg{}, state{};
};
class FixtureTrajectory {
public:
    static constexpr unsigned duration_ms = 24000, interval_ms = 50, freshness_ms = 150, sample_limit = 481;
    bool active() const noexcept { return status == TrajectoryStatus::armed || status == TrajectoryStatus::sample; }
    bool arm(const TrajectoryContext& c, const TrajectorySample& s, std::array<float, 3> origin,
             std::uint64_t now) noexcept {
        if (active() || session == UINT64_MAX || !valid(c, s, origin, now) || s.state != 100 || s.speed > .1f ||
            std::abs(std::remainder(s.heading, 360.f)) > 3.f) return false;
        float distance{};
        for (unsigned i = 0; i < 3; ++i) distance += (s.position[i] - origin[i]) * (s.position[i] - origin[i]);
        if (distance > .25f) return false;
        ++session; count = 0; dropped = 0; sequence_ = 0; context_ = c; origin_ = origin;
        began_ = last_ = now; last_sampled_ = s.sampled; next_ = 0; status = TrajectoryStatus::armed;
        return true;
    }
    std::optional<FixtureTrajectoryEvidence> tick(const TrajectoryContext& c, const TrajectorySample& s,
                                                 std::uint64_t now, bool gameplay) noexcept {
        if (!active()) return {};
        if (now < last_ || now - last_ > freshness_ms) return stop(TrajectoryStatus::gap, now);
        last_ = now;
        if (s.sampled && (now < s.sampled || now - s.sampled > freshness_ms)) return stop(TrajectoryStatus::gap, now);
        if (c != context_ || !valid(c, s, origin_, now) || s.sampled < last_sampled_)
            return stop(TrajectoryStatus::invalid, now);
        if (status == TrajectoryStatus::armed) {
            if (now - began_ > 5000) return stop(TrajectoryStatus::timeout, now);
            if (!gameplay) return {};
            began_ = now; status = TrajectoryStatus::sample;
        } else if (!gameplay) return stop(TrajectoryStatus::invalid, now);
        if (now - began_ >= duration_ms || count >= sample_limit) return stop(TrajectoryStatus::complete, now);
        if (now < next_ || s.sampled == last_sampled_) return {};
        next_ = now + interval_ms; last_sampled_ = s.sampled; ++count;
        auto r = row(now); r.sampled = s.sampled; r.state = s.state;
        r.x_mm = quantize(s.position[0] - origin_[0]); r.y_mm = quantize(s.position[1] - origin_[1]);
        r.z_mm = quantize(s.position[2] - origin_[2]); r.speed_mm_s = static_cast<unsigned>(quantize(s.speed));
        r.heading_mdeg = static_cast<unsigned>(quantize(s.heading)); return r;
    }
    std::optional<FixtureTrajectoryEvidence> cancel(std::uint64_t now) noexcept {
        if (!active()) return {};
        return stop(TrajectoryStatus::cancelled, now);
    }
    void delivery_failed() noexcept { ++dropped; status = TrajectoryStatus::dropped; }
    TrajectoryStatus status{TrajectoryStatus::idle};
    std::uint64_t session{}, dropped{};
    unsigned count{};
private:
    static int quantize(float value) noexcept { return static_cast<int>(std::lround(value * 1000.f)); }
    static bool valid(const TrajectoryContext& c, const TrajectorySample& s,
                      const std::array<float, 3>& origin, std::uint64_t now) noexcept {
        if (!c.valid || !c.generation || !c.revision || !c.object || !s.valid || !s.sampled ||
            now < s.sampled || now - s.sampled > freshness_ms || s.state > 10000 ||
            !std::isfinite(s.speed) || s.speed < 0 || s.speed > 50 ||
            !std::isfinite(s.heading) || s.heading < 0 || s.heading >= 360) return false;
        for (auto part : c.owner) if (!part) return false;
        for (unsigned i = 0; i < 3; ++i)
            if (!std::isfinite(origin[i]) || !std::isfinite(s.position[i]) ||
                std::abs(origin[i]) > 100000 || std::abs(s.position[i] - origin[i]) > (i == 1 ? 20.f : 100.f)) return false;
        return true;
    }
    FixtureTrajectoryEvidence row(std::uint64_t now) noexcept {
        return {session, ++sequence_, now, 0, context_.generation, context_.revision, dropped, status};
    }
    FixtureTrajectoryEvidence stop(TrajectoryStatus reason, std::uint64_t now) noexcept { status = reason; return row(now); }
    TrajectoryContext context_{};
    std::array<float, 3> origin_{};
    std::uint64_t began_{}, last_{}, last_sampled_{}, next_{}, sequence_{};
};
}
