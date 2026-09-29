# ely-arcade-sdk

Shared code for the Ely arcade machine. Used by the arcade menu
([ely-arcade-platform](https://github.com/zhuberty/ely-arcade-platform)) and by
every game (e.g. `game-thick-cube`, `edibles-raylib`).

## Contents

| Path | What |
|---|---|
| `include/arcade_input.h`, `src/arcade_input.cpp` | Action-based input (`ely::IsActionPressed(Player, Action)`) covering keyboard, arcade joystick encoders that emulate a keyboard, and gamepads. |
| `include/resource_dir.h` | `SearchAndSetResourceDir()` helper (from raylib-extras). |
| `include/rlights.h` | raylib lighting helper (define `RLIGHTS_IMPLEMENTATION` in exactly one .cpp). |
| `include/earcut.hpp` | mapbox earcut polygon triangulation. |
| `premake/ely_sdk.lua` | Shared premake helpers (`ely.raylib_project`, `ely.sdk_project`, `ely.app_project`). |
| `tools/premake/` | premake5 binaries for Windows / Linux / macOS. |

Planned (not yet implemented): user accounts, save data / storage, API access.

## Using the SDK from a project

Add this repo as a git submodule (conventionally at `sdk/`), then in the
project's `build/premake5.lua`:

```lua
dofile("../sdk/premake/ely_sdk.lua")
ely.prepare_dirs()                       -- creates build_files/, external/, downloads raylib

workspace "my-game"
    location "../"
    configurations { "Debug", "Release" }
    platforms { "x64", "x86", "ARM64" }
    -- (see ely-arcade-platform/build/premake5.lua for the full workspace block)

ely.raylib_project()
ely.sdk_project("../sdk")
ely.app_project("my-game", "../src", "../sdk")
```

Generate makefiles and build:

```
cd build
../sdk/tools/premake/premake5 gmake        # premake5.exe on Windows
cd ..
make config=release_x64
```

## Input example

```cpp
#include "arcade_input.h"

if (ely::IsActionPressed(ely::Player::One, ely::Action::Up)) { ... }
if (ely::IsActionPressed(ely::Player::Any, ely::Action::Confirm)) { ... }
```

See `include/arcade_input.h` for the binding table.
