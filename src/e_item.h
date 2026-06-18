#ifndef E_ITEM_H
#define E_ITEM_H

#include "e_puppet.h"
#include "ipc.h"

// =============================================================================
//  DECLARATIONS
// =============================================================================

#define MAX_ITEMS 16

// Item kinds. The child process picks its size/look from a spec table in
// e_item.c; add a new variant here AND a matching ITEM_SPECS entry there to
// introduce a new item kind (e.g. a cube, a balloon...).
typedef enum ItemType
{
    ITEM_BALL,
    ITEM_TYPE_COUNT
} ItemType;

// Latest state received from an item child process, plus its IPC handle.
typedef struct
{
    ChildPipe proc;
    ItemType  type;
    bool      hasState; // true after the first STATE message has been received
    Particle  particle; // item physical state in screen coords (radius depends on type)
} Item;

typedef struct
{
    Item items[MAX_ITEMS];
} ItemRegistry;

// =============================================================================
//  FUNCTIONS
// =============================================================================

ItemRegistry CreateItemRegistry(void);
bool         SpawnItem(ItemRegistry *reg, ItemType type, int posX, int posY); // launches a child of the requested kind
void         UpdateItems(ItemRegistry *reg, Puppet *pup);                     // poll IPC, collide, broadcast puppet state
void         CloseAllItems(ItemRegistry *reg);                                // terminate every live child
int          RunItem  (int argc, char **argv);

#endif
