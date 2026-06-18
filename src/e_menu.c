#include "e_menu.h"

#include "raylib.h"

// =============================================================================
//  ITEM TABLE
// =============================================================================

// Single source of truth for menu items.
const MenuItem MENU_ITEMS[MENU_ITEM_COUNT] = {
    {.id = MENU_ITEM_RED,   .action = "RED",   .color = RED   },
    {.id = MENU_ITEM_GREEN, .action = "GREEN", .color = GREEN },
    {.id = MENU_ITEM_BLUE,  .action = "BLUE",  .color = BLUE  },
    {.id = MENU_ITEM_ITEM,  .action = "ITEM",  .color = GRAY  },
};

static const Color MENU_BG_COLOR = {40, 40, 40, 230};

// =============================================================================
//  LAYOUT
// =============================================================================
//
// Items are laid out top-to-bottom, then in a new column.
// columns = ceil(count / MENU_MAX_ROWS)
// rows    = min(count, MENU_MAX_ROWS)

static int MenuColumns(void) { return (MENU_ITEM_COUNT + MENU_MAX_ROWS - 1) / MENU_MAX_ROWS; }
static int MenuRows   (void) { return (MENU_ITEM_COUNT < MENU_MAX_ROWS) ? MENU_ITEM_COUNT : MENU_MAX_ROWS; }
static int MenuItemCol(int i) { return i / MENU_MAX_ROWS; }
static int MenuItemRow(int i) { return i % MENU_MAX_ROWS; }

void MenuWindowSize(int *width, int *height)
{
    *width  = MENU_WIDTH       * MenuColumns();
    *height = MENU_ITEM_HEIGHT * MenuRows();
}

// Map a window-local cursor position to the menu item id under it, or -1.
int MenuPick(Vector2 mouseLocal)
{
    int col = (int)(mouseLocal.x / MENU_WIDTH);
    int row = (int)(mouseLocal.y / MENU_ITEM_HEIGHT);
    int i   = col * MENU_MAX_ROWS + row;

    if (col >= 0 && col < MenuColumns() &&
        row >= 0 && row < MENU_MAX_ROWS &&
        i   >= 0 && i   < MENU_ITEM_COUNT)
        return MENU_ITEMS[i].id;
    return -1;
}

// =============================================================================
//  RENDERING
// =============================================================================

// Walk items in declaration order, placing each into its (col, row) cell.
void DrawMenu(void)
{
    ClearBackground(MENU_BG_COLOR);
    for (int i = 0; i < MENU_ITEM_COUNT; i++)
    {
        int x = MenuItemCol(i) * MENU_WIDTH;
        int y = MenuItemRow(i) * MENU_ITEM_HEIGHT;
        DrawRectangle(x, y + MENU_PADDING, MENU_ICON_SIZE, MENU_ICON_SIZE, MENU_ITEMS[i].color);
        DrawText(MENU_ITEMS[i].action,
                 x + MENU_ICON_SIZE + MENU_PADDING,
                 y + MENU_PADDING,
                 MENU_ICON_SIZE,
                 RAYWHITE);
    }
}
