#pragma once
#include <cmath>
#include <algorithm>

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

    float length() const { return std::sqrt(x * x + y * y); }
    float length_sq() const { return x * x + y * y; }

    Vec2 normalized() const {
        float len = length();
        if (len < 0.0001f) return {0, 0};
        return {x / len, y / len};
    }

    float dot(const Vec2& o) const { return x * o.x + y * o.y; }

    static Vec2 from_angle(float radians) {
        return {std::cos(radians), std::sin(radians)};
    }
};

inline Vec2 operator*(float s, const Vec2& v) { return {v.x * s, v.y * s}; }

inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

inline float clamp(float val, float lo, float hi) {
    return std::max(lo, std::min(hi, val));
}

inline float deg_to_rad(float deg) { return deg * (3.14159265f / 180.0f); }
inline float rad_to_deg(float rad) { return rad * (180.0f / 3.14159265f); }

struct Rect {
    float x, y, w, h;

    bool overlaps(const Rect& o) const {
        return x < o.x + o.w && x + w > o.x &&
               y < o.y + o.h && y + h > o.y;
    }

    bool contains(float px, float py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};
