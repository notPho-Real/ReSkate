#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>

namespace dingosdk::arcade_mode {
// Cross sections of native collision, not a mesh bounding box or grind-contact
// attribution. Only the selected fixture object's native picking hits count.
struct FixtureGeometryContext {
    std::array<std::uintptr_t, 5> owner{};
    std::uint64_t generation{}, revision{}, object{}, physics_changes{};
    bool valid{};
    bool operator==(const FixtureGeometryContext&) const = default;
};
enum class GeometryStatus : unsigned { idle, sampling, complete, cancelled, timeout, unavailable, clipped, discontinuous, armed };
struct FixtureGeometryRay { unsigned index{}; float x{}, z{}; };
class FixtureGeometry {
public:
    static constexpr unsigned samples = 129;
    static constexpr float step = .25f, radius = 8.f;
    bool begin(const FixtureGeometryContext& context, std::uint64_t now) noexcept {
        if (status == GeometryStatus::sampling || !context.valid || !context.generation || !context.object) return false;
        *this = {}; context_ = context; began_ = last_ = now; status = GeometryStatus::sampling; return true;
    }
    bool arm(const FixtureGeometryContext& context, std::uint64_t now) noexcept {
        if (status == GeometryStatus::armed || !begin(context, now)) return false;
        status = GeometryStatus::armed; return true;
    }
    void activate(const FixtureGeometryContext& context, std::uint64_t now, bool gameplay) noexcept {
        if (status != GeometryStatus::armed) return;
        if (context != context_ || !context.valid || now < last_) cancel();
        else if (now - began_ > 5000) status = GeometryStatus::timeout;
        else if (gameplay) { (void)begin(context, now); }
        last_ = now;
    }
    bool result_current(const FixtureGeometryContext& context) const noexcept {
        return status == GeometryStatus::complete && context.valid && context == context_;
    }
    bool current(const FixtureGeometryContext& context, std::uint64_t now) noexcept {
        if (status != GeometryStatus::sampling) return false;
        if (context != context_ || !context.valid || now < last_) cancel();
        else if (now - began_ > 5000 || (pending_ && now - sent_ > 250)) status = GeometryStatus::timeout;
        last_ = now; return status == GeometryStatus::sampling;
    }
    std::optional<FixtureGeometryRay> next(const FixtureGeometryContext& context, std::uint64_t now) noexcept {
        if (!current(context, now) || pending_ || count >= samples) return {};
        pending_ = true; sent_ = now;
        if (!count) return FixtureGeometryRay{};
        const auto n = (count - 1) % 64;
        const float offset = (static_cast<int>(n) - 32 + (n >= 32 ? 1 : 0)) * step;
        return FixtureGeometryRay{count, count <= 64 ? offset : 0.f, count <= 64 ? 0.f : offset};
    }
    void accept(unsigned index, bool available, bool hit, std::uint64_t object, float height) noexcept {
        if (status != GeometryStatus::sampling || !pending_ || index != count) return;
        pending_ = false;
        if (!available || (hit && (!std::isfinite(height) || (object && object != context_.object)))) {
            status = GeometryStatus::unavailable; return;
        }
        const bool ours = hit && object == context_.object;
        if (!count && !ours) { status = GeometryStatus::unavailable; return; }
        if (ours) {
            if (!count) height_min = height_max = height;
            else { if (height < height_min) height_min = height; if (height > height_max) height_max = height; }
        }
        if (!count) { hits_[0][32] = hits_[1][32] = true; }
        else {
            const auto n = (count - 1) % 64;
            hits_[(count - 1) / 64][n + (n >= 32 ? 1 : 0)] = ours;
        }
        ++count;
        if (count == samples) finish();
    }
    void cancel() noexcept {
        if (status == GeometryStatus::sampling || status == GeometryStatus::armed || status == GeometryStatus::complete)
            status = GeometryStatus::cancelled;
    }
    void unavailable() noexcept { if (status == GeometryStatus::sampling) status = GeometryStatus::unavailable; }
    bool pending() const noexcept { return pending_; }
    GeometryStatus status{GeometryStatus::idle};
    unsigned count{};
    std::array<float, 2> low{}, high{};
    float height_min{}, height_max{};
private:
    void finish() noexcept {
        for (unsigned axis = 0; axis < 2; ++axis) {
            if (hits_[axis][0] || hits_[axis][64]) { status = GeometryStatus::clipped; return; }
            unsigned first = 32, last = 32;
            for (unsigned i = 0; i < 65; ++i) if (hits_[axis][i]) { if (i < first) first = i; if (i > last) last = i; }
            for (unsigned i = first; i <= last; ++i) if (!hits_[axis][i]) { status = GeometryStatus::discontinuous; return; }
            low[axis] = (static_cast<int>(first) - 32) * step;
            high[axis] = (static_cast<int>(last) - 32) * step;
        }
        status = GeometryStatus::complete;
    }
    FixtureGeometryContext context_{};
    std::array<std::array<bool, 65>, 2> hits_{};
    std::uint64_t began_{}, last_{}, sent_{};
    bool pending_{};
};
// Called once per validated client tick. Inactive by default; no world writes.
void tick_arcade_fixture_geometry(std::uint64_t now, bool gameplay) noexcept;
}
