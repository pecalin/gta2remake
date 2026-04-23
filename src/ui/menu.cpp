#include "ui/menu.h"
#include "core/input.h"
#include "core/window.h"
#include <cstring>
#include <cstdio>
#include <cmath>

void Menu::init(int screen_w, int screen_h, Window* window) {
    screen_w_ = screen_w;
    window_ = window;
    screen_h_ = screen_h;
}

void Menu::open() {
    set_screen(MenuScreen::PAUSE);
}

void Menu::close() {
    screen_ = MenuScreen::NONE;
    items_.clear();
    selected_ = 0;
    scroll_offset_ = 0;
}

void Menu::set_screen(MenuScreen screen) {
    screen_ = screen;
    selected_ = 0;
    scroll_offset_ = 0;
    items_.clear();

    switch (screen) {
        case MenuScreen::PAUSE:      build_pause_items();      break;
        case MenuScreen::OPTIONS:    build_options_items();     break;
        case MenuScreen::CONTROLS:   build_controls_items();    break;
        case MenuScreen::DEV_MODE:   build_dev_items();         break;
        case MenuScreen::RESOLUTION: build_resolution_items();  break;
        case MenuScreen::NONE: break;
    }
}

void Menu::build_pause_items() {
    MenuItem it;
    it.type = MenuItemType::ACTION;

    it.label = "RESUME";
    it.on_select = [this]() { close(); if (on_resume) on_resume(); };
    items_.push_back(it);

    it.label = "SAVE GAME";
    it.on_select = [this]() { if (on_save) on_save(); };
    items_.push_back(it);

    it.label = "LOAD GAME";
    it.on_select = [this]() { if (on_load) on_load(); };
    items_.push_back(it);

    it.label = "OPTIONS";
    it.on_select = [this]() { set_screen(MenuScreen::OPTIONS); };
    items_.push_back(it);

    it.label = "QUIT GAME";
    it.on_select = [this]() { if (on_quit) on_quit(); };
    items_.push_back(it);
}

void Menu::build_options_items() {
    MenuItem it;

    it.type = MenuItemType::TOGGLE;
    it.label = "FULLSCREEN";
    it.toggle_value = &fullscreen_;
    it.on_select = nullptr;
    it.slider_value = nullptr;
    items_.push_back(it);

    it.type = MenuItemType::ACTION;
    it.toggle_value = nullptr;

    it.label = "RESOLUTION";
    it.on_select = [this]() { set_screen(MenuScreen::RESOLUTION); };
    items_.push_back(it);

    it.label = "CONTROLS";
    it.on_select = [this]() { set_screen(MenuScreen::CONTROLS); };
    items_.push_back(it);

    it.label = "DEV MODE";
    it.on_select = [this]() { set_screen(MenuScreen::DEV_MODE); };
    items_.push_back(it);

    it.label = "BACK";
    it.on_select = [this]() { set_screen(MenuScreen::PAUSE); };
    items_.push_back(it);
}

void Menu::build_controls_items() {
    MenuItem it;
    it.type = MenuItemType::ACTION;
    it.label = "BACK";
    it.on_select = [this]() { set_screen(MenuScreen::OPTIONS); };
    items_.push_back(it);
}

