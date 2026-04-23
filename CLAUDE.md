# Project Rules

## Build

- **Do NOT run cmake or build commands.** The user builds manually on Windows using `build_windows.bat`. Only write/edit source files.
- Target: Windows 10/11 with MinGW-w64 + CMake
- SDL2 libs are in `libs/SDL2_win/` (MinGW versions)

## Tech Stack

- C++17, SDL2, SDL2_image, SDL2_mixer, SDL2_ttf
- Build system: CMake with MinGW Makefiles generator
- Data format: JSON (nlohmann/json when needed)

## Code Style

- Headers use `#include <SDL.h>` (not `<SDL2/SDL.h>`)
- Internal includes use path from `src/`: e.g. `#include "core/game.h"`
- Colored rectangles for all visuals (no sprite assets yet)
