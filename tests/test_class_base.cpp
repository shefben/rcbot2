#include "ff_class_base.h"
#include <assert.h>

class FakeScout : public CFFPlayerClass {
public:
    FakeScout() : CFFPlayerClass(nullptr) {}
    void OnSpawn() override {
        GiveWeapon(1); // assume weapon id 1
        GiveAmmo(0, 30);
        GiveClip(1, 5);
    }
    bool Think() override { return false; }
    bool UseSpecial() override { return false; }
    EBotTask GetIdealTask() override { return BOT_TASK_NONE; }
};

int main() {
    FakeScout s;
    s.OnSpawn();
    assert(!s.GetWeapons().empty() && s.GetWeapons()[0] == 1);
    assert(s.GetAmmo(0) == 30);
    assert(s.GetClip(1) == 5);
    return 0;
}
