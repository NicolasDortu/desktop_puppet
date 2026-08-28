#ifndef E_MENU_H
#define E_MENU_H

#include "e_item.h"

#include "raylib.h"

// =============================================================================
//  DECLARATIONS
// =============================================================================

// The shop sells exactly the item kinds: rows are indexed by ItemType, so a
// clicked row IS the item to spawn (no separate menu-id enum to keep in sync).
typedef struct
{
    const char *label; // shown in the row
    int         price; // shop price in coins
} MenuItem;

// Menu visual constants, shared between the parent (window placement) and child.
#define MENU_WIDTH       190   // pixel width of the shop (name + price + coin)
#define MENU_ITEM_HEIGHT 34    // pixel height of a single row
#define MENU_HEADER      40    // shop header strip (title + coin balance)
#define MENU_FONT_SIZE   16
#define MENU_PADDING     8

// Shared menu definition consumed by both processes, indexed by ItemType.
extern const MenuItem MENU_ITEMS[ITEM_TYPE_COUNT];

// =============================================================================
//  FUNCTIONS
// =============================================================================

void MenuWindowSize(int *width, int *height);  // pixel size of the shop window
int  MenuPick(Vector2 mouseLocal);             // ItemType under the cursor, or -1 (footer excluded)
bool MenuPickSound(Vector2 mouseLocal);        // true if the cursor is on the sound-toggle footer
void DrawMenu(int coins, bool muted);          // render the shop (balance + mute from shared state)

#endif
