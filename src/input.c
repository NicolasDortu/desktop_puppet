#include "input.h"
#include "puppet.h"
#include "menu.h"

#include "raylib.h"

static Vector2 dragOffset = {0, 0};
static Vector2 prevMouse = {0, 0};

// -- Mouse --
MouseState GetMouseState(Puppet *pup)
{
    WPOINT cursorPos;
    GetCursorPos(&cursorPos);
    Vector2 screenMouse = {(float)cursorPos.x, (float)cursorPos.y};

    float dx = screenMouse.x - pup->position.x;
    float dy = screenMouse.y - pup->position.y;

    return (MouseState){
        .screenMouse = screenMouse,
        .mouseOver = (dx * dx + dy * dy) <= pup->radius * pup->radius};
}

// -- Puppet --
void DragPuppet(Puppet *pup)
{
    MouseState ms = GetMouseState(pup);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && ms.mouseOver)
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