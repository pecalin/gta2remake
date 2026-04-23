#pragma once
#include "entities/entity.h"

enum class WeaponType {
    PISTOL,
    MACHINE_GUN,
    ROCKET,
    FLAMETHROWER,
    MOLOTOV,
    GRENADE,
    COUNT
};

struct WeaponInfo {
    WeaponType type;
    const char* name;
    float damage;
    float fire_rate;      // shots per second
    float range;          // pixels
    float speed;          // projectile speed (0 = hitscan)
    int ammo_per_pickup;
    SDL_Color color;
    bool explosive;
    float explosion_radius;
};

inline WeaponInfo get_weapon_info(WeaponType type) {
    switch (type) {
        case WeaponType::PISTOL:
            return {type, "Pistol", 15.0f, 3.0f, 400.0f, 0.0f, 20, {255, 255, 200, 255}, false, 0};
        case WeaponType::MACHINE_GUN:
            return {type, "Machine Gun", 10.0f, 10.0f, 350.0f, 0.0f, 50, {255, 255, 100, 255}, false, 0};
        case WeaponType::ROCKET:
            return {type, "Rocket", 80.0f, 1.0f, 600.0f, 500.0f, 5, {255, 100, 50, 255}, true, 80.0f};
        case WeaponType::FLAMETHROWER:
            return {type, "Flamethrower", 5.0f, 15.0f, 150.0f, 300.0f, 100, {255, 150, 30, 255}, false, 0};
        case WeaponType::MOLOTOV:
            return {type, "Molotov", 40.0f, 1.5f, 300.0f, 350.0f, 10, {200, 100, 30, 255}, true, 60.0f};
        case WeaponType::GRENADE:
            return {type, "Grenade", 70.0f, 1.0f, 250.0f, 300.0f, 5, {60, 100, 60, 255}, true, 90.0f};
        default:
            return {WeaponType::PISTOL, "Pistol", 15.0f, 3.0f, 400.0f, 0.0f, 20, {255, 255, 200, 255}, false, 0};
    }
}

class Projectile : public Entity {
public:
    Projectile() = default;
    Projectile(WeaponType type, Vec2 pos, Vec2 dir, Entity* owner);

    void update(float dt) override;
    void render(SDL_Renderer* renderer, const Camera& camera) const override;

    WeaponType weapon_type() const { return type_; }
    float damage() const { return damage_; }
    Entity* owner() const { return owner_; }
    bool expired() const { return lifetime_ <= 0.0f; }

private:
    WeaponType type_ = WeaponType::PISTOL;
    float damage_ = 15.0f;
    float lifetime_ = 2.0f;
    float max_range_ = 400.0f;
    float distance_traveled_ = 0.0f;
    Entity* owner_ = nullptr;
    bool explosive_ = false;
    float explosion_radius_ = 0.0f;
};
