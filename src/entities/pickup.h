#pragma once
#include "entities/entity.h"
#include "entities/projectile.h" // for WeaponType

enum class PickupType {
    WEAPON_PISTOL,
    WEAPON_MACHINE_GUN,
    WEAPON_ROCKET,
    WEAPON_FLAMETHROWER,
    WEAPON_MOLOTOV,
    WEAPON_GRENADE,
    HEALTH,
    ARMOR,
    SPEED_BOOST,
    COP_BRIBE,
    COUNT
};

struct PickupInfo {
    PickupType type;
    const char* name;
    SDL_Color color;
    float respawn_time;
};

inline PickupInfo get_pickup_info(PickupType type) {
    switch (type) {
        case PickupType::WEAPON_PISTOL:      return {type, "Pistol",       {200, 200, 100, 255}, 30.0f};
        case PickupType::WEAPON_MACHINE_GUN: return {type, "Machine Gun",  {255, 200, 50, 255},  45.0f};
        case PickupType::WEAPON_ROCKET:      return {type, "Rockets",      {255, 80, 50, 255},   60.0f};
        case PickupType::WEAPON_FLAMETHROWER:return {type, "Flamethrower", {255, 150, 30, 255},  60.0f};
        case PickupType::WEAPON_MOLOTOV:     return {type, "Molotov",      {200, 120, 50, 255},  45.0f};
        case PickupType::WEAPON_GRENADE:     return {type, "Grenades",     {60, 120, 60, 255},   45.0f};
        case PickupType::HEALTH:             return {type, "Health",       {50, 200, 50, 255},   30.0f};
        case PickupType::ARMOR:              return {type, "Armor",        {50, 100, 255, 255},  45.0f};
        case PickupType::SPEED_BOOST:        return {type, "Speed",        {255, 255, 50, 255},  60.0f};
        case PickupType::COP_BRIBE:          return {type, "Cop Bribe",    {50, 255, 50, 255},   90.0f};
        default: return {PickupType::HEALTH, "Health", {50, 200, 50, 255}, 30.0f};
    }
}

class Pickup : public Entity {
public:
    Pickup() = default;
    Pickup(PickupType type, Vec2 pos);

    void update(float dt) override;
    void render(SDL_Renderer* renderer, const Camera& camera) const override;

    PickupType pickup_type() const { return type_; }
    bool is_collected() const { return collected_; }
    void collect();

private:
    PickupType type_ = PickupType::HEALTH;
    bool collected_ = false;
    float respawn_timer_ = 0.0f;
    float respawn_time_ = 30.0f;
    float bob_timer_ = 0.0f; // visual bobbing animation
};
