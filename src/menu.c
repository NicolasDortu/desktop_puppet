#include "menu.h"
#include "puppet.h"
#include "ipc.h"

#include "raylib.h"

// =============================================================================
//  SHARED ITEM TABLE
// =============================================================================

// Single source of truth for menu items, linked into both the puppet and the
// menu executables (menu_window.c declares it `extern`).
const MenuItem MENU_ITEMS[MENU_ITEM_COUNT] = {
    {.id = MENU_ITEM_RED,   .action = "RED",   .color = RED   },
    {.id = MENU_ITEM_GREEN, .action = "GREEN", .color = GREEN },
    {.id = MENU_ITEM_BLUE,  .action = "BLUE",  .color = BLUE  },
    {.id = MENU_ITEM_ITEM,  .action = "ITEM",  .color = GRAY  },
};

// =============================================================================
//  MENU IMPLEMENTATION
// =============================================================================

// Build a fresh, closed menu with no live child process.
Menu CreateMenu(void)
{
    return (Menu){
        .isOpen = false,
        .proc   = {0},
    };
}

// Spawn the menu child window at the given screen position.
void OpenMenu(Menu *menu, int screenX, int screenY)
{
    if (menu->isOpen)
        return;

    if (IpcSpawnMenu(&menu->proc, screenX, screenY))
        menu->isOpen = true;
}

// Terminate the child window (if any) and reset state.
void CloseMenu(Menu *menu)
{
    if (!menu->isOpen)
        return;
    IpcCloseMenu(&menu->proc);
    menu->isOpen = false;
}

// Poll the IPC pipe for a click result and dispatch the corresponding action.
void MenuActions(Puppet *pup, Menu *menu, ItemRegistry *items)
{
    if (!menu->isOpen)
        return;

    int  id       = -1;
    bool gotClick = IpcPollMenu(&menu->proc, &id);

    // The child also flips proc.running to false when it exits without a
    // click (user closed/clicked away); mirror that into menu->isOpen.
    if (!menu->proc.running)
        menu->isOpen = false;

    if (!gotClick)
        return;

    switch (id)
    {
    case MENU_ITEM_RED:
        pup->color = RED;
        break;
    case MENU_ITEM_GREEN:
        pup->color = GREEN;
        break;
    case MENU_ITEM_BLUE:
        pup->color = BLUE;
        break;
    case MENU_ITEM_ITEM:
    {
        // Spawn the ball just to the right of the puppet's bounding box.
        int x = (int)(pup->bounds.x + pup->bounds.w + 50);
        int y = (int)(pup->bounds.y);
        SpawnItem(items, x, y);
        break;
    }
    // case ...
    default:
        break;
    }
}