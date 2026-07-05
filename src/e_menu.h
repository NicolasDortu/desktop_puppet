#ifndef E_MENU_H
#define E_MENU_H

#include "raylib.h"

// =============================================================================
//  DECLARATIONS
// =============================================================================

// Stable ids for menu items. Shared between the puppet process and the menu process.
enum MenuItemId
{
    MENU_ITEM_BALL,
    MENU_ITEM_BAT,
    MENU_ITEM_BOMB,
    MENU_ITEM_COUNT
};

typedef struct
{
    int         id;     // id is used to know which item was clicked
    const char *action; // Action of the item (also shown as label)
    int         price;  // shop price in coins
} MenuItem;

// Menu visual constants, shared so the parent (window placement) and child
#define MENU_WIDTH       190   // pixel width of a single column (name + price + coin)
#define MENU_ITEM_HEIGHT 34    // pixel height of a single row
#define MENU_HEADER      40    // shop header strip (title + coin balance)
#define MENU_FONT_SIZE   16
#define MENU_PADDING     8
#define MENU_MAX_ROWS    5     // when more items than this, start a new column

// Shared menu definition consumed by both processes.
extern const MenuItem MENU_ITEMS[MENU_ITEM_COUNT];

// =============================================================================
//  FUNCTIONS
// =============================================================================

void MenuWindowSize(int *width, int *height);  // pixel size of the shop window
int  MenuPick(Vector2 mouseLocal);             // id under the cursor, or -1
void DrawMenu(int coins);                      // render the shop (balance from the parent)

#endif
