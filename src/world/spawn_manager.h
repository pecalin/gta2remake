#pragma once
#include "entities/pedestrian.h"
#include "entities/pickup.h"
#include <vector>
#include <memory>

class Map;
class Camera;

class SpawnManager {
public:
    void init(const Map* map);
    void update(float dt, Vec2 player_pos, const Camera& camera);
    void render(SDL_Renderer* renderer, const Camera& camera) const;

    std::vector<Pedestrian>& pedestrians() { return peds_; }
    std::vector<Pickup>& pickups() { return pickups_; }

    void spawn_pickups_on_map();
    void set_density(float d) { density_mult_ = d; }

private:
    void spawn_ped_near(Vec2 center, float min_dist, float max_dist);

    const Map* map_ = nullptr;
    std::vector<Pedestrian> peds_;
    std::vector<Pickup> pickups_;

    int max_peds_ = 25;
    float spawn_timer_ = 0.0f;
    float spawn_interval_ = 0.5f;
    float density_mult_ = 1.0f;
};
