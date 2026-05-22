#ifndef ITEM_H
#define ITEM_H

#include "puppet.h"
#include "ipc.h"

// =============================================================================
//  DECLARATIONS
// =============================================================================

#define MAX_ITEMS 16

// Latest state received from an item child process, plus its IPC handle.
typedef struct
{
    ChildPipe proc;
    bool      hasState; // true after the first BALL message has been received
    float     x, y;     // current ball center (screen coords)
    float     oldX, oldY;
    float     r;
} Item;

typedef struct
{
    Item items[MAX_ITEMS];
} ItemRegistry;

// =============================================================================
//  FUNCTIONS
// =============================================================================

ItemRegistry CreateItemRegistry(void);
bool         SpawnItem(ItemRegistry *reg, int posX, int posY); // launches bin/item.exe
void         UpdateItems(ItemRegistry *reg, Puppet *pup);       // poll IPC, collide, broadcast puppet state
void         CloseAllItems(ItemRegistry *reg);                  // terminate every live child

#endif
