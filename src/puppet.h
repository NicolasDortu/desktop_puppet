#ifndef PUPPET_H
#define PUPPET_H
#include "config.h"

#include "raylib.h"

// -- Declarations --

// Limb for verlet integration. Require a radius for rendering and collision, and a color for rendering.
typedef struct
{
    Vector2 pos;    // world-space position of the limb's center
    Vector2 oldPos; // previous world-space position for Verlet integration
    float radius;
    Color color;
} PuppetLimb;

// Unique IDs for each limb
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

// Boundaries of the puppet, used for sizing windows and menus.
typedef struct
{
    float x, y, w, h;
} PuppetBounds;

// Bone connecting two limbs, with a rest length to maintain.
typedef struct
{
    LimbId limb1;
    LimbId limb2;
    float length;
} PuppetBone;

// Unique IDs for each bone
typedef enum BoneId
{
    BODY_HEAD,
    BODY_ARM_L,
    BODY_ARM_R,
    BODY_FOOT_L,
    BODY_FOOT_R,
    BONE_COUNT
} BoneId;

// Puppet structure containing all limbs, bones, and physics configuration.
typedef struct Puppet
{
    char name[32];
    Color color;
    float radius;
    PuppetBounds bounds;
    int draggedLimb; // Index of the limb being dragged, or -1
    PhysicsConfig physics;
    PuppetLimb limbs[LIMB_COUNT];
    PuppetBone bones[BONE_COUNT];
} Puppet;

// -- Functions --

Puppet CreatePuppet(const char *name, Color color, float radius, Vector2 startPos);
float LimbsDistance(const Puppet *pup, LimbId limb1, LimbId limb2);

#endif