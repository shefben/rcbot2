import unittest
from ffhelpers import FFMedic, FFHWGuy

class MedicHWGuySpecialTests(unittest.TestCase):
    def test_uber_timer(self):
        medic = FFMedic()
        for _ in range(100):
            medic.think_heal()
        self.assertTrue(medic.use_special(0.0))
        self.assertFalse(medic.use_special(1.0))
        for _ in range(100):
            medic.think_heal()
        self.assertFalse(medic.use_special(5.0))
        self.assertTrue(medic.use_special(6.0))

    def test_spin_toggle(self):
        hw = FFHWGuy()
        self.assertTrue(hw.toggle_spin(0.0))
        self.assertTrue(hw.spinning)
        self.assertFalse(hw.toggle_spin(0.5))
        self.assertTrue(hw.toggle_spin(0.8))
        self.assertFalse(hw.spinning)

if __name__ == '__main__':
    unittest.main()
