#include "world/map.h"
#include <algorithm>

TileType Map::get_tile(int tx, int ty) const {
    if (tx < 0 || tx >= width_ || ty < 0 || ty >= height_)
        return TileType::WATER;
    return tiles_[ty * width_ + tx];
}

TileInfo Map::get_tile_info_at(int tx, int ty) const {
    return get_tile_info(get_tile(tx, ty));
}

bool Map::is_solid(float world_x, float world_y) const {
    int tx = static_cast<int>(world_x) / TILE_SIZE;
    int ty = static_cast<int>(world_y) / TILE_SIZE;
    return is_solid_tile(tx, ty);
}

bool Map::is_solid_tile(int tx, int ty) const {
    return get_tile_info(get_tile(tx, ty)).solid;
}

void Map::set_tile(int x, int y, TileType type) {
    if (x >= 0 && x < width_ && y >= 0 && y < height_)
        tiles_[y * width_ + x] = type;
}

void Map::fill_rect(int x, int y, int w, int h, TileType type) {
    for (int dy = 0; dy < h; dy++)
        for (int dx = 0; dx < w; dx++)
            set_tile(x + dx, y + dy, type);
}

void Map::make_road_h(int y, int x1, int x2) {
    for (int x = x1; x <= x2; x++) {
        set_tile(x, y, TileType::ROAD);
        set_tile(x, y + 1, TileType::ROAD);
    }
}

void Map::make_road_v(int x, int y1, int y2) {
    for (int y = y1; y <= y2; y++) {
        set_tile(x, y, TileType::ROAD);
        set_tile(x + 1, y, TileType::ROAD);
    }
}

void Map::make_building(int x, int y, int w, int h) {
    // Sidewalk border
    fill_rect(x - 1, y - 1, w + 2, h + 2, TileType::SIDEWALK);
    // Building
    fill_rect(x, y, w, h, TileType::BUILDING);
}

void Map::generate_test_city() {
    width_ = 64;
    height_ = 64;
    tiles_.resize(width_ * height_, TileType::GRASS);

    // Create a grid of roads
    // Horizontal roads at rows 6, 16, 26, 36, 46, 56
    for (int road : {6, 16, 26, 36, 46, 56}) {
        if (road + 1 < height_)
            make_road_h(road, 2, width_ - 3);
    }

    // Vertical roads at columns 6, 16, 26, 36, 46, 56
    for (int road : {6, 16, 26, 36, 46, 56}) {
        if (road + 1 < width_)
            make_road_v(road, 2, height_ - 3);
    }

    // Fill city blocks with buildings of various sizes
    // Block positions between roads (road pairs at 6-7, 16-17, etc.)
    struct Block { int x1, y1, x2, y2; };
    Block blocks[] = {
        {8, 8, 15, 15},
        {18, 8, 25, 15},
        {28, 8, 35, 15},
        {38, 8, 45, 15},
        {48, 8, 55, 15},

        {8, 18, 15, 25},
        {18, 18, 25, 25},
        {28, 18, 35, 25},
        {38, 18, 45, 25},
        {48, 18, 55, 25},

        {8, 28, 15, 35},
        {18, 28, 25, 35},
        {28, 28, 35, 35},
        {38, 28, 45, 35},
        {48, 28, 55, 35},

        {8, 38, 15, 45},
        {18, 38, 25, 45},
        {28, 38, 35, 45},
        {38, 38, 45, 45},
        {48, 38, 55, 45},

        {8, 48, 15, 55},
        {18, 48, 25, 55},
        {28, 48, 35, 55},
        {38, 48, 45, 55},
        {48, 48, 55, 55},
    };

    for (auto& b : blocks) {
        // Add sidewalk around the block
        fill_rect(b.x1, b.y1, b.x2 - b.x1 + 1, b.y2 - b.y1 + 1, TileType::SIDEWALK);

        // Place 1-3 buildings per block with some variation
        int bw = b.x2 - b.x1 - 1;
        int bh = b.y2 - b.y1 - 1;

        if (bw >= 4 && bh >= 4) {
            // Split some blocks into multiple buildings
            int cx = b.x1 + 1;
            int cy = b.y1 + 1;

            // Use block position as pseudo-random seed for variety
            int seed = b.x1 * 7 + b.y1 * 13;

            if (seed % 3 == 0) {
                // One large building
                fill_rect(cx, cy, bw, bh, TileType::BUILDING);
            } else if (seed % 3 == 1) {
                // Two buildings side by side
                int half = bw / 2 - 1;
                fill_rect(cx, cy, half, bh, TileType::BUILDING);
                fill_rect(cx + half + 1, cy, bw - half - 1, bh, TileType::BUILDING);
            } else {
                // L-shaped or smaller buildings
                fill_rect(cx, cy, bw, bh / 2, TileType::BUILDING);
                fill_rect(cx, cy + bh / 2 + 1, bw / 2, bh - bh / 2 - 1, TileType::BUILDING);
            }
        }
    }

    // Add a park area (open grass) in one block
    fill_rect(29, 29, 6, 6, TileType::GRASS);

    // Add water (river/lake) along the edges
    fill_rect(0, 0, width_, 2, TileType::WATER);
    fill_rect(0, height_ - 2, width_, 2, TileType::WATER);
    fill_rect(0, 0, 2, height_, TileType::WATER);
    fill_rect(width_ - 2, 0, 2, height_, TileType::WATER);
}

