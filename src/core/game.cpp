#include "core/game.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cmath>

bool Game::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        return false;
    }

    if (!window_.init("GTA2 Clone")) {
        return false;
    }

    camera_.set_viewport_size(Window::WIDTH, Window::HEIGHT);

    map_.generate_test_city();
    collision_grid_.init(map_.pixel_width(), map_.pixel_height());
    spawn_manager_.init(&map_);
    police_ai_.init(&map_);

    // Place player on a road near center
    respawn_point_ = {
        17.0f * Map::TILE_SIZE + Map::TILE_SIZE * 0.5f,
        7.0f * Map::TILE_SIZE + Map::TILE_SIZE * 0.5f
    };
    player_.set_position(respawn_point_);

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // Load vehicle config from file (overrides hardcoded defaults)
    VehicleTypes::load_from_file("assets/data/vehicles.txt");

    spawn_vehicles();

    // Init sprite system
    sprites_.init(window_.renderer());
    load_sprites();

    // Init menu
    menu_.init(Window::WIDTH, Window::HEIGHT, &window_);
    menu_.on_resume = [this]() { paused_ = false; };
    menu_.on_save   = [this]() { save_game(); };
    menu_.on_load   = [this]() { load_game(); };
    menu_.on_quit   = [this]() { running_ = false; };

    running_ = true;
    return true;
}

void Game::spawn_vehicles() {
    SDL_Color colors[] = {
        {100, 100, 220, 255}, {220, 60, 60, 255}, {60, 180, 60, 255},
        {220, 220, 60, 255}, {180, 80, 220, 255}, {220, 140, 40, 255},
        {60, 200, 200, 255}, {200, 200, 200, 255}, {100, 100, 100, 255},
        {180, 140, 100, 255},
    };

    int road_positions[] = {6, 7, 16, 17, 26, 27, 36, 37, 46, 47};
    int num_roads = 10;
    int vehicle_count = 0;
    int max_vehicles = 20;

    for (int i = 0; i < 100 && vehicle_count < max_vehicles; i++) {
        int road_x = road_positions[std::rand() % num_roads];
        int road_y = road_positions[std::rand() % num_roads];
        int offset = 8 + std::rand() % 48;
        bool horizontal = std::rand() % 2;

        float wx, wy, angle;
        if (horizontal) {
            wx = static_cast<float>(offset * Map::TILE_SIZE + Map::TILE_SIZE / 2);
            wy = static_cast<float>(road_y * Map::TILE_SIZE + Map::TILE_SIZE / 2);
            angle = (std::rand() % 2) ? 0.0f : 3.14159f;
        } else {
            wx = static_cast<float>(road_x * Map::TILE_SIZE + Map::TILE_SIZE / 2);
            wy = static_cast<float>(offset * Map::TILE_SIZE + Map::TILE_SIZE / 2);
            angle = (std::rand() % 2) ? 1.5708f : -1.5708f;
        }

        int tx = static_cast<int>(wx) / Map::TILE_SIZE;
        int ty = static_cast<int>(wy) / Map::TILE_SIZE;
        if (map_.get_tile(tx, ty) != TileType::ROAD) continue;

        int type_idx = std::rand() % VehicleTypes::count();
        auto vehicle = std::make_unique<Vehicle>(VehicleTypes::get_by_index(type_idx));
        vehicle->set_position({wx, wy});
        vehicle->set_angle(angle);
        vehicle->set_body_color(colors[std::rand() % 10]);

        vehicles_.push_back(std::move(vehicle));
        vehicle_count++;
    }
}

// Called ONCE per frame — handles all is_pressed() one-shot actions
void Game::handle_frame_input() {
    if (player_.is_wasted() || player_.is_busted()) return;

    // Enter/exit vehicle
    if (input_.is_pressed(Action::ENTER_EXIT_VEHICLE)) {
        if (current_vehicle_) {
            current_vehicle_->exit_vehicle(&player_);
            current_vehicle_ = nullptr;
        } else {
            float best_dist = 999999.0f;
            Vehicle* best = nullptr;
            for (auto& v : vehicles_) {
                if (v->can_enter(player_.position())) {
                    float dist = (v->position() - player_.position()).length();
                    if (dist < best_dist) {
                        best_dist = dist;
                        best = v.get();
                    }
                }
            }
            if (best) {
                best->enter(&player_);
                current_vehicle_ = best;
            }
        }
    }

    // Weapon switching
    handle_weapon_switch();
}

void Game::handle_weapon_switch() {
    if (input_.is_pressed(Action::WEAPON_NEXT)) {
        int w = static_cast<int>(current_weapon_);
        for (int i = 1; i < static_cast<int>(WeaponType::COUNT); i++) {
            int next = (w + i) % static_cast<int>(WeaponType::COUNT);
            if (ammo_[next] > 0 || next == 0) {
                current_weapon_ = static_cast<WeaponType>(next);
                break;
            }
        }
    }
    if (input_.is_pressed(Action::WEAPON_PREV)) {
        int w = static_cast<int>(current_weapon_);
        for (int i = 1; i < static_cast<int>(WeaponType::COUNT); i++) {
            int prev = (w - i + static_cast<int>(WeaponType::COUNT)) % static_cast<int>(WeaponType::COUNT);
            if (ammo_[prev] > 0 || prev == 0) {
                current_weapon_ = static_cast<WeaponType>(prev);
                break;
            }
        }
    }
}

