#include "physics.h"
#include "puppet.h"

#include <math.h>

#include "raylib.h"

// Number of constraint-solver passes per frame.
#define CONSTRAINT_ITERATIONS 3

// =============================================================================
//  VERLET INTEGRATION
// =============================================================================

// Advance a single limb by one frame using Verlet integration.
//
// In Verlet, velocity is implicit:  v = pos - oldPos
// We derive v, apply friction & gravity, move the limb, then handle wall
// collisions by reflecting `oldPos` so the next frame's implicit velocity
// points away from the wall.
static void IntegrateLimb(PuppetLimb *limb, const PhysicsConfig *cfg,
                          int screenWidth, int screenHeight)
{
    // --- Derive velocity from last frame's displacement, apply friction ---
    float vx = (limb->pos.x - limb->oldPos.x) * cfg->friction;
    float vy = (limb->pos.y - limb->oldPos.y) * cfg->friction;

    // --- Kill micro-velocities to stop residual jitter on a settled puppet ---
    if (vx * vx + vy * vy < cfg->minBounce * cfg->minBounce)
    {
        vx = 0.0f;
        vy = 0.0f;
    }

    // --- Integrate: oldPos <- pos, then move pos by velocity + gravity ---
    limb->oldPos = limb->pos;
    limb->pos.x += vx;
    limb->pos.y += vy + cfg->gravity;

    // --- Wall collisions: clamp position, reflect implicit velocity ---
    // The trick: setting `oldPos = pos - v * bounce` makes the next frame's
    // velocity equal to `v * bounce` (with `bounce` typically negative).
    if (limb->pos.y + limb->radius > screenHeight)  // floor
    {
        limb->pos.y    = screenHeight - limb->radius;
        limb->oldPos.y = limb->pos.y - vy * cfg->bounce;
    }
    if (limb->pos.y - limb->radius < 0)             // ceiling
    {
        limb->pos.y    = limb->radius;
        limb->oldPos.y = limb->pos.y - vy * cfg->bounce;
    }
    if (limb->pos.x + limb->radius > screenWidth)   // right wall
    {
        limb->pos.x    = screenWidth - limb->radius;
        limb->oldPos.x = limb->pos.x - vx * cfg->bounce;
    }
    if (limb->pos.x - limb->radius < 0)             // left wall
    {
        limb->pos.x    = limb->radius;
        limb->oldPos.x = limb->pos.x - vx * cfg->bounce;
    }
}

// =============================================================================
//  CONSTRAINT SOLVING
// =============================================================================

// Pull each pair of bone-connected limbs back to the bone's rest length.
// Hard bones snap exactly; soft bones apply only `stiffness` of the correction.
// Dragged limbs are immovable anchors — the other end absorbs the full move.
static void UpdateBones(Puppet *pup)
{
    for (int i = 0; i < BONE_COUNT; i++)
    {
        PuppetBone *bone = &pup->bones[i];
        PuppetLimb *a    = &pup->limbs[bone->limb1];
        PuppetLimb *b    = &pup->limbs[bone->limb2];

        // --- Current vector from a to b ---
        float dx       = b->pos.x - a->pos.x;
        float dy       = b->pos.y - a->pos.y;
        float distance = sqrtf(dx * dx + dy * dy);

        if (distance < 1e-6f) continue;  // avoid division by zero on coincident limbs

        // --- Compute the per-limb correction ---
        //   error    = how far the bone is from its rest length
        //   percent  = fraction of the error each endpoint must absorb
        //              (0.5 = split equally, multiplied by stiffness for soft bones)
        float stiffness = (i >= SOFT_BONE_START) ? pup->physics.stiffness : 1.0f;
        float error     = distance - bone->length;
        float percent   = (error / distance) * 0.5f * stiffness;

        float offsetX = dx * percent;
        float offsetY = dy * percent;

        // --- Move both endpoints toward each other (or apart) ---
        if (pup->draggedLimb != (int)bone->limb1) { a->pos.x += offsetX; a->pos.y += offsetY; }
        if (pup->draggedLimb != (int)bone->limb2) { b->pos.x -= offsetX; b->pos.y -= offsetY; }
    }
}

// Push overlapping limb pairs apart so circles never visually intersect.
// Uses squared-distance check to skip the sqrt for non-colliding pairs.
static void ResolveLimbCollisions(Puppet *pup)
{
    for (int i = 0; i < LIMB_COUNT; i++)
    {
        for (int j = i + 1; j < LIMB_COUNT; j++)
        {
            PuppetLimb *a = &pup->limbs[i];
            PuppetLimb *b = &pup->limbs[j];

            // --- Squared distance vs squared sum of radii (cheap rejection) ---
            float dx      = b->pos.x - a->pos.x;
            float dy      = b->pos.y - a->pos.y;
            float dist2   = dx * dx + dy * dy;
            float minDist = a->radius + b->radius;

            if (dist2 >= minDist * minDist || dist2 < 1e-6f)
                continue;

            // --- Split the overlap equally between the two limbs ---
            float dist    = sqrtf(dist2);
            float overlap = (minDist - dist) / dist * 0.5f;
            float offsetX = dx * overlap;
            float offsetY = dy * overlap;

            if (pup->draggedLimb != i) { a->pos.x -= offsetX; a->pos.y -= offsetY; }
            if (pup->draggedLimb != j) { b->pos.x += offsetX; b->pos.y += offsetY; }
        }
    }
}

// =============================================================================
//  PHYSICS ENTRY POINT
// =============================================================================

// One physics tick:
//   1. Integrate every (non-dragged) limb with Verlet.
//   2. Iterate constraints (bones + limb-limb collisions) for stiffness.
//   3. Recompute the bounding box used by the renderer / window sizing.
void ApplyPhysics(Puppet *pup, int screenWidth, int screenHeight)
{
    // 1. Verlet integration
    for (int i = 0; i < LIMB_COUNT; i++)
    {
        if (pup->draggedLimb != i)
            IntegrateLimb(&pup->limbs[i], &pup->physics, screenWidth, screenHeight);
    }

    // 2. Constraint relaxation
    for (int iter = 0; iter < CONSTRAINT_ITERATIONS; iter++)
    {
        UpdateBones(pup);
        ResolveLimbCollisions(pup);
    }

    // 3. Bounds refresh
    pup->bounds = ComputePuppetBounds(pup);
}