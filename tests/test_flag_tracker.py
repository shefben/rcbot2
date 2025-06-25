import unittest
from ff_flag_tracker import FFFlagTracker
from ffhelpers import FF_TEAM_BLUE

class FlagTrackerTests(unittest.TestCase):
    def test_pickup_and_capture(self):
        tr = FFFlagTracker()
        tr.flag_picked_up(FF_TEAM_BLUE, 1)
        self.assertFalse(tr.get_flag(FF_TEAM_BLUE).at_base)
        self.assertTrue(tr.get_flag(FF_TEAM_BLUE).carried)
        tr.flag_captured(FF_TEAM_BLUE)
        self.assertTrue(tr.get_flag(FF_TEAM_BLUE).at_base)
        self.assertFalse(tr.get_flag(FF_TEAM_BLUE).carried)

if __name__ == '__main__':
    unittest.main()
