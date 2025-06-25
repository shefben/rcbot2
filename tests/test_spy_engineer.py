import unittest
from ffhelpers import FFSpy, BuildingManager, FFEngineer

class SpyEngineerTests(unittest.TestCase):
    def test_cloak_drain(self):
        spy = FFSpy()
        self.assertTrue(spy.use_special(0.0))  # cloak
        for i in range(5):
            spy.think(i+1.0)
        self.assertAlmostEqual(spy.cloak, 80.0)
        for i in range(20):
            spy.think(6.0 + i)
        self.assertFalse(spy.cloaked)
        self.assertEqual(spy.cloak, 0)

    def test_sentry_upgrade(self):
        mgr = BuildingManager()
        eng = FFEngineer(mgr)
        mgr.build_sentry()
        self.assertEqual(mgr.sentry_level, 1)
        mgr.build_sentry()
        self.assertEqual(mgr.sentry_level, 2)
        mgr.build_sentry()
        self.assertEqual(mgr.sentry_level, 3)
        mgr.build_sentry()
        self.assertEqual(mgr.sentry_level, 3)

if __name__ == '__main__':
    unittest.main()
