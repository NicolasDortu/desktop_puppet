#ifndef E_PUPPET_H
#define E_PUPPET_H

#include "config.h"
#include "physics.h"

#include "raylib.h"

// =============================================================================
//  LIMBS
// =============================================================================

typedef Particle PuppetLimb;

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

typedef Bone PuppetBone;

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

// =============================================================================
//  PUPPET
// =============================================================================

typedef struct Puppet
{
    Color      color;
    float      radius;                       // overall puppet size; limb radii are fractions of this
    Body       body;                         // physics state (particles + bones + bounds + cfg)
    PuppetLimb limbs[LIMB_COUNT];            // backing storage for body.particles
    PuppetBone bones[BONE_COUNT];            // backing storage for body.bones
    Color      limbColors[LIMB_COUNT];       // render-only sibling array
} Puppet;

// =============================================================================
//  FUNCTIONS
// =============================================================================

// Build a fully-initialized puppet at `startPos`. Writes into `*pup` so the
// internal Body keeps valid pointers to the caller's `limbs`/`bones` arrays.
void CreatePuppet(Puppet *pup, Color color, float radius, Vector2 startPos);
void DrawPuppet (const Puppet *pup);


#endif
