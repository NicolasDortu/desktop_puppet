#ifndef E_MENU_H
#define E_MENU_H

#include "raylib.h"

// =============================================================================
//  DECLARATIONS
// =============================================================================

// Stable ids for menu items. Shared between the puppet process and the menu process.
enum MenuItemId
{
    MENU_ITEM_RED,
    MENU_ITEM_GREEN,
    MENU_ITEM_BLUE,
    MENU_ITEM_BALL,
    MENU_ITEM_BAT,
    MENU_ITEM_COUNT
};

typedef struct
{
    int         id;     // id is used to know which item was clicked
    const char *action; // Action of the item (also shown as label)
    Color       color;  // Color of the small rectangle next to the label
} MenuItem;

// Menu visual constants, shared so the parent (window placement) and child
#define MENU_WIDTH       120   // pixel width of a single column
#define MENU_ITEM_HEIGHT 30    // pixel height of a single row
#define MENU_ICON_SIZE   16
#define MENU_PADDING     8
#define MENU_MAX_ROWS    5     // when more items than this, start a new column

// Shared menu definition consumed by both processes.
extern const MenuItem MENU_ITEMS[MENU_ITEM_COUNT];

// =============================================================================
//  FUNCTIONS
// =============================================================================

void MenuWindowSize(int *width, int *height);  // pixel size of the menu window
int  MenuPick(Vector2 mouseLocal);             // id under the cursor, or -1
void DrawMenu(void);                           // render all items into the window

#endif
