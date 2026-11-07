"""Inert memory fixtures only; no process, device, network or file writes."""
import struct
import unittest

from grind_board_motion_observer import board_motion_sample


class Memory:
    base, core, context = 0x140000000, 0x20000, 0x30000
    holder, physics, bodies, rig = 0x40000, 0x50000, 0x60000, 0x70000

    def __init__(self):
        self.values = {}
        for address, value in ((self.core, self.base + 0x6176d30),
                               (self.core + 0x3c0, self.context),
                               (self.core + 0x430, self.holder),
                               (self.core + 0x438, self.rig),
                               (self.holder + 0x18, self.physics),
                               (self.physics, self.base + 0x65e67a0),
                               (self.physics + 0x20, self.bodies),
                               (self.bodies + 0x10, self.physics)):
            self.put(address, '<Q', value)
        self.put(self.context + 5140, '<I', 200)
        self.put(self.context + 5136, '<I', 200)
        self.put(self.context + 5144, '<I', 200)
        self.put(self.rig + 0x59a8, '<I', 2)
        self.put(self.bodies, '<I', 9)
        self.put(self.bodies + 0x60, '<I', 0)
        self.put(self.bodies + 0x70, '<3f', 0, 2.03, 4.5)
        self.counts, self.replacements = {}, {}
        self.owner_checks = 0
        self.lose_owner_at = 0

    def put(self, address, fmt, *value):
        self.values[address] = struct.pack(fmt, *value)

    def read(self, address, size):
        n = self.counts.get(address, 0) + 1
        self.counts[address] = n
        value = self.replacements.get(address) if n == 2 else None
        if value is None:
            value = self.values.get(address)
        if value is None:
            raise OSError('unmapped fixture')
        assert len(value) == size
        return value

    def valid(self):
        self.owner_checks += 1
        return not self.lose_owner_at or self.owner_checks < self.lose_owner_at

    def sample(self):
        return board_motion_sample(self.base, self.core, self.context, self.rig, self.read, self.valid)


class BoardMotionTests(unittest.TestCase):
    def test_stable_buffer_and_pending_velocity_are_distinct(self):
        for flags in (0, 8, 0x20, 0x28):
            with self.subTest(flags=flags):
                m = Memory(); m.put(m.bodies + 0x60, '<I', flags)
                r = m.sample()
                self.assertTrue(r['stable_reads'])
                self.assertEqual(r['velocity_pending'], bool(flags & 8))
                self.assertAlmostEqual(r['velocity_buffer_m_s'][1], 2.03, places=5)

    def test_negative_or_zero_velocity_is_retained(self):
        for y in (-2.03, 0):
            with self.subTest(y=y):
                m = Memory(); m.put(m.bodies + 0x70, '<3f', 0, y, 4.5)
                r = m.sample()
                self.assertTrue(r['stable_reads'])
                self.assertLessEqual(r['velocity_buffer_m_s'][1], 0)

    def test_selector_words_are_not_rig_crouch_mode(self):
        m = Memory()
        m.put(m.context + 5140, '<I', 403)
        m.put(m.context + 5136, '<I', 400)
        m.put(m.context + 5144, '<I', 401)
        r = m.sample()
        self.assertTrue(r['stable_reads'])
        self.assertEqual((r['state'], r['selector_1410'], r['selector_1418'], r['crouch_mode']), (403, 400, 401, 2))

    def test_every_layout_field_brackets_owner_and_state(self):
        # Every pointer, vtable, state, count and owner is re-read. Replacements
        # deliberately break the layout; original raw velocity remains marked
        # unstable instead of being relabeled as the new owner's observation.
        for field in Memory().values:
            if field in (Memory.bodies + 0x60, Memory.bodies + 0x70):
                continue
            with self.subTest(field=hex(field)):
                m = Memory(); m.replacements[field] = bytes(len(m.values[field]))
                r = m.sample()
                self.assertIsNotNone(r)
                self.assertFalse(r['stable_reads'])

    def test_initial_layout_rejection(self):
        for field in (Memory.core, Memory.core + 0x3c0, Memory.core + 0x430, Memory.core + 0x438,
                      Memory.holder + 0x18, Memory.physics, Memory.physics + 0x20,
                      Memory.bodies, Memory.bodies + 0x10):
            with self.subTest(field=hex(field)):
                m = Memory(); m.values[field] = bytes(len(m.values[field]))
                self.assertIsNone(m.sample())

    def test_mixed_velocity_and_flags_are_preserved(self):
        for address, fmt, value in ((Memory.bodies + 0x70, '<3f', (0, -1, 4.5)),
                                    (Memory.bodies + 0x60, '<I', (8,))):
            with self.subTest(address=hex(address)):
                m = Memory(); m.replacements[address] = struct.pack(fmt, *value)
                r = m.sample()
                self.assertFalse(r['stable_reads'])
                self.assertGreater(r['velocity_buffer_m_s'][1], 0)
                if address == m.bodies + 0x70:
                    self.assertLess(r['velocity_buffer_after_m_s'][1], 0)
                else:
                    self.assertTrue(r['velocity_pending'])

    def test_owner_loss_at_each_boundary(self):
        for boundary in (1, 2, 3):
            with self.subTest(boundary=boundary):
                m = Memory(); m.lose_owner_at = boundary
                r = m.sample()
                self.assertTrue(r is None or not r['stable_reads'])

    def test_invalid_values_and_unreadable_memory(self):
        for y in (float('nan'), float('inf'), 1000, -1000):
            with self.subTest(y=y):
                m = Memory(); m.put(m.bodies + 0x70, '<3f', 0, y, 0)
                self.assertFalse(m.sample()['stable_reads'])
        m = Memory(); del m.values[m.bodies + 0x70]
        self.assertIsNone(m.sample())
        m = Memory(); m.core = 0x20001
        self.assertIsNone(m.sample())


if __name__ == '__main__':
    unittest.main()
