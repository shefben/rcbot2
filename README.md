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

1. [Install MetaMod:Source][].
2. Download or build the RCBot2 package.
3. Extract the package into your game directory, similar to the process of installing MM:S.
4. Start the server.
5. To verify that the installation was successful, type `rcbotd` in your server console or RCON.
You should see multiple lines starting with "[RCBot]".

Things like the waypointing guide, hookinfo updater, and waypoints themselves are currently not
available here.  You can download those from the [official release thread][].  Waypoints are
also available at [this page][waypoints].

[Install MetaMod:Source]: https://wiki.alliedmods.net/Installing_Metamod:Source
[official release thread]: http://rcbot.bots-united.com/forums/index.php?showtopic=1994
[waypoints]: http://rcbot.bots-united.com/waypoints.php

## Building

### Cloning from source

RCBot2's repo history had all sorts of build artifacts / binaries at various points in time, so
pulling the repository down normally takes an unusually long while.  I'd highly recommend
passing in `--depth 1` or a few to avoid retrieving the files that were removed since then.

### Compiling on Windows / Linux

1. [Install the prerequisites for building SourceMod for your OS.][Building SourceMod]
2. Create a `build/` subdirectory, then run `configure.py`.
	- The project currently assumes GCC 5.4.0 (Ubuntu 16.04 LTS) on Linux, and MSVC version
	1900 (VC++2014.3 v14.00 last I checked).  Other compiler toolchains are not guaranteed to
	work at this time.
	- I use the following options:
	`python ../configure.py -s tf2 --mms_path ${MMS_PATH} --hl2sdk-root ${HL2SDK_ROOT}`
3. Run `ambuild`.  MetaMod:Source plugin is built and the base install files will be available
in `build/package`.

[Building SourceMod]: https://wiki.alliedmods.net/Building_SourceMod
