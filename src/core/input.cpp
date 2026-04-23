#include "core/input.h"
#include <cstring>
#include <cstdlib>

void Input::map_scancode(SDL_Scancode sc, bool down) {
    struct Mapping { SDL_Scancode sc; Action act; };
    static const Mapping mappings[] = {
        {SDL_SCANCODE_W,      Action::MOVE_UP},
        {SDL_SCANCODE_UP,     Action::MOVE_UP},
        {SDL_SCANCODE_S,      Action::MOVE_DOWN},
        {SDL_SCANCODE_DOWN,   Action::MOVE_DOWN},
        {SDL_SCANCODE_A,      Action::MOVE_LEFT},
        {SDL_SCANCODE_LEFT,   Action::MOVE_LEFT},
        {SDL_SCANCODE_D,      Action::MOVE_RIGHT},
        {SDL_SCANCODE_RIGHT,  Action::MOVE_RIGHT},
        {SDL_SCANCODE_LCTRL,  Action::SHOOT},
        {SDL_SCANCODE_RCTRL,  Action::SHOOT},
        {SDL_SCANCODE_F,      Action::ENTER_EXIT_VEHICLE},
        {SDL_SCANCODE_SPACE,  Action::HANDBRAKE},
        {SDL_SCANCODE_E,      Action::WEAPON_NEXT},
        {SDL_SCANCODE_Q,      Action::WEAPON_PREV},
        {SDL_SCANCODE_ESCAPE, Action::PAUSE},
        {SDL_SCANCODE_W,      Action::MENU_UP},
        {SDL_SCANCODE_UP,     Action::MENU_UP},
        {SDL_SCANCODE_S,      Action::MENU_DOWN},
        {SDL_SCANCODE_DOWN,   Action::MENU_DOWN},
        {SDL_SCANCODE_RETURN, Action::MENU_CONFIRM},
        {SDL_SCANCODE_KP_ENTER, Action::MENU_CONFIRM},
        {SDL_SCANCODE_ESCAPE, Action::MENU_BACK},
    };

    for (auto& m : mappings) {
        if (m.sc == sc) {
            int idx = static_cast<int>(m.act);
            held_[idx] = down;
            if (down) pressed_[idx] = true;
        }
    }
}

void Input::update() {
    // Clear one-shot flags
    std::memset(pressed_, 0, sizeof(pressed_));
    mouse_left_pressed_ = false;
    mouse_moved_ = false;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                quit_ = true;
                break;
            case SDL_KEYDOWN:
                if (!e.key.repeat) {
                    map_scancode(e.key.keysym.scancode, true);
                }
                break;
            case SDL_KEYUP:
                map_scancode(e.key.keysym.scancode, false);
                break;
            case SDL_MOUSEMOTION:
                mouse_x_ = e.motion.x;
                mouse_y_ = e.motion.y;
                // Only count as "moved" if it moved more than a couple pixels
                if (last_mouse_x_ >= 0) {
                    int dx = mouse_x_ - last_mouse_x_;
                    int dy = mouse_y_ - last_mouse_y_;
                    if (dx * dx + dy * dy > 4) {
                        mouse_moved_ = true;
                    }
                }
                last_mouse_x_ = mouse_x_;
                last_mouse_y_ = mouse_y_;
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (e.button.button == SDL_BUTTON_LEFT) {
                    mouse_left_ = true;
                    mouse_left_pressed_ = true;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (e.button.button == SDL_BUTTON_LEFT) {
                    mouse_left_ = false;
                }
                break;
        }
    }
}
