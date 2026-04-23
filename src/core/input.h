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

private:
    static constexpr int NUM_ACTIONS = static_cast<int>(Action::COUNT);
    bool held_[NUM_ACTIONS] = {};
    bool pressed_[NUM_ACTIONS] = {};  // set from SDL_KEYDOWN events, cleared each frame
    bool quit_ = false;

    void map_scancode(SDL_Scancode sc, bool down);
};
