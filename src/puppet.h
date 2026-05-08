#ifndef PUPPET_H
#define PUPPET_H
#include "config.h"

#include "raylib.h"

// -- Declarations --
typedef struct
{
    Vector2 position;
    float radius;
} PuppetLimb;

typedef enum
{
    LIMB_BODY,
    LIMB_HEAD,
    LIMB_ARM_L,
    LIMB_ARM_R,
    LIMB_FOOT_L,
    LIMB_FOOT_R,
    LIMB_COUNT
} LimbId;

typedef struct Puppet
{
    char name[32];
    Color color;
    float radius;
    Vector2 position;
    Vector2 velocity;
    bool isDragging;
    PhysicsConfig physics;
    PuppetLimb limbs[LIMB_COUNT];
} Puppet;

// -- Functions --
Puppet CreatePuppet(const char *name, Color color, float radius, Vector2 startPos);

#endif