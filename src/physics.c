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
static void IntegrateParticle(Particle *particle, const PhysicsConfig *cfg, BoundBox screen)
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
    // Walls are the edges of the usable desktop area (taskbar excluded).
    if (particle->pos.y + particle->radius > screen.y + screen.h)  // floor
    {
        particle->pos.y    = screen.y + screen.h - particle->radius;
        particle->oldPos.y = particle->pos.y - vy * cfg->bounce;
    }
    if (particle->pos.y - particle->radius < screen.y)             // ceiling
    {
        particle->pos.y    = screen.y + particle->radius;
        particle->oldPos.y = particle->pos.y - vy * cfg->bounce;
    }
    if (particle->pos.x + particle->radius > screen.x + screen.w)  // right wall
    {
        particle->pos.x    = screen.x + screen.w - particle->radius;
        particle->oldPos.x = particle->pos.x - vx * cfg->bounce;
    }
    if (particle->pos.x - particle->radius < screen.x)             // left wall
    {
        particle->pos.x    = screen.x + particle->radius;
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

// Closest point to `p` on the segment a-b. Writes the point to `*out` and
// returns its parameter `t` along the segment (0 = a, 1 = b).
static float ClosestPointOnSegment(Vector2 a, Vector2 b, Vector2 p, Vector2 *out)
{
    float abx = b.x - a.x;
    float aby = b.y - a.y;
    float len2 = abx * abx + aby * aby;

    float t = 0.0f;
    if (len2 > 1e-6f)
    {
        t = ((p.x - a.x) * abx + (p.y - a.y) * aby) / len2;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
    }

    out->x = a.x + abx * t;
    out->y = a.y + aby * t;
    return t;
}

// Push a circle `c` out of the capsule formed by segment a-b with radius
// a->radius. The capsule's share of the correction is split between its
// endpoints by how close the contact is to each (the nearer end moves more).
// Mirrors ResolveCirclesCollisions' dragged/share semantics.
void ResolveCapsuleCircleCollision(Particle *a, Particle *b, Particle *c)
{
    Vector2 closest;
    float t = ClosestPointOnSegment(a->pos, b->pos, c->pos, &closest);

    float dx      = c->pos.x - closest.x;
    float dy      = c->pos.y - closest.y;
    float dist2   = dx * dx + dy * dy;
    float minDist = a->radius + c->radius;

    if (dist2 >= minDist * minDist || dist2 < 1e-6f)
        return;

    float dist = sqrtf(dist2);
    float nx   = dx / dist;
    float ny   = dy / dist;
    float overlap = minDist - dist;

    // Split the overlap between the capsule and the circle (a dragged side
    // absorbs none, the other takes it all; otherwise 50/50).
    bool  segDragged = a->isDragged || b->isDragged;
    float segShare   = (segDragged && !c->isDragged) ? 0.0f
                     : (c->isDragged && !segDragged) ? 1.0f : 0.5f;
    float cShare     = 1.0f - segShare;

    // Circle moves out along the normal.
    if (!c->isDragged)
    {
        c->pos.x += nx * overlap * cShare;
        c->pos.y += ny * overlap * cShare;
    }

    // Capsule moves in, distributed to its endpoints by proximity to contact.
    float wA = 1.0f - t;
    float wB = t;
    if (!a->isDragged)
    {
        a->pos.x -= nx * overlap * segShare * wA;
        a->pos.y -= ny * overlap * segShare * wA;
    }
    if (!b->isDragged)
    {
        b->pos.x -= nx * overlap * segShare * wB;
        b->pos.y -= ny * overlap * segShare * wB;
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
//   2. Iterate the bone constraints for stiffness.
//   3. Recompute the bounding box used by the renderer / window sizing.
//
// A body's own particles do NOT collide with each other: puppet limbs sit
// partly embedded in the body by design, and the bone network (with its
// diagonals) is what holds the shape.
void ApplyPhysics(Body *body, BoundBox screen)
{
    for (int i = 0; i < body->particleCount; i++)
    {
        if (!body->particles[i].isDragged)
            IntegrateParticle(&body->particles[i], &body->cfg, screen);
    }

    for (int iter = 0; iter < CONSTRAINT_ITERATIONS; iter++)
        UpdateBones(body);

    body->bounds = ComputeBoundBox(body);
}