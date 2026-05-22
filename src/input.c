#include "input.h"
#include "puppet.h"
#include "menu.h"

#include <math.h>

#include "raylib.h"

static Vector2 dragOffset = {0, 0}; // Offset between mouse and the dragged limb's center

// =============================================================================
//  MOUSE
// =============================================================================

// Cursor position in screen-space coordinates. Uses raylib's portable
// GetMousePosition() (window-relative) plus GetWindowPosition() so the same
// code works on every backend GLFW supports.
Vector2 GetScreenMousePos(void)
{
    Vector2 mousePos = GetMousePosition();
    Vector2 winPos   = GetWindowPosition();
    return (Vector2){winPos.x + mousePos.x, winPos.y + mousePos.y};
}

// Get the mouse state relative to the puppet, return the hovered limb index or -1 if none.
MouseState GetMouseState(Puppet *pup)
{
    Vector2 mouseWinPos = GetScreenMousePos();

    // Topmost limb wins (iterate in reverse for proper z-ordering).
    for (int i = LIMB_COUNT - 1; i >= 0; i--)
    {
        PuppetLimb limb = pup->limbs[i];
        if (limb.radius <= 0.0f)
            continue;

        float dx = mouseWinPos.x - limb.pos.x;
        float dy = mouseWinPos.y - limb.pos.y;
        if ((dx * dx + dy * dy) <= limb.radius * limb.radius)
        {
            return (MouseState){
                .mouseWinPos = mouseWinPos,
                .hoveredLimb = i};
        }
    }

    return (MouseState){
        .mouseWinPos = mouseWinPos,
        .hoveredLimb = -1};
}

// =============================================================================
//  PUPPET INPUTS
// =============================================================================

// Drag the puppet limbs with left click, applying an offset to avoid snapping the limb center to the cursor.
void DragPuppet(Puppet *pup)
{
    MouseState ms = GetMouseState(pup);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && ms.hoveredLimb != -1)
    {
        pup->draggedLimb = ms.hoveredLimb;
        PuppetLimb *limb = &pup->limbs[pup->draggedLimb];
        dragOffset.x     = limb->pos.x - ms.mouseWinPos.x;
        dragOffset.y     = limb->pos.y - ms.mouseWinPos.y;
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
    {
        pup->draggedLimb = -1;
    }

    if (pup->draggedLimb != -1)
    {
        PuppetLimb *limb = &pup->limbs[pup->draggedLimb];
        // Store the previous position so verlet preserves the throw velocity on release.
        limb->oldPos = limb->pos;
        limb->pos.x  = ms.mouseWinPos.x + dragOffset.x;
        limb->pos.y  = ms.mouseWinPos.y + dragOffset.y;
    }
}

// =============================================================================
//  MENU INPUTS
// =============================================================================

// Right-click the puppet to open (or close) the menu in its own window,
void ToggleMenu(Puppet *pup, Menu *menu)
{
    if (!IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
        return;

    if (menu->isOpen)
    {
        CloseMenu(menu);
        return;
    }

    // Location of the menu, next to the puppet
    int x = (int)(pup->bounds.x + pup->bounds.w + MENU_PADDING);
    int y = (int)pup->bounds.y;
    OpenMenu(menu, x, y);
}