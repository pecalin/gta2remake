#include "core/sprite.h"
#include "util/math_utils.h"
#include <SDL_image.h>
#include <cstdio>
#include <cmath>

bool SpriteManager::init(SDL_Renderer* renderer) {
    renderer_ = renderer;

    // Init SDL_image for PNG loading
    int img_flags = IMG_INIT_PNG;
    if (!(IMG_Init(img_flags) & img_flags)) {
        std::fprintf(stderr, "SDL_image init failed: %s\n", IMG_GetError());
        return false;
    }

    return true;
}

void SpriteManager::shutdown() {
    for (auto& pair : textures_) {
        if (pair.second) SDL_DestroyTexture(pair.second);
    }
    textures_.clear();
    characters_.clear();
    vehicles_.clear();
    IMG_Quit();
}

SDL_Texture* SpriteManager::load_texture(const std::string& path) {
    // Check cache
    auto it = textures_.find(path);
    if (it != textures_.end()) return it->second;

    SDL_Texture* tex = IMG_LoadTexture(renderer_, path.c_str());
    if (!tex) {
        std::fprintf(stderr, "Failed to load texture: %s (%s)\n", path.c_str(), IMG_GetError());
        return nullptr;
    }

    textures_[path] = tex;
    return tex;
}

bool SpriteManager::load_character(const std::string& name, const std::string& path,
                                    int frame_w, int frame_h, int walk_frames,
                                    int row_up, int row_left, int row_down, int row_right,
                                    int death_row, int death_frames,
                                    int run_row_up, int run_row_left,
                                    int run_row_down, int run_row_right,
                                    int run_frames) {
    SDL_Texture* tex = load_texture(path);
    if (!tex) return false;

    CharacterSprite cs;
    cs.frame_w = frame_w;
    cs.frame_h = frame_h;
    cs.loaded = true;

    // Map direction indices to rows
    // 0=down, 1=left, 2=right, 3=up
    int walk_rows[4] = {row_down, row_left, row_right, row_up};
    int run_rows[4] = {run_row_down, run_row_left, run_row_right, run_row_up};

    for (int dir = 0; dir < 4; dir++) {
        // Walk animation
        int row = walk_rows[dir];
        cs.walk[dir].frames.resize(walk_frames);
        cs.walk[dir].frame_duration = 0.12f;

        for (int f = 0; f < walk_frames; f++) {
            cs.walk[dir].frames[f].texture = tex;
            cs.walk[dir].frames[f].src = {
                f * frame_w,
                row * frame_h,
                frame_w,
                frame_h
            };
        }

        // Idle: first frame of walk
        cs.idle[dir].frames.resize(1);
        cs.idle[dir].frames[0].texture = tex;
        cs.idle[dir].frames[0].src = {
            0,
            row * frame_h,
            frame_w,
            frame_h
        };

        // Run animation (if provided)
        if (run_rows[dir] >= 0 && run_frames > 0) {
            int rrow = run_rows[dir];
            cs.run[dir].frames.resize(run_frames);
            cs.run[dir].frame_duration = 0.08f;  // faster than walk

            for (int f = 0; f < run_frames; f++) {
                cs.run[dir].frames[f].texture = tex;
                cs.run[dir].frames[f].src = {
                    f * frame_w,
                    rrow * frame_h,
                    frame_w,
                    frame_h
                };
            }
            cs.has_run = true;
        }
    }

    // Death animation
    if (death_row >= 0 && death_frames > 0) {
        cs.death.frames.resize(death_frames);
        cs.death.frame_duration = 0.15f;
        for (int f = 0; f < death_frames; f++) {
            cs.death.frames[f].texture = tex;
            cs.death.frames[f].src = {
                f * frame_w,
                death_row * frame_h,
                frame_w,
                frame_h
            };
        }
    }

    characters_[name] = cs;
    std::fprintf(stderr, "Loaded character sprite: %s (%dx%d, %d walk, %d run, %d death frames)\n",
                 name.c_str(), frame_w, frame_h, walk_frames, run_frames, death_frames);
    return true;
}

bool SpriteManager::load_vehicle(const std::string& name, const std::string& path,
                                  int frame_w, int frame_h, int num_angles) {
    SDL_Texture* tex = load_texture(path);
    if (!tex) return false;

    VehicleSprite vs;
    vs.texture = tex;
    vs.num_angles = num_angles;
    vs.frame_w = frame_w;
    vs.frame_h = frame_h;
    vs.loaded = true;

    vehicles_[name] = vs;
    std::fprintf(stderr, "Loaded vehicle sprite: %s (%dx%d, %d angles)\n",
                 name.c_str(), frame_w, frame_h, num_angles);
    return true;
}

CharacterSprite* SpriteManager::get_character(const std::string& name) {
    auto it = characters_.find(name);
    if (it != characters_.end() && it->second.loaded) return &it->second;
    return nullptr;
}

VehicleSprite* SpriteManager::get_vehicle(const std::string& name) {
    auto it = vehicles_.find(name);
    if (it != vehicles_.end() && it->second.loaded) return &it->second;
    return nullptr;
}

