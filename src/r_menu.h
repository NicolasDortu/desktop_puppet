#ifndef R_MENU_H
#define R_MENU_H

#include "e_puppet.h"
#include "r_sync.h"
#include "r_item.h" // MenuActions spawns items (ItemRegistry / SpawnItem)
#include "ipc.h"

// =============================================================================
//  MENU ROLE
// =============================================================================
//
//  The menu child runs a short-lived popup window (see RunMenu) and reports the
//  clicked item into the SharedState MenuSlot. The puppet process owns a Menu
//  and uses the parent-side API below to open it, poll the result and dispatch
//  the chosen action.

typedef struct
{
    bool      isOpen;
    ChildProc proc;
} Menu;

// =============================================================================
//  FUNCTIONS
// =============================================================================

// Parent side.
void OpenMenu(Menu *menu, SharedState *shared, unsigned long parentPid,
              int screenX, int screenY);
void CloseMenu(Menu *menu);
void MenuActions(Puppet *pup, Menu *menu, ItemRegistry *reg,
                 SharedState *shared, unsigned long parentPid);  // poll click + dispatch
void ToggleMenu(Puppet *pup, Menu *menu, SharedState *shared, unsigned long parentPid);

// Child entry point.
int  RunMenu(int argc, char **argv);

#endif
