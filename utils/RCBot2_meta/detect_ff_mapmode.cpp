#include "bot_ff.h"
#include <lua.hpp>
#include <algorithm>

FFMapMode DetectFFMapMode(const std::string &mapname, lua_State *L)
{
    std::string name = mapname;
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if (name.find("hunted") != std::string::npos)
        return FF_MODE_VIP;
    if (name.find("dustbowl") != std::string::npos || name.find("_ad") != std::string::npos)
        return FF_MODE_AD;
    if (name.find("cz2") != std::string::npos)
        return FF_MODE_TC;
    if (name.find("invade") != std::string::npos)
        return FF_MODE_INVADE;
    return FF_MODE_CTF;
}
