#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
//  PHYSICS
// =============================================================================

// Physics tuning, shared by every body (puppet and items).
// ponytail: plain constants — re-add a per-body PhysicsConfig only if a body
// ever needs different tuning.
#define PHYS_GRAVITY     0.50f  // px/frame² downward acceleration
#define PHYS_FRICTION    0.99f  // velocity retained per frame (1.0 = none)
#define PHYS_BOUNCE     -0.70f  // velocity multiplier on wall/floor impact
#define PHYS_MIN_BOUNCE  0.03f  // speed below which residual motion is killed
#define PHYS_STIFFNESS   0.25f  // fraction of correction applied by soft bones

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

// The puppet earns a coin when it "gets hurt": a hard wall impact or a solid shove from an item.
#define HURT_WALL_SPEED      25.0f // px/frame impact speed on a wall that counts as a hit
#define HURT_ITEM_PUSH       4.0f  // px of limb displacement by an item in one frame
#define HURT_COOLDOWN_FRAMES 30    // min frames between two coins
#define COINS_MAX            9999  // shop balance cap

// =============================================================================
//  ENGINE
// =============================================================================

#define TARGET_FPS 60

#endif