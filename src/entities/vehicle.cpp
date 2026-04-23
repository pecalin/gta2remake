#include "entities/vehicle.h"
#include "entities/player.h"
#include "world/map.h"
#include "core/input.h"
#include "core/camera.h"
#include <cmath>

Vehicle::Vehicle() {
    width_ = 28.0f;
    height_ = 48.0f;
    params_ = VehicleTypes::sedan();
    hp_ = params_.max_hp;
}

Vehicle::Vehicle(const VehicleParams& params) : params_(params) {
    hp_ = params_.max_hp;
    // Size varies by vehicle type
    if (params_.mass > 25.0f) {
        // Large vehicles (bus, truck)
        width_ = 32.0f;
        height_ = 64.0f;
    } else if (params_.mass > 18.0f) {
        // Medium-large (van, pacifier)
        width_ = 30.0f;
        height_ = 52.0f;
    } else if (params_.mass < 10.0f) {
        // Small (bug, compact)
        width_ = 22.0f;
        height_ = 38.0f;
    } else {
        // Normal sedan/sports
        width_ = 28.0f;
        height_ = 48.0f;
    }
}

void Vehicle::handle_input(const Input& input, float dt) {
    DriveMods default_mods;
    handle_input(input, dt, default_mods);
}

void Vehicle::handle_input(const Input& input, float dt, const DriveMods& mods) {
    if (!driver_) return;

    float steer = 0.0f;
    float accel = 0.0f;
    float brake = 0.0f;
    bool handbrake = false;

    if (input.is_down(Action::MOVE_LEFT))  steer -= 1.0f;
    if (input.is_down(Action::MOVE_RIGHT)) steer += 1.0f;
    if (input.is_down(Action::MOVE_UP))    accel = 1.0f;
    if (input.is_down(Action::MOVE_DOWN))  brake = 1.0f;
    if (input.is_down(Action::HANDBRAKE))  handbrake = true;

    // Apply dev modifiers to a copy of params
    VehicleParams modded = params_;
    modded.thrust *= mods.accel;
    modded.brake_friction *= mods.brake;
    modded.turn_ratio *= mods.turn;
    modded.turn_in *= mods.turn;
    modded.max_speed *= mods.max_speed;
    modded.handbrake_slide *= mods.drift;

    VehiclePhysics::update(state_, modded, steer, accel, brake, handbrake, dt);

    // Sync entity state from physics state
    pos_ = state_.position;
    vel_ = state_.velocity;
    angle_ = state_.angle;
}

void Vehicle::update(float dt) {
    if (on_fire_) {
        fire_timer_ -= dt;
        if (fire_timer_ <= 0.0f) {
            hp_ = 0; // explode
            on_fire_ = false;
            active_ = false;
        }
    }

    // Update driver position to match vehicle
    if (driver_) {
        driver_->set_position(pos_);
        driver_->set_angle(angle_);
    }
}