// Determine aim angle: mouse > aimbot > facing direction
float Game::get_aim_angle() {
    const auto& dev = menu_.dev();
    Vec2 player_screen = camera_.world_to_screen(player_.position());

    // 1. Mouse aim — if mouse has moved recently, aim toward cursor
    if (use_mouse_aim_) {
        float dx = static_cast<float>(input_.mouse_x()) - player_screen.x;
        float dy = static_cast<float>(input_.mouse_y()) - player_screen.y;
        if (dx * dx + dy * dy > 16.0f) {
            return std::atan2(dy, dx);
        }
    }

    // 2. Aimbot — only if enabled in dev settings
    if (dev.aimbot) {
        float best_dist = dev.aimbot_range;
        Vec2 best_target = {0, 0};
        bool found = false;

        // Check pedestrians
        for (auto& ped : spawn_manager_.pedestrians()) {
            if (!ped.active() || ped.state() == PedState::DEAD) continue;
            float dist = (ped.position() - player_.position()).length();
            if (dist < best_dist) {
                best_dist = dist;
                best_target = ped.position();
                found = true;
            }
        }

        // Check cops
        for (auto& cop : police_ai_.cops()) {
            if (!cop.alive) continue;
            float dist = (cop.position - player_.position()).length();
            if (dist < best_dist) {
                best_dist = dist;
                best_target = cop.position;
                found = true;
            }
        }

        if (found) {
            Vec2 dir = best_target - player_.position();
            return std::atan2(dir.y, dir.x);
        }
    }

    // 3. Fallback — keep previous aim angle (no snapping to move direction)
    return aim_angle_;
}

// Shooting is handled per-tick in update() since it uses is_down() for continuous fire

void Game::check_pickup_collisions() {
    for (auto& pickup : spawn_manager_.pickups()) {
        if (pickup.is_collected()) continue;

        Vec2 diff = player_.position() - pickup.position();
        if (diff.length() < 20.0f) {
            PickupType pt = pickup.pickup_type();
            bool collected = false;

            switch (pt) {
                case PickupType::WEAPON_PISTOL:
                    ammo_[static_cast<int>(WeaponType::PISTOL)] += 20;
                    collected = true; break;
                case PickupType::WEAPON_MACHINE_GUN:
                    ammo_[static_cast<int>(WeaponType::MACHINE_GUN)] += 50;
                    collected = true; break;
                case PickupType::WEAPON_ROCKET:
                    ammo_[static_cast<int>(WeaponType::ROCKET)] += 5;
                    collected = true; break;
                case PickupType::WEAPON_FLAMETHROWER:
                    ammo_[static_cast<int>(WeaponType::FLAMETHROWER)] += 100;
                    collected = true; break;
                case PickupType::WEAPON_MOLOTOV:
                    ammo_[static_cast<int>(WeaponType::MOLOTOV)] += 10;
                    collected = true; break;
                case PickupType::WEAPON_GRENADE:
                    ammo_[static_cast<int>(WeaponType::GRENADE)] += 5;
                    collected = true; break;
                case PickupType::HEALTH:
                    player_.heal(50);
                    collected = true; break;
                case PickupType::ARMOR:
                    player_.add_armor(50);
                    collected = true; break;
                case PickupType::COP_BRIBE:
                    wanted_system_.clear();
                    collected = true; break;
                default:
                    collected = true; break;
            }

            if (collected) {
                pickup.collect();
                player_.add_score(100);
            }
        }
    }
}

