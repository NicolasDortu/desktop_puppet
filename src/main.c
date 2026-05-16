#include "config.h"
#include "puppet.h"
#include "physics.h"
#include "input.h"
#include "renderer.h"
#include "menu.h"

#include "raylib.h"

int main(void)
{
    // =========================================================================
    //  SETUP
    // =========================================================================
    float radius = 100.0f;
    InitGameWindow((int)(2 * radius));

    ScreenWidthHeight win = GetScreenSize();

    Vector2 startPos = { win.screenWidth / 2.0f, win.screenHeight / 2.0f };
    Puppet  ball     = CreatePuppet("Bally", RED, radius, startPos);
    Menu    menu     = CreateMenu();

    SetTargetFPS(TARGET_FPS);

    // =========================================================================
    //  MAIN LOOP
    // =========================================================================
    while (!WindowShouldClose())
    {
        // -- Input --
        DragPuppet(&ball);
        ToggleMenu(&ball, &menu);
        MenuActions(&ball, &menu);

        // -- Simulation --
        ApplyPhysics(&ball, win.screenWidth, win.screenHeight);
        UpdateWindow(&ball);

        // -- Render --
        MenuLayout layout = ComputeMenuLayout(&ball, &menu);
        BeginDrawing();
            ClearBackground(BLANK);
            DrawPuppet(&ball);
            DrawMenu(&menu, layout);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}