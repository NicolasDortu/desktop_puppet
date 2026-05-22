#include "entities.h"
#include "config.h"
#include "menu.h"
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

static const Color MENU_BG_COLOR = {40, 40, 40, 230};

// Frames to wait before reacting to focus loss, so the window does not close
// itself before the OS has finished promoting it to foreground.
#define MENU_FOCUS_GRACE_FRAMES 10

int RunMenu(int argc, char **argv)
{
    int posX = (argc > 2) ? atoi(argv[2]) : 100;
    int posY = (argc > 3) ? atoi(argv[3]) : 100;

    int width  = MENU_WIDTH;
    int height = MENU_ITEM_HEIGHT * MENU_ITEM_COUNT;

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
            int     i = (int)(m.y / MENU_ITEM_HEIGHT);
            if (i >= 0 && i < MENU_ITEM_COUNT && m.x >= 0 && m.x <= width)
                chosen = MENU_ITEMS[i].id;
        }

        // -- Auto-close when the user clicks outside the menu window --
        if (frame > MENU_FOCUS_GRACE_FRAMES && !IsWindowFocused())
            break;

        // -- Render --
        BeginDrawing();
            ClearBackground(MENU_BG_COLOR);
            for (int i = 0; i < MENU_ITEM_COUNT; i++)
            {
                int y = i * MENU_ITEM_HEIGHT;
                DrawRectangle(0, y + MENU_PADDING, MENU_ICON_SIZE, MENU_ICON_SIZE, MENU_ITEMS[i].color);
                DrawText(MENU_ITEMS[i].action,
                         MENU_ICON_SIZE + MENU_PADDING,
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
