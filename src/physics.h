#ifndef PHYSICS_H
#define PHYSICS_H

#include "config.h"

#include <stdbool.h>

#include "raylib.h"

// =============================================================================
//  TYPES
// =============================================================================

// A single point-mass driven by Verlet integration.
typedef struct
{
    Vector2 pos;    // current world-space position of the particle's center
    Vector2 oldPos; // previous world-space position (defines velocity)
    float   radius;
} Particle;

// Distance constraint between two particles within a Body.
// Hard bones snap exactly to `length`; soft bones only apply `cfg.stiffness`
// of the correction each iteration.
typedef struct
{
    int   particle1; // index into Body.particles
    int   particle2; // index into Body.particles
    float length;    // rest length
    bool  soft;      // false = rigid, true = scaled by cfg.stiffness
} Bone;

// A self-contained physics object: particles wired up by bones, plus tuning
// and a cached bounding box refreshed every tick.
typedef struct
{
    Particle      *particles;       // not owned; backed by the entity
    int            particleCount;
    int            draggedParticle; // index of the pinned particle, or -1

    Bone          *bones;           // may be NULL
    int            boneCount;

    BoundBox       bounds;          // refreshed by ApplyPhysics

    PhysicsConfig  cfg;
} Body;

// =============================================================================
//  FUNCTIONS
// =============================================================================

// One physics tick on `body`: verlet integration + constraint solving + bounds refresh.
void     ApplyPhysics      (Body *body, int screenWidth, int screenHeight);

// Push two overlapping circles apart. `moveA`/`moveB` choose which side
// absorbs the correction: 50/50 if both, full on the moving side otherwise.
void     ResolveCircles    (Particle *a, Particle *b, bool moveA, bool moveB);

// Axis-aligned bounding box around every particle of `body`.
BoundBox ComputeBoundBox   (const Body *body);

// Euclidean distance between the centers of two particles.
float    ParticlesDistance (const Particle *a, const Particle *b);

#endif