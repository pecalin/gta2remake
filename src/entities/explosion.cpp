#include "entities/explosion.h"
#include "core/camera.h"

Explosion::Explosion(Vec2 pos, float radius, float damage)
    : radius_(radius), damage_(damage) {
    pos_ = pos;
    width_ = radius * 2;
    height_ = radius * 2;
    timer_ = 0.6f;
    max_timer_ = 0.6f;
}

void Explosion::update(float dt) {
    timer_ -= dt;
    if (timer_ <= 0.0f) {
        active_ = false;
    }
}

void Explosion::render(SDL_Renderer* renderer, const Camera& camera) const {
    if (!active_) return;

    Vec2 screen = camera.world_to_screen(pos_);
    float progress = 1.0f - (timer_ / max_timer_);  // 0 -> 1

    // Expanding circle effect with multiple rings
    float current_radius = radius_ * (0.3f + progress * 0.7f);
    int alpha = static_cast<int>(255 * (1.0f - progress));

    // Outer ring (orange/red)
    for (float r = current_radius; r > current_radius * 0.3f; r -= 3.0f) {
        float ring_progress = r / current_radius;
        int rr = static_cast<int>(255 * ring_progress);
        int gg = static_cast<int>(200 * (1.0f - ring_progress));
        SDL_SetRenderDrawColor(renderer, rr, gg, 0,
            static_cast<Uint8>(alpha * ring_progress));

        SDL_Rect ring = {
            static_cast<int>(screen.x - r),
            static_cast<int>(screen.y - r),
            static_cast<int>(r * 2),
            static_cast<int>(r * 2)
        };
        SDL_RenderDrawRect(renderer, &ring);
    }

    // Center flash (white/yellow)
    if (progress < 0.3f) {
        float flash_r = current_radius * 0.4f * (1.0f - progress / 0.3f);
        SDL_SetRenderDrawColor(renderer, 255, 255, 200,
            static_cast<Uint8>(200 * (1.0f - progress / 0.3f)));
        SDL_Rect flash = {
            static_cast<int>(screen.x - flash_r),
            static_cast<int>(screen.y - flash_r),
            static_cast<int>(flash_r * 2),
            static_cast<int>(flash_r * 2)
        };
        SDL_RenderFillRect(renderer, &flash);
    }
}
