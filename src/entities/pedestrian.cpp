#include "entities/pedestrian.h"
#include "world/map.h"
#include "core/camera.h"
#include "core/sprite.h"
#include <cstdlib>
#include <cmath>

Pedestrian::Pedestrian() {
    width_ = 8.0f;
    height_ = 8.0f;

    // Random clothing color
    int r = 50 + std::rand() % 200;
    int g = 50 + std::rand() % 200;
    int b = 50 + std::rand() % 200;
    shirt_color_ = {static_cast<Uint8>(r), static_cast<Uint8>(g), static_cast<Uint8>(b), 255};
    color_ = shirt_color_;

    state_timer_ = 1.0f + static_cast<float>(std::rand() % 30) / 10.0f;
}

void Pedestrian::update(float dt) {
    if (!active_) return;

    anim_time_ += dt;

    switch (state_) {
        case PedState::IDLE:   update_idle(dt);   break;
        case PedState::WANDER: update_wander(dt); break;
        case PedState::FLEE:   update_flee(dt);   break;
        case PedState::DEAD:   update_dead(dt);   break;
    }

    // Update facing direction from velocity
    if (vel_.length_sq() > 1.0f) {
        if (std::abs(vel_.x) > std::abs(vel_.y)) {
            facing_dir_ = (vel_.x < 0) ? 1 : 2;
        } else {
            facing_dir_ = (vel_.y < 0) ? 3 : 0;
        }
    }
}

void Pedestrian::update_idle(float dt) {
    state_timer_ -= dt;
    vel_ = {0, 0};

    if (state_timer_ <= 0.0f) {
        state_ = PedState::WANDER;
        pick_wander_target();
    }
}

void Pedestrian::update_wander(float dt) {
    Vec2 diff = wander_target_ - pos_;
    float dist = diff.length();

    if (dist < 5.0f) {
        // Reached target
        state_ = PedState::IDLE;
        state_timer_ = 1.0f + static_cast<float>(std::rand() % 30) / 10.0f;
        vel_ = {0, 0};
        return;
    }

    Vec2 dir = diff.normalized();
    vel_ = dir * walk_speed_;
    angle_ = std::atan2(dir.y, dir.x);
    pos_ += vel_ * dt;

    // Basic map collision - stop and pick new target if hitting a wall
    if (map_ && map_->is_solid(pos_.x, pos_.y)) {
        pos_ -= vel_ * dt; // undo
        pick_wander_target();
    }

    state_timer_ -= dt;
    if (state_timer_ <= 0.0f) {
        // Timeout — pick new target
        pick_wander_target();
    }
}

void Pedestrian::update_flee(float dt) {
    vel_ = flee_direction_ * run_speed_;
    pos_ += vel_ * dt;
    angle_ = std::atan2(flee_direction_.y, flee_direction_.x);

    // Basic map collision
    if (map_ && map_->is_solid(pos_.x, pos_.y)) {
        pos_ -= vel_ * dt;
        // Change direction
        flee_direction_ = {-flee_direction_.y, flee_direction_.x};
    }

    state_timer_ -= dt;
    if (state_timer_ <= 0.0f) {
        state_ = PedState::WANDER;
        pick_wander_target();
    }
}

void Pedestrian::update_dead(float dt) {
    vel_ = {0, 0};
    death_anim_time_ += dt;
    corpse_timer_ -= dt;
    if (corpse_timer_ <= 0.0f) {
        active_ = false;
    }
}

void Pedestrian::pick_wander_target() {
    // Pick random point within ~5 tiles
    float range = 5.0f * Map::TILE_SIZE;
    float rx = pos_.x + (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 2.0f * range;
    float ry = pos_.y + (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 2.0f * range;

    // Try to find a walkable position (sidewalk or road)
    if (map_) {
        int tx = static_cast<int>(rx) / Map::TILE_SIZE;
        int ty = static_cast<int>(ry) / Map::TILE_SIZE;
        TileType tile = map_->get_tile(tx, ty);
        if (tile == TileType::SIDEWALK || tile == TileType::ROAD || tile == TileType::GRASS) {
            wander_target_ = {rx, ry};
        } else {
            // Stay near current position
            wander_target_ = pos_ + Vec2(
                (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 100.0f,
                (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * 100.0f
            );
        }
    } else {
        wander_target_ = {rx, ry};
    }

    state_timer_ = 3.0f + static_cast<float>(std::rand() % 50) / 10.0f;
}

void Pedestrian::flee_from(Vec2 threat_pos) {
    if (state_ == PedState::DEAD) return;

    Vec2 away = (pos_ - threat_pos).normalized();
    flee_direction_ = away;
    state_ = PedState::FLEE;
    state_timer_ = 3.0f + static_cast<float>(std::rand() % 20) / 10.0f;
}

void Pedestrian::kill() {
    state_ = PedState::DEAD;
    health_ = 0;
    corpse_timer_ = 10.0f;
    death_anim_time_ = 0.0f;
    color_ = {150, 30, 30, 200};
}

void Pedestrian::take_damage(int amount) {
    health_ -= amount;
    if (health_ <= 0) {
        kill();
    }
}

void Pedestrian::render(SDL_Renderer* renderer, const Camera& camera) const {
    if (!active_) return;

    Vec2 screen = camera.world_to_screen(pos_);

    if (state_ == PedState::DEAD) {
        // Corpse — flat dark rectangle
        SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b, color_.a);
        SDL_Rect dst = {
            static_cast<int>(screen.x - 5),
            static_cast<int>(screen.y - 3),
            10, 6
        };
        SDL_RenderFillRect(renderer, &dst);

        // Blood
        SDL_SetRenderDrawColor(renderer, 150, 20, 20, 120);
        SDL_Rect blood = {dst.x - 2, dst.y - 1, 14, 8};
        SDL_RenderFillRect(renderer, &blood);
        return;
    }

    // Shadow
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 40);
    SDL_Rect shadow = {
        static_cast<int>(screen.x - width_ * 0.5f + 1),
        static_cast<int>(screen.y - height_ * 0.5f + 1),
        static_cast<int>(width_),
        static_cast<int>(height_)
    };
    SDL_RenderFillRect(renderer, &shadow);

    // Body (shirt color)
    SDL_SetRenderDrawColor(renderer, shirt_color_.r, shirt_color_.g, shirt_color_.b, 255);
    SDL_Rect body = {
        static_cast<int>(screen.x - width_ * 0.5f),
        static_cast<int>(screen.y - height_ * 0.5f),
        static_cast<int>(width_),
        static_cast<int>(height_)
    };
    SDL_RenderFillRect(renderer, &body);

    // Head (skin color, smaller square on top based on facing)
    SDL_SetRenderDrawColor(renderer, 230, 180, 140, 255);
    Vec2 head_dir = Vec2::from_angle(angle_);
    SDL_Rect head = {
        static_cast<int>(screen.x + head_dir.x * 3 - 2),
        static_cast<int>(screen.y + head_dir.y * 3 - 2),
        4, 4
    };
    SDL_RenderFillRect(renderer, &head);
}

void Pedestrian::render_sprite(SpriteManager& sprites, const Camera& camera) const {
    if (!active_) return;

    Vec2 screen = camera.world_to_screen(pos_);

    if (state_ == PedState::DEAD) {
        sprites.draw_character_death("civilian", screen, death_anim_time_, 0.4f);
        return;
    }

    bool moving = vel_.length_sq() > 1.0f;
    bool running = (state_ == PedState::FLEE);  // run when fleeing, walk otherwise

    sprites.draw_character("civilian", screen, facing_dir_, anim_time_, moving, 0.4f, running);
}
