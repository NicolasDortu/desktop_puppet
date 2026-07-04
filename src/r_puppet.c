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

    BoundBox screen = GetScreenArea();

    // Shared memory: this process owns the region; children map it by our PID.
    ShmRegion     shm;
    SharedState  *shared;
    unsigned long selfPid;
    if (!SyncHostCreate(&shm, &shared, &selfPid))
    {
        CloseWindow();
        return 1;
    }

    Vector2      startPos = {screen.x + screen.w / 2.0f, screen.y + screen.h / 2.0f};
    Puppet       pup;
    CreatePuppet(&pup, radius, startPos);
    Menu         menu  = {0};
    ItemRegistry items = {0};

    SetTargetFPS(TARGET_FPS);

    unsigned int lastBlast = shared->blast.seq;

    // -- Main loop --
    while (!WindowShouldClose())
    {
        // Input
        DragBody(&pup.body);
        ToggleMenu(&pup, &menu, shared, selfPid);
        MenuActions(&pup, &menu, &items, shared, selfPid);

        // A bomb went off: kick our limbs (each process kicks its own body).
        if (shared->blast.seq != lastBlast)
        {
            lastBlast = shared->blast.seq;
            ApplyBlastToBody(&pup.body, shared->blast.pos, shared->blast.radius, shared->blast.power);
        }

        // Simulation
        ApplyPhysics(&pup.body, screen);
        EnforcePuppetPose(&pup);
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
