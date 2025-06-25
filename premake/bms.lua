project "RCBot2-BMS"
    language "C++"
    kind "SharedLib"
    cppdialect "C++17"
    targetname "rcbot2.2.bms"
    defines { "SOURCE_ENGINE=11" }

    local Dir_SDK = "hl2sdk-bms"

	includedirs { 
        path.join(Path_HL2SDKROOT, Dir_SDK, "public"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "engine"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "mathlib"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "vstdlib"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "tier0"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "tier1"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "toolframework"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "public", "game", "server"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "game", "shared"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "game", "server"),
        path.join(Path_HL2SDKROOT, Dir_SDK, "common"),
        path.join(Path_SM, "public"),
        path.join(Path_SM, "public", "extensions"),
        path.join(Path_SM, "sourcepawn", "include"),
        path.join(Path_SM, "public", "amtl", "amtl"),
        path.join(Path_SM, "public", "amtl"),
        path.join(Path_MMS, "core"),
        path.join(Path_MMS, "core", "sourcehook"),
        "../",
        "../rcbot",
        "../utils/RCBot2_meta",
        "../versioning/",
        "../build/includes/",
        "../sm_ext/",
	}
	files {
        "../loader/**.cpp",
        "../rcbot/**.h",
        "../rcbot/**.cpp",
        "../rcbot_subcmds/**.h",
        "../rcbot_subcmds/**.cpp",
        "../sm_ext/**.h",
        "../sm_ext/**.cpp",
        "../utils/RCBot2_meta/**.h",
        "../utils/RCBot2_meta/**.cpp",
        "../versioning/**.h",
        "../versioning/**.cpp",
	}

    filter { "system:Linux" }
        defines { "NO_HOOK_MALLOC", "NO_MALLOC_OVERRIDE" }

    filter {}
        defines { "RCBOT_MAXPLAYERS=65", "SM_EXT" }
