#ifndef E_ITEM_H
#define E_ITEM_H

#include "physics.h"

#include "raylib.h"

// =============================================================================
//  DECLARATIONS
// =============================================================================

// Item kinds. Each kind picks its shape/size/look from a spec table in e_item.c;
// add a new variant here AND a matching ITEM_SPECS entry there.
typedef enum ItemType
{
    ITEM_BALL,
    ITEM_BAT,
    ITEM_BOMB,
    ITEM_TYPE_COUNT
} ItemType;

// Bought items are temporary: everything despawns after ITEM_LIFETIME except
// the bomb, which ends itself much sooner by exploding.
#define ITEM_LIFETIME     60.0  // seconds a bought item stays around

// Bomb tuning: fuse burn time, then a radial velocity kick applied by every
// process to its own particles (see BlastSlot in r_sync.h).
#define BOMB_FUSE_TIME    3.0   // seconds from spawn to detonation
#define BOMB_BOOM_TIME    0.45  // seconds the explosion visual lasts
#define BOMB_BLAST_RADIUS 260.0f // px reach of the blast
#define BOMB_BLAST_POWER  22.0f  // px/frame velocity kick at the center

// Physical shape of an item, used to pick the right collision routine.
typedef enum ItemShape
{
    ITEM_SHAPE_CIRCLE,  // a single particle (radius)
    ITEM_SHAPE_CAPSULE  // two particles + a rigid bone (radius = half thickness)
} ItemShape;

// Backing storage sizes for an item's Body. The largest item (the bat) needs
// two particles and one bone; the ball uses one particle and no bones.
#define ITEM_MAX_PARTICLES 2
#define ITEM_MAX_BONES     1

// An item entity: a small physics Body, just like the puppet. The ball is the
// degenerate 1-particle case; the bat is 2 particles joined by 1 rigid bone.
typedef struct
{
    ItemType type;
    Body     body;                          // physics state (points into the arrays below)
    Particle particles[ITEM_MAX_PARTICLES]; // backing storage for body.particles
    Bone     bones[ITEM_MAX_BONES];         // backing storage for body.bones
} Item;

// =============================================================================
//  FUNCTIONS
// =============================================================================

// Build the item's body at `startPos`. Writes into `*item` so the embedded Body
// keeps valid pointers to item's own arrays (reuses ApplyPhysics like the puppet).
void      CreateItem(Item *item, ItemType type, Vector2 startPos);

// Draw the item relative to its window box origin (the window follows it).
void      DrawItem(const Item *item);

// Window box for an item: body bounds plus headroom for decorations drawn
// outside the physics shape (the bomb's fuse). Feed this to UpdateWindow.
BoundBox  ItemWindowBounds(const Item *item);

// Cartoon blast visual, drawn at `center` (window-local), growing to
// `maxRadius` as `progress` runs 0 -> 1.
void      DrawExplosion(Vector2 center, float maxRadius, float progress);

// Collision shape of a kind, so callers can pick the right resolve routine.
ItemShape ItemShapeOf(ItemType type);

#endif
