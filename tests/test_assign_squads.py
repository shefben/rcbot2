import unittest
from ffhelpers import assign_squads_ff

class SquadAssignTests(unittest.TestCase):
    def test_basic_split(self):
        bots = ['b1','b2','b3','b4','b5']
        offense, defense = assign_squads_ff(bots)
        self.assertEqual(offense, ['b1','b2','b3'])
        self.assertEqual(defense, ['b4','b5'])

if __name__ == '__main__':
    unittest.main()