void Game::check_projectile_hits() {
    const auto& dev = menu_.dev();
    auto& projectiles = weapon_system_.projectiles();

    for (auto& proj : projectiles) {
        if (!proj.active()) continue;

        // Swept hit test: check along the bullet's travel path this tick
        // so fast bullets don't skip over targets
        Vec2 proj_vel = proj.velocity();
        float proj_speed = proj_vel.length();
        Vec2 proj_dir = (proj_speed > 0.01f) ? proj_vel * (1.0f / proj_speed) : Vec2(0, 0);
        float step = 8.0f; // check every 8 pixels along path
        int num_steps = std::max(1, static_cast<int>(proj_speed * Timer::FIXED_DT / step));
        float hit_radius = 16.0f;

        // Check vs pedestrians
        for (auto& ped : spawn_manager_.pedestrians()) {
            if (!ped.active() || ped.state() == PedState::DEAD) continue;
            bool hit = false;
            for (int s = 0; s <= num_steps && !hit; s++) {
                Vec2 check_pos = proj.position() - proj_dir * (s * step);
                if ((check_pos - ped.position()).length() < hit_radius) hit = true;
            }
            if (hit) {
                int dmg = dev.one_hit_kill ? 9999 : static_cast<int>(proj.damage());
                ped.take_damage(dmg);
                proj.set_active(false);
                if (ped.state() == PedState::DEAD) {
                    player_.add_score(50);
                    player_.add_money(100);
                    wanted_system_.add_crime(std::max(1, static_cast<int>(2 * dev.wanted_gain)));
                }
                break;
            }
        }
        if (!proj.active()) continue;  // consumed by ped hit

        // Check vs cops
        if (proj.owner() == &player_) {
            for (auto& cop : police_ai_.cops()) {
                if (!cop.alive) continue;
                bool hit = false;
                for (int s = 0; s <= num_steps && !hit; s++) {
                    Vec2 check_pos = proj.position() - proj_dir * (s * step);
                    if ((check_pos - cop.position).length() < hit_radius) hit = true;
                }
                if (hit) {
                    int cdmg = dev.one_hit_kill ? 9999 : static_cast<int>(proj.damage());
                    cop.health -= cdmg;
                    proj.set_active(false);
                    if (cop.health <= 0) {
                        cop.alive = false;
                        cop.state = CopState::INACTIVE;
                        player_.add_score(200);
                        wanted_system_.add_crime(std::max(1, static_cast<int>(3 * dev.wanted_gain)));
                    }
                    break;
                }
            }
        }
        if (!proj.active()) continue;  // consumed by cop hit

        // Check vs player (cop bullets)
        if (proj.owner() != &player_ && proj.owner() != nullptr) {
            // This is a cop shooting at player — skip (cops don't have owner set to player)
        }
        if (proj.owner() == nullptr) {
            // Cop projectile — check vs player
            Vec2 diff = proj.position() - player_.position();
            if (diff.length() < 10.0f && !player_.in_vehicle()) {
                if (!menu_.dev().invincible) player_.take_damage(static_cast<int>(proj.damage()));
                proj.set_active(false);
            }
            // Check vs player's vehicle
            if (current_vehicle_) {
                Rect vbb = current_vehicle_->bounding_box();
                if (vbb.contains(proj.position().x, proj.position().y)) {
                    current_vehicle_->take_damage(static_cast<int>(proj.damage()));
                    proj.set_active(false);
                }
            }
        }

        // Check vs vehicles
        if (proj.owner() == &player_) {
            for (auto& v : vehicles_) {
                if (!v->active() || v.get() == current_vehicle_) continue;
                Rect vbb = v->bounding_box();
                if (vbb.contains(proj.position().x, proj.position().y)) {
                    v->take_damage(static_cast<int>(proj.damage()));
                    proj.set_active(false);
                    if (v->is_destroyed()) {
                        weapon_system_.create_explosion(v->position(), 80.0f, 50.0f);
                        camera_.add_shake(8.0f);
                        player_.add_score(200);
                        player_.add_money(v->params().value * 10);
                        scare_peds_near(v->position(), 400.0f);
                        wanted_system_.add_crime(std::max(1, static_cast<int>(1 * dev.wanted_gain)));
                    }
                    break;
                }
            }
        }
    }

    // Explosion damage
    for (auto& exp : weapon_system_.explosions()) {
        if (!exp.active() || exp.damage_applied()) continue;
        exp.mark_damage_applied();

        for (auto& ped : spawn_manager_.pedestrians()) {
            if (!ped.active() || ped.state() == PedState::DEAD) continue;
            float dist = (ped.position() - exp.position()).length();
            if (dist < exp.radius()) {
                float dmg_mult = 1.0f - (dist / exp.radius());
                int edm = dev.one_hit_kill ? 9999 : static_cast<int>(exp.damage() * dmg_mult);
                ped.take_damage(edm);
                if (ped.state() == PedState::DEAD) {
                    player_.add_score(50);
                    wanted_system_.add_crime(std::max(1, static_cast<int>(2 * dev.wanted_gain)));
                }
            }
        }

        for (auto& v : vehicles_) {
            if (!v->active()) continue;
            float dist = (v->position() - exp.position()).length();
            if (dist < exp.radius()) {
                float dmg_mult = 1.0f - (dist / exp.radius());
                v->take_damage(static_cast<int>(exp.damage() * dmg_mult));
                Vec2 push = (v->position() - exp.position()).normalized() * 200.0f * dmg_mult;
                v->set_velocity(v->velocity() + push);
            }
        }

        // Damage player
        float player_dist = (player_.position() - exp.position()).length();
        if (player_dist < exp.radius()) {
            float dmg = exp.damage() * (1.0f - player_dist / exp.radius());
            if (!dev.invincible) player_.take_damage(static_cast<int>(dmg));
        }

        // Damage cops
        for (auto& cop : police_ai_.cops()) {
            if (!cop.alive) continue;
            float dist = (cop.position - exp.position()).length();
            if (dist < exp.radius()) {
                int cdmg = dev.one_hit_kill ? 9999 : static_cast<int>(exp.damage() * (1.0f - dist / exp.radius()));
                cop.health -= cdmg;
                if (cop.health <= 0) {
                    cop.alive = false;
                    cop.state = CopState::INACTIVE;
                    wanted_system_.add_crime(std::max(1, static_cast<int>(3 * dev.wanted_gain)));
                }
            }
        }

        scare_peds_near(exp.position(), 500.0f);
        camera_.add_shake(12.0f);
    }
}

void Game::check_vehicle_ped_collisions() {
    if (!current_vehicle_) return;
    const auto& dev = menu_.dev();

    float speed = current_vehicle_->state().velocity.length();
    if (speed < 50.0f) return;

    Rect vbb = current_vehicle_->bounding_box();

    for (auto& ped : spawn_manager_.pedestrians()) {
        if (!ped.active() || ped.state() == PedState::DEAD) continue;
        Rect pbb = ped.bounding_box();
        if (vbb.overlaps(pbb)) {
            float damage = dev.one_hit_kill ? 9999.0f : speed * 0.3f;
            ped.take_damage(static_cast<int>(damage));
            if (ped.state() == PedState::DEAD) {
                player_.add_score(25);
                wanted_system_.add_crime(std::max(1, static_cast<int>(2 * dev.wanted_gain)));
            }
        }
    }

    // Run over cops
    for (auto& cop : police_ai_.cops()) {
        if (!cop.alive || cop.in_vehicle) continue;
        Rect cop_bb = {cop.position.x - 5, cop.position.y - 6, 10, 12};
        if (vbb.overlaps(cop_bb)) {
            float damage = dev.one_hit_kill ? 9999.0f : speed * 0.4f;
            cop.health -= static_cast<int>(damage);
            if (cop.health <= 0) {
                cop.alive = false;
                cop.state = CopState::INACTIVE;
                wanted_system_.add_crime(std::max(1, static_cast<int>(3 * dev.wanted_gain)));
            }
        }
    }
}

void Game::check_cop_player_collision() {
    if (player_.is_wasted() || player_.is_busted()) return;

    // Cops can arrest player on foot at low wanted levels
    if (!player_.in_vehicle() && wanted_system_.level() <= 2 && wanted_system_.level() > 0) {
        for (auto& cop : police_ai_.cops()) {
            if (!cop.alive || cop.in_vehicle) continue;
            float dist = (cop.position - player_.position()).length();
            if (dist < 15.0f) {
                // BUSTED!
                wanted_system_.busted();
                player_.set_busted(true);
                player_.add_money(-player_.money() / 10); // lose 10% money

                // Clear weapons except pistol
                for (int i = 1; i < static_cast<int>(WeaponType::COUNT); i++) {
                    ammo_[i] = 0;
                }
                current_weapon_ = WeaponType::PISTOL;
                break;
            }
        }
    }
}

