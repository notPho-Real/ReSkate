"""Read-only board-root evidence for a separately reviewed bounded runner.

Source: skater_state.h, native_board_velocity(), and upstream
Extension/Skater/grind_exit_control.cpp::board_velocity. The caller supplies
read(address, size) and a fresh, executable-pinned local-owner predicate.
This module opens no process, sends no controls and writes no memory.

Equal bracket reads are not an atomic physics step. The velocity is the native
body buffer: pending bit8 means its velocity has not necessarily been applied
to the physics world. Neither a positive Y value nor state200 alone proves an
A-button rail pop, native trajectory call, early flip, HUD or combo continuity.
Correlate with physical input, native identities and both measured step clocks.
"""
import math
import struct


def board_motion_sample(base, core, context, rig, read, valid):
    """Return bounded raw evidence, or None for unreadable/unowned data."""
    def pointer(p):
        return isinstance(p, int) and 0x10000 < p < 0x7fffffff8000 and not p & 7

    def unpack(p, fmt):
        return struct.unpack(fmt, read(p, struct.calcsize(fmt)))

    def u64(p):
        return unpack(p, '<Q')[0]

    def u32(p):
        return unpack(p, '<I')[0]

    def layout():
        if (not valid() or u64(core) != base + 0x6176d30 or
                u64(core + 0x3c0) != context or u64(core + 0x438) != rig):
            return None
        state = u32(context + 5140)
        # Existing grind receipts call context+5144 "mode" (often400 on rail).
        # InputSession instead uses the rig's crouch mode at+0x59a8. Keep them
        # separate: the former cannot prove rail_pop_input_ready's mode<=2.
        selector_1410 = u32(context + 5136)
        selector_1418 = u32(context + 5144)
        crouch_mode = u32(rig + 0x59a8)
        holder = u64(core + 0x430)
        if not pointer(holder):
            return None
        physics = u64(holder + 0x18)
        if not pointer(physics) or u64(physics) != base + 0x65e67a0:
            return None
        bodies = u64(physics + 0x20)
        if not pointer(bodies) or u32(bodies) != 9 or u64(bodies + 0x10) != physics:
            return None
        return state, selector_1410, selector_1418, crouch_mode, holder, physics, bodies

    if not all(pointer(p) for p in (base, core, context, rig)) or base > 0x7fffffffffff - 0x65e67a0:
        return None
    try:
        before = layout()
        if before is None:
            return None
        body = before[-1]  # Native board_root_body=0, physics_body_size=0x130.
        flags = u32(body + 0x60)
        velocity = unpack(body + 0x70, '<3f')
        again_velocity = unpack(body + 0x70, '<3f')
        again_flags = u32(body + 0x60)
        after = layout()
        owned = valid()
        finite = all(math.isfinite(v) and abs(v) < 1000 for v in velocity + again_velocity)
        # Retain inconsistent before/after values as explicitly invalid raw
        # evidence; a later valid bracket must not erase the earlier reading.
        return dict(stable_reads=bool(owned and finite and before == after and
                                     velocity == again_velocity and flags == again_flags),
                    state=before[0], selector_1410=before[1], selector_1418=before[2], crouch_mode=before[3],
                    layout_before=before, layout_after=after,
                    velocity_buffer_m_s=velocity, velocity_buffer_after_m_s=again_velocity,
                    pending_flags=flags, pending_flags_after=again_flags,
                    velocity_pending=bool((flags | again_flags) & 8),
                    owner_valid_after=bool(owned))
    except (OSError, EOFError, struct.error):
        return None
