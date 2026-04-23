#pragma once
#include "entities/entity.h"

class Explosion : public Entity {
public:
    Explosion() = default;
    Explosion(Vec2 pos, float radius, float damage);

    void update(float dt) override;
    void render(SDL_Renderer* renderer, const Camera& camera) const override;

    bool finished() const { return timer_ <= 0.0f; }
    float radius() const { return radius_; }
    float damage() const { return damage_; }
    bool damage_applied() const { return damage_applied_; }
    void mark_damage_applied() { damage_applied_ = true; }

private:
    float radius_ = 80.0f;
    float damage_ = 50.0f;
    float timer_ = 0.6f;
    float max_timer_ = 0.6f;
    bool damage_applied_ = false;
};
