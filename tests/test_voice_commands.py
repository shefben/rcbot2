import unittest
from ffhelpers import process_voice_command, FF_VC_INCOMING, FF_VC_NEED_DISPENSER

class VoiceCommandTests(unittest.TestCase):
    def test_known_commands(self):
        self.assertEqual(process_voice_command('Incoming!'), FF_VC_INCOMING)
        self.assertEqual(process_voice_command('Need Dispenser'), FF_VC_NEED_DISPENSER)

    def test_unknown(self):
        self.assertIsNone(process_voice_command('Hello'))

if __name__ == '__main__':
    unittest.main()
