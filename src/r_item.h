#ifndef R_ITEM_H
#define R_ITEM_H

#include "e_puppet.h"
#include "e_item.h"
#include "r_sync.h"
#include "ipc.h"

// =============================================================================
//  ITEM ROLE
// =============================================================================
//
//  Item children each run in their own window (see RunItem) and publish their
//  particle into a SharedState slot. The puppet process owns an ItemRegistry
//  and uses the parent-side API below to spawn/update/close them.

// Parent-side bookkeeping for one item child: its process handle and kind. The
// item's physical state lives in SharedState.items[i]; this only tracks the
// child process.
typedef struct
{
    ChildProc proc;
    ItemType  type;
    bool      active;
} ItemSlotMeta;

typedef struct
{
    ItemSlotMeta items[MAX_ITEMS];
} ItemRegistry;

// =============================================================================
//  FUNCTIONS
// =============================================================================

// Parent side.
bool SpawnItem(ItemRegistry *reg, SharedState *shared, unsigned long parentPid,
               ItemType type, int posX, int posY);       // launch a child of the requested kind
// Publish limbs, collide, reap. Returns the biggest limb displacement caused
// by an item this frame (px) — the puppet's "how hard was I hit" signal.
float UpdateItems(ItemRegistry *reg, SharedState *shared, Puppet *pup);
void CloseAllItems(ItemRegistry *reg);                                 // terminate every live child

// Child entry point.
int  RunItem(int argc, char **argv);

#endif
