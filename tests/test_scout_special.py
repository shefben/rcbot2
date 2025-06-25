import unittest
from ffhelpers import FFScout

class ScoutSpecialTests(unittest.TestCase):
    def test_use_special_cooldown(self):
        sc = FFScout()
        self.assertTrue(sc.use_special(0.0))
        self.assertTrue(sc.grenade_primed)
        self.assertEqual(sc.conc, 1)
        self.assertAlmostEqual(sc.next_special_use, 10.0)
        # should fail during cooldown
        self.assertFalse(sc.use_special(5.0))
        # after cooldown
        sc.grenade_primed = False
        self.assertTrue(sc.use_special(10.0))
        self.assertEqual(sc.conc, 0)
        self.assertTrue(sc.grenade_primed)

if __name__ == '__main__':
    unittest.main()
