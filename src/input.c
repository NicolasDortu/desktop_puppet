#include "input.h"
#include "puppet.h"
#include "menu.h"

#include "raylib.h"

static Vector2 dragOffset = {0, 0}; // Offset between mouse and puppet center when dragging
static Vector2 prevMouse = {0, 0};

// -- Mouse --

// Get the mouse state relative to the puppet, return the hovered limb index or -1 if none.
MouseState GetMouseState(Puppet *pup)
{
    WPOINT cursorPos;
    GetCursorPos(&cursorPos);
    Vector2 screenMouse = {(float)cursorPos.x, (float)cursorPos.y};

    float dx = screenMouse.x - pup->position.x;
    float dy = screenMouse.y - pup->position.y;

    // Get the hovered limb by checking if the mouse is within any limb's circle, starting from the topmost limb for proper z-ordering.
    for (int i = LIMB_COUNT - 1; i >= 0; i--)
    {
        PuppetLimb limb = pup->limbs[i];
        float limbDx = dx - limb.position.x;
        float limbDy = dy - limb.position.y;
        if ((limbDx * limbDx + limbDy * limbDy) <= limb.radius * limb.radius)
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

void DragPuppet(Puppet *pup)
{
    MouseState ms = GetMouseState(pup);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && ms.hoveredLimb != -1)
    {
        pup->isDragging = true;
        dragOffset.x = pup->position.x - ms.screenMouse.x;
        dragOffset.y = pup->position.y - ms.screenMouse.y;
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        pup->isDragging = false;

    if (pup->isDragging)
    {
        pup->position.x = ms.screenMouse.x + dragOffset.x;
        pup->position.y = ms.screenMouse.y + dragOffset.y;
        pup->velocity.x = ms.screenMouse.x - prevMouse.x;
        pup->velocity.y = ms.screenMouse.y - prevMouse.y;
    }

    prevMouse = ms.screenMouse;
}

// -- Menu --

void ToggleMenu(Puppet *pup, Menu *menu)
{
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
        menu->isOpen = !menu->isOpen;
}

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