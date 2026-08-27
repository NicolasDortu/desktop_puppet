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
// `wallImpact` accumulates the biggest incoming speed among wall contacts, so
// the caller can tell a hard crash from resting contact.
static void IntegrateParticle(Particle *particle, BoundBox screen, float *wallImpact)
{
    // --- Derive velocity from last frame's displacement, apply friction ---
    float vx = (particle->pos.x - particle->oldPos.x) * PHYS_FRICTION;
    float vy = (particle->pos.y - particle->oldPos.y) * PHYS_FRICTION;

    // --- Kill micro-velocities to stop residual jitter on a settled puppet ---
    if (vx * vx + vy * vy < PHYS_MIN_BOUNCE * PHYS_MIN_BOUNCE)
    {
        vx = 0.0f;
        vy = 0.0f;
    }

    // --- Integrate: oldPos <- pos, then move pos by velocity + gravity ---
    particle->oldPos = particle->pos;
    particle->pos.x += vx;
    particle->pos.y += vy + PHYS_GRAVITY;

    // --- Wall collisions: clamp position, reflect implicit velocity ---
    // Walls are the edges of the usable desktop area (taskbar excluded).
    if (particle->pos.y + particle->radius > screen.y + screen.h)  // floor
    {
        particle->pos.y    = screen.y + screen.h - particle->radius;
        particle->oldPos.y = particle->pos.y - vy * PHYS_BOUNCE;
        if (vy > *wallImpact) *wallImpact = vy;
    }
    if (particle->pos.y - particle->radius < screen.y)             // ceiling
    {
        particle->pos.y    = screen.y + particle->radius;
        particle->oldPos.y = particle->pos.y - vy * PHYS_BOUNCE;
        if (-vy > *wallImpact) *wallImpact = -vy;
    }
    if (particle->pos.x + particle->radius > screen.x + screen.w)  // right wall
    {
        particle->pos.x    = screen.x + screen.w - particle->radius;
        particle->oldPos.x = particle->pos.x - vx * PHYS_BOUNCE;
        if (vx > *wallImpact) *wallImpact = vx;
    }
    if (particle->pos.x - particle->radius < screen.x)             // left wall
    {
        particle->pos.x    = screen.x + particle->radius;
        particle->oldPos.x = particle->pos.x - vx * PHYS_BOUNCE;
        if (-vx > *wallImpact) *wallImpact = -vx;
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

static float Clamp01(float v)
{
    return (v < 0.0f) ? 0.0f : (v > 1.0f) ? 1.0f : v;
}

// Closest points between segments p1-q1 and p2-q2 (Ericson, RTCD 5.1.9).
// Writes the segment parameters to *outS / *outT (0 = start, 1 = end).
static void ClosestPointsSegments(Vector2 p1, Vector2 q1, Vector2 p2, Vector2 q2,
                                  float *outS, float *outT)
{
    float d1x = q1.x - p1.x, d1y = q1.y - p1.y; // direction of segment A
    float d2x = q2.x - p2.x, d2y = q2.y - p2.y; // direction of segment B
    float rx  = p1.x - p2.x, ry  = p1.y - p2.y;
    float a   = d1x * d1x + d1y * d1y;          // squared length of A
    float e   = d2x * d2x + d2y * d2y;          // squared length of B
    float f   = d2x * rx + d2y * ry;
    float s, t;

    if (a <= 1e-6f && e <= 1e-6f)      // both degenerate to points
    {
        s = t = 0.0f;
    }
    else if (a <= 1e-6f)               // A is a point
    {
        s = 0.0f;
        t = Clamp01(f / e);
    }
    else
    {
        float c = d1x * rx + d1y * ry;
        if (e <= 1e-6f)                // B is a point
        {
            t = 0.0f;
            s = Clamp01(-c / a);
        }
        else                           // the general case
        {
            float b     = d1x * d2x + d1y * d2y;
            float denom = a * e - b * b;         // >= 0; 0 when parallel
            s = (denom > 1e-6f) ? Clamp01((b * f - c * e) / denom) : 0.0f;
            t = (b * s + f) / e;
            if      (t < 0.0f) { t = 0.0f; s = Clamp01(-c / a); }
            else if (t > 1.0f) { t = 1.0f; s = Clamp01((b - c) / a); }
        }
    }

    *outS = s;
    *outT = t;
}

// Push two capsules apart: A = segment a1-a2 with radius a1->radius, B =
// segment b1-b2 with radius b1->radius. Contact is the closest point pair
// between the segments (which also catches two shafts crossing mid-segment).
// Mirrors the dragged/share semantics of the other resolvers; each capsule's
// correction is distributed to its endpoints by proximity to the contact.
void ResolveCapsulesCollision(Particle *a1, Particle *a2, Particle *b1, Particle *b2)
{
    float s, t;
    ClosestPointsSegments(a1->pos, a2->pos, b1->pos, b2->pos, &s, &t);

    Vector2 pa = { a1->pos.x + (a2->pos.x - a1->pos.x) * s,
                   a1->pos.y + (a2->pos.y - a1->pos.y) * s };
    Vector2 pb = { b1->pos.x + (b2->pos.x - b1->pos.x) * t,
                   b1->pos.y + (b2->pos.y - b1->pos.y) * t };

    float dx      = pb.x - pa.x;
    float dy      = pb.y - pa.y;
    float dist2   = dx * dx + dy * dy;
    float minDist = a1->radius + b1->radius;

    if (dist2 >= minDist * minDist || dist2 < 1e-6f)
        return;

    float dist    = sqrtf(dist2);
    float nx      = dx / dist;
    float ny      = dy / dist;
    float overlap = minDist - dist;

    bool  aDragged = a1->isDragged || a2->isDragged;
    bool  bDragged = b1->isDragged || b2->isDragged;
    float aShare   = (aDragged && !bDragged) ? 0.0f
                   : (bDragged && !aDragged) ? 1.0f : 0.5f;
    float bShare   = 1.0f - aShare;

    // A backs away against the normal, B along it, endpoint-weighted.
    if (!a1->isDragged) { a1->pos.x -= nx * overlap * aShare * (1.0f - s); a1->pos.y -= ny * overlap * aShare * (1.0f - s); }
    if (!a2->isDragged) { a2->pos.x -= nx * overlap * aShare * s;          a2->pos.y -= ny * overlap * aShare * s;          }
    if (!b1->isDragged) { b1->pos.x += nx * overlap * bShare * (1.0f - t); b1->pos.y += ny * overlap * bShare * (1.0f - t); }
    if (!b2->isDragged) { b2->pos.x += nx * overlap * bShare * t;          b2->pos.y += ny * overlap * bShare * t;          }
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
        float stiffness = bone->soft ? PHYS_STIFFNESS : 1.0f;
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
//  BLAST
// =============================================================================

// Kick every particle within `radius` of `center` straight away from it.
// `power` is the velocity injected at the center, fading linearly to zero at
// the edge. Verlet: pushing oldPos back adds velocity without moving the
// particle; the next integration step turns it into motion.
void ApplyBlastToBody(Body *body, Vector2 center, float radius, float power)
{
    for (int i = 0; i < body->particleCount; i++)
    {
        Particle *p = &body->particles[i];
        if (p->isDragged)
            continue;

        float dx    = p->pos.x - center.x;
        float dy    = p->pos.y - center.y;
        float dist2 = dx * dx + dy * dy;
        if (dist2 >= radius * radius || dist2 < 1e-6f)
            continue;

        float dist = sqrtf(dist2);
        float kick = power * (1.0f - dist / radius) / dist; // /dist normalizes (dx,dy)
        p->oldPos.x -= dx * kick;
        p->oldPos.y -= dy * kick;
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

// Inflate a window box by speed-proportional slack. SetWindowPosition takes
// effect with about a frame of lag, so at high speed the drawing would land
// outside the real window and clip away mid-throw; the slack keeps ~two
// frames of travel inside.
BoundBox AddSpeedSlack(const Body *body, BoundBox b)
{
    float maxV2 = 0.0f;
    for (int i = 0; i < body->particleCount; i++)
    {
        float vx = body->particles[i].pos.x - body->particles[i].oldPos.x;
        float vy = body->particles[i].pos.y - body->particles[i].oldPos.y;
        float v2 = vx * vx + vy * vy;
        if (v2 > maxV2)
            maxV2 = v2;
    }

    float slack = 4.0f + 2.0f * sqrtf(maxV2);
    b.x -= slack;
    b.y -= slack;
    b.w += 2.0f * slack;
    b.h += 2.0f * slack;
    return b;
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
    body->wallImpact = 0.0f;
    for (int i = 0; i < body->particleCount; i++)
    {
        if (!body->particles[i].isDragged)
            IntegrateParticle(&body->particles[i], screen, &body->wallImpact);
    }

    for (int iter = 0; iter < CONSTRAINT_ITERATIONS; iter++)
        UpdateBones(body);

    body->bounds = ComputeBoundBox(body);
}