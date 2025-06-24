#include "ff/classes/ff_class_soldier.h"
#include "ff/classes/ff_class_pyro.h"
#include "ff/classes/ff_class_demoman.h"
#include <assert.h>

float g_ff_test_time = 0.0f;

int main()
{
    CFFClassSoldier soldier(nullptr);
    soldier.OnSpawn();
    assert(soldier.GetAmmo(0) == 4);
    assert(soldier.UseSpecial());
    assert(soldier.GetAmmo(0) == 3);
    g_ff_test_time = 5.0f;
    assert(!soldier.UseSpecial());
    g_ff_test_time = 10.0f;
    assert(soldier.UseSpecial());
    
    g_ff_test_time = 0.0f;
    CFFClassPyro pyro(nullptr);
    pyro.OnSpawn();
    assert(pyro.GetAmmo(1) == 100);
    assert(pyro.UseSpecial());
    assert(pyro.GetAmmo(1) == 90);
    
    g_ff_test_time = 0.0f;
    CFFClassDemoman demo(nullptr);
    demo.OnSpawn();
    assert(demo.GetAmmo(2) == 4);
    assert(demo.UseSpecial());
    assert(demo.GetAmmo(2) == 3);

    return 0;
}
