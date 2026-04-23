#include "world/spawn_manager.h"
#include "world/map.h"
#include "core/camera.h"
#include <cstdlib>

void SpawnManager::init(const Map* map) {
    map_ = map;
    spawn_pickups_on_map();
}

void SpawnManager::spawn_pickups_on_map() {
    if (!map_) return;

    // Place weapon pickups at various road intersections
    struct PickupSpawn {
        int tx, ty;
        PickupType type;
    };

    PickupSpawn spawns[] = {
        {7, 7, PickupType::WEAPON_PISTOL},
        {17, 7, PickupType::WEAPON_MACHINE_GUN},
        {27, 7, PickupType::WEAPON_ROCKET},
        {37, 7, PickupType::HEALTH},
        {47, 7, PickupType::ARMOR},
        {7, 17, PickupType::WEAPON_MOLOTOV},
        {17, 17, PickupType::WEAPON_GRENADE},
        {27, 17, PickupType::WEAPON_FLAMETHROWER},
        {37, 17, PickupType::COP_BRIBE},
        {47, 17, PickupType::HEALTH},
        {7, 27, PickupType::WEAPON_PISTOL},
        {27, 27, PickupType::WEAPON_MACHINE_GUN},
        {47, 27, PickupType::WEAPON_ROCKET},
        {7, 37, PickupType::ARMOR},
        {27, 37, PickupType::HEALTH},
        {47, 37, PickupType::WEAPON_GRENADE},
        {17, 47, PickupType::WEAPON_MACHINE_GUN},
        {37, 47, PickupType::WEAPON_FLAMETHROWER},
    };

    for (auto& sp : spawns) {
        Vec2 pos = {
            static_cast<float>(sp.tx * Map::TILE_SIZE + Map::TILE_SIZE / 2),
            static_cast<float>(sp.ty * Map::TILE_SIZE + Map::TILE_SIZE / 2)
        };
        pickups_.emplace_back(sp.type, pos);
    }
}

void SpawnManager::update(float dt, Vec2 player_pos, const Camera& camera) {
    // Spawn pedestrians near player if below max count
    spawn_timer_ -= dt;

    int active_count = 0;
    for (auto& ped : peds_) {
        if (ped.active() && ped.state() != PedState::DEAD) active_count++;
    }

    int adjusted_max = static_cast<int>(max_peds_ * density_mult_);
    if (adjusted_max < 0) adjusted_max = 0;

    if (spawn_timer_ <= 0.0f && active_count < adjusted_max) {
        spawn_ped_near(player_pos, 400.0f, 800.0f);
        spawn_timer_ = spawn_interval_;
    }

    // Update peds
    for (auto& ped : peds_) {
        if (ped.active()) {
            ped.update(dt);
        }
    }

    // Despawn peds far from player
    for (auto& ped : peds_) {
        if (!ped.active()) continue;
        float dist = (ped.position() - player_pos).length();
        if (dist > 1200.0f && ped.state() != PedState::DEAD) {
            ped.set_active(false);
        }
    }

    // Remove inactive peds
    peds_.erase(
        std::remove_if(peds_.begin(), peds_.end(),
                       [](const Pedestrian& p) { return !p.active(); }),
        peds_.end());

    // Update pickups
    for (auto& pickup : pickups_) {
        pickup.update(dt);
    }
}

void SpawnManager::render(SDL_Renderer* renderer, const Camera& camera) const {
    for (auto& pickup : pickups_) {
        pickup.render(renderer, camera);
    }
    for (auto& ped : peds_) {
        ped.render(renderer, camera);
    }
}

void SpawnManager::spawn_ped_near(Vec2 center, float min_dist, float max_dist) {
    if (!map_) return;

    // Try a few random positions
    for (int attempt = 0; attempt < 10; attempt++) {
        float angle = static_cast<float>(std::rand()) / RAND_MAX * 6.28318f;
        float dist = min_dist + static_cast<float>(std::rand()) / RAND_MAX * (max_dist - min_dist);

        Vec2 pos = center + Vec2::from_angle(angle) * dist;

        int tx = static_cast<int>(pos.x) / Map::TILE_SIZE;
        int ty = static_cast<int>(pos.y) / Map::TILE_SIZE;

        TileType tile = map_->get_tile(tx, ty);
        if (tile == TileType::SIDEWALK || tile == TileType::GRASS) {
            Pedestrian ped;
            ped.set_position(pos);
            ped.set_map(map_);
            peds_.push_back(ped);
            return;
        }
    }
}
