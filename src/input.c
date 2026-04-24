#include "input.h"
#include "config.h"
#include "pet.h"
#include "menu.h"

#include "raylib.h"

static Vector2 dragOffset = {0, 0};
static Vector2 prevMouse = {0, 0};

MouseState GetMouseState(Pet *pet)
{
    WPOINT cursorPos;
    GetCursorPos(&cursorPos);
    Vector2 screenMouse = {(float)cursorPos.x, (float)cursorPos.y};

    float dx = screenMouse.x - pet->position.x;
    float dy = screenMouse.y - pet->position.y;

    return (MouseState){
        .screenMouse = screenMouse,
        .mouseOver = (dx * dx + dy * dy) <= pet->radius * pet->radius};
}

void DragPet(Pet *pet)
{
    MouseState ms = GetMouseState(pet);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && ms.mouseOver)
    {
        pet->isDragging = true;
        dragOffset.x = pet->position.x - ms.screenMouse.x;
        dragOffset.y = pet->position.y - ms.screenMouse.y;
    }

    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        pet->isDragging = false;

    if (pet->isDragging)
    {
        pet->position.x = ms.screenMouse.x + dragOffset.x;
        pet->position.y = ms.screenMouse.y + dragOffset.y;
        pet->velocity.x = ms.screenMouse.x - prevMouse.x;
        pet->velocity.y = ms.screenMouse.y - prevMouse.y;
    }

    prevMouse = ms.screenMouse;
}

void ToggleMenu(Pet *pet, Menu *menu)
{
    MouseState ms = GetMouseState(pet);

    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
        menu->isOpen = !menu->isOpen;
}

// TODO: removed the hardcoded logic to make it more customizable
int GetClickedMenuItem(Menu *menu, Pet *pet)
{
    if (!menu->isOpen)
        return -1;
    if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        return -1;

    Vector2 m = GetMousePosition();
    int menuX = (int)(2 * pet->radius);

    for (int i = 0; i < menu->itemCount; i++)
    {
        int itemY = MENU_PADDING + i * MENU_ITEM_HEIGHT;
        if (m.x >= menuX && m.x <= menuX + MENU_WIDTH && m.y >= itemY && m.y <= itemY + MENU_ITEM_HEIGHT)
        {
            menu->isOpen = false;
            return menu->items[i].id;
        }
    }
    return -1;
}