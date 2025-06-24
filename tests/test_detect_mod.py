import unittest
import botconfig

class DetectModTests(unittest.TestCase):
    def test_ff_extension(self):
        self.assertEqual(botconfig.detect_mod('castle.ff'), botconfig.MOD_FF)

    def test_ff_prefix(self):
        self.assertEqual(botconfig.detect_mod('ff_bases.bsp'), botconfig.MOD_FF)

    def test_unknown(self):
        self.assertEqual(botconfig.detect_mod('cp_badlands.bsp'), botconfig.MOD_UNKNOWN)

if __name__ == '__main__':
    unittest.main()
