cmake_minimum_required(VERSION 3.10)
project(SimpleLuaPlugin)

# LuaJIT or standard Lua
# For this example, let's assume Lua headers are in "lua/" and library is "liblua.a" or "lua.dll"
# In a real scenario, you'd use find_package(Lua) or specific paths
# For this conceptual setup, we'll just define where we expect them.
include_directories(lua) # Assuming lua headers are in a 'lua' subdirectory
# link_directories(lua/lib) # Assuming lua library is in 'lua/lib'

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add source file for the plugin
add_library(SimpleLuaPlugin SHARED plugin_main.cpp LuaBridge.cpp)

# Define platform-specific extensions and prefixes
if(WIN32)
    set_target_properties(SimpleLuaPlugin PROPERTIES PREFIX "" SUFFIX ".dll")
    # Link against lua.dll (or equivalent)
    # target_link_libraries(SimpleLuaPlugin lua54) # Example for Lua 5.4
elseif(APPLE)
    set_target_properties(SimpleLuaPlugin PROPERTIES PREFIX "lib" SUFFIX ".dylib")
    # target_link_libraries(SimpleLuaPlugin lua)
else()
    set_target_properties(SimpleLuaPlugin PROPERTIES PREFIX "lib" SUFFIX ".so")
    # target_link_libraries(SimpleLuaPlugin lua)
endif()

# For Source engine, we might need to specify specific compiler options
# e.g., -m32 for 32-bit builds if the engine is 32-bit
# For now, this is a basic setup.

# Conceptual: In a real environment, you'd link against Source SDK libraries
# target_link_libraries(SimpleLuaPlugin tier1 steam_api interfaces_valve)

install(TARGETS SimpleLuaPlugin DESTINATION game/addons) # Example install path