void Game::handle_player_death() {
    if (player_.is_dead() && !player_.is_wasted()) {
        player_.set_wasted(true);

        // Exit vehicle if in one
        if (current_vehicle_) {
            current_vehicle_->exit_vehicle(&player_);
            current_vehicle_ = nullptr;
        }

        // Clear weapons except pistol
        for (int i = 1; i < static_cast<int>(WeaponType::COUNT); i++) {
            ammo_[i] = 0;
        }
        current_weapon_ = WeaponType::PISTOL;
    }

    // After wasted/busted timer expires, respawn
    if (player_.is_wasted() && player_.wasted_timer() <= 0.0f) {
        int lives = player_.lives();
        if (lives > 1) {
            player_.respawn(respawn_point_);
            // lives is reset in respawn, set it manually
            // Actually respawn doesn't touch lives, we need to subtract
        }
        player_.respawn(respawn_point_);
        wanted_system_.clear();
        police_ai_.clear();
    }

    if (player_.is_busted() && player_.busted_timer() <= 0.0f) {
        player_.respawn(respawn_point_);
        wanted_system_.clear();
        police_ai_.clear();
    }
}

void Game::scare_peds_near(Vec2 pos, float radius) {
    for (auto& ped : spawn_manager_.pedestrians()) {
        if (!ped.active() || ped.state() == PedState::DEAD) continue;
        float dist = (ped.position() - pos).length();
        if (dist < radius) {
            ped.flee_from(pos);
        }
    }
}

void Game::run() {
    timer_.start();

    while (running_) {
        timer_.tick();

        input_.update();
        if (input_.quit_requested()) {
            running_ = false;
            break;
        }

        // Toggle pause menu with ESC (only when menu is closed)
        if (input_.is_pressed(Action::PAUSE) && !menu_.is_open()) {
            paused_ = true;
            menu_.open();
        } else if (paused_ && menu_.is_open()) {
            menu_.handle_input(input_);
            while (timer_.should_update()) {}
        } else {
            // Process one-shot inputs ONCE per frame (before physics ticks)
            handle_frame_input();

            // Then run physics ticks (only continuous actions like movement)
            while (timer_.should_update()) {
                update(Timer::FIXED_DT);
            }
        }

        render();
    }
}

void Game::save_game() {
    // TODO: serialize game state to file
}

void Game::load_game() {
    // TODO: deserialize game state from file
}

void Game::load_sprites() {
    // ---- Character sprites ----
    // LPC spritesheet format: rows are directions, columns are animation frames
    // Adjust row numbers to match your spritesheet layout
    // Default LPC "walkcycle" sheet: row 0=up, 1=left, 2=down, 3=right, 9 frames each, 64x64 per frame
    // LPC full spritesheet: rows are 0-indexed
    // Walk: row 8=up, 9=left, 10=down, 11=right (9 frames each)
    // Death: row 20 (6 frames)
    // LPC full spritesheet (0-indexed rows):
    //   Walk: 8=up, 9=left, 10=down, 11=right (9 frames)
    //   Death: 20 (6 frames)
    //   Run:  38=up, 39=left, 40=down, 41=right (9 frames)
    //
    // Player always uses RUN animation for movement
    sprites_.load_character("player", "assets/sprites/characters/player.png",
                            64, 64, 9,
                            8, 9, 10, 11,       // walk rows (up,left,down,right)
                            20, 6,               // death row, death frames
                            38, 39, 40, 41, 8);  // run rows (up,left,down,right), run frames

    // Civilians: walk normally, run when fleeing
    sprites_.load_character("civilian", "assets/sprites/characters/civilian.png",
                            64, 64, 9,
                            8, 9, 10, 11,
                            20, 6,
                            38, 39, 40, 41, 8);

    // Cops: walk on patrol, run when pursuing
    sprites_.load_character("cop", "assets/sprites/characters/cop.png",
                            64, 64, 9,
                            8, 9, 10, 11,
                            20, 6,
                            38, 39, 40, 41, 8);

    // ---- Vehicle sprites ----
    // Option A: Single image (game rotates it with SDL_RenderCopyEx)
    //   load_vehicle("car_name", path, width, height, 1)
    //
    // Option B: Pre-rotated spritesheet (single row, N angles from east clockwise)
    //   load_vehicle("car_name", path, frame_w, frame_h, num_angles)

    // Vehicle names must match the names in vehicles.txt
    sprites_.load_vehicle("Romero",    "assets/sprites/vehicles/romero.png",    48, 80, 1);
    sprites_.load_vehicle("Wellard",   "assets/sprites/vehicles/wellard.png",   48, 80, 1);
    sprites_.load_vehicle("Box Truck", "assets/sprites/vehicles/boxtruck.png",  52, 96, 1);
    sprites_.load_vehicle("Bus",       "assets/sprites/vehicles/bus.png",       52, 110, 1);
    sprites_.load_vehicle("Cop Car",   "assets/sprites/vehicles/copcar.png",    48, 80, 1);
    sprites_.load_vehicle("Taxi",      "assets/sprites/vehicles/taxi.png",      48, 80, 1);
    sprites_.load_vehicle("Bank Van",  "assets/sprites/vehicles/bankvan.png",   50, 88, 1);
    sprites_.load_vehicle("Beamer",    "assets/sprites/vehicles/beamer.png",    48, 80, 1);
    sprites_.load_vehicle("Bug",       "assets/sprites/vehicles/bug.png",       40, 64, 1);
    sprites_.load_vehicle("Pacifier",  "assets/sprites/vehicles/pacifier.png",  50, 88, 1);

    // Note: missing sprite files are silently skipped — colored rectangles are used instead
}

