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
} Body;

// =============================================================================
//  FUNCTIONS
// =============================================================================

void     ApplyPhysics             (Body *body, int screenWidth, int screenHeight);
void     ResolveCirclesCollisions (Particle *a, Particle *b);
void     ResolveCapsuleCircleCollision (Particle *a, Particle *b, Particle *c); // capsule a-b (radius a->radius) vs circle c
BoundBox ComputeBoundBox          (const Body *body);
float    ParticlesDistance        (const Particle *a, const Particle *b);

#endif