#include "r_puppet.h"
#include "r_item.h"
#include "r_menu.h"
#include "r_sync.h"
#include "e_puppet.h"
#include "ipc.h"
#include "input.h"
#include "physics.h"
#include "renderer.h"
#include "config.h"

#include "raylib.h"

// =============================================================================
//  PUPPET ROLE  (parent process)
// =============================================================================
//
//  Owns the shared-memory region (mapped by every child) and the main window,
//  and drives the menu + item children each frame. Children are spawned by the
//  menu/item host APIs (see r_menu.h / r_item.h).
// =============================================================================

int RunPuppet(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    // -- Setup --
    float radius = 100.0f;
    InitOverlayWindow((int)(2 * radius), (int)(2 * radius));

    ScreenWidthHeight win = GetScreenSize();

    // Shared memory: this process owns the region; children map it by our PID.
    ShmRegion     shm;
    SharedState  *shared;
    unsigned long selfPid;
    if (!SyncHostCreate(&shm, &shared, &selfPid))
    {
        CloseWindow();
        return 1;
    }

    Vector2      startPos = {win.screenWidth / 2.0f, win.screenHeight / 2.0f};
    Puppet       pup;
    CreatePuppet(&pup, radius, startPos);
    Menu         menu  = {0};
    ItemRegistry items = {0};

    SetTargetFPS(TARGET_FPS);

    // -- Main loop --
    while (!WindowShouldClose())
    {
        // Input
        DragBody(&pup.body);
        ToggleMenu(&pup, &menu, shared, selfPid);
        MenuActions(&pup, &menu, &items, shared, selfPid);

        // Simulation
        ApplyPhysics(&pup.body, win.screenWidth, win.screenHeight);
        UpdateItems(&items, shared, &pup);
        UpdateWindow(pup.body.bounds);

        // Render
        BeginDrawing();
            DrawPuppet(&pup);
        EndDrawing();
    }

    // -- Teardown --
    CloseAllItems(&items);
    CloseMenu(&menu);
    IpcShmClose(&shm);
    CloseWindow();
    return 0;
}
