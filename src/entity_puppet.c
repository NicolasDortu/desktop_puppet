#include "entities.h"
#include "config.h"
#include "puppet.h"
#include "physics.h"
#include "input.h"
#include "renderer.h"
#include "menu.h"
#include "item.h"

#include "raylib.h"

// =============================================================================
//  PUPPET ROLE
// =============================================================================
//
//  The "main" desktop puppet window: a verlet-physics doll that the user can
//  drag, right-click to open the menu, and collide with spawned items.
// =============================================================================

int RunPuppet(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    // -- Setup --
    float radius = 100.0f;
    InitOverlayWindow((int)(2 * radius), (int)(2 * radius));

    ScreenWidthHeight win = GetScreenSize();

    Vector2      startPos = {win.screenWidth / 2.0f, win.screenHeight / 2.0f};
    Puppet       ball     = CreatePuppet("Bally", YELLOW, radius, startPos);
    Menu         menu     = CreateMenu();
    ItemRegistry items    = CreateItemRegistry();

    SetTargetFPS(TARGET_FPS);

    // -- Main loop --
    while (!WindowShouldClose())
    {
        // Input
        DragPuppet(&ball);
        ToggleMenu(&ball, &menu);
        MenuActions(&ball, &menu, &items);

        // Simulation
        ApplyPhysics(&ball, win.screenWidth, win.screenHeight);
        UpdateItems(&items, &ball);
        UpdateWindow(&ball);

        // Render
        BeginDrawing();
            ClearBackground(BLANK);
            DrawPuppet(&ball);
        EndDrawing();
    }

    // -- Teardown --
    CloseAllItems(&items);
    CloseMenu(&menu);
    CloseWindow();
    return 0;
}
