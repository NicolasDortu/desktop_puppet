#include "input.h"
#include "puppet.h"
#include "menu.h"

#include <math.h>

#include "raylib.h"

static Vector2 dragOffset = {0, 0}; // Offset between mouse and the dragged limb's center

// -- Mouse --

// Get the mouse state relative to the puppet, return the hovered limb index or -1 if none.
MouseState GetMouseState(Puppet *pup)
{
    WPOINT cursorPos;
    GetCursorPos(&cursorPos);
    Vector2 screenMouse = {(float)cursorPos.x, (float)cursorPos.y};

    // Topmost limb wins (iterate in reverse for proper z-ordering).
    for (int i = LIMB_COUNT - 1; i >= 0; i--)
    {
        PuppetLimb limb = pup->limbs[i];
        if (limb.radius <= 0.0f)
            continue;

        float dx = screenMouse.x - limb.pos.x;
        float dy = screenMouse.y - limb.pos.y;
        if ((dx * dx + dy * dy) <= limb.radius * limb.radius)
        {
            return (MouseState){
                .screenMouse = screenMouse,
                .hoveredLimb = i};
        }
    }

    return (MouseState){
        .screenMouse = screenMouse,
        .hoveredLimb = -1};
}

// -- Puppet --

// Drag the puppet limbs with the mouse, applying an offset to avoid snapping the limb center to the cursor.
void DragPuppet(Puppet *pup)
{
    MouseState ms = GetMouseState(pup);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && ms.hoveredLimb != -1)
    {
        pup->draggedLimb = ms.hoveredLimb;
        PuppetLimb *limb = &pup->limbs[pup->draggedLimb];
        dragOffset.x = limb->pos.x - ms.screenMouse.x;
        dragOffset.y = limb->pos.y - ms.screenMouse.y;
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
        limb->pos.x = ms.screenMouse.x + dragOffset.x;
        limb->pos.y = ms.screenMouse.y + dragOffset.y;
    }
}

// -- Menu --

void ToggleMenu(Puppet *pup, Menu *menu)
{
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
        menu->isOpen = !menu->isOpen;
}

// Get the clicked menu item id, or -1 if none. Closes the menu if an item was clicked.
int GetClickedMenuItem(Menu *menu, Puppet *pup)
{
    if (!menu->isOpen)
        return -1;
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        return -1;

    Vector2 m = GetMousePosition();
    MenuLayout layout = ComputeMenuLayout(pup, menu);

    for (int i = 0; i < menu->itemCount; i++)
    {
        if (CheckCollisionPointRec(m, GetMenuItemRect(menu, layout, i)))
        {
            menu->isOpen = false;
            return menu->items[i].id;
        }
    }
    return -1;
}