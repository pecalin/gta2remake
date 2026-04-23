#pragma once
#include "util/math_utils.h"

class WantedSystem {
public:
    int level() const { return level_; }

    void add_crime(int severity);  // 1=minor, 2=kill ped, 3=kill cop
    void update(float dt, bool cops_can_see_player);
    void clear();                  // cop bribe
    void busted();                 // player arrested

    bool is_busted() const { return busted_state_; }
    void reset_busted() { busted_state_ = false; }

    float heat() const { return heat_; }

private:
    int level_ = 0;
    float heat_ = 0.0f;          // accumulated crime, maps to level
    float decay_timer_ = 0.0f;   // time since last seen by cops
    float decay_delay_ = 15.0f;  // seconds before heat decays
    bool busted_state_ = false;

    void recalc_level();
};
