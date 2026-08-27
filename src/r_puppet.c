#include "r_puppet.h"
#include "r_item.h"
#include "r_menu.h"
#include "r_coin.h"
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
    Menu         menu       = {0};
    ItemRegistry items      = {0};
    CoinPopups   coinPopups = {0};

    SetTargetFPS(TARGET_FPS);

    unsigned int lastBlast    = shared->blast.seq;
    int          coins        = 0; // shop balance; this process is the only writer
    int          hurtCooldown = 0;

    // -- Main loop --
    while (!WindowShouldClose())
    {
        // Input
        DragBody(&pup.body);
        ToggleMenu(&pup, &menu, shared, selfPid);
        MenuActions(&pup, &menu, &items, shared, selfPid, &coins);

        // A bomb went off: kick our limbs (each process kicks its own body).
        if (shared->blast.seq != lastBlast)
        {
            lastBlast = shared->blast.seq;
            ApplyBlastToBody(&pup.body, shared->blast.pos, shared->blast.radius, shared->blast.power);
        }

        // Simulation
        ApplyPhysics(&pup.body, screen);
        EnforcePuppetPose(&pup);
        float itemPush = UpdateItems(&items, shared, &pup);

        // -- Coins: getting hurt pays out (hard wall crash or a solid item hit) --
        if (hurtCooldown > 0)
            hurtCooldown--;
        if (hurtCooldown == 0 &&
            (pup.body.wallImpact > HURT_WALL_SPEED || itemPush > HURT_ITEM_PUSH))
        {
            if (coins < COINS_MAX)
                coins++;
            PuppetLimb head = pup.limbs[LIMB_HEAD];
            SpawnCoinPopup(&coinPopups, selfPid,
                           (int)head.pos.x, (int)(head.pos.y - head.radius - 20));
            hurtCooldown = HURT_COOLDOWN_FRAMES;
        }
        shared->coins = coins;

        UpdateWindow(PuppetWindowBounds(&pup));

        // Render
        BeginDrawing();
            DrawPuppet(&pup);
        EndDrawing();
    }

    // -- Teardown --
    CloseCoinPopups(&coinPopups);
    CloseAllItems(&items);
    CloseMenu(&menu);
    IpcShmClose(&shm);
    CloseWindow();
    return 0;
}