void Game::update(float dt) {
    const auto& dev = menu_.dev();

    // Handle overlay states
    player_.update_overlay_timers(dt);
    handle_player_death();

    if (!player_.is_wasted() && !player_.is_busted()) {
        // Mouse aim stays active once mouse has moved
        if (input_.mouse_moved()) use_mouse_aim_ = true;

        // Compute aim angle
        aim_angle_ = get_aim_angle();

        // Shooting (Ctrl or left mouse button)
        fire_cooldown_ -= dt;
        bool wants_shoot = input_.is_down(Action::SHOOT) || input_.mouse_left();
        if (wants_shoot && fire_cooldown_ <= 0.0f && !player_.in_vehicle()) {
            WeaponInfo info = get_weapon_info(current_weapon_);
            int ammo_idx = static_cast<int>(current_weapon_);
            if (ammo_[ammo_idx] > 0 || current_weapon_ == WeaponType::PISTOL) {
                weapon_system_.fire(current_weapon_, player_.position(), aim_angle_, &player_);
                fire_cooldown_ = 1.0f / info.fire_rate;

                if (current_weapon_ != WeaponType::PISTOL && !dev.infinite_ammo) {
                    ammo_[ammo_idx]--;
                    if (ammo_[ammo_idx] <= 0) {
                        current_weapon_ = WeaponType::PISTOL;
                    }
                }

                scare_peds_near(player_.position(), 300.0f);

                auto cop_positions = police_ai_.get_cop_positions();
                for (auto& cp : cop_positions) {
                    if ((cp - player_.position()).length() < 400.0f) {
                        wanted_system_.add_crime(std::max(1, static_cast<int>(1 * dev.wanted_gain)));
                        break;
                    }
                }
            }
        }

        if (current_vehicle_) {
            // Build drive mods from dev settings
            DriveMods mods;
            mods.accel = dev.car_acceleration;
            mods.brake = dev.car_braking;
            mods.turn = dev.car_turn_speed;
            mods.max_speed = dev.car_max_speed;
            mods.drift = dev.car_drift;

            current_vehicle_->handle_input(input_, dt, mods);
            current_vehicle_->update(dt);
            current_vehicle_->resolve_map_collision(map_);
            player_.set_position(current_vehicle_->position());

            if (current_vehicle_->state().velocity.length() > 150.0f) {
                scare_peds_near(current_vehicle_->position(), 150.0f);
            }

            // Vehicle destroyed while driving
            if (current_vehicle_->is_destroyed()) {
                if (!dev.invincible) player_.take_damage(30);
                current_vehicle_->exit_vehicle(&player_);
                weapon_system_.create_explosion(current_vehicle_->position(), 100.0f, 60.0f);
                current_vehicle_->set_active(false);
                current_vehicle_ = nullptr;
                camera_.add_shake(15.0f);
            }
        } else {
            // Apply player speed multiplier
            player_.set_speed_multiplier(dev.player_speed);
            player_.handle_input(input_, dt);
            player_.update(dt);
            player_.resolve_map_collision(map_);
        }

        // Collision checks
        check_pickup_collisions();
        check_projectile_hits();
        check_vehicle_ped_collisions();
        if (!dev.no_police) check_cop_player_collision();
    }

    // Update all vehicles (unoccupied ones coast and collide with map)
    for (auto& v : vehicles_) {
        if (v.get() != current_vehicle_) {
            v->update(dt);
            if (v->velocity().length() > 1.0f) {
                v->resolve_map_collision(map_);
            }
        }
    }

    // Remove destroyed vehicles
    for (auto& v : vehicles_) {
        if (v->is_destroyed() && v.get() != current_vehicle_ && v->active()) {
            weapon_system_.create_explosion(v->position(), 80.0f, 40.0f);
            v->set_active(false);
        }
    }

    // Weapon system
    weapon_system_.update(dt, map_);

    // Spawn manager — apply NPC density
    spawn_manager_.set_density(dev.npc_density);
    spawn_manager_.update(dt, player_.position(), camera_);

    // Wanted system
    if (dev.no_police) {
        wanted_system_.clear();
    }
    player_.set_wanted_level(wanted_system_.level());
    bool cops_see = police_ai_.can_see_player(player_);
    wanted_system_.update(dt, cops_see);

    // Police AI
    if (!dev.no_police) {
        police_ai_.update(dt, player_, wanted_system_.level(), weapon_system_);
    } else {
        police_ai_.clear();
    }

    camera_.set_target(player_.position());
    camera_.update(dt);
}

void Game::render() {
    SDL_Renderer* r = window_.renderer();

    SDL_SetRenderDrawColor(r, 20, 20, 30, 255);
    SDL_RenderClear(r);

    // Map
    map_.render(r, camera_);

    // Y-sorted entities
    struct Drawable {
        float y;
        enum Type { VEHICLE, PLAYER, PED, COP } type;
        int index;
    };
    std::vector<Drawable> drawables;

    for (int i = 0; i < static_cast<int>(vehicles_.size()); i++) {
        if (vehicles_[i]->active())
            drawables.push_back({vehicles_[i]->position().y, Drawable::VEHICLE, i});
    }

    auto& peds = spawn_manager_.pedestrians();
    for (int i = 0; i < static_cast<int>(peds.size()); i++) {
        if (peds[i].active())
            drawables.push_back({peds[i].position().y, Drawable::PED, i});
    }

    auto& cops = police_ai_.cops();
    for (int i = 0; i < static_cast<int>(cops.size()); i++) {
        if (cops[i].alive)
            drawables.push_back({cops[i].position.y, Drawable::COP, i});
    }

    if (!player_.in_vehicle())
        drawables.push_back({player_.position().y, Drawable::PLAYER, 0});

    std::sort(drawables.begin(), drawables.end(),
              [](const Drawable& a, const Drawable& b) { return a.y < b.y; });

    // Pickups first (on ground)
    for (auto& pickup : spawn_manager_.pickups())
        pickup.render(r, camera_);

    // Sorted entities
    for (auto& d : drawables) {
        switch (d.type) {
            case Drawable::VEHICLE:
                // Use sprite if available, otherwise colored rectangle
                if (sprites_.get_vehicle(vehicles_[d.index]->params().name)) {
                    Vec2 vs = camera_.world_to_screen(vehicles_[d.index]->position());
                    sprites_.draw_vehicle(vehicles_[d.index]->params().name, vs,
                                          vehicles_[d.index]->angle());
                } else {
                    vehicles_[d.index]->render(r, camera_);
                }
                break;
            case Drawable::PED:
                if (sprites_.get_character("civilian")) {
                    peds[d.index].render_sprite(sprites_, camera_);
                } else {
                    peds[d.index].render(r, camera_);
                }
                break;
            case Drawable::PLAYER:
                if (sprites_.get_character("player")) {
                    player_.render_sprite(sprites_, camera_);
                } else {
                    player_.render(r, camera_);
                }
                break;
            case Drawable::COP:     break; // cops rendered separately
        }
    }

    // Police (rendered via PoliceAI for proper visuals)
    if (sprites_.get_character("cop")) {
        police_ai_.render_sprites(sprites_, r, camera_);
    } else {
        police_ai_.render(r, camera_);
    }

    // Weapons/explosions on top
    weapon_system_.render(r, camera_);

    // HUD
    render_hud(r);
    render_minimap(r);
    render_overlay(r);

    // Crosshair / aim indicator (only when on foot and not in menus)
    if (!player_.in_vehicle() && !player_.is_wasted() && !player_.is_busted() && !paused_) {
        Vec2 player_screen = camera_.world_to_screen(player_.position());
        // Mouse cursor crosshair
        if (use_mouse_aim_) {
            int mx = input_.mouse_x();
            int my = input_.mouse_y();
            SDL_SetRenderDrawColor(r, 255, 255, 50, 180);
            SDL_RenderDrawLine(r, mx - 8, my, mx - 3, my);
            SDL_RenderDrawLine(r, mx + 3, my, mx + 8, my);
            SDL_RenderDrawLine(r, mx, my - 8, mx, my - 3);
            SDL_RenderDrawLine(r, mx, my + 3, mx, my + 8);
        }
    }

    // Pause menu on top of everything
    if (paused_ && menu_.is_open()) {
        menu_.render(r);
    }

    SDL_RenderPresent(r);
}

