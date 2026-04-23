#include "systems/wanted_system.h"
#include <algorithm>

void WantedSystem::add_crime(int severity) {
    heat_ += static_cast<float>(severity);
    decay_timer_ = 0.0f;
    recalc_level();
}

void WantedSystem::update(float dt, bool cops_can_see_player) {
    if (level_ == 0) return;

    if (cops_can_see_player) {
        decay_timer_ = 0.0f;
    } else {
        decay_timer_ += dt;

        // Decay heat when out of sight, higher levels take longer
        float needed_delay = decay_delay_ + level_ * 5.0f;
        if (decay_timer_ > needed_delay) {
            heat_ -= 0.5f * dt;
            if (heat_ < 0.0f) heat_ = 0.0f;
            recalc_level();
        }
    }
}

void WantedSystem::clear() {
    heat_ = 0.0f;
    level_ = 0;
    decay_timer_ = 0.0f;
}

void WantedSystem::busted() {
    busted_state_ = true;
    clear();
}

void WantedSystem::recalc_level() {
    if (heat_ >= 20.0f)      level_ = 6;
    else if (heat_ >= 15.0f) level_ = 5;
    else if (heat_ >= 10.0f) level_ = 4;
    else if (heat_ >= 6.0f)  level_ = 3;
    else if (heat_ >= 3.0f)  level_ = 2;
    else if (heat_ >= 1.0f)  level_ = 1;
    else                     level_ = 0;
}
