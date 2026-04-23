#pragma once
#include "util/math_utils.h"

// Collision detection utilities
namespace Collision {
    // Check if two AABBs overlap
    inline bool aabb_overlap(const Rect& a, const Rect& b) {
        return a.overlaps(b);
    }

    // Get overlap resolution vector (push a out of b)
    inline Vec2 aabb_resolve(const Rect& a, const Rect& b) {
        float dx_right = (b.x + b.w) - a.x;
        float dx_left  = (a.x + a.w) - b.x;
        float dy_down  = (b.y + b.h) - a.y;
        float dy_up    = (a.y + a.h) - b.y;

        float min_dx = (dx_right < dx_left) ? dx_right : -dx_left;
        float min_dy = (dy_down < dy_up) ? dy_down : -dy_up;

        if (std::abs(min_dx) < std::abs(min_dy)) {
            return {min_dx, 0};
        } else {
            return {0, min_dy};
        }
    }

    // Point in circle
    inline bool point_in_circle(Vec2 point, Vec2 center, float radius) {
        return (point - center).length_sq() <= radius * radius;
    }

    // Circle vs circle
    inline bool circle_overlap(Vec2 c1, float r1, Vec2 c2, float r2) {
        float dist_sq = (c1 - c2).length_sq();
        float sum_r = r1 + r2;
        return dist_sq <= sum_r * sum_r;
    }
}
