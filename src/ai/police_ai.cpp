#include "ai/police_ai.h"
#include "entities/player.h"
#include "world/map.h"
#include "systems/weapon_system.h"
#include "core/camera.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

void PoliceAI::init(const Map* map) {
    map_ = map;
}

void PoliceAI::clear() {
    cops_.clear();
    cop_vehicles_.clear();
}

bool PoliceAI::can_see_player(const Player& player) const {
    for (auto& cop : cops_) {
        if (!cop.alive || cop.state == CopState::INACTIVE) continue;
        float dist = (cop.position - player.position()).length();
        if (dist < sight_range_) {
            return true;
        }
    }
    return false;
}

std::vector<Vec2> PoliceAI::get_cop_positions() const {
    std::vector<Vec2> positions;
    for (auto& cop : cops_) {
        if (cop.alive && cop.state != CopState::INACTIVE) {
            positions.push_back(cop.position);
        }
    }
    return positions;
}

void PoliceAI::spawn_response(int wanted_level, Vec2 player_pos) {
    if (wanted_level <= 0) return;

    int active_cops = 0;
    for (auto& cop : cops_) {
        if (cop.alive && cop.state != CopState::INACTIVE) active_cops++;
    }

    int max_cops = max_cops_for_level_[std::min(wanted_level, 6)];
    if (active_cops >= max_cops) return;

    // Spawn cop at random position outside player's view but not too far
    float spawn_dist = 500.0f + static_cast<float>(std::rand() % 300);
    float spawn_angle = static_cast<float>(std::rand()) / RAND_MAX * 6.28318f;

    Vec2 spawn_pos = player_pos + Vec2::from_angle(spawn_angle) * spawn_dist;

    // Make sure spawn position is on a road
    if (map_) {
        int tx = static_cast<int>(spawn_pos.x) / Map::TILE_SIZE;
        int ty = static_cast<int>(spawn_pos.y) / Map::TILE_SIZE;
        TileType tile = map_->get_tile(tx, ty);
        if (tile != TileType::ROAD && tile != TileType::SIDEWALK) {
            // Try to find a nearby road
            for (int dy = -3; dy <= 3; dy++) {
                for (int dx = -3; dx <= 3; dx++) {
                    if (map_->get_tile(tx + dx, ty + dy) == TileType::ROAD) {
                        spawn_pos.x = static_cast<float>((tx + dx) * Map::TILE_SIZE + Map::TILE_SIZE / 2);
                        spawn_pos.y = static_cast<float>((ty + dy) * Map::TILE_SIZE + Map::TILE_SIZE / 2);
                        goto found_road;
                    }
                }
            }
            return; // no road found, skip spawn
            found_road:;
        }
    }

    CopUnit cop;
    cop.position = spawn_pos;
    cop.alive = true;
    cop.health = 80 + wanted_level * 10;

    // At level 1-2: foot cops. Level 3+: car cops.
    if (wanted_level <= 2) {
        cop.state = CopState::PURSUE_FOOT;
        cop.in_vehicle = false;
        cop.color = {50, 50, 220, 255};
    } else {
        cop.state = CopState::PURSUE_CAR;
        cop.in_vehicle = true;
        cop.color = {30, 30, 180, 255};
    }

    cop.last_known_player_pos = player_pos;
    cops_.push_back(cop);
}