void Game::render_hud(SDL_Renderer* r) {
    // Health bar
    SDL_SetRenderDrawColor(r, 40, 40, 40, 200);
    SDL_Rect hud_bg = {10, 10, 204, 20};
    SDL_RenderFillRect(r, &hud_bg);
    int health_w = static_cast<int>(player_.health() * 2.0f);
    SDL_SetRenderDrawColor(r, 220, 40, 40, 255);
    SDL_Rect health_bar = {12, 12, health_w, 16};
    SDL_RenderFillRect(r, &health_bar);
    SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
    SDL_RenderDrawRect(r, &hud_bg);

    // Armor bar
    if (player_.armor() > 0) {
        SDL_SetRenderDrawColor(r, 40, 40, 40, 200);
        SDL_Rect armor_bg = {10, 34, 204, 16};
        SDL_RenderFillRect(r, &armor_bg);
        int armor_w = static_cast<int>(player_.armor() * 2.0f);
        SDL_SetRenderDrawColor(r, 40, 100, 220, 255);
        SDL_Rect armor_bar = {12, 36, armor_w, 12};
        SDL_RenderFillRect(r, &armor_bar);
        SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
        SDL_RenderDrawRect(r, &armor_bg);
    }

    // Wanted stars
    for (int i = 0; i < 6; i++) {
        SDL_Rect star = {220 + i * 22, 12, 18, 16};
        if (i < player_.wanted_level()) {
            SDL_SetRenderDrawColor(r, 255, 200, 0, 255);
            SDL_RenderFillRect(r, &star);
        }
        SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
        SDL_RenderDrawRect(r, &star);
    }

    // Lives
    for (int i = 0; i < player_.lives(); i++) {
        SDL_SetRenderDrawColor(r, 255, 220, 50, 255);
        SDL_Rect life = {220 + i * 16, 32, 12, 12};
        SDL_RenderFillRect(r, &life);
    }

    // Current weapon indicator (bottom right)
    WeaponInfo wi = get_weapon_info(current_weapon_);
    SDL_SetRenderDrawColor(r, 40, 40, 40, 200);
    SDL_Rect weapon_bg = {Window::WIDTH - 200, Window::HEIGHT - 40, 190, 30};
    SDL_RenderFillRect(r, &weapon_bg);

    // Draw weapon icon
    {
        int ix = Window::WIDTH - 196;
        int iy = Window::HEIGHT - 36;
        SDL_Color wc = wi.color;
        SDL_Rect rc;  // reusable rect

        auto fill = [&](int x, int y, int w, int h) {
            rc = {x, y, w, h};
            SDL_RenderFillRect(r, &rc);
        };

        switch (current_weapon_) {
            case WeaponType::PISTOL:
                SDL_SetRenderDrawColor(r, wc.r, wc.g, wc.b, 255);
                fill(ix+6, iy+2, 14, 6);    // barrel
                fill(ix+6, iy+8, 8, 10);    // grip
                SDL_SetRenderDrawColor(r, 60, 60, 60, 255);
                fill(ix+18, iy+3, 4, 4);    // muzzle
                break;
            case WeaponType::MACHINE_GUN:
                SDL_SetRenderDrawColor(r, wc.r, wc.g, wc.b, 255);
                fill(ix+2, iy+4, 20, 5);    // barrel
                fill(ix+8, iy+9, 6, 9);     // grip
                SDL_SetRenderDrawColor(r, 180, 150, 50, 255);
                fill(ix+14, iy+9, 4, 7);    // magazine
                SDL_SetRenderDrawColor(r, 60, 60, 60, 255);
                fill(ix, iy+5, 3, 3);       // muzzle
                break;
            case WeaponType::ROCKET:
                SDL_SetRenderDrawColor(r, 80, 100, 80, 255);
                fill(ix+4, iy+6, 18, 6);    // tube
                SDL_SetRenderDrawColor(r, wc.r, wc.g, wc.b, 255);
                fill(ix, iy+4, 6, 10);      // warhead
                SDL_SetRenderDrawColor(r, 60, 60, 60, 255);
                fill(ix+8, iy+12, 6, 6);    // grip
                break;
            case WeaponType::FLAMETHROWER:
                SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
                fill(ix+2, iy+6, 14, 5);    // barrel
                SDL_SetRenderDrawColor(r, wc.r, wc.g, wc.b, 255);
                fill(ix, iy+3, 5, 5);       // flame tip
                SDL_SetRenderDrawColor(r, 200, 60, 60, 255);
                fill(ix+14, iy+3, 8, 14);   // tank
                break;
            case WeaponType::MOLOTOV:
                SDL_SetRenderDrawColor(r, 100, 60, 30, 255);
                fill(ix+8, iy+8, 8, 12);    // bottle body
                SDL_SetRenderDrawColor(r, 80, 50, 20, 255);
                fill(ix+10, iy+3, 4, 6);    // neck
                SDL_SetRenderDrawColor(r, wc.r, wc.g, wc.b, 255);
                fill(ix+9, iy, 6, 4);       // rag/flame
                break;
            case WeaponType::GRENADE:
                SDL_SetRenderDrawColor(r, wc.r, wc.g, wc.b, 255);
                fill(ix+5, iy+6, 12, 14);   // body
                fill(ix+7, iy+4, 8, 4);     // top
                SDL_SetRenderDrawColor(r, 200, 200, 50, 255);
                fill(ix+9, iy+1, 4, 4);     // pin
                SDL_SetRenderDrawColor(r, 40, 60, 40, 255);
                fill(ix+8, iy+11, 6, 2);    // stripe
                break;
            default:
                SDL_SetRenderDrawColor(r, wc.r, wc.g, wc.b, 255);
                fill(ix, iy, 22, 22);
                break;
        }
    }

    int ammo_idx = static_cast<int>(current_weapon_);
    int ammo_max = get_weapon_info(current_weapon_).ammo_per_pickup * 3;
    float ammo_pct = clamp(static_cast<float>(ammo_[ammo_idx]) / std::max(ammo_max, 1), 0.0f, 1.0f);
    if (current_weapon_ == WeaponType::PISTOL) ammo_pct = 1.0f;

    SDL_SetRenderDrawColor(r, 200, 200, 100, 255);
    SDL_Rect ammo_bar = {Window::WIDTH - 168, Window::HEIGHT - 32, static_cast<int>(150 * ammo_pct), 14};
    SDL_RenderFillRect(r, &ammo_bar);
    SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
    SDL_RenderDrawRect(r, &weapon_bg);

    // Score (top right area, below minimap)
    SDL_SetRenderDrawColor(r, 40, 40, 40, 180);
    SDL_Rect score_bg = {Window::WIDTH - 162, 200, 152, 20};
    SDL_RenderFillRect(r, &score_bg);

    // Score as filled bar proportional (rough visual)
    float score_fill = clamp(static_cast<float>(player_.score()) / 10000.0f, 0.0f, 1.0f);
    SDL_SetRenderDrawColor(r, 200, 180, 50, 255);
    SDL_Rect score_bar = {Window::WIDTH - 160, 202, static_cast<int>(148 * score_fill), 16};
    SDL_RenderFillRect(r, &score_bar);

    // Money indicator
    SDL_SetRenderDrawColor(r, 40, 40, 40, 180);
    SDL_Rect money_bg = {Window::WIDTH - 162, 224, 152, 20};
    SDL_RenderFillRect(r, &money_bg);
    float money_fill = clamp(static_cast<float>(player_.money()) / 50000.0f, 0.0f, 1.0f);
    SDL_SetRenderDrawColor(r, 50, 200, 50, 255);
    SDL_Rect money_bar = {Window::WIDTH - 160, 226, static_cast<int>(148 * money_fill), 16};
    SDL_RenderFillRect(r, &money_bar);

    // Vehicle HUD
    if (current_vehicle_) {
        float speed = current_vehicle_->state().velocity.length();
        int speed_w = static_cast<int>(clamp(speed / 3.0f, 0.0f, 200.0f));

        SDL_SetRenderDrawColor(r, 40, 40, 40, 200);
        SDL_Rect speed_bg = {10, Window::HEIGHT - 40, 204, 20};
        SDL_RenderFillRect(r, &speed_bg);
        SDL_SetRenderDrawColor(r, 40, 200, 40, 255);
        SDL_Rect speed_bar = {12, Window::HEIGHT - 38, speed_w, 16};
        SDL_RenderFillRect(r, &speed_bar);
        SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
        SDL_RenderDrawRect(r, &speed_bg);

        int gear = current_vehicle_->state().current_gear;
        for (int g = 0; g < 3; g++) {
            SDL_Rect gear_box = {220 + g * 22, Window::HEIGHT - 40, 18, 20};
            if (g + 1 == gear) {
                SDL_SetRenderDrawColor(r, 40, 200, 40, 255);
                SDL_RenderFillRect(r, &gear_box);
            }
            SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
            SDL_RenderDrawRect(r, &gear_box);
        }

        if (current_vehicle_->state().handbrake_on) {
            SDL_SetRenderDrawColor(r, 255, 60, 60, 255);
            SDL_Rect hb = {290, Window::HEIGHT - 40, 24, 20};
            SDL_RenderFillRect(r, &hb);
        }

        if (current_vehicle_->state().is_skidding) {
            SDL_SetRenderDrawColor(r, 255, 200, 0, 200);
            SDL_Rect skid = {318, Window::HEIGHT - 40, 24, 20};
            SDL_RenderFillRect(r, &skid);
        }

        // Vehicle HP
        if (current_vehicle_->hp() < current_vehicle_->params().max_hp) {
            float vhp_pct = static_cast<float>(current_vehicle_->hp()) / current_vehicle_->params().max_hp;
            SDL_SetRenderDrawColor(r, 40, 40, 40, 200);
            SDL_Rect vhp_bg = {10, Window::HEIGHT - 64, 204, 16};
            SDL_RenderFillRect(r, &vhp_bg);
            SDL_SetRenderDrawColor(r, 200, 100, 40, 255);
            SDL_Rect vhp_bar = {12, Window::HEIGHT - 62, static_cast<int>(200 * vhp_pct), 12};
            SDL_RenderFillRect(r, &vhp_bar);
        }
    }

    // Enter vehicle hint
    if (!current_vehicle_ && !player_.is_wasted() && !player_.is_busted()) {
        for (auto& v : vehicles_) {
            if (v->can_enter(player_.position())) {
                Vec2 screen = camera_.world_to_screen(v->position());
                SDL_SetRenderDrawColor(r, 255, 255, 255, 200);
                SDL_Rect hint = {
                    static_cast<int>(screen.x - 14),
                    static_cast<int>(screen.y - v->height() * 0.5f - 18),
                    28, 14
                };
                SDL_RenderFillRect(r, &hint);
                SDL_SetRenderDrawColor(r, 40, 40, 40, 255);
                SDL_Rect fkey = {hint.x + 8, hint.y + 2, 12, 10};
                SDL_RenderFillRect(r, &fkey);
                break;
            }
        }
    }
}