void Vehicle::render(SDL_Renderer* renderer, const Camera& camera) const {
    if (!active_) return;

    Vec2 screen = camera.world_to_screen(pos_);

    // Shadow (rotated to match car body, offset down-right)
    {
        float s_cos = std::cos(angle_);
        float s_sin = std::sin(angle_);
        float s_half_len = height_ * 0.5f;
        float s_half_wid = width_ * 0.5f;
        float s_fwd_x = s_cos, s_fwd_y = s_sin;
        float s_rgt_x = -s_sin, s_rgt_y = s_cos;
        float s_cx = screen.x + 3.0f;
        float s_cy = screen.y + 3.0f;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 50);
        for (float t = -s_half_len; t <= s_half_len; t += 1.0f) {
            float lx = s_cx + s_fwd_x * t;
            float ly = s_cy + s_fwd_y * t;
            SDL_RenderDrawLine(renderer,
                static_cast<int>(lx - s_rgt_x * s_half_wid),
                static_cast<int>(ly - s_rgt_y * s_half_wid),
                static_cast<int>(lx + s_rgt_x * s_half_wid),
                static_cast<int>(ly + s_rgt_y * s_half_wid));
        }
    }

    // Draw a rotated rectangle using scanlines.
    // The car's forward direction is along `angle_`.
    // `height_` is the car's LENGTH (along forward), `width_` is the car's WIDTH (side to side).
    // Forward vector = along angle, Right vector = perpendicular to angle.
    float cos_a = std::cos(angle_);
    float sin_a = std::sin(angle_);

    // half-length along forward, half-width perpendicular
    float half_len = height_ * 0.5f;   // long axis = forward
    float half_wid = width_ * 0.5f;    // short axis = sideways

    // Forward direction unit vector
    float fwd_x = cos_a;
    float fwd_y = sin_a;
    // Right direction unit vector (perpendicular)
    float rgt_x = -sin_a;
    float rgt_y =  cos_a;

    // Four corners: forward/back * right/left
    Vec2 corners[4] = {
        {screen.x - fwd_x*half_len - rgt_x*half_wid, screen.y - fwd_y*half_len - rgt_y*half_wid},
        {screen.x - fwd_x*half_len + rgt_x*half_wid, screen.y - fwd_y*half_len + rgt_y*half_wid},
        {screen.x + fwd_x*half_len + rgt_x*half_wid, screen.y + fwd_y*half_len + rgt_y*half_wid},
        {screen.x + fwd_x*half_len - rgt_x*half_wid, screen.y + fwd_y*half_len - rgt_y*half_wid},
    };

    SDL_Point points[5];
    for (int i = 0; i < 4; i++)
        points[i] = {static_cast<int>(corners[i].x), static_cast<int>(corners[i].y)};
    points[4] = points[0];

    SDL_Color c = body_color_;
    if (on_fire_) {
        static int flicker = 0;
        flicker++;
        c = (flicker % 4 < 2) ? SDL_Color{255, 100, 0, 255} : SDL_Color{255, 30, 0, 255};
    }

    // Fill: sweep scanlines along the forward axis, each line spans the width
    for (float t = -half_len; t <= half_len; t += 1.0f) {
        float cx = screen.x + fwd_x * t;
        float cy = screen.y + fwd_y * t;
        float x1 = cx - rgt_x * half_wid;
        float y1 = cy - rgt_y * half_wid;
        float x2 = cx + rgt_x * half_wid;
        float y2 = cy + rgt_y * half_wid;

        // Windshield stripe near front (lighter color)
        if (t > half_len * 0.35f && t < half_len * 0.55f) {
            SDL_SetRenderDrawColor(renderer,
                static_cast<Uint8>(std::min(255, c.r + 80)),
                static_cast<Uint8>(std::min(255, c.g + 80)),
                static_cast<Uint8>(std::min(255, c.b + 100)), 255);
        } else {
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        }
        SDL_RenderDrawLine(renderer,
            static_cast<int>(x1), static_cast<int>(y1),
            static_cast<int>(x2), static_cast<int>(y2));
    }

    // Outline
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderDrawLines(renderer, points, 5);

    // Headlights at front
    float nose_x = screen.x + fwd_x * half_len;
    float nose_y = screen.y + fwd_y * half_len;
    SDL_SetRenderDrawColor(renderer, 255, 255, 200, 200);
    // Two small headlight dots
    float hl_offset = half_wid * 0.6f;
    SDL_Rect hl1 = {
        static_cast<int>(nose_x - rgt_x * hl_offset - 1),
        static_cast<int>(nose_y - rgt_y * hl_offset - 1), 3, 3
    };
    SDL_Rect hl2 = {
        static_cast<int>(nose_x + rgt_x * hl_offset - 1),
        static_cast<int>(nose_y + rgt_y * hl_offset - 1), 3, 3
    };
    SDL_RenderFillRect(renderer, &hl1);
    SDL_RenderFillRect(renderer, &hl2);

    // Tail lights at back (red)
    float tail_x = screen.x - fwd_x * half_len;
    float tail_y = screen.y - fwd_y * half_len;
    SDL_SetRenderDrawColor(renderer, 255, 30, 30, 200);
    SDL_Rect tl1 = {
        static_cast<int>(tail_x - rgt_x * hl_offset - 1),
        static_cast<int>(tail_y - rgt_y * hl_offset - 1), 3, 3
    };
    SDL_Rect tl2 = {
        static_cast<int>(tail_x + rgt_x * hl_offset - 1),
        static_cast<int>(tail_y + rgt_y * hl_offset - 1), 3, 3
    };
    SDL_RenderFillRect(renderer, &tl1);
    SDL_RenderFillRect(renderer, &tl2);

    // HP bar above vehicle (if damaged)
    if (hp_ < params_.max_hp) {
        int bar_w = static_cast<int>(width_);
        int bar_h = 4;
        float hp_ratio = static_cast<float>(hp_) / params_.max_hp;

        SDL_Rect bg = {
            static_cast<int>(screen.x - bar_w * 0.5f),
            static_cast<int>(screen.y - half_len - 10),
            bar_w, bar_h
        };
        SDL_SetRenderDrawColor(renderer, 60, 0, 0, 180);
        SDL_RenderFillRect(renderer, &bg);

        SDL_Rect hp_bar = {bg.x, bg.y, static_cast<int>(bar_w * hp_ratio), bar_h};
        if (hp_ratio > 0.5f)
            SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
        else if (hp_ratio > 0.25f)
            SDL_SetRenderDrawColor(renderer, 255, 200, 0, 255);
        else
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &hp_bar);
    }
}

