#include "r_menu.h"
#include "r_item.h"
#include "r_sync.h"
#include "e_puppet.h"
#include "e_menu.h"
#include "ipc.h"
#include "renderer.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"

// =============================================================================
//  MENU ROLE
// =============================================================================
//
//  A small undecorated topmost popup listing MENU_ITEMS. This file owns both
//  halves of the menu role:
//
//    PARENT SIDE (runs inside the puppet process):
//      OpenMenu / CloseMenu / MenuActions / ToggleMenu.
//    CHILD SIDE (runs inside the `main.exe menu ...` process):
//      RunMenu -- writes the clicked id into SharedState then exits.
//
//  Usage (set by the parent via argv): main.exe menu <x> <y> <parentPid>
// =============================================================================

// =============================================================================
//  PARENT SIDE
// =============================================================================

// Spawn the menu child window at the given screen position.
void OpenMenu(Menu *menu, SharedState *shared, unsigned long parentPid,
              int screenX, int screenY)
{
    if (menu->isOpen)
        return;

    // Clear any stale result before the child can publish a new one.
    shared->menu = (MenuSlot){0};

    char args[64];
    snprintf(args, sizeof args, "%d %d %lu", screenX, screenY, parentPid);
    if (IpcSpawnChild(&menu->proc, "menu", args))
        menu->isOpen = true;
}

// Terminate the child window (if any) and reset state.
void CloseMenu(Menu *menu)
{
    if (!menu->isOpen)
        return;
    IpcKillChild(&menu->proc);
    menu->isOpen = false;
}

// Poll the shared MenuSlot for a click result and dispatch the chosen action.
// The shop's purchase check lives HERE, not in the menu child: the child only
// grays out rows it can't afford, the parent owns the balance.
void MenuActions(Puppet *pup, Menu *menu, ItemRegistry *reg,
                 SharedState *shared, unsigned long parentPid, int *coins)
{
    if (!menu->isOpen)
        return;

    if (!shared->menu.done)
    {
        if (!IpcChildRunning(&menu->proc))
            menu->isOpen = false;
        return;
    }

    int id = shared->menu.chosenId; // an ItemType (MenuPick returns row = type)
    CloseMenu(menu);
    shared->menu.done = false;

    if (id < 0 || id >= ITEM_TYPE_COUNT || *coins < MENU_ITEMS[id].price)
        return;

    // Spawn the bought item just to the right of the puppet's bounding box;
    // only a successful spawn costs coins.
    int x = (int)(pup->body.bounds.x + pup->body.bounds.w + 50);
    int y = (int)(pup->body.bounds.y);
    if (SpawnItem(reg, shared, parentPid, (ItemType)id, x, y))
        *coins -= MENU_ITEMS[id].price;
}

// Right-click the puppet to open (or close) the menu in its own window.
void ToggleMenu(Puppet *pup, Menu *menu, SharedState *shared, unsigned long parentPid)
{
    if (!IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
        return;

    if (menu->isOpen)
    {
        CloseMenu(menu);
        return;
    }

    // Location of the menu, next to the puppet.
    int x = (int)(pup->body.bounds.x + pup->body.bounds.w + MENU_PADDING);
    int y = (int)pup->body.bounds.y;
    OpenMenu(menu, shared, parentPid, x, y);
}

// =============================================================================
//  CHILD SIDE
// =============================================================================

// Frames to wait before reacting to focus loss, so the window does not close
// itself before the OS has finished promoting it to foreground.
#define MENU_FOCUS_GRACE_FRAMES 10

int RunMenu(int argc, char **argv)
{
    int           posX      = (argc > 2) ? atoi(argv[2]) : 100;
    int           posY      = (argc > 3) ? atoi(argv[3]) : 100;
    unsigned long parentPid = (argc > 4) ? strtoul(argv[4], NULL, 10) : 0;

    int width, height;
    MenuWindowSize(&width, &height);

    // -- Setup --
    InitOverlayWindow(width, height);
    SetWindowPosition(posX, posY);
    SetTargetFPS(TARGET_FPS);

    ShmRegion    shm;
    SharedState *shared;
    void        *parentH;
    if (!SyncChildAttach(parentPid, &shm, &shared, &parentH))
    {
        CloseWindow();
        return 1;
    }

    int chosen = -1;
    int frame  = 0;

    while (!WindowShouldClose() && IpcProcessAlive(parentH) && chosen == -1)
    {
        // -- Input: click selects an item --
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            chosen = MenuPick(GetMousePosition());

        // -- Auto-close when the user clicks outside the menu window --
        if (frame > MENU_FOCUS_GRACE_FRAMES && !IsWindowFocused())
            break;

        // -- Render --
        BeginDrawing();
            DrawMenu(shared->coins);
        EndDrawing();

        frame++;
    }

    // Publish the choice (if any) into the shared slot before exiting.
    if (chosen != -1)
    {
        shared->menu.chosenId = chosen;
        shared->menu.done     = true;
    }

    IpcShmClose(&shm);
    CloseWindow();
    return (chosen != -1) ? 0 : 1; // 1 = closed without a selection
}
