#include "core/window.h"
#include <cstdio>

bool Window::init(const std::string& title) {
    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIDTH, HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!window_) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    renderer_ = SDL_CreateRenderer(window_, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    // Set logical size so the game always renders at WIDTH x HEIGHT
    // regardless of actual window/fullscreen resolution.
    // SDL will scale and letterbox automatically.
    SDL_RenderSetLogicalSize(renderer_, WIDTH, HEIGHT);

    display_w_ = WIDTH;
    display_h_ = HEIGHT;

    return true;
}

void Window::set_resolution(int w, int h) {
    display_w_ = w;
    display_h_ = h;

    if (fullscreen_) {
        SDL_SetWindowFullscreen(window_, 0);  // exit fullscreen first
        SDL_SetWindowSize(window_, w, h);
        SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowSize(window_, w, h);
        SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }

    // Logical size stays the same — SDL handles scaling
    SDL_RenderSetLogicalSize(renderer_, WIDTH, HEIGHT);
}

void Window::set_fullscreen(bool fs) {
    fullscreen_ = fs;

    if (fs) {
        SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(window_, 0);
        SDL_SetWindowSize(window_, display_w_, display_h_);
        SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }

    // Logical size stays the same
    SDL_RenderSetLogicalSize(renderer_, WIDTH, HEIGHT);
}

void Window::shutdown() {
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    renderer_ = nullptr;
    window_ = nullptr;
}