void Game::render_minimap(SDL_Renderer* r) {
    int mm_size = 150;
    int mm_x = Window::WIDTH - mm_size - 10;
    int mm_y = 40;

    SDL_SetRenderDrawColor(r, 20, 20, 20, 180);
    SDL_Rect mm_bg = {mm_x - 2, mm_y - 2, mm_size + 4, mm_size + 4};
    SDL_RenderFillRect(r, &mm_bg);

    float mm_scale = static_cast<float>(mm_size) / map_.pixel_width();
    for (int ty = 0; ty < map_.height(); ty++) {
        for (int tx = 0; tx < map_.width(); tx++) {
            TileInfo info = map_.get_tile_info_at(tx, ty);
            int px = mm_x + static_cast<int>(tx * Map::TILE_SIZE * mm_scale);
            int py = mm_y + static_cast<int>(ty * Map::TILE_SIZE * mm_scale);
            int ps = std::max(1, static_cast<int>(Map::TILE_SIZE * mm_scale));

            SDL_SetRenderDrawColor(r, info.color.r / 2, info.color.g / 2, info.color.b / 2, 255);
            SDL_Rect dot = {px, py, ps, ps};
            SDL_RenderFillRect(r, &dot);
        }
    }

    // Player dot
    int pp_x = mm_x + static_cast<int>(player_.position().x * mm_scale);
    int pp_y = mm_y + static_cast<int>(player_.position().y * mm_scale);
    SDL_SetRenderDrawColor(r, 255, 255, 50, 255);
    SDL_Rect player_dot = {pp_x - 2, pp_y - 2, 5, 5};
    SDL_RenderFillRect(r, &player_dot);

    // Cop dots (flashing blue)
    static int cop_flash = 0;
    cop_flash++;
    if (cop_flash % 10 < 6) {
        for (auto& cop : police_ai_.cops()) {
            if (!cop.alive) continue;
            int cx = mm_x + static_cast<int>(cop.position.x * mm_scale);
            int cy = mm_y + static_cast<int>(cop.position.y * mm_scale);
            SDL_SetRenderDrawColor(r, 80, 80, 255, 255);
            SDL_Rect cop_dot = {cx - 1, cy - 1, 3, 3};
            SDL_RenderFillRect(r, &cop_dot);
        }
    }

    SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
    SDL_RenderDrawRect(r, &mm_bg);
}