void Menu::build_dev_items() {
    MenuItem it;

    // --- Toggles ---
    it.type = MenuItemType::TOGGLE;
    it.on_select = nullptr;
    it.slider_value = nullptr;

    it.label = "INVINCIBLE";
    it.toggle_value = &dev_.invincible;
    items_.push_back(it);

    it.label = "NO POLICE";
    it.toggle_value = &dev_.no_police;
    items_.push_back(it);

    it.label = "INFINITE AMMO";
    it.toggle_value = &dev_.infinite_ammo;
    items_.push_back(it);

    it.label = "ONE HIT KILL";
    it.toggle_value = &dev_.one_hit_kill;
    items_.push_back(it);

    it.label = "NO PED DAMAGE";
    it.toggle_value = &dev_.no_ped_damage;
    items_.push_back(it);

    it.label = "AIMBOT";
    it.toggle_value = &dev_.aimbot;
    items_.push_back(it);

    // --- Sliders ---
    it.type = MenuItemType::SLIDER;
    it.toggle_value = nullptr;
    it.slider_min = 0.1f;
    it.slider_max = 5.0f;
    it.slider_step = 0.1f;

    it.label = "CAR ACCEL";
    it.slider_value = &dev_.car_acceleration;
    items_.push_back(it);

    it.label = "CAR BRAKE";
    it.slider_value = &dev_.car_braking;
    items_.push_back(it);

    it.label = "CAR TURN SPEED";
    it.slider_value = &dev_.car_turn_speed;
    items_.push_back(it);

    it.label = "CAR MAX SPEED";
    it.slider_value = &dev_.car_max_speed;
    items_.push_back(it);

    it.label = "CAR DRIFT";
    it.slider_value = &dev_.car_drift;
    items_.push_back(it);

    it.label = "PLAYER SPEED";
    it.slider_value = &dev_.player_speed;
    items_.push_back(it);

    it.label = "NPC DENSITY";
    it.slider_value = &dev_.npc_density;
    items_.push_back(it);

    it.label = "TRAFFIC DENSITY";
    it.slider_value = &dev_.traffic_density;
    items_.push_back(it);

    it.label = "COP AGGRESSION";
    it.slider_value = &dev_.cop_aggression;
    items_.push_back(it);

    it.label = "WANTED GAIN";
    it.slider_value = &dev_.wanted_gain;
    items_.push_back(it);

    it.label = "AIMBOT RANGE";
    it.slider_value = &dev_.aimbot_range;
    it.slider_min = 100.0f;
    it.slider_max = 800.0f;
    it.slider_step = 25.0f;
    items_.push_back(it);

    // Reset step for any items after
    it.slider_min = 0.1f;
    it.slider_max = 5.0f;
    it.slider_step = 0.1f;

    // --- Back ---
    it.type = MenuItemType::ACTION;
    it.toggle_value = nullptr;
    it.slider_value = nullptr;
    it.label = "BACK";
    it.on_select = [this]() { set_screen(MenuScreen::OPTIONS); };
    items_.push_back(it);
}

void Menu::build_resolution_items() {
    struct ResOption { int w; int h; const char* label; };
    ResOption resolutions[] = {
        {800,  600,  "800X600"},
        {1024, 768,  "1024X768"},
        {1280, 720,  "1280X720"},
        {1366, 768,  "1366X768"},
        {1600, 900,  "1600X900"},
        {1920, 1080, "1920X1080"},
        {2560, 1440, "2560X1440"},
    };

    MenuItem it;
    it.type = MenuItemType::ACTION;
    it.toggle_value = nullptr;
    it.slider_value = nullptr;

    for (auto& res : resolutions) {
        int rw = res.w;
        int rh = res.h;
        it.label = res.label;
        it.on_select = [this, rw, rh]() {
            if (window_) window_->set_resolution(rw, rh);
        };
        items_.push_back(it);
    }

    it.label = "BACK";
    it.on_select = [this]() { set_screen(MenuScreen::OPTIONS); };
    items_.push_back(it);
}

