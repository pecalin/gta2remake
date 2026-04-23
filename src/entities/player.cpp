#include "entities/player.h"
#include "world/map.h"
#include "core/camera.h"
#include <cmath>
#include <algorithm>

Player::Player() {
    width_ = 12.0f;
    height_ = 12.0f;
    color_ = {255, 220, 50, 255};
}

void Player::handle_input(const Input& input, float dt) {
    if (in_vehicle_ || is_dead() || wasted_ || busted_) return;

    Vec2 move_dir = {0, 0};
    if (input.is_down(Action::MOVE_UP))    move_dir.y -= 1.0f;
    if (input.is_down(Action::MOVE_DOWN))  move_dir.y += 1.0f;
    if (input.is_down(Action::MOVE_LEFT))  move_dir.x -= 1.0f;
    if (input.is_down(Action::MOVE_RIGHT)) move_dir.x += 1.0f;

    if (move_dir.length_sq() > 0.01f) {
        move_dir = move_dir.normalized();
        vel_ = move_dir * move_speed_ * speed_mult_;
        angle_ = std::atan2(move_dir.y, move_dir.x);
    } else {
        vel_ = {0, 0};
    }
}

void Player::update(float dt) {
    if (in_vehicle_ || wasted_ || busted_) return;
    pos_ += vel_ * dt;
}

void Player::resolve_map_collision(const Map& map) {
    if (in_vehicle_) return;

    float half_w = width_ * 0.5f;
    float half_h = height_ * 0.5f;

    if (vel_.x > 0) {
        if (map.is_solid(pos_.x + half_w, pos_.y - half_h + 1) ||
            map.is_solid(pos_.x + half_w, pos_.y + half_h - 1)) {
            int tile_x = static_cast<int>(pos_.x + half_w) / Map::TILE_SIZE;
            pos_.x = static_cast<float>(tile_x * Map::TILE_SIZE) - half_w - 0.01f;
            vel_.x = 0;
        }
    } else if (vel_.x < 0) {
        if (map.is_solid(pos_.x - half_w, pos_.y - half_h + 1) ||
            map.is_solid(pos_.x - half_w, pos_.y + half_h - 1)) {
            int tile_x = static_cast<int>(pos_.x - half_w) / Map::TILE_SIZE;
            pos_.x = static_cast<float>((tile_x + 1) * Map::TILE_SIZE) + half_w + 0.01f;
            vel_.x = 0;
        }
    }

    if (vel_.y > 0) {
        if (map.is_solid(pos_.x - half_w + 1, pos_.y + half_h) ||
            map.is_solid(pos_.x + half_w - 1, pos_.y + half_h)) {
            int tile_y = static_cast<int>(pos_.y + half_h) / Map::TILE_SIZE;
            pos_.y = static_cast<float>(tile_y * Map::TILE_SIZE) - half_h - 0.01f;
            vel_.y = 0;
        }
    } else if (vel_.y < 0) {
        if (map.is_solid(pos_.x - half_w + 1, pos_.y - half_h) ||
            map.is_solid(pos_.x + half_w - 1, pos_.y - half_h)) {
            int tile_y = static_cast<int>(pos_.y - half_h) / Map::TILE_SIZE;
            pos_.y = static_cast<float>((tile_y + 1) * Map::TILE_SIZE) + half_h + 0.01f;
            vel_.y = 0;
        }
    }
}

void Player::heal(int amount) {
    health_ = std::min(100, health_ + amount);
}

void Player::add_armor(int amount) {
    armor_ = std::min(100, armor_ + amount);
}

void Player::take_damage(int amount) {
    if (wasted_ || busted_) return;

    // Armor absorbs damage first
    if (armor_ > 0) {
        int armor_absorbed = std::min(armor_, amount);
        armor_ -= armor_absorbed;
        amount -= armor_absorbed;
    }

    health_ -= amount;
    if (health_ <= 0) {
        health_ = 0;
    }
}

void Player::respawn(Vec2 pos) {
    pos_ = pos;
    vel_ = {0, 0};
    health_ = 100;
    armor_ = 0;
    wasted_ = false;
    busted_ = false;
    wasted_timer_ = 0.0f;
    busted_timer_ = 0.0f;
    in_vehicle_ = false;
}

void Player::update_overlay_timers(float dt) {
    if (wasted_) {
        wasted_timer_ -= dt;
    }
    if (busted_) {
        busted_timer_ -= dt;
    }
}

void Player::render(SDL_Renderer* renderer, const Camera& camera) const {
    if (!active_ || in_vehicle_) return;

    Vec2 screen = camera.world_to_screen(pos_);

    // Shadow
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 60);
    SDL_Rect shadow = {
        static_cast<int>(screen.x - width_ * 0.5f + 2),
        static_cast<int>(screen.y - height_ * 0.5f + 2),
        static_cast<int>(width_),
        static_cast<int>(height_)
    };
    SDL_RenderFillRect(renderer, &shadow);

    // Body
    SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b, color_.a);
    SDL_Rect body = {
        static_cast<int>(screen.x - width_ * 0.5f),
        static_cast<int>(screen.y - height_ * 0.5f),
        static_cast<int>(width_),
        static_cast<int>(height_)
    };
    SDL_RenderFillRect(renderer, &body);

    // Direction indicator
    Vec2 dir = Vec2::from_angle(angle_);
    int lx = static_cast<int>(screen.x + dir.x * width_ * 0.7f);
    int ly = static_cast<int>(screen.y + dir.y * height_ * 0.7f);
    SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
    SDL_RenderDrawLine(renderer,
        static_cast<int>(screen.x), static_cast<int>(screen.y),
        lx, ly);
}
