#ifndef SYNC_H
#define SYNC_H

#include "e_puppet.h"

// =============================================================================
//  SHARED MEMORY LAYOUT
// =============================================================================
//
// Single region created by the puppet and mapped by every child. The struct IS
// the synced state: the puppet publishes its limbs, each child publishes its
// own particle, and the menu child publishes its clicked id. Torn reads are
// harmless because every field is rewritten the next frame.

#define MAX_ITEMS 16

typedef struct
{
    Particle particle; // written by the item child every frame
    bool     active;   // true once the child has published at least once
} ItemSlot;

typedef struct
{
    int  chosenId; // written by the menu child on click
    bool done;     // set true by the menu child once a choice is made
} MenuSlot;

typedef struct SharedState
{
    int      limbCount;         // puppet -> children
    Particle limbs[LIMB_COUNT]; // puppet -> children (refreshed each frame)
    ItemSlot items[MAX_ITEMS];  // each item child -> puppet (one slot per child)
    MenuSlot menu;              // menu child -> puppet
} SharedState;

#endif
