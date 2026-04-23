#pragma once
#include <SDL.h>

enum class TileType : uint8_t {
    ROAD,
    SIDEWALK,
    GRASS,
    BUILDING,
    WATER,
    BUILDING_ROOF,  // rendered on top
    COUNT
};

struct TileInfo {
    TileType type;
    bool solid;
    int height;  // 0-3 for pseudo-3D buildings
    SDL_Color color;
};

// Default tile properties
inline TileInfo get_tile_info(TileType type) {
    switch (type) {
        case TileType::ROAD:
            return {type, false, 0, {80, 80, 80, 255}};       // dark gray
        case TileType::SIDEWALK:
            return {type, false, 0, {160, 160, 150, 255}};    // light gray
        case TileType::GRASS:
            return {type, false, 0, {60, 130, 50, 255}};      // green
        case TileType::BUILDING:
            return {type, true, 2, {140, 100, 70, 255}};      // brown
        case TileType::WATER:
            return {type, true, 0, {40, 80, 180, 255}};       // blue
        case TileType::BUILDING_ROOF:
            return {type, true, 3, {120, 90, 60, 255}};       // darker brown
        default:
            return {TileType::GRASS, false, 0, {60, 130, 50, 255}};
    }
}
