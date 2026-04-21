#include "input.h"
#include "pet.h"

#include "raylib.h"

static Vector2 dragOffset = {0, 0};
static Vector2 prevMouse = {0, 0};

void DragPet(Pet *pet)
{
    // Get screen-space mouse position (avoids jitter from moving window)
    WPOINT cursorPos;
    GetCursorPos(&cursorPos);
    Vector2 screenMouse = {(float)cursorPos.x, (float)cursorPos.y};

    // Check if mouse is over the ball (screen space)
    float dx = screenMouse.x - pet->position.x;
    float dy = screenMouse.y - pet->position.y;
    bool mouseOver = (dx * dx + dy * dy) <= pet->radius * pet->radius;

    // Start dragging
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouseOver)
    {
        pet->isDragging = true;
        dragOffset.x = pet->position.x - screenMouse.x;
        dragOffset.y = pet->position.y - screenMouse.y;
    }

    // Stop dragging
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
    {
        pet->isDragging = false;
    }

    if (pet->isDragging)
    {
        pet->position.x = screenMouse.x + dragOffset.x;
        pet->position.y = screenMouse.y + dragOffset.y;
        // Track mouse velocity so the ball can be "thrown"
        pet->velocity.x = screenMouse.x - prevMouse.x;
        pet->velocity.y = screenMouse.y - prevMouse.y;
    }

    prevMouse = screenMouse;
}