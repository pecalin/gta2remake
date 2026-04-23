#include "entities/projectile.h"
#include "core/camera.h"
#include <cmath>

Projectile::Projectile(WeaponType type, Vec2 pos, Vec2 dir, Entity* owner)
    : type_(type), owner_(owner) {
    WeaponInfo info = get_weapon_info(type);
    damage_ = info.damage;
    max_range_ = info.range;
    explosive_ = info.explosive;
    explosion_radius_ = info.explosion_radius;
    color_ = info.color;

    pos_ = pos;
    vel_ = dir.normalized() * info.speed;
    angle_ = std::atan2(dir.y, dir.x);

    width_ = 4.0f;
    height_ = 4.0f;

    if (type == WeaponType::ROCKET) {
        width_ = 6.0f;
        height_ = 6.0f;
    }

    lifetime_ = max_range_ / std::max(info.speed, 1.0f);
    if (lifetime_ > 5.0f) lifetime_ = 5.0f;
}

void Projectile::update(float dt) {
    if (!active_) return;

    pos_ += vel_ * dt;
    distance_traveled_ += vel_.length() * dt;
    lifetime_ -= dt;

    // Gravity for grenades/molotov (simulated as speed decrease)
    if (type_ == WeaponType::GRENADE || type_ == WeaponType::MOLOTOV) {
        vel_ *= (1.0f - 1.5f * dt);  // slow down
        if (vel_.length() < 20.0f) {
            lifetime_ = 0.0f; // detonate when stopped
        }
    }

    if (distance_traveled_ >= max_range_ || lifetime_ <= 0.0f) {
        active_ = false;
    }
}

void Projectile::render(SDL_Renderer* renderer, const Camera& camera) const {
    if (!active_) return;

    Vec2 screen = camera.world_to_screen(pos_);

    // Trail
    Vec2 trail_end = pos_ - vel_.normalized() * 8.0f;
    Vec2 trail_screen = camera.world_to_screen(trail_end);
    SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b, 150);
    SDL_RenderDrawLine(renderer,
        static_cast<int>(trail_screen.x), static_cast<int>(trail_screen.y),
        static_cast<int>(screen.x), static_cast<int>(screen.y));

    // Projectile body
    SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b, 255);
    SDL_Rect dst = {
        static_cast<int>(screen.x - width_ * 0.5f),
        static_cast<int>(screen.y - height_ * 0.5f),
        static_cast<int>(width_),
        static_cast<int>(height_)
    };
    SDL_RenderFillRect(renderer, &dst);
}
