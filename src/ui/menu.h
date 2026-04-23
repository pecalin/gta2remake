#pragma once
#include <SDL.h>
#include <string>
#include <vector>
#include <functional>

class Input;
class Window;

// Dev settings — live-tunable values accessible from the pause menu
struct DevSettings {
    // Toggles
    bool invincible = false;
    bool no_police = false;
    bool infinite_ammo = false;
    bool no_ped_damage = false;
    bool one_hit_kill = false;
    bool aimbot = false;
    float aimbot_range = 300.0f;  // pixels

    // Tunable values (multipliers, 0.1 - 5.0 range)
    float car_acceleration = 1.0f;
    float car_braking = 1.0f;
    float car_turn_speed = 1.0f;
    float car_max_speed = 1.0f;
    float car_drift = 1.0f;
    float player_speed = 1.0f;
    float npc_density = 1.0f;
    float traffic_density = 1.0f;
    float cop_aggression = 1.0f;
    float wanted_gain = 1.0f;
};

enum class MenuScreen {
    NONE,
    PAUSE,
    OPTIONS,
    CONTROLS,
    DEV_MODE,
    RESOLUTION,
};

enum class MenuItemType {
    ACTION,
    TOGGLE,
    SLIDER,
};

struct MenuItem {
    std::string label;
    MenuItemType type = MenuItemType::ACTION;
    std::function<void()> on_select;
    bool* toggle_value = nullptr;
    float* slider_value = nullptr;
    float slider_min = 0.1f;
    float slider_max = 5.0f;
    float slider_step = 0.1f;
    bool enabled = true;
};

class Menu {
public:
    void init(int screen_w, int screen_h, Window* window);

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
    void build_resolution_items();

    void render_text_line(SDL_Renderer* renderer, const std::string& text,
                          int x, int y, int char_w, int char_h,
                          SDL_Color color);
    void render_char(SDL_Renderer* renderer, char ch, int x, int y,
                     int w, int h, SDL_Color color);

    MenuScreen screen_ = MenuScreen::NONE;
    std::vector<MenuItem> items_;
    int selected_ = 0;
    int scroll_offset_ = 0;

    float repeat_delay_ = 0.35f;
    float repeat_rate_ = 0.06f;
    float repeat_timer_ = 0.0f;
    int repeat_dir_ = 0;

    int screen_w_ = 1280;
    int screen_h_ = 720;

    Window* window_ = nullptr;
    bool fullscreen_ = false;

    DevSettings dev_;
};
