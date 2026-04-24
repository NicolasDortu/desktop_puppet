#include "config.h"
#include "pet.h"
#include "physics.h"
#include "input.h"
#include "renderer.h"
#include "menu.h"

#include "raylib.h"

int main(void)
{
    Vector2 startPos = {0.0f, 0.0f};
    Pet ball = CreatePet("Bally", PET_CAT, RED, 40.0f, startPos);
    Menu menu = CreateMenu();

    ScreenWidthHeight win = SetWindow(&ball);

    ball.position = (Vector2){win.screenWidth / 2.0f, win.screenHeight / 2.0f};

    SetTargetFPS(TARGET_FPS);

    while (!WindowShouldClose())
    {
        // TODO: mettre les args dans le bon ordre
        DragPet(&ball);
        ToggleMenu(&ball, &menu);
        MenuActions(&menu, &ball);
        UpdateWindow(&ball, &menu);
        if (!ball.isDragging)
            ApplyPhysics(&ball, win.screenWidth, win.screenHeight);

        BeginDrawing();
        ClearBackground(BLANK);
        RenderWindow(&ball, &menu);
        RenderMenu(&menu, &ball);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}