void Menu::handle_input(const Input& input) {
    if (screen_ == MenuScreen::NONE) return;

    if (input.is_pressed(Action::MENU_DOWN)) {
        selected_++;
        if (selected_ >= static_cast<int>(items_.size()))
            selected_ = 0;
    }

    if (input.is_pressed(Action::MENU_UP)) {
        selected_--;
        if (selected_ < 0)
            selected_ = static_cast<int>(items_.size()) - 1;
    }

    if (selected_ >= 0 && selected_ < static_cast<int>(items_.size())) {
        auto& item = items_[selected_];

        if (input.is_pressed(Action::MENU_CONFIRM)) {
            if (item.type == MenuItemType::ACTION && item.on_select) {
                item.on_select();
            } else if (item.type == MenuItemType::TOGGLE && item.toggle_value) {
                *item.toggle_value = !(*item.toggle_value);
                // Apply fullscreen toggle immediately
                if (item.toggle_value == &fullscreen_ && window_) {
                    window_->set_fullscreen(fullscreen_);
                }
            }
        }

        // Left/right for sliders — with hold-to-repeat
        if (item.type == MenuItemType::SLIDER && item.slider_value) {
            bool right_held = input.is_down(Action::MOVE_RIGHT);
            bool left_held = input.is_down(Action::MOVE_LEFT);
            int dir = right_held ? 1 : (left_held ? -1 : 0);

            bool do_step = false;

            if (dir != 0) {
                if (input.is_pressed(Action::MOVE_RIGHT) || input.is_pressed(Action::MOVE_LEFT)) {
                    // First press — immediate step + start repeat timer
                    do_step = true;
                    repeat_dir_ = dir;
                    repeat_timer_ = repeat_delay_;
                } else if (dir == repeat_dir_) {
                    // Holding — count down and repeat
                    repeat_timer_ -= 1.0f / 60.0f;  // menu runs at display refresh
                    if (repeat_timer_ <= 0.0f) {
                        do_step = true;
                        repeat_timer_ = repeat_rate_;
                    }
                }
            } else {
                repeat_dir_ = 0;
                repeat_timer_ = 0.0f;
            }

            if (do_step) {
                *item.slider_value += item.slider_step * dir;
                if (*item.slider_value > item.slider_max)
                    *item.slider_value = item.slider_max;
                if (*item.slider_value < item.slider_min)
                    *item.slider_value = item.slider_min;
            }
        } else {
            // Not on a slider — reset repeat state
            if (!input.is_down(Action::MOVE_LEFT) && !input.is_down(Action::MOVE_RIGHT)) {
                repeat_dir_ = 0;
                repeat_timer_ = 0.0f;
            }
        }
    }

    if (input.is_pressed(Action::MENU_BACK)) {
        switch (screen_) {
            case MenuScreen::PAUSE:
                close();
                if (on_resume) on_resume();
                break;
            case MenuScreen::OPTIONS:
                set_screen(MenuScreen::PAUSE);
                break;
            case MenuScreen::CONTROLS:
                set_screen(MenuScreen::OPTIONS);
                break;
            case MenuScreen::DEV_MODE:
                set_screen(MenuScreen::OPTIONS);
                break;
            case MenuScreen::RESOLUTION:
                set_screen(MenuScreen::OPTIONS);
                break;
            case MenuScreen::NONE:
                break;
        }
    }
}

// ---- Pixel font ----

