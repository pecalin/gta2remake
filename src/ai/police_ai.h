#pragma once
#include "entities/entity.h"
#include "entities/vehicle.h"
#include <vector>
#include <memory>

class Map;
class Player;
class WeaponSystem;
class SpriteManager;

enum class CopState {
    INACTIVE,
    PATROL,
    PURSUE_FOOT,
    PURSUE_CAR,
    SEARCH
};

struct CopUnit {
    Vec2 position;
    Vec2 velocity;
    float angle = 0.0f;
    CopState state = CopState::INACTIVE;
    float state_timer = 0.0f;
    int health = 80;
    bool alive = true;
    bool in_vehicle = false;
    float fire_cooldown = 0.0f;
    float search_timer = 0.0f;
    Vec2 last_known_player_pos;
    SDL_Color color = {50, 50, 200, 255};
    int facing_dir = 0;       // 0=down, 1=left, 2=right, 3=up
    float anim_time = 0.0f;

    // Vehicle for car-based cops
    Vehicle* vehicle = nullptr;
};

class PoliceAI {
public:
    void init(const Map* map);
    void update(float dt, const Player& player, int wanted_level,
                WeaponSystem& weapons);
    void render(SDL_Renderer* renderer, const Camera& camera) const;
    void render_sprites(SpriteManager& sprites, SDL_Renderer* renderer, const Camera& camera) const;

    // Can any cop see the player?
    bool can_see_player(const Player& player) const;

    // Get all active cop positions for threat detection
    std::vector<Vec2> get_cop_positions() const;

    void clear();

    std::vector<CopUnit>& cops() { return cops_; }

private:
    void spawn_response(int wanted_level, Vec2 player_pos);
    void update_cop(CopUnit& cop, float dt, const Player& player,
                    int wanted_level, WeaponSystem& weapons);
    void despawn_far_cops(Vec2 player_pos);

    const Map* map_ = nullptr;
    std::vector<CopUnit> cops_;
    std::vector<std::unique_ptr<Vehicle>> cop_vehicles_;

    float spawn_timer_ = 0.0f;
    int max_cops_for_level_[7] = {0, 1, 3, 5, 8, 12, 16};
    float cop_speed_foot_ = 100.0f;
    float cop_speed_car_ = 250.0f;
    float sight_range_ = 400.0f;
};
