#pragma once
#include <SDL.h>
#include <string>

class Window {
public:
    static constexpr int WIDTH = 1280;
    static constexpr int HEIGHT = 720;

    bool init(const std::string& title);
    void shutdown();

    SDL_Renderer* renderer() const { return renderer_; }
    SDL_Window* window() const { return window_; }

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
};
