#pragma once
#include "util/math_utils.h"
#include <SDL.h>

class Camera;

class Entity {
public:
    virtual ~Entity() = default;

    virtual void update(float dt) {}
    virtual void render(SDL_Renderer* renderer, const Camera& camera) const;

    Vec2 position() const { return pos_; }
    void set_position(Vec2 p) { pos_ = p; }

    Vec2 velocity() const { return vel_; }
    void set_velocity(Vec2 v) { vel_ = v; }

    float angle() const { return angle_; }
    void set_angle(float a) { angle_ = a; }

    float width() const { return width_; }
    float height() const { return height_; }
    void set_size(float w, float h) { width_ = w; height_ = h; }

    SDL_Color color() const { return color_; }
    void set_color(SDL_Color c) { color_ = c; }

    bool active() const { return active_; }
    void set_active(bool a) { active_ = a; }

    Rect bounding_box() const {
        return {pos_.x - width_ * 0.5f, pos_.y - height_ * 0.5f, width_, height_};
    }

protected:
    Vec2 pos_ = {0, 0};
    Vec2 vel_ = {0, 0};
    float angle_ = 0.0f;
    float width_ = 16.0f;
    float height_ = 16.0f;
    SDL_Color color_ = {255, 255, 255, 255};
    bool active_ = true;
};
