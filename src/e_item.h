#ifndef E_ITEM_H
#define E_ITEM_H

#include "e_puppet.h"
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
    Particle  ball;     // ball state in screen coords
} Item;

typedef struct
{
    Item items[MAX_ITEMS];
} ItemRegistry;

// =============================================================================
//  FUNCTIONS
// =============================================================================

ItemRegistry CreateItemRegistry(void);
bool         SpawnItem(ItemRegistry *reg, int posX, int posY);  // launches bin/item.exe
void         UpdateItems(ItemRegistry *reg, Puppet *pup);       // poll IPC, collide, broadcast puppet state
void         CloseAllItems(ItemRegistry *reg);                  // terminate every live child
int          RunItem  (int argc, char **argv);

#endif
