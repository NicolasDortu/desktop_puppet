#ifndef PUPPET_H
#define PUPPET_H
#include "config.h"

#include "raylib.h"

// =============================================================================
//  LIMBS
// =============================================================================

// A single point-mass driven by Verlet integration.
typedef struct
{
    Vector2 pos;    // current world-space position of the limb's center
    Vector2 oldPos; // previous world-space position (defines velocity)
    float   radius; // used for rendering AND limb-limb collision
    Color   color;
} PuppetLimb;

// Index of each limb within Puppet.limbs[].
typedef enum LimbId
{
    LIMB_BODY,
    LIMB_HEAD,
    LIMB_ARM_L,
    LIMB_ARM_R,
    LIMB_FOOT_L,
    LIMB_FOOT_R,
    LIMB_COUNT
} LimbId;

// =============================================================================
//  BONES
// =============================================================================

// Distance constraint between two limbs.
// Hard bones snap exactly to `length`; soft bones approach it gradually.
typedef struct
{
    LimbId limb1;
    LimbId limb2;
    float  length; // rest length, captured at puppet creation
} PuppetBone;

// Index of each bone within Puppet.bones[].
typedef enum BoneId
{
    // -- Hard bones: rigid skeleton --
    BODY_HEAD,
    BODY_ARM_L,
    BODY_ARM_R,
    BODY_FOOT_L,
    BODY_FOOT_R,
    HEAD_FOOT_L,   // diagonals prevent the puppet from folding in on itself
    HEAD_FOOT_R,

    // -- Soft bones: pull arms back to rest pose without locking them --
    HEAD_ARM_L,
    HEAD_ARM_R,

    BONE_COUNT
} BoneId;

// Bones with index >= SOFT_BONE_START use the soft `stiffness` factor.
#define SOFT_BONE_START HEAD_ARM_L

// =============================================================================
//  PUPPET
// =============================================================================

// Axis-aligned bounding box around all limbs; drives window sizing/positioning.
typedef struct
{
    float x, y, w, h;
} PuppetBounds;

typedef struct Puppet
{
    char          name[32];
    Color         color;
    float         radius;              // overall puppet size; limb radii are fractions of this
    PuppetBounds  bounds;              // recomputed every physics step
    int           draggedLimb;         // index of the limb being dragged, or -1
    PhysicsConfig physics;
    PuppetLimb    limbs[LIMB_COUNT];
    PuppetBone    bones[BONE_COUNT];
} Puppet;

// =============================================================================
//  FUNCTIONS
// =============================================================================

Puppet       CreatePuppet(const char *name, Color color, float radius, Vector2 startPos);
PuppetBounds ComputePuppetBounds(const Puppet *pup);
float        LimbsDistance(const Puppet *pup, LimbId limb1, LimbId limb2);

#endif