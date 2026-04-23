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
    bool is_down(Action a) const { return current_[static_cast<int>(a)]; }
    bool is_pressed(Action a) const { return current_[static_cast<int>(a)] && !previous_[static_cast<int>(a)]; }
    bool quit_requested() const { return quit_; }

private:
    static constexpr int NUM_ACTIONS = static_cast<int>(Action::COUNT);
    bool current_[NUM_ACTIONS] = {};
    bool previous_[NUM_ACTIONS] = {};
    bool quit_ = false;
};
