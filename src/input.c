#include "input.h"

#include "raylib.h"

// =============================================================================
//  MOUSE
// =============================================================================

// Cursor position in screen-space coordinates.
Vector2 GetScreenMousePos(void)
{
    Vector2 mousePos = GetMousePosition();
    Vector2 winPos   = GetWindowPosition();
    return (Vector2){winPos.x + mousePos.x, winPos.y + mousePos.y};
}

// Return true if the cursor is inside the circle.
bool MouseInsideCircle(Vector2 mouseScreen, Vector2 center, float radius)
{
    float dx = mouseScreen.x - center.x;
    float dy = mouseScreen.y - center.y;

    return ((dx * dx + dy * dy) <= radius * radius);
}

// =============================================================================
//  BODY DRAGGING
// =============================================================================

// Topmost particle under the cursor (iterate in reverse for proper z-ordering).
int GetHoveredParticle(const Body *body, Vector2 mouseScreenPos)
{
    for (int i = body->particleCount - 1; i >= 0; i--)
    {
        const Particle *p = &body->particles[i];
        if (MouseInsideCircle(mouseScreenPos, p->pos, p->radius))
            return i;
    }
    return -1;
}

// Handle left-click drag for any Entity body.
void DragBody(Body *body)
{
    static Vector2 dragOffset = {0, 0};

    Vector2 mouse = GetScreenMousePos();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        int hovered = GetHoveredParticle(body, mouse);
        if (hovered != -1)
        {
            body->draggedParticle = hovered;
            Particle *p           = &body->particles[hovered];
            dragOffset.x          = p->pos.x - mouse.x;
            dragOffset.y          = p->pos.y - mouse.y;
        }
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        body->draggedParticle = -1;

    if (body->draggedParticle != -1)
    {
        Particle *p = &body->particles[body->draggedParticle];
        p->oldPos   = p->pos; // Preserve verlet throw velocity on release.
        p->pos.x    = mouse.x + dragOffset.x;
        p->pos.y    = mouse.y + dragOffset.y;
    }
}