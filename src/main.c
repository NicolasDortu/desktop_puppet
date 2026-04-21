#include "config.h"
#include "pet.h"
#include "physics.h"
#include "input.h"
#include "renderer.h"

#include "raylib.h"

int main(void)
{
    Vector2 startPos = {0.0f, 0.0f};
    Pet ball = CreatePet("Bally", PET_CAT, RED, 40.0f, startPos);

    int winSize = GetWinSize(&ball);

    // Transparent, borderless, always-on-top window sized to the ball
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST | FLAG_WINDOW_TRANSPARENT);
    InitWindow(winSize, winSize, "Desktop Toy");

    int screenWidth = GetMonitorWidth(GetCurrentMonitor());
    int screenHeight = GetMonitorHeight(GetCurrentMonitor());

    ball.position = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f};

    SetTargetFPS(TARGET_FPS);

    while (!WindowShouldClose())
    {
        DragPet(&ball);

        if (!ball.isDragging)
            ApplyPhysics(&ball, screenWidth, screenHeight);

        RenderGame(&ball);
    }

    CloseWindow();
    return 0;
}