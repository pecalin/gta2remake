#pragma once
#include "core/window.h"
#include "core/input.h"
#include "core/timer.h"
#include "core/camera.h"
#include "world/map.h"
#include "world/collision_grid.h"
#include "world/spawn_manager.h"
#include "entities/player.h"
#include "entities/vehicle.h"
#include "systems/weapon_system.h"
#include "systems/wanted_system.h"
#include "ai/police_ai.h"
#include "ui/menu.h"
#include "core/sprite.h"
#include <vector>
#include <memory>

class Game {
public:
    bool init();
    void run();
    void shutdown();

private:
    void handle_frame_input();  // once per frame — all is_pressed() actions
    void update(float dt);      // per physics tick — continuous actions only
    void render();
    void spawn_vehicles();
    void handle_weapon_switch();
    float get_aim_angle();  // returns aim angle in radians
    void check_pickup_collisions();
    void check_projectile_hits();
    void check_vehicle_ped_collisions();
    void check_cop_player_collision();
    void scare_peds_near(Vec2 pos, float radius);
    void handle_player_death();
    void render_hud(SDL_Renderer* r);
    void render_minimap(SDL_Renderer* r);
    void render_overlay(SDL_Renderer* r);

    void save_game();
    void load_game();
    void load_sprites();

    Window window_;
    Input input_;
    Timer timer_;
    Camera camera_;
    Map map_;
    Player player_;
    CollisionGrid collision_grid_;
    SpawnManager spawn_manager_;
    WeaponSystem weapon_system_;
    WantedSystem wanted_system_;
    PoliceAI police_ai_;
    Menu menu_;
    SpriteManager sprites_;

    std::vector<std::unique_ptr<Vehicle>> vehicles_;
    Vehicle* current_vehicle_ = nullptr;

    // Player weapon state
    WeaponType current_weapon_ = WeaponType::PISTOL;
    int ammo_[static_cast<int>(WeaponType::COUNT)] = {999, 0, 0, 0, 0, 0};
    float fire_cooldown_ = 0.0f;

    // Respawn point
    Vec2 respawn_point_ = {0, 0};

    // Aiming
    bool use_mouse_aim_ = false;  // switches to true when mouse moves
    float aim_angle_ = 0.0f;

    bool running_ = false;
    bool paused_ = false;
};
