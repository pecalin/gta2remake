#pragma once
#include <SDL.h>
#include <string>

class Window {
public:
    // Logical resolution — game always renders at this size
    static constexpr int WIDTH = 1280;
    static constexpr int HEIGHT = 720;

    bool init(const std::string& title);
    void shutdown();

    SDL_Renderer* renderer() const { return renderer_; }
    SDL_Window* window() const { return window_; }

    void set_resolution(int w, int h);
    void set_fullscreen(bool fs);
    bool is_fullscreen() const { return fullscreen_; }

    int display_width() const { return display_w_; }
    int display_height() const { return display_h_; }

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    int display_w_ = WIDTH;
    int display_h_ = HEIGHT;
    bool fullscreen_ = false;
};
