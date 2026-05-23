#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
//  PHYSICS
// =============================================================================

// Default tuning values for the puppet's physics simulation.
#define DEFAULT_GRAVITY     0.50f  // px/frame² downward acceleration
#define DEFAULT_FRICTION    0.99f  // velocity retained per frame (1.0 = none)
#define DEFAULT_BOUNCE     -0.70f  // velocity multiplier on wall/floor impact
#define DEFAULT_MIN_BOUNCE  0.03f  // speed below which residual motion is killed
#define DEFAULT_STIFFNESS   0.05f  // fraction of correction applied by soft bones

typedef struct
{
    float gravity;   // px/frame² downward acceleration
    float friction;  // velocity multiplier applied each frame
    float bounce;    // velocity multiplier on wall/floor collision
    float minBounce; // speed threshold below which motion is zeroed (anti-jitter)
    float stiffness; // softness of soft bones (0 = floppy, 1 = rigid)
} PhysicsConfig;

// =============================================================================
//  GEOMETRY
// =============================================================================

// Bounding box of entity used for window sizing & positioning.
typedef struct
{
    float x, y, w, h;
} BoundBox;

// =============================================================================
//  ENGINE
// =============================================================================

#define TARGET_FPS 60

#endif