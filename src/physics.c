#include "physics.h"

#include <math.h>

// Number of constraint-solver passes per frame.
#define CONSTRAINT_ITERATIONS 3

// =============================================================================
//  VERLET INTEGRATION
// =============================================================================

// Advance a single particle by one frame using Verlet Integration.
//
// In Verlet, velocity is implicit:  v = pos - oldPos
// We derive v, apply friction & gravity, move the particle, then handle wall collisions
// by reflecting `oldPos` so the next frame's implicit velocity points away from the wall.
static void IntegrateParticle(Particle *particle, const PhysicsConfig *cfg, int screenWidth, int screenHeight)
{
    // --- Derive velocity from last frame's displacement, apply friction ---
    float vx = (particle->pos.x - particle->oldPos.x) * cfg->friction;
    float vy = (particle->pos.y - particle->oldPos.y) * cfg->friction;

    // --- Kill micro-velocities to stop residual jitter on a settled puppet ---
    if (vx * vx + vy * vy < cfg->minBounce * cfg->minBounce)
    {
        vx = 0.0f;
        vy = 0.0f;
    }

    // --- Integrate: oldPos <- pos, then move pos by velocity + gravity ---
    particle->oldPos = particle->pos;
    particle->pos.x += vx;
    particle->pos.y += vy + cfg->gravity;

    // --- Wall collisions: clamp position, reflect implicit velocity ---
    if (particle->pos.y + particle->radius > screenHeight)  // floor
    {
        particle->pos.y    = screenHeight - particle->radius;
        particle->oldPos.y = particle->pos.y - vy * cfg->bounce;
    }
    if (particle->pos.y - particle->radius < 0)             // ceiling
    {
        particle->pos.y    = particle->radius;
        particle->oldPos.y = particle->pos.y - vy * cfg->bounce;
    }
    if (particle->pos.x + particle->radius > screenWidth)   // right wall
    {
        particle->pos.x    = screenWidth - particle->radius;
        particle->oldPos.x = particle->pos.x - vx * cfg->bounce;
    }
    if (particle->pos.x - particle->radius < 0)             // left wall
    {
        particle->pos.x    = particle->radius;
        particle->oldPos.x = particle->pos.x - vx * cfg->bounce;
    }
}

// =============================================================================
//  CONSTRAINT SOLVING
// =============================================================================

// Push two overlapping circles apart along their center-to-center axis.
void ResolveCirclesCollisions(Particle *a, Particle *b)
{
    float dx      = b->pos.x - a->pos.x;
    float dy      = b->pos.y - a->pos.y;
    float dist2   = dx * dx + dy * dy;
    float minDist = a->radius + b->radius;

    if (dist2 >= minDist * minDist || dist2 < 1e-6f)
        return;

    float dist    = sqrtf(dist2);
    float share   = (a->isDragged || b->isDragged) ? 1.0f : 0.5f; // if a particle is dragged, the other absord 100% of the overlap
    float overlap = (minDist - dist) / dist * share;
    float offsetX = dx * overlap;
    float offsetY = dy * overlap;

    if (!a->isDragged) { a->pos.x -= offsetX; a->pos.y -= offsetY; }
    if (!b->isDragged) { b->pos.x += offsetX; b->pos.y += offsetY; }
}

// Push every pair of overlapping particles in `body` apart.
static void ResolveBodyCollisions(Body *body)
{
    for (int i = 0; i < body->particleCount; i++)
    {
        for (int j = i + 1; j < body->particleCount; j++)
        {
            ResolveCirclesCollisions(&body->particles[i], &body->particles[j]);
        }
    }
}

// Pull each pair of bone-connected particles back to the bone's rest length.
// Hard bones snap exactly; soft bones apply only `stiffness` of the correction.
static void UpdateBones(Body *body)
{
    for (int i = 0; i < body->boneCount; i++)
    {
        Bone     *bone  = &body->bones[i];
        Particle *a     = &body->particles[bone->particle1];
        Particle *b     = &body->particles[bone->particle2];

        // --- Current vector from a to b ---
        float dx       = b->pos.x - a->pos.x;
        float dy       = b->pos.y - a->pos.y;
        float distance = sqrtf(dx * dx + dy * dy);

        if (distance < 1e-6f) continue;  // avoid division by zero on coincident limbs

        // --- Compute the per-limb correction ---
        float stiffness = bone->soft ? body->cfg.stiffness : 1.0f;
        float error     = distance - bone->length;                  // how far the bone is from its rest length
        float percent   = (error / distance) * 0.5f * stiffness;    // fraction of the error each endpoint must absorb

        float offsetX = dx * percent;
        float offsetY = dy * percent;

        // --- Move both endpoints toward each other (or apart) ---
        if (!a->isDragged) { a->pos.x += offsetX; a->pos.y += offsetY; }
        if (!b->isDragged) { b->pos.x -= offsetX; b->pos.y -= offsetY; }
    }
}

// =============================================================================
//  GEOMETRY QUERIES
// =============================================================================

// Euclidean distance between the centers of two particles.
float ParticlesDistance(const Particle *a, const Particle *b)
{
    float dx = b->pos.x - a->pos.x;
    float dy = b->pos.y - a->pos.y;
    return sqrtf(dx * dx + dy * dy);
}

// =============================================================================
//  BOUNDS
// =============================================================================

// Axis-aligned bounding box around every particle (accounting for their radii).
BoundBox ComputeBoundBox(const Body *body)
{
    float minX =  1e30f, minY =  1e30f;
    float maxX = -1e30f, maxY = -1e30f;

    for (int i = 0; i < body->particleCount; i++)
    {
        const Particle *particle = &body->particles[i];

        float lx = particle->pos.x - particle->radius;
        float ly = particle->pos.y - particle->radius;
        float rx = particle->pos.x + particle->radius;
        float ry = particle->pos.y + particle->radius;

        if (lx < minX) minX = lx;
        if (ly < minY) minY = ly;
        if (rx > maxX) maxX = rx;
        if (ry > maxY) maxY = ry;
    }

    return (BoundBox){.x = minX, .y = minY, .w = maxX - minX, .h = maxY - minY};
}

// =============================================================================
//  PHYSICS ENTRY POINT
// =============================================================================

// One physics tick:
//   1. Integrate every (non-dragged) particle with Verlet.
//   2. Iterate constraints (bones + particle-particle collisions) for stiffness.
//   3. Recompute the bounding box used by the renderer / window sizing.
void ApplyPhysics(Body *body, int screenWidth, int screenHeight)
{
    for (int i = 0; i < body->particleCount; i++)
    {
        if (!body->particles[i].isDragged)
            IntegrateParticle(&body->particles[i], &body->cfg, screenWidth, screenHeight);
    }

    for (int iter = 0; iter < CONSTRAINT_ITERATIONS; iter++)
    {
        UpdateBones(body);
        ResolveBodyCollisions(body);
    }

    body->bounds = ComputeBoundBox(body);
}