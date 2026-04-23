#pragma once
#include "entities/entity.h"

class Map;

enum class PedState {
    IDLE,
    WANDER,
    FLEE,
    DEAD
};

class Pedestrian : public Entity {
public:
    Pedestrian();

    void update(float dt) override;
    void render(SDL_Renderer* renderer, const Camera& camera) const override;

    void set_map(const Map* map) { map_ = map; }

    PedState state() const { return state_; }
    void flee_from(Vec2 threat_pos);
    void kill();

    int health() const { return health_; }
    void take_damage(int amount);

private:
    void update_idle(float dt);
    void update_wander(float dt);
    void update_flee(float dt);
    void update_dead(float dt);

    void pick_wander_target();

    PedState state_ = PedState::IDLE;
    const Map* map_ = nullptr;

    Vec2 wander_target_ = {0, 0};
    float state_timer_ = 0.0f;
    float walk_speed_ = 60.0f;
    float run_speed_ = 130.0f;
    Vec2 flee_direction_ = {0, 0};
    int health_ = 30;
    float corpse_timer_ = 10.0f;

    // Visual variety
    SDL_Color shirt_color_ = {100, 100, 200, 255};
};
