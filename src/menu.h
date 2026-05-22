#ifndef MENU_H
#define MENU_H

#include "raylib.h"
#include "puppet.h"
#include "ipc.h"
#include "item.h"

// =============================================================================
//  DECLARATIONS
// =============================================================================

// Stable ids for menu items. Shared between the puppet process (which reacts
// to clicks) and the menu process (which reports the clicked id over a pipe).
enum MenuItemId
{
    MENU_ITEM_RED,
    MENU_ITEM_GREEN,
    MENU_ITEM_BLUE,
    MENU_ITEM_ITEM,
    MENU_ITEM_COUNT
};

typedef struct
{
    int         id;     // id is used to know which item was clicked
    const char *action; // Action of the item (also shown as label)
    Color       color;  // Color of the small rectangle next to the label
} MenuItem;

// Menu visual constants, shared so parent (window placement) and child
// (rendering) agree on the window size.
#define MENU_WIDTH       120
#define MENU_ITEM_HEIGHT 30
#define MENU_ICON_SIZE   16
#define MENU_PADDING     8

// Parent-side menu state: tracks whether a child window is currently open.
typedef struct Menu
{
    bool        isOpen;
    MenuProcess proc;
} Menu;

// Shared menu definition consumed by both processes.
extern const MenuItem MENU_ITEMS[MENU_ITEM_COUNT];

// =============================================================================
//  FUNCTIONS
// =============================================================================

Menu CreateMenu(void);                                                  // Build the (closed) menu state
void OpenMenu(Menu *menu, int screenX, int screenY);                    // Spawn the menu child window
void CloseMenu(Menu *menu);                                             // Close the child window if open
void MenuActions(Puppet *pup, Menu *menu, ItemRegistry *items);         // Poll IPC and apply the chosen action

#endif