void Map::render(SDL_Renderer* renderer, const Camera& camera) const {
    // Determine visible tile range
    int start_tx = std::max(0, static_cast<int>(camera.left()) / TILE_SIZE - 1);
    int start_ty = std::max(0, static_cast<int>(camera.top()) / TILE_SIZE - 1);
    int end_tx = std::min(width_, static_cast<int>(camera.right()) / TILE_SIZE + 2);
    int end_ty = std::min(height_, static_cast<int>(camera.bottom()) / TILE_SIZE + 2);

    // Render ground tiles
    for (int ty = start_ty; ty < end_ty; ty++) {
        for (int tx = start_tx; tx < end_tx; tx++) {
            TileInfo info = get_tile_info_at(tx, ty);
            Vec2 screen = camera.world_to_screen({
                static_cast<float>(tx * TILE_SIZE),
                static_cast<float>(ty * TILE_SIZE)
            });

            SDL_Rect dst = {
                static_cast<int>(screen.x),
                static_cast<int>(screen.y),
                TILE_SIZE, TILE_SIZE
            };

            SDL_SetRenderDrawColor(renderer, info.color.r, info.color.g, info.color.b, info.color.a);
            SDL_RenderFillRect(renderer, &dst);

            // Draw subtle grid lines for roads
            if (info.type == TileType::ROAD) {
                SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
                SDL_RenderDrawRect(renderer, &dst);

                // Center lane marking
                SDL_SetRenderDrawColor(renderer, 200, 200, 50, 100);
                SDL_Rect line = {dst.x + TILE_SIZE / 2 - 1, dst.y, 2, TILE_SIZE};
                SDL_RenderFillRect(renderer, &line);
            }
        }
    }

    // Render building pseudo-3D sides
    render_building_sides(renderer, camera);
}

void Map::render_building_sides(SDL_Renderer* renderer, const Camera& camera) const {
    int start_tx = std::max(0, static_cast<int>(camera.left()) / TILE_SIZE - 1);
    int start_ty = std::max(0, static_cast<int>(camera.top()) / TILE_SIZE - 1);
    int end_tx = std::min(width_, static_cast<int>(camera.right()) / TILE_SIZE + 2);
    int end_ty = std::min(height_, static_cast<int>(camera.bottom()) / TILE_SIZE + 2);

    constexpr int SIDE_HEIGHT = 16; // pixels per height level

    for (int ty = start_ty; ty < end_ty; ty++) {
        for (int tx = start_tx; tx < end_tx; tx++) {
            TileInfo info = get_tile_info_at(tx, ty);
            if (info.height <= 0) continue;

            // Check south side (tile below is shorter)
            TileInfo south = get_tile_info_at(tx, ty + 1);
            if (south.height < info.height) {
                int h_diff = info.height - south.height;
                Vec2 screen = camera.world_to_screen({
                    static_cast<float>(tx * TILE_SIZE),
                    static_cast<float>((ty + 1) * TILE_SIZE)
                });

                SDL_Rect side = {
                    static_cast<int>(screen.x),
                    static_cast<int>(screen.y) - h_diff * SIDE_HEIGHT,
                    TILE_SIZE,
                    h_diff * SIDE_HEIGHT
                };

                // Darker shade for south wall
                SDL_SetRenderDrawColor(renderer,
                    static_cast<Uint8>(info.color.r * 0.5f),
                    static_cast<Uint8>(info.color.g * 0.5f),
                    static_cast<Uint8>(info.color.b * 0.5f), 255);
                SDL_RenderFillRect(renderer, &side);
            }

            // Check east side (tile to right is shorter)
            TileInfo east = get_tile_info_at(tx + 1, ty);
            if (east.height < info.height) {
                int h_diff = info.height - east.height;
                Vec2 screen = camera.world_to_screen({
                    static_cast<float>((tx + 1) * TILE_SIZE),
                    static_cast<float>(ty * TILE_SIZE)
                });

                SDL_Rect side = {
                    static_cast<int>(screen.x),
                    static_cast<int>(screen.y) - h_diff * SIDE_HEIGHT,
                    h_diff * SIDE_HEIGHT / 2, // narrower for perspective
                    TILE_SIZE
                };

                // Slightly different shade for east wall
                SDL_SetRenderDrawColor(renderer,
                    static_cast<Uint8>(info.color.r * 0.6f),
                    static_cast<Uint8>(info.color.g * 0.6f),
                    static_cast<Uint8>(info.color.b * 0.6f), 255);
                SDL_RenderFillRect(renderer, &side);
            }
        }
    }
}
