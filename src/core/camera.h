#pragma once
#include "util/math_utils.h"

class Camera {
public:
    void set_target(Vec2 target) { target_ = target; }
    void set_viewport_size(int w, int h) { vp_w_ = w; vp_h_ = h; }

    void update(float dt);

    // Convert world coordinates to screen coordinates
    Vec2 world_to_screen(Vec2 world) const;
    // Convert screen coordinates to world coordinates
    Vec2 screen_to_world(Vec2 screen) const;

    // Visible world bounds
    float left() const   { return pos_.x - vp_w_ * 0.5f; }
    float top() const    { return pos_.y - vp_h_ * 0.5f; }
    float right() const  { return pos_.x + vp_w_ * 0.5f; }
    float bottom() const { return pos_.y + vp_h_ * 0.5f; }

    Vec2 position() const { return pos_; }

    void add_shake(float intensity) { shake_intensity_ += intensity; }

private:
    Vec2 pos_ = {0, 0};
    Vec2 target_ = {0, 0};
    int vp_w_ = 1280;
    int vp_h_ = 720;
    float follow_speed_ = 5.0f;
    float shake_intensity_ = 0.0f;
    Vec2 shake_offset_ = {0, 0};
};
