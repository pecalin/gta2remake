#include "world/collision_grid.h"
#include "entities/entity.h"

void CollisionGrid::init(int world_w, int world_h) {
    cols_ = (world_w + CELL_SIZE - 1) / CELL_SIZE;
    rows_ = (world_h + CELL_SIZE - 1) / CELL_SIZE;
    cells_.resize(cols_ * rows_);
}

void CollisionGrid::clear() {
    for (auto& cell : cells_) {
        cell.clear();
    }
}

void CollisionGrid::insert(Entity* entity) {
    if (!entity || !entity->active()) return;

    Rect bb = entity->bounding_box();
    int cx1 = static_cast<int>(bb.x) / CELL_SIZE;
    int cy1 = static_cast<int>(bb.y) / CELL_SIZE;
    int cx2 = static_cast<int>(bb.x + bb.w) / CELL_SIZE;
    int cy2 = static_cast<int>(bb.y + bb.h) / CELL_SIZE;

    for (int cy = cy1; cy <= cy2; cy++) {
        for (int cx = cx1; cx <= cx2; cx++) {
            int idx = cell_index(cx, cy);
            if (idx >= 0) {
                cells_[idx].push_back(entity);
            }
        }
    }
}

void CollisionGrid::get_nearby(const Vec2& pos, float radius, std::vector<Entity*>& out) const {
    int cx1 = static_cast<int>(pos.x - radius) / CELL_SIZE;
    int cy1 = static_cast<int>(pos.y - radius) / CELL_SIZE;
    int cx2 = static_cast<int>(pos.x + radius) / CELL_SIZE;
    int cy2 = static_cast<int>(pos.y + radius) / CELL_SIZE;

    for (int cy = cy1; cy <= cy2; cy++) {
        for (int cx = cx1; cx <= cx2; cx++) {
            int idx = cell_index(cx, cy);
            if (idx >= 0) {
                for (Entity* e : cells_[idx]) {
                    out.push_back(e);
                }
            }
        }
    }
}