void Vehicle::resolve_map_collision(const Map& map) {
    // Check corners of vehicle bounding box against solid tiles
    float hw = width_ * 0.5f;
    float hh = height_ * 0.5f;

    // Simple AABB collision (ignoring rotation for now)
    // Check right edge
    if (vel_.x > 0 &&
        (map.is_solid(pos_.x + hw, pos_.y - hh + 2) ||
         map.is_solid(pos_.x + hw, pos_.y + hh - 2))) {
        int tx = static_cast<int>(pos_.x + hw) / Map::TILE_SIZE;
        pos_.x = static_cast<float>(tx * Map::TILE_SIZE) - hw - 0.1f;
        state_.velocity.x *= -0.3f;  // bounce
        state_.position = pos_;
    }
    // Left edge
    if (vel_.x < 0 &&
        (map.is_solid(pos_.x - hw, pos_.y - hh + 2) ||
         map.is_solid(pos_.x - hw, pos_.y + hh - 2))) {
        int tx = static_cast<int>(pos_.x - hw) / Map::TILE_SIZE;
        pos_.x = static_cast<float>((tx + 1) * Map::TILE_SIZE) + hw + 0.1f;
        state_.velocity.x *= -0.3f;
        state_.position = pos_;
    }
    // Bottom edge
    if (vel_.y > 0 &&
        (map.is_solid(pos_.x - hw + 2, pos_.y + hh) ||
         map.is_solid(pos_.x + hw - 2, pos_.y + hh))) {
        int ty = static_cast<int>(pos_.y + hh) / Map::TILE_SIZE;
        pos_.y = static_cast<float>(ty * Map::TILE_SIZE) - hh - 0.1f;
        state_.velocity.y *= -0.3f;
        state_.position = pos_;
    }
    // Top edge
    if (vel_.y < 0 &&
        (map.is_solid(pos_.x - hw + 2, pos_.y - hh) ||
         map.is_solid(pos_.x + hw - 2, pos_.y - hh))) {
        int ty = static_cast<int>(pos_.y - hh) / Map::TILE_SIZE;
        pos_.y = static_cast<float>((ty + 1) * Map::TILE_SIZE) + hh + 0.1f;
        state_.velocity.y *= -0.3f;
        state_.position = pos_;
    }

    vel_ = state_.velocity;
}

bool Vehicle::can_enter(const Vec2& player_pos) const {
    if (!active_ || is_destroyed() || driver_) return false;
    Vec2 diff = player_pos - pos_;
    return diff.length() < enter_radius_;
}

void Vehicle::enter(Player* driver) {
    driver_ = driver;
    driver_->set_in_vehicle(true);
    driver_->set_position(pos_);
    state_.position = pos_;
    state_.velocity = vel_;
    state_.angle = angle_;
}

void Vehicle::exit_vehicle(Player* driver) {
    if (!driver_) return;

    // Place player beside the vehicle
    Vec2 right = {-std::sin(angle_), std::cos(angle_)};
    Vec2 exit_pos = pos_ + right * (width_ * 0.5f + 10.0f);
    driver_->set_position(exit_pos);
    driver_->set_velocity({0, 0});
    driver_->set_in_vehicle(false);
    driver_ = nullptr;
}

void Vehicle::take_damage(int amount) {
    hp_ -= amount;
    if (hp_ <= params_.max_hp / 4 && hp_ > 0 && !on_fire_) {
        on_fire_ = true;
        fire_timer_ = 5.0f; // 5 seconds until explosion
    }
}
