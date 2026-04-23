#pragma once
#include <SDL.h>

enum class Action {
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
    SHOOT,
    ENTER_EXIT_VEHICLE,
    HANDBRAKE,
    WEAPON_NEXT,
    WEAPON_PREV,
    PAUSE,
    MENU_UP,
    MENU_DOWN,
    MENU_CONFIRM,
    MENU_BACK,
    COUNT
};

class Input {
public:
    void update();

    // Key is currently held down (for continuous actions like movement)
    bool is_down(Action a) const { return held_[static_cast<int>(a)]; }

    // Key was pressed this frame — event-driven, never misses a tap
    bool is_pressed(Action a) const { return pressed_[static_cast<int>(a)]; }

    bool quit_requested() const { return quit_; }

    // Mouse state
    int mouse_x() const { return mouse_x_; }
    int mouse_y() const { return mouse_y_; }
    bool mouse_left() const { return mouse_left_; }
    bool mouse_left_pressed() const { return mouse_left_pressed_; }
    bool mouse_moved() const { return mouse_moved_; }

private:
    static constexpr int NUM_ACTIONS = static_cast<int>(Action::COUNT);
    bool held_[NUM_ACTIONS] = {};
    bool pressed_[NUM_ACTIONS] = {};
    bool quit_ = false;

    // Mouse
    int mouse_x_ = 0;
    int mouse_y_ = 0;
    bool mouse_left_ = false;
    bool mouse_left_pressed_ = false;
    bool mouse_moved_ = false;
    int last_mouse_x_ = -1;
    int last_mouse_y_ = -1;

    void map_scancode(SDL_Scancode sc, bool down);
};
