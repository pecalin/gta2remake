#pragma once
#include "util/math_utils.h"
#include <vector>
#include <cstdint>

class Entity;

class CollisionGrid {
public:
    static constexpr int CELL_SIZE = 128;

    void init(int world_w, int world_h);
    void clear();

    void insert(Entity* entity);
    void get_nearby(const Vec2& pos, float radius, std::vector<Entity*>& out) const;

private:
    int cols_ = 0;
    int rows_ = 0;
    std::vector<std::vector<Entity*>> cells_;

    int cell_index(int cx, int cy) const {
        if (cx < 0 || cx >= cols_ || cy < 0 || cy >= rows_) return -1;
        return cy * cols_ + cx;
    }
};
