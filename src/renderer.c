#include "renderer.h"
#include "config.h"
#include "pet.h"

#include "raylib.h"

int GetWinSize(Pet *pet)
{
    return (int)(2 * (pet->radius + PADDING));
}

void RenderGame(Pet *pet)
{
    // Move window so the ball is always at its center
    SetWindowPosition(
        (int)(pet->position.x - pet->radius - PADDING),
        (int)(pet->position.y - pet->radius - PADDING));

    BeginDrawing();
    ClearBackground(BLANK);
    DrawCircleV((Vector2){
                    pet->radius + PADDING, pet->radius + PADDING},
                pet->radius, pet->color);
    EndDrawing();
}
