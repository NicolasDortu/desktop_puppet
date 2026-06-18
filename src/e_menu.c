#include "e_menu.h"
#include "e_item.h"
#include "config.h"
#include "renderer.h"
#include "ipc.h"

#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"

// =============================================================================
//  MENU ROLE
// =============================================================================
//
//  Small undecorated topmost window listing MENU_ITEMS. On left-click the
//  chosen item id is written to stdout (the parent reads it via a pipe) and
//  the process exits. Auto-closes if the user clicks outside it.
//
//  Usage (set by the parent via argv): main.exe menu <screenX> <screenY>
// =============================================================================

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

    char args[64];
    snprintf(args, sizeof args, "%d %d", screenX, screenY);
    if (IpcSpawnBidi(&menu->proc, "menu", args))
        menu->isOpen = true;
}

// Terminate the child window (if any) and reset state.
void CloseMenu(Menu *menu)
{
    if (!menu->isOpen)
        return;
    IpcCloseChild(&menu->proc);
    menu->isOpen = false;
}

// Poll the IPC pipe for a click result and dispatch the corresponding action.
void MenuActions(Puppet *pup, Menu *menu, ItemRegistry *items)
{
    if (!menu->isOpen)
        return;

    // The menu child prints "<id>\n" on click then exits. Read the single
    // line (if any) and mirror the child's exit into our isOpen flag.
    int  id       = -1;
    char line[32];
    bool gotClick = IpcReadLine(&menu->proc, line, sizeof line);
    if (gotClick)
        id = atoi(line);

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
        int x = (int)(pup->body.bounds.x + pup->body.bounds.w + 50);
        int y = (int)(pup->body.bounds.y);
        SpawnItem(items, ITEM_BALL, x, y);
        break;
    }
    // case ...
    default:
        break;
    }
}

static const Color MENU_BG_COLOR = {40, 40, 40, 230};

// Frames to wait before reacting to focus loss, so the window does not close
// itself before the OS has finished promoting it to foreground.
#define MENU_FOCUS_GRACE_FRAMES 10

// -- Layout math: items are laid out top-to-bottom, then in a new column.
//    columns = ceil(count / MENU_MAX_ROWS); rows = min(count, MENU_MAX_ROWS) --
static int MenuColumns(void) { return (MENU_ITEM_COUNT + MENU_MAX_ROWS - 1) / MENU_MAX_ROWS; }
static int MenuRows   (void) { return (MENU_ITEM_COUNT < MENU_MAX_ROWS) ? MENU_ITEM_COUNT : MENU_MAX_ROWS; }
static int MenuItemCol(int i) { return i / MENU_MAX_ROWS; }
static int MenuItemRow(int i) { return i % MENU_MAX_ROWS; }

int RunMenu(int argc, char **argv)
{
    int posX = (argc > 2) ? atoi(argv[2]) : 100;
    int posY = (argc > 3) ? atoi(argv[3]) : 100;

    int columns = MenuColumns();
    int rows    = MenuRows();
    int width   = MENU_WIDTH       * columns;
    int height  = MENU_ITEM_HEIGHT * rows;

    // Silence raylib's stdout logs so they don't pollute the pipe.
    SetTraceLogLevel(LOG_NONE);

    InitOverlayWindow(width, height);
    SetWindowPosition(posX, posY);
    SetTargetFPS(TARGET_FPS);

    int chosen = -1;
    int frame  = 0;

    while (!WindowShouldClose() && chosen == -1)
    {
        // -- Input: click selects an item --
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            Vector2 m = GetMousePosition();
            int col = (int)(m.x / MENU_WIDTH);
            int row = (int)(m.y / MENU_ITEM_HEIGHT);
            int i   = col * MENU_MAX_ROWS + row;
            if (col >= 0 && col < columns &&
                row >= 0 && row < MENU_MAX_ROWS &&
                i   >= 0 && i   < MENU_ITEM_COUNT)
                chosen = MENU_ITEMS[i].id;
        }

        // -- Auto-close when the user clicks outside the menu window --
        if (frame > MENU_FOCUS_GRACE_FRAMES && !IsWindowFocused())
            break;

        // -- Render: walk items in declaration order, place into (col,row) cells --
        BeginDrawing();
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
        EndDrawing();

        frame++;
    }

    CloseWindow();

    if (chosen != -1)
    {
        printf("%d\n", chosen);
        fflush(stdout);
        return 0;
    }
    return 1; // closed without a selection
}
