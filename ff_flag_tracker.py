class FlagState:
    def __init__(self):
        self.at_base = True
        self.carried = False
        self.dropped = False
        self.carrier = 0

class FFFlagTracker:
    def __init__(self):
        self.flags = {2: FlagState(), 3: FlagState(), 4: FlagState(), 5: FlagState()}

    def flag_picked_up(self, team, userid):
        f = self.flags[team]
        f.at_base = False
        f.carried = True
        f.dropped = False
        f.carrier = userid

    def flag_returned(self, team):
        f = self.flags[team]
        f.at_base = True
        f.carried = False
        f.dropped = False
        f.carrier = 0

    def flag_captured(self, team):
        self.flag_returned(team)

    def get_flag(self, team):
        return self.flags[team]
