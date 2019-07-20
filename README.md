# RCBot2 for Windows and Linux (TF2, HL2:DM, DOD:S)

## Information

This is a fork of [the official RCBot2 plugin][rcbot2] written by Cheeseh.

This repository was spun off of the source repository to use the same build tooling as
MetaMod:Source and SourceMod do, removing the need for Valve's cross platform make conversion
tool and keeping copies of Source SDK files.

The [bots-united.com discord][] and [forums][bots-united forums] are the places to ask for
general RCBot2 support. I'm not present in either of those; file an issue on this repository if
you need support for this particular project. 

[rcbot2]: http://rcbot.bots-united.com/
[bots-united.com discord]: https://discord.gg/BbxR5wY
[bots-united forums]: http://rcbot.bots-united.com/forums/index.php?showforum=18

## Installation

Proper install instructions for this fork coming soon.

## Building

### Windows

Currently unsupported.  It should be the same as the instructions for building on Linux, except
you will need to modify the `AMBuildScript` to support MSVC
([please send in a pull request to the MM:S project if you do!][mms-repo]).

[mms-repo]: https://github.com/alliedmodders/metamod-source

### Linux

1. [Install the prerequisites for building SourceMod][Building SourceMod]
2. Create a `build/` subdirectory, then run `configure.py`.
	* The project currently assumes GCC 5.4.0 (Ubuntu 16.04 LTS)
	* `python ../configure.py -s tf2 --mms_path ${MMS_PATH} --hl2sdk-root ${HL2SDK_ROOT}`
3. Extension is built as `build/RCBot2Meta_i486/RCBot2Meta_i486.so`.
	* This may change in the future to support building against different SDKs.

[Building SourceMod]: https://wiki.alliedmods.net/Building_SourceMod