static const unsigned char* get_font_glyph(char ch) {
    static const unsigned char G_SPACE[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    static const unsigned char G_EXCL[]  = {0x04,0x04,0x04,0x04,0x04,0x00,0x04};
    static const unsigned char G_DASH[]  = {0x00,0x00,0x00,0x1F,0x00,0x00,0x00};
    static const unsigned char G_PLUS[]  = {0x00,0x04,0x04,0x1F,0x04,0x04,0x00};
    static const unsigned char G_SLASH[] = {0x01,0x02,0x02,0x04,0x08,0x08,0x10};
    static const unsigned char G_COLON[] = {0x00,0x04,0x00,0x00,0x00,0x04,0x00};
    static const unsigned char G_DOT[]   = {0x00,0x00,0x00,0x00,0x00,0x00,0x04};
    static const unsigned char G_0[] = {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E};
    static const unsigned char G_1[] = {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E};
    static const unsigned char G_2[] = {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F};
    static const unsigned char G_3[] = {0x0E,0x11,0x01,0x06,0x01,0x11,0x0E};
    static const unsigned char G_4[] = {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02};
    static const unsigned char G_5[] = {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E};
    static const unsigned char G_6[] = {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E};
    static const unsigned char G_7[] = {0x1F,0x01,0x02,0x04,0x08,0x08,0x08};
    static const unsigned char G_8[] = {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E};
    static const unsigned char G_9[] = {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C};
    static const unsigned char G_A[] = {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11};
    static const unsigned char G_B[] = {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E};
    static const unsigned char G_C[] = {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E};
    static const unsigned char G_D[] = {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E};
    static const unsigned char G_E[] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F};
    static const unsigned char G_F[] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10};
    static const unsigned char G_G[] = {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F};
    static const unsigned char G_H[] = {0x11,0x11,0x11,0x1F,0x11,0x11,0x11};
    static const unsigned char G_I[] = {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E};
    static const unsigned char G_J[] = {0x07,0x02,0x02,0x02,0x02,0x12,0x0C};
    static const unsigned char G_K[] = {0x11,0x12,0x14,0x18,0x14,0x12,0x11};
    static const unsigned char G_L[] = {0x10,0x10,0x10,0x10,0x10,0x10,0x1F};
    static const unsigned char G_M[] = {0x11,0x1B,0x15,0x15,0x11,0x11,0x11};
    static const unsigned char G_N[] = {0x11,0x19,0x15,0x13,0x11,0x11,0x11};
    static const unsigned char G_O[] = {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E};
    static const unsigned char G_P[] = {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10};
    static const unsigned char G_Q[] = {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D};
    static const unsigned char G_R[] = {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11};
    static const unsigned char G_S[] = {0x0E,0x11,0x10,0x0E,0x01,0x11,0x0E};
    static const unsigned char G_T[] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x04};
    static const unsigned char G_U[] = {0x11,0x11,0x11,0x11,0x11,0x11,0x0E};
    static const unsigned char G_V[] = {0x11,0x11,0x11,0x11,0x0A,0x0A,0x04};
    static const unsigned char G_W[] = {0x11,0x11,0x11,0x15,0x15,0x1B,0x11};
    static const unsigned char G_X[] = {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11};
    static const unsigned char G_Y[] = {0x11,0x11,0x0A,0x04,0x04,0x04,0x04};
    static const unsigned char G_Z[] = {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F};

    switch (ch) {
        case ' ': return G_SPACE; case '!': return G_EXCL;
        case '-': return G_DASH;  case '+': return G_PLUS;
        case '/': return G_SLASH; case ':': return G_COLON;
        case '.': return G_DOT;
        case '0': return G_0; case '1': return G_1; case '2': return G_2;
        case '3': return G_3; case '4': return G_4; case '5': return G_5;
        case '6': return G_6; case '7': return G_7; case '8': return G_8;
        case '9': return G_9;
        case 'A': return G_A; case 'B': return G_B; case 'C': return G_C;
        case 'D': return G_D; case 'E': return G_E; case 'F': return G_F;
        case 'G': return G_G; case 'H': return G_H; case 'I': return G_I;
        case 'J': return G_J; case 'K': return G_K; case 'L': return G_L;
        case 'M': return G_M; case 'N': return G_N; case 'O': return G_O;
        case 'P': return G_P; case 'Q': return G_Q; case 'R': return G_R;
        case 'S': return G_S; case 'T': return G_T; case 'U': return G_U;
        case 'V': return G_V; case 'W': return G_W; case 'X': return G_X;
        case 'Y': return G_Y; case 'Z': return G_Z;
        default: return G_SPACE;
    }
}

void Menu::render_char(SDL_Renderer* renderer, char ch, int x, int y,
                        int w, int h, SDL_Color color) {
    const unsigned char* glyph = get_font_glyph(ch);
    if (!glyph) return;

    int px_w = w / 5;
    int px_h = h / 7;
    if (px_w < 1) px_w = 1;
    if (px_h < 1) px_h = 1;

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    for (int row = 0; row < 7; row++) {
        unsigned char bits = glyph[row];
        for (int col = 0; col < 5; col++) {
            if (bits & (0x10 >> col)) {
                SDL_Rect px = {x + col * px_w, y + row * px_h, px_w, px_h};
                SDL_RenderFillRect(renderer, &px);
            }
        }
    }
}