void PoliceAI::update_cop(CopUnit& cop, float dt, const Player& player,
                           int wanted_level, WeaponSystem& weapons) {
    if (!cop.alive) return;

    float dist_to_player = (cop.position - player.position()).length();
    cop.fire_cooldown -= dt;

    switch (cop.state) {
        case CopState::PURSUE_FOOT: {
            // Chase player on foot
            Vec2 dir = (player.position() - cop.position).normalized();
            cop.velocity = dir * cop_speed_foot_;
            cop.position += cop.velocity * dt;
            cop.angle = std::atan2(dir.y, dir.x);
            cop.last_known_player_pos = player.position();

            // Map collision (simple - stop at solid tiles)
            if (map_ && map_->is_solid(cop.position.x, cop.position.y)) {
                cop.position -= cop.velocity * dt;
                // Try to go around
                Vec2 alt1 = {dir.y, -dir.x};
                Vec2 test1 = cop.position + alt1 * cop_speed_foot_ * dt;
                if (!map_->is_solid(test1.x, test1.y)) {
                    cop.position = test1;
                }
            }

            // Shoot at player if close enough and wanted level 3+
            if (wanted_level >= 3 && dist_to_player < 250.0f && cop.fire_cooldown <= 0.0f) {
                weapons.fire(WeaponType::PISTOL, cop.position, cop.angle, nullptr);
                cop.fire_cooldown = 0.8f - wanted_level * 0.05f;
            }

            // Arrest attempt: if very close, bust player
            if (dist_to_player < 15.0f && wanted_level <= 2) {
                // TODO: trigger busted
            }

            // If player too far, switch to search
            if (dist_to_player > sight_range_ * 1.5f) {
                cop.state = CopState::SEARCH;
                cop.search_timer = 15.0f;
            }
            break;
        }

        case CopState::PURSUE_CAR: {
            // Chase in car (faster)
            Vec2 dir = (player.position() - cop.position).normalized();
            float chase_speed = cop_speed_car_ + wanted_level * 20.0f;
            cop.velocity = dir * chase_speed;
            cop.position += cop.velocity * dt;
            cop.angle = std::atan2(dir.y, dir.x);
            cop.last_known_player_pos = player.position();

            if (map_ && map_->is_solid(cop.position.x, cop.position.y)) {
                cop.position -= cop.velocity * dt;
                Vec2 alt = {dir.y, -dir.x};
                Vec2 test = cop.position + alt * chase_speed * dt;
                if (!map_->is_solid(test.x, test.y)) {
                    cop.position = test;
                }
            }

            // Shoot from car at level 3+
            if (wanted_level >= 3 && dist_to_player < 300.0f && cop.fire_cooldown <= 0.0f) {
                weapons.fire(WeaponType::MACHINE_GUN, cop.position, cop.angle, nullptr);
                cop.fire_cooldown = 0.5f;
            }

            if (dist_to_player > sight_range_ * 2.0f) {
                cop.state = CopState::SEARCH;
                cop.search_timer = 20.0f;
            }
            break;
        }

        case CopState::SEARCH: {
            // Go to last known position, look around
            Vec2 dir = (cop.last_known_player_pos - cop.position).normalized();
            float search_speed = cop.in_vehicle ? cop_speed_car_ * 0.5f : cop_speed_foot_ * 0.7f;
            float dist_to_target = (cop.last_known_player_pos - cop.position).length();

            if (dist_to_target > 20.0f) {
                cop.velocity = dir * search_speed;
                cop.position += cop.velocity * dt;
                cop.angle = std::atan2(dir.y, dir.x);
            } else {
                cop.velocity = {0, 0};
            }

            // If spotted player again, resume pursuit
            if (dist_to_player < sight_range_) {
                cop.state = cop.in_vehicle ? CopState::PURSUE_CAR : CopState::PURSUE_FOOT;
                break;
            }

            cop.search_timer -= dt;
            if (cop.search_timer <= 0.0f) {
                cop.state = CopState::INACTIVE;
                cop.alive = false;
            }
            break;
        }

        case CopState::PATROL:
        case CopState::INACTIVE:
            break;
    }
}

void PoliceAI::despawn_far_cops(Vec2 player_pos) {
    for (auto& cop : cops_) {
        if (!cop.alive) continue;
        float dist = (cop.position - player_pos).length();
        if (dist > 1500.0f) {
            cop.alive = false;
            cop.state = CopState::INACTIVE;
        }
    }

    // Remove dead cops
    cops_.erase(
        std::remove_if(cops_.begin(), cops_.end(),
                       [](const CopUnit& c) { return !c.alive; }),
        cops_.end());
}

