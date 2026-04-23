#include "core/camera.h"
#include <cstdlib>

void Camera::update(float dt) {
    // Smooth follow
    pos_.x = lerp(pos_.x, target_.x, follow_speed_ * dt);
    pos_.y = lerp(pos_.y, target_.y, follow_speed_ * dt);

    // Screen shake decay
    if (shake_intensity_ > 0.1f) {
        shake_offset_.x = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 2.0f * shake_intensity_;
        shake_offset_.y = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 2.0f * shake_intensity_;
        shake_intensity_ *= 0.9f;
    } else {
        shake_intensity_ = 0.0f;
        shake_offset_ = {0, 0};
    }
}

Vec2 Camera::world_to_screen(Vec2 world) const {
    return {
        world.x - (pos_.x + shake_offset_.x) + vp_w_ * 0.5f,
        world.y - (pos_.y + shake_offset_.y) + vp_h_ * 0.5f
    };
}

Vec2 Camera::screen_to_world(Vec2 screen) const {
    return {
        screen.x + (pos_.x + shake_offset_.x) - vp_w_ * 0.5f,
        screen.y + (pos_.y + shake_offset_.y) - vp_h_ * 0.5f
    };
}
