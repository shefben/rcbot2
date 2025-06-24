import unittest
import ffhelpers as ff

class RandomChoiceTests(unittest.TestCase):
    def test_random_team(self):
        for _ in range(20):
            team = ff.random_team()
            self.assertIn(team, ff.FF_TEAMS)

    def test_random_class(self):
        for _ in range(20):
            cls = ff.random_class()
            self.assertGreaterEqual(cls, ff.FF_CLASS_SCOUT)
            self.assertLessEqual(cls, ff.FF_CLASS_CIVILIAN)

if __name__ == '__main__':
    unittest.main()