void Game::render_overlay(SDL_Renderer* r) {
    // WASTED overlay
    if (player_.is_wasted()) {
        SDL_SetRenderDrawColor(r, 150, 0, 0, 140);
        SDL_Rect overlay = {0, 0, Window::WIDTH, Window::HEIGHT};
        SDL_RenderFillRect(r, &overlay);

        // Big "WASTED" text box
        SDL_SetRenderDrawColor(r, 200, 30, 30, 255);
        SDL_Rect text_bg = {Window::WIDTH / 2 - 120, Window::HEIGHT / 2 - 30, 240, 60};
        SDL_RenderFillRect(r, &text_bg);
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_RenderDrawRect(r, &text_bg);

        // "W" shape inside
        int cx = Window::WIDTH / 2;
        int cy = Window::HEIGHT / 2;
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        // Crude "WASTED" letters as lines
        // W
        SDL_RenderDrawLine(r, cx-80, cy-10, cx-75, cy+10);
        SDL_RenderDrawLine(r, cx-75, cy+10, cx-70, cy-2);
        SDL_RenderDrawLine(r, cx-70, cy-2, cx-65, cy+10);
        SDL_RenderDrawLine(r, cx-65, cy+10, cx-60, cy-10);
    }

    // BUSTED overlay
    if (player_.is_busted()) {
        SDL_SetRenderDrawColor(r, 0, 0, 150, 140);
        SDL_Rect overlay = {0, 0, Window::WIDTH, Window::HEIGHT};
        SDL_RenderFillRect(r, &overlay);

        SDL_SetRenderDrawColor(r, 30, 30, 200, 255);
        SDL_Rect text_bg = {Window::WIDTH / 2 - 120, Window::HEIGHT / 2 - 30, 240, 60};
        SDL_RenderFillRect(r, &text_bg);
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_RenderDrawRect(r, &text_bg);

        // "B" shape inside
        int cx = Window::WIDTH / 2;
        int cy = Window::HEIGHT / 2;
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_RenderDrawLine(r, cx-10, cy-12, cx-10, cy+12);
        SDL_RenderDrawLine(r, cx-10, cy-12, cx+5, cy-12);
        SDL_RenderDrawLine(r, cx+5, cy-12, cx+10, cy-6);
        SDL_RenderDrawLine(r, cx+10, cy-6, cx-10, cy);
        SDL_RenderDrawLine(r, cx-10, cy, cx+10, cy+6);
        SDL_RenderDrawLine(r, cx+10, cy+6, cx+5, cy+12);
        SDL_RenderDrawLine(r, cx+5, cy+12, cx-10, cy+12);
    }
}

void Game::shutdown() {
    vehicles_.clear();
    weapon_system_.clear();
    police_ai_.clear();
    sprites_.shutdown();
    window_.shutdown();
    SDL_Quit();
}
