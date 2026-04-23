#pragma once
#include "entities/entity.h"
#include "physics/vehicle_physics.h"

class Map;
class Input;
class Player;

struct DriveMods {
    float accel;
    float brake;
    float turn;
    float max_speed;
    float drift;

    DriveMods()
        : accel(1.0f), brake(1.0f), turn(1.0f), max_speed(1.0f), drift(1.0f) {}
};

class Vehicle : public Entity {
public:
    Vehicle();
    Vehicle(const VehicleParams& params);

    void update(float dt) override;
    void render(SDL_Renderer* renderer, const Camera& camera) const override;

    // Drive this vehicle with player input
    void handle_input(const Input& input, float dt, const DriveMods& mods);
    void handle_input(const Input& input, float dt);

    // Collision with map
    void resolve_map_collision(const Map& map);

    // Player enter/exit
    bool can_enter(const Vec2& player_pos) const;
    void enter(Player* driver);
    void exit_vehicle(Player* driver);

    bool is_occupied() const { return driver_ != nullptr; }
    Player* driver() const { return driver_; }

    const VehicleParams& params() const { return params_; }
    const VehicleState& state() const { return state_; }

    int hp() const { return hp_; }
    void take_damage(int amount);
    bool is_destroyed() const { return hp_ <= 0; }
    bool is_on_fire() const { return on_fire_; }

    SDL_Color body_color() const { return body_color_; }
    void set_body_color(SDL_Color c) { body_color_ = c; }

private:
    VehicleParams params_;
    VehicleState state_;
    Player* driver_ = nullptr;

    int hp_ = 100;
    bool on_fire_ = false;
    float fire_timer_ = 0.0f;

    SDL_Color body_color_ = {100, 100, 200, 255};
    float enter_radius_ = 40.0f;
};
