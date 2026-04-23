#include "physics/vehicle_physics.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

// Scale factor: GCI values are normalized (0-1 range for speeds).
// We multiply by this to get pixel/second values.
static constexpr float SPEED_SCALE = 800.0f;
static constexpr float THRUST_SCALE = 2000.0f;
static constexpr float TURN_SCALE = 12.0f;

void VehiclePhysics::update(VehicleState& state, const VehicleParams& params,
                             float steer_input, float accel_input, float brake_input,
                             bool handbrake, float dt) {
    Vec2 forward = Vec2::from_angle(state.angle);
    Vec2 right = {-forward.y, forward.x};

    float speed = state.velocity.length();
    float forward_speed = state.velocity.dot(forward);
    float lateral_speed = state.velocity.dot(right);

    // Determine gear
    float norm_speed = speed / (params.max_speed * SPEED_SCALE);
    float gear_mult = params.gear1_mult;
    state.current_gear = 1;
    if (norm_speed > params.gear3_speed) {
        gear_mult = params.gear3_mult;
        state.current_gear = 3;
    } else if (norm_speed > params.gear2_speed) {
        gear_mult = params.gear2_mult;
        state.current_gear = 2;
    }

    // Apply thrust
    float thrust_force = 0.0f;
    if (accel_input > 0.01f) {
        thrust_force = params.thrust * THRUST_SCALE * gear_mult * accel_input;
    }

    // Reverse (slower)
    if (brake_input > 0.01f && forward_speed <= 5.0f) {
        thrust_force = -params.thrust * THRUST_SCALE * 0.4f * brake_input;
    }

    state.velocity += forward * thrust_force * dt;

    // Steering with speed-dependent curve:
    // - Below min_steer_speed: steering scales up linearly (no tank spinning)
    // - At mid speed: full steering
    // - At high speed: steering reduces (wider turning radius)
    float max_px_speed = params.max_speed * SPEED_SCALE;
    float min_steer_speed = 30.0f;   // px/s - below this, steering fades out
    float peak_steer_speed = max_px_speed * 0.2f;  // full steering at ~20% of max speed

    float turn_amount = steer_input * params.turn_ratio * TURN_SCALE;

    if (speed < min_steer_speed) {
        // Very low speed: scale steering with speed (prevent tank spin)
        float low_factor = speed / min_steer_speed;
        turn_amount *= low_factor * low_factor;  // quadratic ramp-up
    } else if (speed > peak_steer_speed) {
        // High speed: reduce steering (wider turning radius)
        float high_ratio = (speed - peak_steer_speed) / (max_px_speed - peak_steer_speed);
        high_ratio = clamp(high_ratio, 0.0f, 1.0f);
        // Smooth curve: at max speed, steering is reduced to ~30% of peak
        float reduction = 1.0f / (1.0f + high_ratio * 3.0f);
        turn_amount *= reduction;
    }

    // Only steer when actually moving
    if (speed > 3.0f) {
        float turn_sign = (forward_speed >= 0) ? 1.0f : -1.0f;
        state.angle += turn_amount * turn_sign * dt;
    }

    // Recalculate after steering change
    forward = Vec2::from_angle(state.angle);
    right = {-forward.y, forward.x};
    forward_speed = state.velocity.dot(forward);
    lateral_speed = state.velocity.dot(right);

    // Lateral friction (grip)
    float grip = params.rear_end_stability;
    state.handbrake_on = handbrake;
    if (handbrake) {
        grip *= params.handbrake_slide; // reduced grip = more slide
    }

    // Kill lateral velocity based on grip
    float lateral_friction = clamp(grip * 8.0f * dt, 0.0f, 1.0f);
    lateral_speed *= (1.0f - lateral_friction);

    // Braking (when moving forward and pressing brake)
    if (brake_input > 0.01f && forward_speed > 5.0f) {
        float brake_force = params.brake_friction * 3.0f * dt * brake_input;
        forward_speed *= (1.0f - clamp(brake_force, 0.0f, 0.95f));
    }

    // Rolling friction
    forward_speed *= (1.0f - 0.8f * dt);

    // Reconstruct velocity
    state.velocity = forward * forward_speed + right * lateral_speed;

    // Speed cap (reuse max_px_speed from steering section)
    if (state.velocity.length() > max_px_speed) {
        state.velocity = state.velocity.normalized() * max_px_speed;
    }

    // Skid detection
    state.is_skidding = std::abs(lateral_speed) > params.skid_threshold * SPEED_SCALE * 0.3f;

    // Update speed
    state.speed = state.velocity.dot(forward);

    // Update position
    state.position += state.velocity * dt;
}

