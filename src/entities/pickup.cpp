#include "entities/pickup.h"
#include "core/camera.h"
#include <cmath>

Pickup::Pickup(PickupType type, Vec2 pos) : type_(type) {
    pos_ = pos;
    width_ = 14.0f;
    height_ = 14.0f;

    PickupInfo info = get_pickup_info(type);
    color_ = info.color;
    respawn_time_ = info.respawn_time;
}

void Pickup::update(float dt) {
    bob_timer_ += dt * 3.0f;

    if (collected_) {
        respawn_timer_ -= dt;
        if (respawn_timer_ <= 0.0f) {
            collected_ = false;
        }
    }
}

void Pickup::render(SDL_Renderer* renderer, const Camera& camera) const {
    if (collected_) return;

    Vec2 screen = camera.world_to_screen(pos_);

    // Bobbing animation
    float bob = std::sin(bob_timer_) * 2.0f;

    // Glow effect (pulsing outer square)
    float pulse = (std::sin(bob_timer_ * 1.5f) + 1.0f) * 0.5f;
    int glow_size = static_cast<int>(width_ + 4 + pulse * 4);
    SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b, static_cast<Uint8>(60 + pulse * 60));
    SDL_Rect glow = {
        static_cast<int>(screen.x - glow_size * 0.5f),
        static_cast<int>(screen.y - glow_size * 0.5f + bob),
        glow_size, glow_size
    };
    SDL_RenderFillRect(renderer, &glow);

    // Main pickup square
    SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b, 255);
    SDL_Rect dst = {
        static_cast<int>(screen.x - width_ * 0.5f),
        static_cast<int>(screen.y - height_ * 0.5f + bob),
        static_cast<int>(width_),
        static_cast<int>(height_)
    };
    SDL_RenderFillRect(renderer, &dst);

    // Inner detail (darker center)
    SDL_SetRenderDrawColor(renderer,
        static_cast<Uint8>(color_.r * 0.5f),
        static_cast<Uint8>(color_.g * 0.5f),
        static_cast<Uint8>(color_.b * 0.5f), 255);
    SDL_Rect inner = {dst.x + 3, dst.y + 3, dst.w - 6, dst.h - 6};
    SDL_RenderFillRect(renderer, &inner);
}

void Pickup::collect() {
    collected_ = true;
    respawn_timer_ = respawn_time_;
}
