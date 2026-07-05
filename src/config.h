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
#define DEFAULT_STIFFNESS   0.25f  // fraction of correction applied by soft bones

typedef struct
{
    float gravity;   // px/frame² downward acceleration
    float friction;  // velocity multiplier applied each frame
    float bounce;    // velocity multiplier on wall/floor collision
    float minBounce; // speed threshold below which motion is zeroed (anti-jitter)
    float stiffness; // softness of soft bones (0 = floppy, 1 = rigid)
} PhysicsConfig;

// Single source of truth for the default physics tuning.
#define DEFAULT_PHYSICS_CONFIG ((PhysicsConfig){ \
    .gravity   = DEFAULT_GRAVITY,                \
    .friction  = DEFAULT_FRICTION,               \
    .bounce    = DEFAULT_BOUNCE,                 \
    .minBounce = DEFAULT_MIN_BOUNCE,             \
    .stiffness = DEFAULT_STIFFNESS,              \
})

// =============================================================================
//  GEOMETRY
// =============================================================================

// Bounding box of entity used for window sizing & positioning.
typedef struct
{
    float x, y, w, h;
} BoundBox;

// Window padding around bounding box making sure the whole entity is always visible.
#define WINDOW_MARGIN 2

// =============================================================================
//  SHOP / HURT
// =============================================================================

// The puppet earns a coin when it "gets hurt": a hard wall impact or a solid
// shove from an item. Thresholds filter out resting contact; the cooldown
// stops a single crash from paying out every frame.
#define HURT_WALL_SPEED      8.0f  // px/frame impact speed on a wall that counts as a hit
#define HURT_ITEM_PUSH       2.5f  // px of limb displacement by an item in one frame
#define HURT_COOLDOWN_FRAMES 30    // min frames between two coins
#define COINS_MAX            9999  // shop balance cap

// =============================================================================
//  ENGINE
// =============================================================================

#define TARGET_FPS 60

#endif