// ---- Vehicle type definitions (ported from nyc.gci) ----

namespace VehicleTypes {

static VehicleParams s_types[10];
static bool s_initialized = false;

static void init_types() {
    if (s_initialized) return;
    s_initialized = true;

    // Romero (model 0) - sedan
    s_types[0] = {0, "Romero", false, 50,
        16.5f, 1.0f, 0.5f, 1.75f, 0.145f, 0.45f, 1.25f, 0.18f,
        0.152f, 0.245f, 1.0f, 0.065f,
        0.55f, 0.68f, 1.0f, 0.107f, 0.165f, 100};

    // Wellard (model 1) - sports-ish
    s_types[1] = {1, "Wellard", false, 50,
        14.5f, 1.0f, 0.55f, 2.0f, 0.65f, 0.35f, 1.5f, 0.25f,
        0.22f, 0.38f, 1.0f, 0.139f,
        0.5f, 0.65f, 1.0f, 0.125f, 0.228f, 100};

    // Box Truck (model 7)
    s_types[2] = {7, "Box Truck", false, 90,
        28.0f, 0.5f, 0.7f, 2.0f, 0.25f, 0.65f, 2.5f, 0.45f,
        0.185f, 0.175f, 0.75f, 0.065f,
        0.545f, 0.675f, 1.0f, 0.088f, 0.114f, 150};

    // Bus (model 11)
    s_types[3] = {11, "Bus", false, 60,
        30.0f, 0.5f, 0.5f, 2.0f, 0.25f, 0.65f, 2.5f, 0.75f,
        0.235f, 0.215f, 0.75f, 0.085f,
        0.55f, 0.75f, 1.0f, 0.1f, 0.161f, 200};

    // Cop Car (model 12)
    s_types[4] = {12, "Cop Car", true, 60,
        14.5f, 1.0f, 0.5f, 2.0f, 0.433f, 0.4f, 1.25f, 0.4f,
        0.15f, 0.415f, 1.0f, 0.115f,
        0.55f, 0.68f, 1.0f, 0.18f, 0.29f, 120};

    // Taxi (use Minx - model 13)
    s_types[5] = {13, "Taxi", false, 50,
        14.5f, 0.75f, 0.5f, 1.75f, 0.145f, 0.45f, 1.25f, 0.18f,
        0.14f, 0.24f, 1.0f, 0.085f,
        0.55f, 0.68f, 1.0f, 0.12f, 0.166f, 100};

    // G4 Bank Van (model 4)
    s_types[6] = {4, "Bank Van", false, 25,
        24.0f, 0.5f, 0.5f, 1.5f, 0.5f, 0.4f, 2.0f, 0.75f,
        0.17f, 0.186f, 0.5f, 0.075f,
        0.55f, 0.675f, 1.0f, 0.081f, 0.13f, 150};

    // Beamer (model 5) - muscle
    s_types[7] = {5, "Beamer", true, 40,
        16.0f, 1.0f, 0.6f, 3.0f, 0.75f, 0.4f, 2.0f, 0.5f,
        0.15f, 0.385f, 1.0f, 0.14f,
        0.575f, 0.75f, 1.0f, 0.185f, 0.275f, 100};

    // Bug (model 8) - compact
    s_types[8] = {8, "Bug", false, 10,
        6.3f, 1.0f, 0.45f, 1.265f, 0.3f, 0.175f, 1.5f, 0.65f,
        0.095f, 0.235f, 1.0f, 0.05f,
        0.55f, 0.625f, 1.0f, 0.125f, 0.152f, 60};

    // Pacifier (model 3) - pickup truck
    s_types[9] = {3, "Pacifier", false, 30,
        27.0f, 0.5f, 0.6f, 0.9f, 0.25f, 0.75f, 1.8f, 0.15f,
        0.225f, 0.247f, 0.5f, 0.1f,
        0.55f, 0.8f, 1.0f, 0.103f, 0.192f, 130};
}

// Parse vehicles.txt and override hardcoded defaults
void load_from_file(const char* path) {
    init_types();  // ensure defaults are set first

    FILE* f = std::fopen(path, "r");
    if (!f) {
        std::fprintf(stderr, "vehicles.txt not found at %s, using defaults\n", path);
        return;
    }

    std::fprintf(stderr, "Loading vehicle config from %s\n", path);

    int current = -1;  // index into s_types
    char line[256];

    while (std::fgets(line, sizeof(line), f)) {
        // Strip trailing newline/whitespace
        int len = static_cast<int>(std::strlen(line));
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r' || line[len-1] == ' '))
            line[--len] = '\0';

        // Skip empty lines and comments
        if (len == 0 || line[0] == '#') continue;

        // Section header [Name]
        if (line[0] == '[') {
            char name[128] = {};
            if (std::sscanf(line, "[%127[^]]]", name) == 1) {
                // Find vehicle by name
                current = -1;
                for (int i = 0; i < 10; i++) {
                    if (std::strcmp(s_types[i].name, name) == 0) {
                        current = i;
                        break;
                    }
                }
                if (current < 0) {
                    std::fprintf(stderr, "  Unknown vehicle: %s\n", name);
                }
            }
            continue;
        }

        if (current < 0) continue;

        // key = value
        char key[64] = {};
        float val = 0;
        if (std::sscanf(line, "%63s = %f", key, &val) == 2) {
            VehicleParams& p = s_types[current];
            if      (std::strcmp(key, "model") == 0)       p.model = static_cast<int>(val);
            else if (std::strcmp(key, "turbo") == 0)       p.turbo = val > 0.5f;
            else if (std::strcmp(key, "value") == 0)       p.value = static_cast<int>(val);
            else if (std::strcmp(key, "max_hp") == 0)      p.max_hp = static_cast<int>(val);
            else if (std::strcmp(key, "mass") == 0)        p.mass = val;
            else if (std::strcmp(key, "front_drive") == 0) p.front_drive_bias = val;
            else if (std::strcmp(key, "front_mass") == 0)  p.front_mass_bias = val;
            else if (std::strcmp(key, "brake") == 0)       p.brake_friction = val;
            else if (std::strcmp(key, "turn_in") == 0)     p.turn_in = val;
            else if (std::strcmp(key, "turn_ratio") == 0)  p.turn_ratio = val;
            else if (std::strcmp(key, "stability") == 0)   p.rear_end_stability = val;
            else if (std::strcmp(key, "handbrake") == 0)   p.handbrake_slide = val;
            else if (std::strcmp(key, "thrust") == 0)      p.thrust = val;
            else if (std::strcmp(key, "max_speed") == 0)   p.max_speed = val;
            else if (std::strcmp(key, "anti") == 0)        p.anti_strength = val;
            else if (std::strcmp(key, "skid") == 0)        p.skid_threshold = val;
            else if (std::strcmp(key, "gear1_mult") == 0)  p.gear1_mult = val;
            else if (std::strcmp(key, "gear2_mult") == 0)  p.gear2_mult = val;
            else if (std::strcmp(key, "gear3_mult") == 0)  p.gear3_mult = val;
            else if (std::strcmp(key, "gear2_speed") == 0) p.gear2_speed = val;
            else if (std::strcmp(key, "gear3_speed") == 0) p.gear3_speed = val;
        }
    }

    std::fclose(f);
    std::fprintf(stderr, "Vehicle config loaded.\n");
}

VehicleParams sedan()        { init_types(); return s_types[0]; }
VehicleParams sports()       { init_types(); return s_types[1]; }
VehicleParams truck()        { init_types(); return s_types[2]; }
VehicleParams bus()          { init_types(); return s_types[3]; }
VehicleParams cop_car()      { init_types(); return s_types[4]; }
VehicleParams taxi()         { init_types(); return s_types[5]; }
VehicleParams van()          { init_types(); return s_types[6]; }
VehicleParams muscle()       { init_types(); return s_types[7]; }
VehicleParams compact()      { init_types(); return s_types[8]; }
VehicleParams pickup_truck() { init_types(); return s_types[9]; }

const VehicleParams& get_by_index(int index) {
    init_types();
    int idx = static_cast<int>(clamp(static_cast<float>(index), 0.0f, 9.0f));
    return s_types[idx];
}

int count() { return 10; }

} // namespace VehicleTypes