void Menu::render_text_line(SDL_Renderer* renderer, const std::string& text,
                             int x, int y, int char_w, int char_h,
                             SDL_Color color) {
    for (int i = 0; i < static_cast<int>(text.size()); i++) {
        render_char(renderer, text[i], x + i * (char_w + 2), y, char_w, char_h, color);
    }
}

// Format float as string like "1.5"
static std::string float_to_str(float v) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(v));
    // Convert to uppercase for our font
    for (int i = 0; buf[i]; i++) {
        if (buf[i] >= 'a' && buf[i] <= 'z') buf[i] -= 32;
    }
    return std::string(buf);
}

void Menu::render(SDL_Renderer* renderer) {
    if (screen_ == MenuScreen::NONE) return;

    // Dim background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
    SDL_Rect fullscreen = {0, 0, screen_w_, screen_h_};
    SDL_RenderFillRect(renderer, &fullscreen);

    // Title text
    const char* title = "PAUSED";
    if (screen_ == MenuScreen::OPTIONS)  title = "OPTIONS";
    if (screen_ == MenuScreen::CONTROLS) title = "CONTROLS";
    if (screen_ == MenuScreen::DEV_MODE) title = "DEV MODE";
    if (screen_ == MenuScreen::RESOLUTION) title = "RESOLUTION";

    // Panel sizing
    int panel_w = (screen_ == MenuScreen::DEV_MODE) ? 500 : 400;
    int item_row_h = (screen_ == MenuScreen::DEV_MODE) ? 32 : 45;
    int max_visible = 14;
    int num_items = static_cast<int>(items_.size());

    // Extra height for controls info
    int extra_h = 0;
    if (screen_ == MenuScreen::CONTROLS) extra_h = 6 * 28 + 10;

    int visible_count = (num_items > max_visible) ? max_visible : num_items;
    int panel_h = 60 + extra_h + visible_count * item_row_h + 30;
    int panel_x = (screen_w_ - panel_w) / 2;
    int panel_y = (screen_h_ - panel_h) / 2;

    // Scrolling: keep selected visible
    if (selected_ < scroll_offset_) scroll_offset_ = selected_;
    if (selected_ >= scroll_offset_ + max_visible) scroll_offset_ = selected_ - max_visible + 1;

    // Panel background
    SDL_SetRenderDrawColor(renderer, 25, 25, 40, 230);
    SDL_Rect panel = {panel_x, panel_y, panel_w, panel_h};
    SDL_RenderFillRect(renderer, &panel);

    // Panel border
    SDL_SetRenderDrawColor(renderer, 200, 180, 50, 255);
    SDL_RenderDrawRect(renderer, &panel);
    SDL_Rect panel_inner = {panel_x + 2, panel_y + 2, panel_w - 4, panel_h - 4};
    SDL_RenderDrawRect(renderer, &panel_inner);

    // Title
    int title_char_w = 15;
    int title_char_h = 21;
    int title_w = static_cast<int>(std::strlen(title)) * (title_char_w + 2);
    int title_x = panel_x + (panel_w - title_w) / 2;
    int title_y = panel_y + 15;
    render_text_line(renderer, title, title_x, title_y,
                     title_char_w, title_char_h, {200, 180, 50, 255});

    // Separator
    SDL_SetRenderDrawColor(renderer, 200, 180, 50, 120);
    SDL_Rect sep = {panel_x + 20, title_y + title_char_h + 8, panel_w - 40, 2};
    SDL_RenderFillRect(renderer, &sep);

    // Controls info display
    if (screen_ == MenuScreen::CONTROLS) {
        int info_y = title_y + title_char_h + 20;
        int cw = 10, ch = 14;
        SDL_Color dim = {160, 160, 180, 255};
        SDL_Color bright = {220, 200, 100, 255};

        struct ControlLine { const char* action; const char* key; };
        ControlLine controls[] = {
            {"MOVE",        "WASD / ARROWS"},
            {"SHOOT",       "LEFT CTRL"},
            {"ENTER CAR",   "F"},
            {"HANDBRAKE",   "SPACE"},
            {"WEAPON",      "Q / E"},
            {"PAUSE",       "ESC"},
        };

        for (int i = 0; i < 6; i++) {
            int ly = info_y + i * 28;
            render_text_line(renderer, controls[i].action,
                             panel_x + 30, ly, cw, ch, bright);
            render_text_line(renderer, controls[i].key,
                             panel_x + 200, ly, cw, ch, dim);
        }
    }

    // Item area
    int item_start_y = title_y + title_char_h + 20 + extra_h;

    int item_char_w = (screen_ == MenuScreen::DEV_MODE) ? 9 : 12;
    int item_char_h = (screen_ == MenuScreen::DEV_MODE) ? 13 : 17;

    // Scroll indicator at top
    if (scroll_offset_ > 0) {
        SDL_SetRenderDrawColor(renderer, 200, 180, 50, 150);
        int ax = panel_x + panel_w / 2;
        int ay = item_start_y - 8;
        SDL_RenderDrawLine(renderer, ax - 8, ay + 4, ax, ay);
        SDL_RenderDrawLine(renderer, ax, ay, ax + 8, ay + 4);
    }

    for (int vi = 0; vi < visible_count && vi + scroll_offset_ < num_items; vi++) {
        int i = vi + scroll_offset_;
        int item_y = item_start_y + vi * item_row_h;
        bool is_selected = (i == selected_);
        auto& item = items_[i];

        // Selection highlight
        if (is_selected) {
            SDL_SetRenderDrawColor(renderer, 200, 180, 50, 40);
            SDL_Rect highlight = {panel_x + 10, item_y - 3, panel_w - 20, item_row_h - 2};
            SDL_RenderFillRect(renderer, &highlight);

            SDL_SetRenderDrawColor(renderer, 200, 180, 50, 255);
            int arrow_x = panel_x + 18;
            int arrow_cy = item_y + item_char_h / 2;
            SDL_RenderDrawLine(renderer, arrow_x, arrow_cy - 5, arrow_x + 6, arrow_cy);
            SDL_RenderDrawLine(renderer, arrow_x + 6, arrow_cy, arrow_x, arrow_cy + 5);

            SDL_SetRenderDrawColor(renderer, 200, 180, 50, 80);
            SDL_Rect sel_border = {panel_x + 10, item_y - 3, panel_w - 20, item_row_h - 2};
            SDL_RenderDrawRect(renderer, &sel_border);
        }

        SDL_Color text_color;
        if (is_selected)
            text_color = {255, 240, 100, 255};
        else
            text_color = {200, 200, 210, 255};

        int label_x = panel_x + 35;

        if (item.type == MenuItemType::ACTION) {
            // Centered for action items in non-dev menus
            if (screen_ != MenuScreen::DEV_MODE) {
                int text_w = static_cast<int>(item.label.size()) * (item_char_w + 2);
                label_x = panel_x + (panel_w - text_w) / 2;
            }
            render_text_line(renderer, item.label, label_x, item_y,
                             item_char_w, item_char_h, text_color);

        } else if (item.type == MenuItemType::TOGGLE) {
            // Label on left, checkbox on right
            render_text_line(renderer, item.label, label_x, item_y,
                             item_char_w, item_char_h, text_color);

            bool on = item.toggle_value && *item.toggle_value;
            int box_x = panel_x + panel_w - 55;
            int box_y = item_y + 1;
            int box_s = item_char_h - 2;

            // Checkbox outline
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_Rect box = {box_x, box_y, box_s, box_s};
            SDL_RenderDrawRect(renderer, &box);

            if (on) {
                // Filled green check
                SDL_SetRenderDrawColor(renderer, 50, 220, 50, 255);
                SDL_Rect fill = {box_x + 2, box_y + 2, box_s - 4, box_s - 4};
                SDL_RenderFillRect(renderer, &fill);
            }

            // ON/OFF text
            SDL_Color state_col = on ? SDL_Color{50, 220, 50, 255} : SDL_Color{150, 60, 60, 255};
            render_text_line(renderer, on ? "ON" : "OFF",
                             box_x + box_s + 6, item_y, item_char_w, item_char_h, state_col);

        } else if (item.type == MenuItemType::SLIDER) {
            // Label on left
            render_text_line(renderer, item.label, label_x, item_y,
                             item_char_w, item_char_h, text_color);

            float val = item.slider_value ? *item.slider_value : 0.0f;
            float pct = (val - item.slider_min) / (item.slider_max - item.slider_min);

            // Slider bar on right
            int bar_x = panel_x + panel_w - 170;
            int bar_y = item_y + 3;
            int bar_w = 100;
            int bar_h = item_char_h - 6;

            // Background
            SDL_SetRenderDrawColor(renderer, 60, 60, 80, 255);
            SDL_Rect bar_bg = {bar_x, bar_y, bar_w, bar_h};
            SDL_RenderFillRect(renderer, &bar_bg);

            // Fill
            int fill_w = static_cast<int>(bar_w * pct);
            SDL_Color bar_col;
            if (val < 0.95f)
                bar_col = {100, 150, 220, 255};
            else if (val < 1.05f)
                bar_col = {50, 200, 50, 255};   // green at default (1.0)
            else
                bar_col = {220, 160, 50, 255};

            SDL_SetRenderDrawColor(renderer, bar_col.r, bar_col.g, bar_col.b, 255);
            SDL_Rect bar_fill = {bar_x, bar_y, fill_w, bar_h};
            SDL_RenderFillRect(renderer, &bar_fill);

            // Border
            SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            SDL_RenderDrawRect(renderer, &bar_bg);

            // Value text
            std::string val_str = float_to_str(val);
            render_text_line(renderer, val_str,
                             bar_x + bar_w + 8, item_y,
                             item_char_w, item_char_h, text_color);

            // Left/right arrows when selected
            if (is_selected) {
                SDL_SetRenderDrawColor(renderer, 200, 180, 50, 255);
                // Left arrow
                SDL_RenderDrawLine(renderer, bar_x - 10, bar_y + bar_h / 2,
                                   bar_x - 4, bar_y);
                SDL_RenderDrawLine(renderer, bar_x - 10, bar_y + bar_h / 2,
                                   bar_x - 4, bar_y + bar_h);
                // Right arrow
                int rx = bar_x + bar_w + 4;
                SDL_RenderDrawLine(renderer, rx, bar_y,
                                   rx + 6, bar_y + bar_h / 2);
                SDL_RenderDrawLine(renderer, rx + 6, bar_y + bar_h / 2,
                                   rx, bar_y + bar_h);
            }
        }
    }

    // Scroll indicator at bottom
    if (scroll_offset_ + max_visible < num_items) {
        SDL_SetRenderDrawColor(renderer, 200, 180, 50, 150);
        int ax = panel_x + panel_w / 2;
        int ay = item_start_y + visible_count * item_row_h + 2;
        SDL_RenderDrawLine(renderer, ax - 8, ay, ax, ay + 4);
        SDL_RenderDrawLine(renderer, ax, ay + 4, ax + 8, ay);
    }

    // Footer hint
    int footer_y = panel_y + panel_h - 20;
    SDL_Color hint_color = {120, 120, 140, 255};
    if (screen_ == MenuScreen::DEV_MODE) {
        render_text_line(renderer, "UP/DOWN:NAV  ENTER:TOGGLE  LEFT/RIGHT:ADJUST  ESC:BACK",
                         panel_x + 15, footer_y, 6, 9, hint_color);
    } else {
        render_text_line(renderer, "UP/DOWN:SELECT  ENTER:OK  ESC:BACK",
                         panel_x + 20, footer_y, 7, 10, hint_color);
    }
}