void PoliceAI::update(float dt, const Player& player, int wanted_level,
                       WeaponSystem& weapons) {
    // Spawn cops based on wanted level
    spawn_timer_ -= dt;
    if (spawn_timer_ <= 0.0f && wanted_level > 0) {
        spawn_response(wanted_level, player.position());
        spawn_timer_ = 3.0f - wanted_level * 0.3f;
        if (spawn_timer_ < 1.0f) spawn_timer_ = 1.0f;
    }

    // Update each cop
    for (auto& cop : cops_) {
        update_cop(cop, dt, player, wanted_level, weapons);
    }

    // Despawn far away cops
    despawn_far_cops(player.position());

    // If wanted level drops to 0, deactivate all cops
    if (wanted_level == 0) {
        for (auto& cop : cops_) {
            if (cop.state == CopState::PURSUE_FOOT || cop.state == CopState::PURSUE_CAR) {
                cop.state = CopState::SEARCH;
                cop.search_timer = 5.0f;
            }
        }
    }
}

void PoliceAI::render(SDL_Renderer* renderer, const Camera& camera) const {
    for (auto& cop : cops_) {
        if (!cop.alive) continue;

        Vec2 screen = camera.world_to_screen(cop.position);

        if (cop.in_vehicle) {
            // Cop car: larger blue rectangle
            float hw = 14.0f, hh = 22.0f;
            float cos_a = std::cos(cop.angle);
            float sin_a = std::sin(cop.angle);

            // Draw rotated car body
            for (float t = -hh; t <= hh; t += 1.0f) {
                float x1r = -hw * cos_a - t * sin_a + screen.x;
                float y1r = -hw * sin_a + t * cos_a + screen.y;
                float x2r =  hw * cos_a - t * sin_a + screen.x;
                float y2r =  hw * sin_a + t * cos_a + screen.y;

                // Blue body with white top stripe
                if (std::abs(t) < 5.0f) {
                    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                } else {
                    SDL_SetRenderDrawColor(renderer, cop.color.r, cop.color.g, cop.color.b, 255);
                }
                SDL_RenderDrawLine(renderer,
                    static_cast<int>(x1r), static_cast<int>(y1r),
                    static_cast<int>(x2r), static_cast<int>(y2r));
            }

            // Flashing siren lights (red/blue alternating)
            static int flash_timer = 0;
            flash_timer++;
            SDL_Color siren = (flash_timer % 20 < 10) ?
                SDL_Color{255, 0, 0, 255} : SDL_Color{0, 0, 255, 255};

            float light_x = screen.x - sin_a * 5.0f;
            float light_y = screen.y + cos_a * 5.0f;
            SDL_SetRenderDrawColor(renderer, siren.r, siren.g, siren.b, 200);
            SDL_Rect light = {
                static_cast<int>(light_x - 3), static_cast<int>(light_y - 3), 6, 6
            };
            SDL_RenderFillRect(renderer, &light);
        } else {
            // Cop on foot: small blue rectangle with badge
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 40);
            SDL_Rect shadow = {
                static_cast<int>(screen.x - 5), static_cast<int>(screen.y - 5), 10, 10
            };
            SDL_RenderFillRect(renderer, &shadow);

            SDL_SetRenderDrawColor(renderer, cop.color.r, cop.color.g, cop.color.b, 255);
            SDL_Rect body = {
                static_cast<int>(screen.x - 5), static_cast<int>(screen.y - 6), 10, 12
            };
            SDL_RenderFillRect(renderer, &body);

            // Hat (darker blue cap)
            Vec2 hat_dir = Vec2::from_angle(cop.angle);
            SDL_SetRenderDrawColor(renderer, 20, 20, 120, 255);
            SDL_Rect hat = {
                static_cast<int>(screen.x + hat_dir.x * 4 - 2),
                static_cast<int>(screen.y + hat_dir.y * 4 - 2),
                5, 5
            };
            SDL_RenderFillRect(renderer, &hat);
        }
    }
}
