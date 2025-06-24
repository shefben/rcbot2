import unittest
from ffhelpers import SpecialAbilityScheduler, BOT_TASK_ATTACK

class FakeClass:
    def __init__(self):
        self.called = 0
    def use_special(self, now: float):
        self.called += 1
        return True
    def get_ideal_task(self):
        return BOT_TASK_ATTACK

class SchedulerTests(unittest.TestCase):
    def test_schedule(self):
        sched = SpecialAbilityScheduler()
        pc = FakeClass()
        sched.register(pc, 5.0)
        sched.bot_frame(0.0)
        self.assertEqual(pc.called, 1)
        sched.bot_frame(1.0)
        self.assertEqual(pc.called, 1)
        sched.bot_frame(5.0)
        self.assertEqual(pc.called, 2)
        sched.pause_specials(pc, 5.0, 5.0)
        sched.bot_frame(9.0)
        self.assertEqual(pc.called, 2)
        sched.bot_frame(10.0)
        self.assertEqual(pc.called, 3)

if __name__ == '__main__':
    unittest.main()
