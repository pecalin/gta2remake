# GTA2 Clone

A 2D top-down open-world action game inspired by GTA2, built from scratch in C++17 with SDL2.

![Status](https://img.shields.io/badge/status-in%20development-yellow)
![Platform](https://img.shields.io/badge/platform-Windows%2010%2F11-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B17-orange)

## Features

### Core Gameplay
- **On-foot movement** — 8-directional walking with collision against buildings and water
- **Vehicle system** — enter/exit 10 vehicle types, each with unique arcade physics ported from GTA2's original vehicle data
- **Weapons** — 6 weapon types: Pistol, Machine Gun, Rocket Launcher, Flamethrower, Molotov, Grenades
- **Combat** — shoot pedestrians, blow up vehicles (they catch fire then explode), area-of-effect explosions with screen shake

### Open World
- **Procedural city** — 64x64 tile city with roads, sidewalks, buildings (pseudo-3D walls), grass, and water borders
- **Pedestrians** — ~25 AI civilians that wander sidewalks and flee from gunfire, explosions, and speeding vehicles
- **Vehicles** — 20 parked cars scattered around the city in 10 different colors

### Police System
- **6-star wanted level** — escalating police response from foot cops to SWAT to army
- **Crime tracking** — killing pedestrians, cops, or destroying vehicles raises your wanted level
- **Police AI** — cops pursue on foot or in cars, shoot at higher wanted levels, and search your last known position
- **Escape** — stay out of sight to let your wanted level decay, or pick up a Cop Bribe to clear it instantly
- **Busted/Wasted** — get arrested or killed, lose weapons, respawn

### Vehicle Physics
Arcade-style physics with parameters ported from GTA2's original `nyc.gci` configuration:
- 3-gear transmission with speed-dependent gear shifting
- Steering that tightens at low speed and loosens at high speed
- Handbrake drifting
- Mass-based collision response between vehicles
- Skid detection

### HUD & UI
- Health bar, armor bar, wanted level stars, lives display
- Current weapon and ammo indicator
- Vehicle speed gauge, gear indicator, handbrake/skid indicators
- Minimap with player position and cop locations
- Wasted/Busted full-screen overlays

### Dev Mode (In-Game)
Pause menu (ESC) → Options → Dev Mode, with live-tunable settings:

**Toggles:**
- Invincible, No Police, Infinite Ammo, One Hit Kill

**Sliders (0.1x - 5.0x):**
- Car Acceleration, Braking, Turn Speed, Max Speed, Drift
- Player Speed, NPC Density, Traffic Density, Cop Aggression, Wanted Gain

## Controls

| Action | Key |
|--------|-----|
| Move / Steer | WASD or Arrow Keys |
| Shoot | Left Ctrl |
| Enter/Exit Vehicle | F |
| Handbrake (Drift) | Space |
| Switch Weapon | Q / E |
| Pause Menu | ESC |

**In menus:** Up/Down to navigate, Enter to select, ESC to go back.
**Dev Mode sliders:** Left/Right arrows to adjust values.

## Building

### Requirements
- **CMake** 3.16+ ([download](https://cmake.org/download/))
- **MinGW-w64** with g++ ([download](https://github.com/niXman/mingw-builds-binaries/releases))
- Both must be in your system PATH

SDL2 libraries are bundled in `libs/SDL2_win/` — no separate installation needed.

### Build & Run
```
build_windows.bat
```

This will:
1. Configure with CMake (MinGW Makefiles)
2. Build a Release executable
3. Copy required SDL2 DLLs to the output directory
4. Launch the game

The executable is at `build_win/gta2.exe`.

### Manual Build
```bash
mkdir build_win
cd build_win
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . -j%NUMBER_OF_PROCESSORS%
```

## Project Structure

```
src/
  core/       — Game loop, window, input, camera, timer
  entities/   — Player, Vehicle, Pedestrian, Projectile, Pickup, Explosion
  world/      — Tile map, collision grid, spawn manager
  physics/    — Vehicle physics engine, collision detection
  systems/    — Weapon system, wanted level, score
  ai/         — Pedestrian AI, police AI, traffic AI (WIP)
  ui/         — Pause menu with dev mode
  util/       — Vec2 math, helpers
```

## Tech Stack

- **C++17** — no external dependencies beyond SDL2
- **SDL2** — window, rendering, input, audio
- **SDL2_image / SDL2_mixer / SDL2_ttf** — linked but not yet actively used (ready for sprites, sound, fonts)
- **Custom pixel font** — built-in 5x7 bitmap font for all menu text

## Art Style

Currently uses **colored rectangles** for all visuals — no sprite assets required. Vehicles are drawn as rotated filled rectangles with headlights, tail lights, and a windshield stripe. Buildings have pseudo-3D wall faces. This is intentional for rapid gameplay iteration; pixel art sprites can be dropped in later.

## Roadmap

- [x] Phase 1: Engine foundation (movement, map, camera)
- [x] Phase 2: Vehicle system with arcade physics
- [x] Phase 3: Weapons and combat
- [x] Phase 4: Pedestrians and spawning
- [x] Phase 5: Police and wanted system
- [x] Dev Mode with live-tunable game values
- [ ] Phase 6: Mission system (data-driven JSON missions)
- [ ] Phase 7: Menus, save/load, audio, polish
- [ ] Pixel art sprites
- [ ] Traffic AI (AI-driven vehicles on roads)
- [ ] Gang system

## License

This is a personal project for educational purposes. Not affiliated with Rockstar Games.
