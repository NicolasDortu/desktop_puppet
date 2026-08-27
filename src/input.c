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

// Extra pixels added to a particle's radius when testing for a grab, so thin
// parts (e.g. the bat's ends) stay comfortably clickable.
#define GRAB_PADDING 6.0f

// Topmost particle under the cursor (iterate in reverse for proper z-ordering).
int GetHoveredParticle(const Body *body, Vector2 mouseScreenPos)
{
    for (int i = body->particleCount - 1; i >= 0; i--)
    {
        const Particle *p = &body->particles[i];
        if (MouseInsideCircle(mouseScreenPos, p->pos, p->radius + GRAB_PADDING))
            return i;
    }
    return -1;
}

// Frames of cursor movement averaged into the throw velocity. The single
// final-frame delta is unusable: the hand naturally decelerates right at
// release, so fast flicks often ended with a ~0 delta and died mid-air.
#define THROW_SMOOTH_FRAMES 6

// Handle left-click drag for any Entity body.
void DragBody(Body *body)
{
    static Vector2 dragOffset = {0, 0};
    static int     draggedIdx = -1;
    static Vector2 deltas[THROW_SMOOTH_FRAMES]; // recent cursor deltas -> throw velocity
    static int     deltaIdx  = 0;
    static Vector2 lastMouse = {0, 0};

    Vector2 mouse = GetScreenMousePos();

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        int hovered = GetHoveredParticle(body, mouse);
        if (hovered != -1)
        {
            Particle *p             = &body->particles[hovered];
            draggedIdx              = hovered;
            p->isDragged            = true;
            dragOffset.x            = p->pos.x - mouse.x;
            dragOffset.y            = p->pos.y - mouse.y;

            for (int i = 0; i < THROW_SMOOTH_FRAMES; i++)
                deltas[i] = (Vector2){0, 0};
            lastMouse = mouse;
        }
    }

    // End the drag on the release event, but ALSO when the button is simply
    // no longer down or the window lost focus: a popup window appearing
    // mid-drag (e.g. a coin) can eat the release event, which used to leave
    // the limb glued in mid-air with physics skipping it.
    if (draggedIdx != -1 &&
        (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) ||
         !IsMouseButtonDown(MOUSE_LEFT_BUTTON) ||
         !IsWindowFocused()))
    {
        // Throw velocity = average cursor speed over the last few frames.
        Vector2 v = {0, 0};
        for (int i = 0; i < THROW_SMOOTH_FRAMES; i++)
        {
            v.x += deltas[i].x;
            v.y += deltas[i].y;
        }
        v.x /= THROW_SMOOTH_FRAMES;
        v.y /= THROW_SMOOTH_FRAMES;

        Particle *p  = &body->particles[draggedIdx];
        p->isDragged = false;
        p->oldPos.x  = p->pos.x - v.x; // Verlet: oldPos behind pos = velocity
        p->oldPos.y  = p->pos.y - v.y;
        draggedIdx   = -1;
    }

    if (draggedIdx != -1)
    {
        deltas[deltaIdx] = (Vector2){ mouse.x - lastMouse.x, mouse.y - lastMouse.y };
        deltaIdx  = (deltaIdx + 1) % THROW_SMOOTH_FRAMES;
        lastMouse = mouse;

        Particle *p = &body->particles[draggedIdx];
        p->oldPos   = p->pos;
        p->pos.x    = mouse.x + dragOffset.x;
        p->pos.y    = mouse.y + dragOffset.y;
    }
}


