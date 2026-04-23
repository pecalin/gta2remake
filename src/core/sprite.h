#pragma once
#include <SDL.h>
#include "util/math_utils.h"
#include <string>
#include <unordered_map>
#include <vector>

// A single sprite frame (region within a texture)
struct SpriteFrame {
    SDL_Texture* texture = nullptr;
    SDL_Rect src = {0, 0, 0, 0};  // source rect within the texture
};

// Animated sprite: a set of frames for one direction/action
struct SpriteAnim {
    std::vector<SpriteFrame> frames;
    float frame_duration = 0.1f;  // seconds per frame
};

// Character spritesheet: animations for each direction
// LPC format: rows for walk (4 directions) + optional death row
struct CharacterSprite {
    // Indexed by direction: 0=down, 1=left, 2=right, 3=up
    SpriteAnim walk[4];
    SpriteAnim run[4];
    SpriteAnim idle[4];  // first frame of walk as idle
    SpriteAnim death;
    bool has_run = false;
    int frame_w = 64;
    int frame_h = 64;
    bool loaded = false;
};

// Vehicle sprite: one image per rotation angle
struct VehicleSprite {
    SDL_Texture* texture = nullptr;
    int num_angles = 1;       // how many rotation frames in the sheet
    int frame_w = 0;
    int frame_h = 0;
    bool loaded = false;
};

class SpriteManager {
public:
    bool init(SDL_Renderer* renderer);
    void shutdown();

    // Load a character spritesheet (LPC format)
    // Expected layout: PNG with rows of walk animations
    //   Row 0: walk up    (usually 9 frames in LPC, we use the middle frames)
    //   Row 1: walk left
    //   Row 2: walk down
    //   Row 3: walk right
    // frame_w/frame_h: size of each frame in the sheet
    // num_frames: frames per row
    // row indices are 0-based, -1 to skip optional animations
    bool load_character(const std::string& name, const std::string& path,
                        int frame_w, int frame_h, int walk_frames,
                        int row_up, int row_left, int row_down, int row_right,
                        int death_row = -1, int death_frames = 0,
                        int run_row_up = -1, int run_row_left = -1,
                        int run_row_down = -1, int run_row_right = -1,
                        int run_frames = 0);

    // Load a vehicle spritesheet
    // Expected layout: single row of rotation frames, angle 0 = facing right (east)
    // The game will pick the frame closest to the vehicle's current angle
    bool load_vehicle(const std::string& name, const std::string& path,
                      int frame_w, int frame_h, int num_angles);

    // Get loaded sprites (returns nullptr if not loaded)
    CharacterSprite* get_character(const std::string& name);
    VehicleSprite* get_vehicle(const std::string& name);

    // Draw a character sprite (idle/walk/run)
    // use_run: true = use run anim if available, false = walk
    void draw_character(const std::string& name, Vec2 screen_pos,
                        int direction, float anim_time, bool moving,
                        float scale = 1.0f, bool use_run = false);

    // Draw death animation — returns true if animation is still playing
    bool draw_character_death(const std::string& name, Vec2 screen_pos,
                              float anim_time, float scale = 1.0f);

    // Draw a vehicle sprite at a given angle
    void draw_vehicle(const std::string& name, Vec2 screen_pos,
                      float angle_rad, float scale = 1.0f);

    SDL_Renderer* renderer() const { return renderer_; }

private:
    SDL_Texture* load_texture(const std::string& path);

    SDL_Renderer* renderer_ = nullptr;
    std::unordered_map<std::string, SDL_Texture*> textures_;
    std::unordered_map<std::string, CharacterSprite> characters_;
    std::unordered_map<std::string, VehicleSprite> vehicles_;
};
