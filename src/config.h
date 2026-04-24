#ifndef CONFIG_H
#define CONFIG_H

// --- Physics ---
// Default physics value
#define DEFAULT_GRAVITY 0.5f
#define DEFAULT_FRICTION 0.99f
#define DEFAULT_BOUNCE -0.7f
#define DEFAULT_MIN_BOUNCE_VEL 1.0f

typedef struct
{
    float gravity;      // pixels per frame² downward acceleration
    float friction;     // horizontal velocity multiplier per frame
    float bounce;       // velocity multiplier on wall/floor collision
    float minBounceVel; // velocity below which vertical bounce stops
} PhysicsConfig;

// --- Engine ---
#define TARGET_FPS 60

#endif