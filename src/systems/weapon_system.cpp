#include "systems/weapon_system.h"
#include "world/map.h"
#include "core/camera.h"

void WeaponSystem::update(float dt, const Map& map) {
    // Update projectiles
    for (auto& proj : projectiles_) {
        if (!proj.active()) continue;
        proj.update(dt);

        // Check map collision
        if (map.is_solid(proj.position().x, proj.position().y)) {
            proj.set_active(false);

            // Create explosion for explosive weapons
            WeaponInfo info = get_weapon_info(proj.weapon_type());
            if (info.explosive) {
                create_explosion(proj.position(), info.explosion_radius, info.damage);
            }
        }

        if (proj.expired()) {
            WeaponInfo info = get_weapon_info(proj.weapon_type());
            if (info.explosive) {
                create_explosion(proj.position(), info.explosion_radius, info.damage);
            }
            proj.set_active(false);
        }
    }

    // Update explosions
    for (auto& exp : explosions_) {
        if (!exp.active()) continue;
        exp.update(dt);
    }

    // Cleanup dead projectiles (keep vector compact)
    projectiles_.erase(
        std::remove_if(projectiles_.begin(), projectiles_.end(),
                       [](const Projectile& p) { return !p.active(); }),
        projectiles_.end());

    explosions_.erase(
        std::remove_if(explosions_.begin(), explosions_.end(),
                       [](const Explosion& e) { return !e.active(); }),
        explosions_.end());
}

void WeaponSystem::render(SDL_Renderer* renderer, const Camera& camera) const {
    for (auto& proj : projectiles_) {
        proj.render(renderer, camera);
    }
    for (auto& exp : explosions_) {
        exp.render(renderer, camera);
    }
}

void WeaponSystem::fire(WeaponType type, Vec2 pos, float angle, Entity* owner) {
    WeaponInfo info = get_weapon_info(type);

    Vec2 dir = Vec2::from_angle(angle);

    if (info.speed <= 0.0f) {
        // Hitscan weapon — create a very fast, short-lived projectile
        Projectile proj(type, pos + dir * 15.0f, dir, owner);
        proj.set_velocity(dir * 2000.0f); // very fast
        projectiles_.push_back(proj);
    } else {
        // Projectile weapon
        Projectile proj(type, pos + dir * 15.0f, dir, owner);
        projectiles_.push_back(proj);
    }
}

void WeaponSystem::create_explosion(Vec2 pos, float radius, float damage) {
    explosions_.emplace_back(pos, radius, damage);
}

void WeaponSystem::clear() {
    projectiles_.clear();
    explosions_.clear();
}
