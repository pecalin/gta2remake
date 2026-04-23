#include "core/input.h"
#include <cstring>

void Input::update() {
    std::memcpy(previous_, current_, sizeof(current_));

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            quit_ = true;
        }
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);

    current_[static_cast<int>(Action::MOVE_UP)]    = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
    current_[static_cast<int>(Action::MOVE_DOWN)]  = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];
    current_[static_cast<int>(Action::MOVE_LEFT)]  = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
    current_[static_cast<int>(Action::MOVE_RIGHT)] = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
    current_[static_cast<int>(Action::SHOOT)]              = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
    current_[static_cast<int>(Action::ENTER_EXIT_VEHICLE)] = keys[SDL_SCANCODE_F];
    current_[static_cast<int>(Action::HANDBRAKE)]          = keys[SDL_SCANCODE_SPACE];
    current_[static_cast<int>(Action::WEAPON_NEXT)]        = keys[SDL_SCANCODE_E];
    current_[static_cast<int>(Action::WEAPON_PREV)]        = keys[SDL_SCANCODE_Q];
    current_[static_cast<int>(Action::PAUSE)]              = keys[SDL_SCANCODE_ESCAPE];

    // Menu navigation (same keys but used separately when menu is open)
    current_[static_cast<int>(Action::MENU_UP)]      = keys[SDL_SCANCODE_UP]     || keys[SDL_SCANCODE_W];
    current_[static_cast<int>(Action::MENU_DOWN)]    = keys[SDL_SCANCODE_DOWN]   || keys[SDL_SCANCODE_S];
    current_[static_cast<int>(Action::MENU_CONFIRM)] = keys[SDL_SCANCODE_RETURN] || keys[SDL_SCANCODE_KP_ENTER];
    current_[static_cast<int>(Action::MENU_BACK)]    = keys[SDL_SCANCODE_ESCAPE];
}
