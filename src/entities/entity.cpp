#include "entities/entity.h"
#include "core/camera.h"

void Entity::render(SDL_Renderer* renderer, const Camera& camera) const {
    if (!active_) return;

    Vec2 screen = camera.world_to_screen(pos_);
    SDL_Rect dst = {
        static_cast<int>(screen.x - width_ * 0.5f),
        static_cast<int>(screen.y - height_ * 0.5f),
        static_cast<int>(width_),
        static_cast<int>(height_)
    };

    SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b, color_.a);
    SDL_RenderFillRect(renderer, &dst);
}
