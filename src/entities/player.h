#pragma once
#include "entities/entity.h"
#include "core/input.h"

class Map;
class SpriteManager;

class Player : public Entity {
public:
    Player();

    void handle_input(const Input& input, float dt);
    void update(float dt) override;
    void render(SDL_Renderer* renderer, const Camera& camera) const override;
    void render_sprite(SpriteManager& sprites, const Camera& camera) const;

    int facing_direction() const { return facing_dir_; }
    float anim_time() const { return anim_time_; }
    bool is_moving() const { return vel_.length_sq() > 1.0f; }

    void resolve_map_collision(const Map& map);

    bool in_vehicle() const { return in_vehicle_; }
    void set_in_vehicle(bool v) { in_vehicle_ = v; }

    int health() const { return health_; }
    int armor() const { return armor_; }
    int money() const { return money_; }
    int score() const { return score_; }
    int lives() const { return lives_; }
    int wanted_level() const { return wanted_level_; }

    void set_wanted_level(int w) { wanted_level_ = w; }
    void add_money(int amount) { money_ += amount; }
    void add_score(int amount) { score_ += amount; }

    void heal(int amount);
    void add_armor(int amount);
    void take_damage(int amount);

    bool is_dead() const { return health_ <= 0; }
    void respawn(Vec2 pos);
    void set_speed_multiplier(float m) { speed_mult_ = m; }

    // Busted/Wasted overlay states
    bool is_wasted() const { return wasted_; }
    bool is_busted() const { return busted_; }
    void set_wasted(bool w) { wasted_ = w; wasted_timer_ = 3.0f; }
    void set_busted(bool b) { busted_ = b; busted_timer_ = 3.0f; }
    float wasted_timer() const { return wasted_timer_; }
    float busted_timer() const { return busted_timer_; }
    void update_overlay_timers(float dt);

private:
    float move_speed_ = 150.0f;
    float speed_mult_ = 1.0f;
    bool in_vehicle_ = false;
    int facing_dir_ = 0;       // 0=down, 1=left, 2=right, 3=up
    float anim_time_ = 0.0f;

    int health_ = 100;
    int armor_ = 0;
    int money_ = 0;
    int score_ = 0;
    int lives_ = 3;
    int wanted_level_ = 0;

    bool wasted_ = false;
    bool busted_ = false;
    float wasted_timer_ = 0.0f;
    float busted_timer_ = 0.0f;
};
