#pragma once
#include "world/tile.h"
#include "core/camera.h"
#include <vector>
#include <SDL.h>

class Map {
public:
    static constexpr int TILE_SIZE = 64;

    void generate_test_city();

    void render(SDL_Renderer* renderer, const Camera& camera) const;

    int width() const { return width_; }
    int height() const { return height_; }
    int pixel_width() const { return width_ * TILE_SIZE; }
    int pixel_height() const { return height_ * TILE_SIZE; }

    TileType get_tile(int tx, int ty) const;
    TileInfo get_tile_info_at(int tx, int ty) const;
    bool is_solid(float world_x, float world_y) const;
    bool is_solid_tile(int tx, int ty) const;

    // Get tile coordinates from world position
    int world_to_tile_x(float wx) const { return static_cast<int>(wx) / TILE_SIZE; }
    int world_to_tile_y(float wy) const { return static_cast<int>(wy) / TILE_SIZE; }

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<TileType> tiles_;

    void set_tile(int x, int y, TileType type);
    void fill_rect(int x, int y, int w, int h, TileType type);
    void make_road_h(int y, int x1, int x2);
    void make_road_v(int x, int y1, int y2);
    void make_building(int x, int y, int w, int h);

    void render_building_sides(SDL_Renderer* renderer, const Camera& camera) const;
};
