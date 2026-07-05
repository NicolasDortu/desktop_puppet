#ifndef PHYSICS_H
#define PHYSICS_H

#include "config.h"

#include <stdbool.h>

#include "raylib.h"

// =============================================================================
//  DECLARATIONS
// =============================================================================

// A single point-mass driven by Verlet Integration.
typedef struct
{
    Vector2 pos;       // current world-space position of the particle's center
    Vector2 oldPos;    // previous world-space position (defines velocity)
    float   radius;    //
    bool    isDragged; // true if the particle is being dragged
} Particle;

// Distance constraint between two particles within a Body.
// Hard bones snap exactly to `length`.
// Soft bones only apply `cfg.stiffness` of the correction each iteration.
typedef struct
{
    int   particle1; // index into Body.particles
    int   particle2; // index into Body.particles
    float length;    // rest length
    bool  soft;      // false = rigid, true = scaled by cfg.stiffness
} Bone;

// A self-contained physics object: particles wired up by bones and related logic.
typedef struct
{
    Particle      *particles;
    int            particleCount;
    Bone          *bones;
    int            boneCount;
    BoundBox       bounds;
    PhysicsConfig  cfg;
    float          wallImpact;   // biggest wall-impact speed seen during the last ApplyPhysics
} Body;

// =============================================================================
//  FUNCTIONS
// =============================================================================

void     ApplyPhysics             (Body *body, BoundBox screen); // screen = usable desktop area (walls)
void     ResolveCirclesCollisions (Particle *a, Particle *b);
void     ResolveCapsuleCircleCollision (Particle *a, Particle *b, Particle *c); // capsule a-b (radius a->radius) vs circle c
void     ResolveCapsulesCollision (Particle *a1, Particle *a2, Particle *b1, Particle *b2); // capsule a1-a2 vs capsule b1-b2
void     ApplyBlastToBody         (Body *body, Vector2 center, float radius, float power);  // radial velocity kick (bomb)
BoundBox ComputeBoundBox          (const Body *body);
float    ParticlesDistance        (const Particle *a, const Particle *b);

#endif