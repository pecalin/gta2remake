#pragma once
#include "util/math_utils.h"

struct VehicleParams {
    int model = 0;
    const char* name = "Unknown";
    bool turbo = false;
    int value = 50;         // money when crushed

    float mass = 14.0f;
    float front_drive_bias = 1.0f;   // 0=RWD, 0.5=AWD, 1.0=FWD
    float front_mass_bias = 0.5f;
    float brake_friction = 2.0f;
    float turn_in = 0.145f;          // how fast steering engages
    float turn_ratio = 0.45f;        // max steering angle factor
    float rear_end_stability = 1.25f; // higher = less oversteer
    float handbrake_slide = 0.18f;   // slide factor with handbrake
    float thrust = 0.15f;            // acceleration force
    float max_speed = 300.0f;        // pixels/s (scaled from GCI)
    float anti_strength = 1.0f;
    float skid_threshold = 0.085f;
    float gear1_mult = 0.55f;
    float gear2_mult = 0.68f;
    float gear3_mult = 1.0f;
    float gear2_speed = 0.107f;
    float gear3_speed = 0.165f;

    int max_hp = 100;
};

// Vehicle type registry — loads from assets/data/vehicles.txt if available
namespace VehicleTypes {
    void load_from_file(const char* path);  // call once at startup

    VehicleParams sedan();
    VehicleParams sports();
    VehicleParams truck();
    VehicleParams bus();
    VehicleParams cop_car();
    VehicleParams taxi();
    VehicleParams van();
    VehicleParams muscle();
    VehicleParams compact();
    VehicleParams pickup_truck();

    const VehicleParams& get_by_index(int index);
    int count();
}

struct VehicleState {
    Vec2 position;
    Vec2 velocity;
    float angle = 0.0f;       // radians
    float angular_vel = 0.0f;
    float speed = 0.0f;       // signed scalar speed along forward
    int current_gear = 1;
    bool is_skidding = false;
    bool handbrake_on = false;
};

class VehiclePhysics {
public:
    // Inputs: steer (-1 to 1), accel (0 to 1), brake (0 to 1), handbrake (bool)
    static void update(VehicleState& state, const VehicleParams& params,
                       float steer_input, float accel_input, float brake_input,
                       bool handbrake, float dt);
};
