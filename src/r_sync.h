#ifndef R_SYNC_H
#define R_SYNC_H

#include "e_puppet.h"
#include "e_item.h"
#include "ipc.h"

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
    Particle particles[ITEM_MAX_PARTICLES]; // written by the item child every frame
    ItemType type;                          // kind of item in this slot (picks the collision shape)
    bool     active;                        // true once the child has published at least once
} ItemSlot;

typedef struct
{
    int  chosenId; // written by the menu child on click
    bool done;     // set true by the menu child once a choice is made
} MenuSlot;

// One-shot blast event, written by an exploding bomb child. Every process
// applies the kick to its OWN particles when it sees `seq` change (each keeps
// its last-seen value). ponytail: single slot — two bombs detonating in the
// same instant may lose one blast; add a small ring buffer if that ever hurts.
typedef struct
{
    Vector2      pos;    // blast center (screen space)
    float        radius; // blast reach in px
    float        power;  // velocity kick at the center, px/frame
    unsigned int seq;    // bumped once per detonation, AFTER the fields above
} BlastSlot;

typedef struct SharedState
{
    int       limbCount;         // puppet -> children
    Particle  limbs[LIMB_COUNT]; // puppet -> children (refreshed each frame)
    ItemSlot  items[MAX_ITEMS];  // each item child -> puppet (one slot per child)
    MenuSlot  menu;              // menu child -> puppet
    BlastSlot blast;             // exploding bomb -> everyone
} SharedState;

// =============================================================================
//  REGION SETUP
// =============================================================================

// Parent: create the shared region (named after our own PID) and map it.
// Returns the mapped view in `*shared` and our PID in `*selfPid` (to pass to
// children). Returns false on failure.
bool SyncHostCreate(ShmRegion *shm, SharedState **shared, unsigned long *selfPid);

// Child: open the parent's region (named after `parentPid`) and grab a handle
// used to detect when the parent dies. Returns false if the region can't be
// mapped.
bool SyncChildAttach(unsigned long parentPid, ShmRegion *shm,
                     SharedState **shared, void **parentH);

#endif
