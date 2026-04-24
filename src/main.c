#include "config.h"
#include "puppet.h"
#include "physics.h"
#include "input.h"
#include "renderer.h"
#include "menu.h"

#include "raylib.h"

int main(void)
{
    Vector2 startPos = {0.0f, 0.0f};
    Puppet ball = CreatePuppet("Bally", PUPPET_STANDARD, RED, 40.0f, startPos);
    Menu menu = CreateMenu();

    InitPuppetWindow(&ball);
    ScreenWidthHeight win = GetScreenSize();

    ball.position = (Vector2){win.screenWidth / 2.0f, win.screenHeight / 2.0f};

    SetTargetFPS(TARGET_FPS);

    while (!WindowShouldClose())
    {
        DragPuppet(&ball);
        ToggleMenu(&ball, &menu);
        MenuActions(&ball, &menu);
        UpdateWindow(&ball, &menu);
        ApplyPhysics(&ball, win.screenWidth, win.screenHeight);
        MenuLayout layout = ComputeMenuLayout(&ball, &menu);
        BeginDrawing();
        ClearBackground(BLANK);
        DrawPuppet(&ball, layout);
        DrawMenu(&menu, layout);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}