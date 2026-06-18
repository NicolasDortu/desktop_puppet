#ifndef E_ITEM_H
#define E_ITEM_H

#include "physics.h"

#include "raylib.h"

// =============================================================================
//  DECLARATIONS
// =============================================================================

// Item kinds. Each kind picks its size/look from a spec table in e_item.c
typedef enum ItemType
{
    ITEM_BALL,
    ITEM_TYPE_COUNT
} ItemType;

// =============================================================================
//  FUNCTIONS
// =============================================================================

// Build the particle at `startPos` and wire a 1-particle Body around it so it
// reuses ApplyPhysics (verlet + walls) just like the puppet's limbs.
void CreateItem(Particle *p, Body *body, ItemType type, Vector2 startPos);

// Draw the item centered in its own (2R x 2R) window.
void DrawItem(ItemType type);

// Side length (in pixels) of the item's square window.
int  ItemWindowSize(ItemType type);

#endif
