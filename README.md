# ely-arcade-sdk

Shared code for the Ely arcade machine. Used by the arcade menu
([ely-arcade-platform](https://github.com/zhuberty/ely-arcade-platform)) and by
every game (e.g. `game-thick-cube`, `edibles-raylib`).

## Contents

| Path | What |
|---|---|
| `include/arcade_input.h`, `src/arcade_input.cpp` | Action-based input (`arcade::IsActionPressed(Player, Action)`) covering keyboard, arcade joystick encoders that emulate a keyboard, and gamepads. |
| `include/resource_dir.h` | `SearchAndSetResourceDir()` helper (from raylib-extras). |
| `include/rlights.h` | raylib lighting helper (define `RLIGHTS_IMPLEMENTATION` in exactly one .cpp). |
| `include/earcut.hpp` | mapbox earcut polygon triangulation. |
| `premake/arcade_sdk.lua` | Shared premake helpers (`arcade.raylib_project`, `arcade.sdk_project`, `arcade.app_project`). |
| `tools/premake/` | premake5 binaries for Windows / Linux / macOS. |

Planned (not yet implemented): user accounts, save data / storage, API access.

## Using the SDK from a project

Add this repo as a git submodule (conventionally at `sdk/`), then in the
project's `build/premake5.lua`:

```lua
dofile("../sdk/premake/arcade_sdk.lua")
arcade.prepare_dirs()                       -- creates build_files/, external/, downloads raylib

arcade.workspace("my-game")                 -- configs, platforms, output dir

arcade.raylib_project()
arcade.sdk_project("../sdk")
arcade.app_project("my-game", "../src", "../sdk")
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

if (arcade::IsActionPressed(arcade::Player::One, arcade::Action::Up)) { ... }
if (arcade::IsActionPressed(arcade::Player::Any, arcade::Action::Confirm)) { ... }
```

See `include/arcade_input.h` for the binding table.
