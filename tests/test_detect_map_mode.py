import unittest
from ffhelpers import detect_ff_map_mode, FF_MODE_CTF, FF_MODE_VIP, FF_MODE_AD, FF_MODE_TC, FF_MODE_INVADE

class MapModeDetectTests(unittest.TestCase):
    def test_modes(self):
        self.assertEqual(detect_ff_map_mode('ff_2fort'), FF_MODE_CTF)
        self.assertEqual(detect_ff_map_mode('ff_hunted'), FF_MODE_VIP)
        self.assertEqual(detect_ff_map_mode('ff_dustbowl'), FF_MODE_AD)
        self.assertEqual(detect_ff_map_mode('cz2'), FF_MODE_TC)
        self.assertEqual(detect_ff_map_mode('ff_invade1'), FF_MODE_INVADE)

if __name__ == '__main__':
    unittest.main()
