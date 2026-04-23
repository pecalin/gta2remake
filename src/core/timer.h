#pragma once
#include <SDL.h>

class Timer {
public:
    static constexpr float FIXED_DT = 1.0f / 60.0f;
    static constexpr Uint32 FIXED_DT_MS = 1000 / 60;

    void start() {
        last_time_ = SDL_GetPerformanceCounter();
        freq_ = static_cast<double>(SDL_GetPerformanceFrequency());
    }

    void tick() {
        Uint64 now = SDL_GetPerformanceCounter();
        double elapsed = static_cast<double>(now - last_time_) / freq_;
        last_time_ = now;

        // Clamp large spikes (e.g. window drag, breakpoint)
        if (elapsed > 0.25) elapsed = 0.25;
        accumulator_ += static_cast<float>(elapsed);
    }

    bool should_update() {
        if (accumulator_ >= FIXED_DT) {
            accumulator_ -= FIXED_DT;
            return true;
        }
        return false;
    }

    float accumulator() const { return accumulator_; }
    float alpha() const { return accumulator_ / FIXED_DT; }

private:
    Uint64 last_time_ = 0;
    double freq_ = 1.0;
    float accumulator_ = 0.0f;
};
