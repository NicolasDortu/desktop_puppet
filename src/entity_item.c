#include "entities.h"
#include "config.h"
#include "input.h"
#include "renderer.h"
#include "ipc.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "raylib.h"

// =============================================================================
//  ITEM ROLE
// =============================================================================
//
//  Standalone draggable ball in its own transparent, undecorated, topmost
//  window. Every frame it:
//    * reads the parent's LIMBS broadcast from stdin (non-blocking),
//    * runs its own verlet physics + dragging + screen-edge bounces,
//    * resolves collisions against the received puppet limbs,
//    * sends its updated state to the parent via stdout.
//
//  Usage (set by the parent via argv): main.exe item <screenX> <screenY>
// =============================================================================

// Local physics tuning. Mirrors PhysicsConfig defaults from config.h so a
// thrown ball feels consistent with the puppet's limbs.
#define ITEM_RADIUS      30.0f
#define ITEM_GRAVITY     DEFAULT_GRAVITY
#define ITEM_FRICTION    DEFAULT_FRICTION
#define ITEM_BOUNCE      DEFAULT_BOUNCE
#define ITEM_MIN_BOUNCE  DEFAULT_MIN_BOUNCE
#define MAX_LIMBS        16

int RunItem(int argc, char **argv)
{
    float startX = (argc > 2) ? (float)atoi(argv[2]) : 300.0f;
    float startY = (argc > 3) ? (float)atoi(argv[3]) : 300.0f;

    // raylib's stdout logs would corrupt the IPC pipe; silence them entirely.
    SetTraceLogLevel(LOG_NONE);

    int winSize = (int)(2 * ITEM_RADIUS);
    InitOverlayWindow(winSize, winSize);
    SetTargetFPS(TARGET_FPS);

    IpcChildInit();

    ScreenWidthHeight screen = GetScreenSize();

    // -- Ball state (screen coords) --
    float x = startX, y = startY;
    float oldX = x, oldY = y;

    bool  dragging = false;
    float dragOffX = 0.0f, dragOffY = 0.0f;

    // -- Latest puppet snapshot received from parent --
    int   limbCount = 0;
    float lx[MAX_LIMBS], ly[MAX_LIMBS], lr[MAX_LIMBS];

    while (!WindowShouldClose())
    {
        // -- 1. Drain incoming messages; keep the most recent LIMBS line --
        char line[IPC_LINE_CAP];
        while (IpcChildReadLine(line, sizeof line))
        {
            int n, consumed;
            if (sscanf(line, "LIMBS %d%n", &n, &consumed) == 1 && n >= 0 && n <= MAX_LIMBS)
            {
                limbCount = n;
                const char *p = line + consumed;
                for (int i = 0; i < n; i++)
                {
                    float xi, yi, ri;
                    int   c2;
                    if (sscanf(p, " %f %f %f%n", &xi, &yi, &ri, &c2) == 3)
                    {
                        lx[i] = xi; ly[i] = yi; lr[i] = ri;
                        p += c2;
                    }
                    else
                    {
                        limbCount = i;
                        break;
                    }
                }
            }
        }

        // -- 2. Dragging input (uses screen-space cursor) --
        Vector2 m  = GetScreenMousePos();
        float   mx = m.x;
        float   my = m.y;

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            float dx = mx - x;
            float dy = my - y;
            if (dx * dx + dy * dy <= ITEM_RADIUS * ITEM_RADIUS)
            {
                dragging = true;
                dragOffX = x - mx;
                dragOffY = y - my;
            }
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
            dragging = false;

        // -- 3. Physics: drag-anchor or verlet integration --
        if (dragging)
        {
            // Preserve previous position so verlet retains throw velocity on release.
            oldX = x; oldY = y;
            x = mx + dragOffX;
            y = my + dragOffY;
        }
        else
        {
            float vx = (x - oldX) * ITEM_FRICTION;
            float vy = (y - oldY) * ITEM_FRICTION;
            if (vx * vx + vy * vy < ITEM_MIN_BOUNCE * ITEM_MIN_BOUNCE)
            {
                vx = 0.0f;
                vy = 0.0f;
            }
            oldX = x; oldY = y;
            x += vx;
            y += vy + ITEM_GRAVITY;

            if (y + ITEM_RADIUS > screen.screenHeight) { y = screen.screenHeight - ITEM_RADIUS; oldY = y - vy * ITEM_BOUNCE; }
            if (y - ITEM_RADIUS < 0)                   { y = ITEM_RADIUS;                       oldY = y - vy * ITEM_BOUNCE; }
            if (x + ITEM_RADIUS > screen.screenWidth)  { x = screen.screenWidth - ITEM_RADIUS;  oldX = x - vx * ITEM_BOUNCE; }
            if (x - ITEM_RADIUS < 0)                   { x = ITEM_RADIUS;                       oldX = x - vx * ITEM_BOUNCE; }
        }

        // -- 4. Collide against received puppet limbs (push self away by half overlap) --
        for (int i = 0; i < limbCount; i++)
        {
            float dx = x - lx[i];
            float dy = y - ly[i];
            float d2 = dx * dx + dy * dy;
            float minD = ITEM_RADIUS + lr[i];
            if (d2 >= minD * minD || d2 < 1e-6f)
                continue;

            float d    = sqrtf(d2);
            float push = (minD - d) * 0.5f;
            x += (dx / d) * push;
            y += (dy / d) * push;
        }

        // -- 5. Move the OS window so the ball stays centered in it --
        SetWindowPosition((int)(x - ITEM_RADIUS), (int)(y - ITEM_RADIUS));

        // -- 6. Send our state to the parent. Broken pipe => parent died. --
        char out[128];
        snprintf(out, sizeof out, "BALL %.2f %.2f %.2f %.2f %.2f\n",
                 x, y, oldX, oldY, ITEM_RADIUS);
        if (!IpcChildWriteLine(out))
            break;

        // -- 7. Render --
        BeginDrawing();
            ClearBackground(BLANK);
            DrawCircle((int)ITEM_RADIUS, (int)ITEM_RADIUS, ITEM_RADIUS, GRAY);
            DrawCircleLines((int)ITEM_RADIUS, (int)ITEM_RADIUS, ITEM_RADIUS, BLACK);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
