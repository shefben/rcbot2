import unittest
import ffhelpers as ff

class ParseFFLoadoutTests(unittest.TestCase):
    def test_valid_loadout(self):
        info = {"weapon": 6, "team": ff.FF_TEAM_BLUE}
        result = ff.parse_ff_class_loadout(info)
        self.assertEqual(result["weapon"], "shotgun")
        self.assertEqual(result["team"], ff.FF_TEAM_BLUE)

    def test_invalid_team(self):
        info = {"weapon": 2, "team": 99}
        result = ff.parse_ff_class_loadout(info)
        self.assertEqual(result["team"], ff.FF_TEAM_BLUE)

    def test_yellow_team(self):
        info = {"weapon": 8, "team": ff.FF_TEAM_YELLOW}
        result = ff.parse_ff_class_loadout(info)
        self.assertEqual(result["team"], ff.FF_TEAM_YELLOW)
        self.assertEqual(result["weapon"], "nailgun")

if __name__ == '__main__':
    unittest.main()