void SpriteManager::draw_character(const std::string& name, Vec2 screen_pos,
                                    int direction, float anim_time, bool moving,
                                    float scale, bool use_run) {
    CharacterSprite* cs = get_character(name);
    if (!cs) return;

    direction = direction & 3;  // clamp to 0-3

    // Pick animation: run > walk > idle
    SpriteAnim* anim_ptr;
    if (!moving) {
        anim_ptr = &cs->idle[direction];
    } else if (use_run && cs->has_run && !cs->run[direction].frames.empty()) {
        anim_ptr = &cs->run[direction];
    } else {
        anim_ptr = &cs->walk[direction];
    }
    SpriteAnim& anim = *anim_ptr;
    if (anim.frames.empty()) return;

    // Pick frame based on time
    int total_frames = static_cast<int>(anim.frames.size());
    int frame_idx = 0;
    if (total_frames > 1 && moving) {
        float cycle_time = total_frames * anim.frame_duration;
        float t = std::fmod(anim_time, cycle_time);
        frame_idx = static_cast<int>(t / anim.frame_duration) % total_frames;
    }

    SpriteFrame& frame = anim.frames[frame_idx];
    if (!frame.texture) return;

    int draw_w = static_cast<int>(cs->frame_w * scale);
    int draw_h = static_cast<int>(cs->frame_h * scale);

    SDL_Rect dst = {
        static_cast<int>(screen_pos.x - draw_w * 0.5f),
        static_cast<int>(screen_pos.y - draw_h * 0.5f),
        draw_w, draw_h
    };

    SDL_RenderCopy(renderer_, frame.texture, &frame.src, &dst);
}

bool SpriteManager::draw_character_death(const std::string& name, Vec2 screen_pos,
                                          float anim_time, float scale) {
    CharacterSprite* cs = get_character(name);
    if (!cs || cs->death.frames.empty()) return false;

    int total_frames = static_cast<int>(cs->death.frames.size());
    float cycle_time = total_frames * cs->death.frame_duration;

    // Clamp to last frame (don't loop death anim)
    int frame_idx;
    if (anim_time >= cycle_time) {
        frame_idx = total_frames - 1;  // stay on last frame
    } else {
        frame_idx = static_cast<int>(anim_time / cs->death.frame_duration);
        if (frame_idx >= total_frames) frame_idx = total_frames - 1;
    }

    SpriteFrame& frame = cs->death.frames[frame_idx];
    if (!frame.texture) return false;

    int draw_w = static_cast<int>(cs->frame_w * scale);
    int draw_h = static_cast<int>(cs->frame_h * scale);

    SDL_Rect dst = {
        static_cast<int>(screen_pos.x - draw_w * 0.5f),
        static_cast<int>(screen_pos.y - draw_h * 0.5f),
        draw_w, draw_h
    };

    SDL_RenderCopy(renderer_, frame.texture, &frame.src, &dst);

    return anim_time < cycle_time;  // true = still playing
}

void SpriteManager::draw_vehicle(const std::string& name, Vec2 screen_pos,
                                  float angle_rad, float scale) {
    VehicleSprite* vs = get_vehicle(name);
    if (!vs || !vs->texture) return;

    if (vs->num_angles <= 1) {
        // Single image — use SDL rotation
        int draw_w = static_cast<int>(vs->frame_w * scale);
        int draw_h = static_cast<int>(vs->frame_h * scale);

        SDL_Rect src = {0, 0, vs->frame_w, vs->frame_h};
        SDL_Rect dst = {
            static_cast<int>(screen_pos.x - draw_w * 0.5f),
            static_cast<int>(screen_pos.y - draw_h * 0.5f),
            draw_w, draw_h
        };

        // Convert radians to degrees for SDL (SDL uses degrees, clockwise)
        double angle_deg = static_cast<double>(angle_rad) * (180.0 / 3.14159265);

        SDL_RenderCopyEx(renderer_, vs->texture, &src, &dst,
                         angle_deg, nullptr, SDL_FLIP_NONE);
    } else {
        // Spritesheet with pre-rotated frames in a single row
        // Frame 0 = facing right (east, angle=0), going clockwise
        float angle_deg = angle_rad * (180.0f / 3.14159265f);
        if (angle_deg < 0) angle_deg += 360.0f;

        float angle_per_frame = 360.0f / vs->num_angles;
        int frame_idx = static_cast<int>(std::round(angle_deg / angle_per_frame)) % vs->num_angles;

        SDL_Rect src = {
            frame_idx * vs->frame_w, 0,
            vs->frame_w, vs->frame_h
        };

        int draw_w = static_cast<int>(vs->frame_w * scale);
        int draw_h = static_cast<int>(vs->frame_h * scale);

        SDL_Rect dst = {
            static_cast<int>(screen_pos.x - draw_w * 0.5f),
            static_cast<int>(screen_pos.y - draw_h * 0.5f),
            draw_w, draw_h
        };

        SDL_RenderCopy(renderer_, vs->texture, &src, &dst);
    }
}
