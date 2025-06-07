-- This is a Lua script to test the C++ bridge

-- 1. Print a message directly from Lua
print("[Lua] Hello from test_callbacks.lua!")

-- 2. Call the C++ function PrintMessageToConsole
-- This function is registered in C++ and should print to the game/server console
local messageToCpp = "This is a message from Lua, being sent to C++ PrintMessageToConsole!"
PrintMessageToConsole(messageToCpp)

print("[Lua] Called PrintMessageToConsole from Lua.")

-- 3. Call the C++ function GetControlPointData and print its results
print("[Lua] Calling GetControlPointData from Lua...")
local cpData = GetControlPointData()

if cpData then
    print("[Lua] Received data from GetControlPointData:")
    for cpID, data in pairs(cpData) do
        print("  Control Point ID: " .. tostring(cpID))
        if type(data) == "table" then
            for key, value in pairs(data) do
                print("    " .. tostring(key) .. ": " .. tostring(value))
            end
        else
            print("    Unexpected data format for CP ID " .. tostring(cpID))
        end
    end
else
    print("[Lua] GetControlPointData returned nil or no data.")
end

print("[Lua] test_callbacks.lua execution finished.")
