#pragma once
#include <SDL.h>
#include <string>
#include <vector>
#include <functional>

class Input;

// Dev settings — live-tunable values accessible from the pause menu
struct DevSettings {
    // Toggles
    bool invincible = false;
    bool no_police = false;
    bool infinite_ammo = false;
    bool no_ped_damage = false;   // peds can't hurt player
    bool one_hit_kill = false;    // player kills everything in one hit

    // Tunable values (multipliers, 0.1 - 5.0 range)
    float car_acceleration = 1.0f;
    float car_braking = 1.0f;
    float car_turn_speed = 1.0f;
    float car_max_speed = 1.0f;
    float car_drift = 1.0f;       // handbrake slide amount
    float player_speed = 1.0f;
    float npc_density = 1.0f;     // pedestrian population multiplier
    float traffic_density = 1.0f; // traffic vehicle multiplier
    float cop_aggression = 1.0f;  // how fast cops respond / how many spawn
    float wanted_gain = 1.0f;     // crime heat multiplier
};

enum class MenuScreen {
    NONE,
    PAUSE,
    OPTIONS,
    CONTROLS,
    DEV_MODE,
};

enum class MenuItemType {
    ACTION,     // press enter to activate
    TOGGLE,     // press enter to flip bool
    SLIDER,     // left/right to adjust float value
};

struct MenuItem {
    std::string label;
    MenuItemType type = MenuItemType::ACTION;
    std::function<void()> on_select;   // for ACTION type
    bool* toggle_value = nullptr;      // for TOGGLE type
    float* slider_value = nullptr;     // for SLIDER type
    float slider_min = 0.1f;
    float slider_max = 5.0f;
    float slider_step = 0.1f;
    bool enabled = true;
};

class Menu {
public:
    void init(int screen_w, int screen_h);

    bool is_open() const { return screen_ != MenuScreen::NONE; }

    void open();
    void close();

    void handle_input(const Input& input);
    void render(SDL_Renderer* renderer);

    DevSettings& dev() { return dev_; }
    const DevSettings& dev() const { return dev_; }

    // Callbacks set by Game
    std::function<void()> on_resume;
    std::function<void()> on_save;
    std::function<void()> on_load;
    std::function<void()> on_quit;

private:
    void set_screen(MenuScreen screen);
    void build_pause_items();
    void build_options_items();
    void build_controls_items();
    void build_dev_items();

    void render_text_line(SDL_Renderer* renderer, const std::string& text,
                          int x, int y, int char_w, int char_h,
                          SDL_Color color);
    void render_char(SDL_Renderer* renderer, char ch, int x, int y,
                     int w, int h, SDL_Color color);

    MenuScreen screen_ = MenuScreen::NONE;
    std::vector<MenuItem> items_;
    int selected_ = 0;
    int scroll_offset_ = 0;   // for long menus

    int screen_w_ = 1280;
    int screen_h_ = 720;

    DevSettings dev_;
};
