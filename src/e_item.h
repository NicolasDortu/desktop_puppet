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
    ITEM_TYPE_COUNT
} ItemType;

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

// Draw the item relative to its bounding-box origin (the window follows bounds).
void      DrawItem(const Item *item);

// Collision shape of a kind, so callers can pick the right resolve routine.
ItemShape ItemShapeOf(ItemType type);

#endif
