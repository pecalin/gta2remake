#pragma once
#include "entities/projectile.h"
#include "entities/explosion.h"
#include <vector>
#include <memory>

class Player;
class Camera;
class Map;

class WeaponSystem {
public:
    void update(float dt, const Map& map);
    void render(SDL_Renderer* renderer, const Camera& camera) const;

    void fire(WeaponType type, Vec2 pos, float angle, Entity* owner);
    void create_explosion(Vec2 pos, float radius, float damage);

    std::vector<Projectile>& projectiles() { return projectiles_; }
    std::vector<Explosion>& explosions() { return explosions_; }

    void clear();

private:
    std::vector<Projectile> projectiles_;
    std::vector<Explosion> explosions_;
};
