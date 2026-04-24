#include "renderer.h"
#include "config.h"
#include "pet.h"
#include "menu.h"

#include "raylib.h"

ScreenWidthHeight SetWindow(Pet *pet)
{
    int winSize = (int)(2 * pet->radius);

    // Transparent, borderless, always-on-top window sized to the ball
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST | FLAG_WINDOW_TRANSPARENT);
    InitWindow(winSize, winSize, "Desktop Toy");

    return (ScreenWidthHeight){
        .screenWidth = GetMonitorWidth(GetCurrentMonitor()),
        .screenHeight = GetMonitorHeight(GetCurrentMonitor())};
}

void UpdateWindow(Pet *pet, Menu *menu)
{
    int petBox = (int)(2 * pet->radius);
    int menuH = menu->isOpen ? (menu->itemCount * MENU_ITEM_HEIGHT + 2 * MENU_PADDING) : 0;
    int menuW = menu->isOpen ? (MENU_WIDTH + MENU_PADDING) : 0;
    int overflow = (petBox - menuH < 0) ? menuH - petBox : 0;

    SetWindowSize(petBox + menuW, petBox + overflow);
    SetWindowPosition(
        (int)(pet->position.x - pet->radius),
        (int)(pet->position.y - pet->radius) - overflow);
}

// TODO: renamed into RenderWindow and add _RenderMenu into this function -> Make only one function that will manage all
void RenderWindow(Pet *pet, Menu *menu)
{
    int petBox = (int)(2 * pet->radius);
    int menuH = menu->isOpen ? (menu->itemCount * MENU_ITEM_HEIGHT + 2 * MENU_PADDING) : 0;
    int overflow = (petBox - menuH < 0) ? menuH - petBox : 0;

    DrawCircleV((Vector2){pet->radius, pet->radius + overflow},
                pet->radius, pet->color);
}

void RenderMenu(Menu *menu, Pet *pet)
{
    if (!menu->isOpen)
        return;

    int petBox = (int)(2 * pet->radius);
    int h = (menu->itemCount * MENU_ITEM_HEIGHT + 2 * MENU_PADDING) - 1; // -1 otherwise the bottom line is hided by the window limit
    int overflow = (petBox - h < 0) ? h - petBox : 0;
    int x = petBox + MENU_PADDING;
    int y = petBox - h + overflow;

    DrawRectangle(x, y, MENU_WIDTH, h, (Color){40, 40, 40, 230});
    DrawRectangleLines(x, y, MENU_WIDTH, h, LIGHTGRAY);

    for (int i = 0; i < menu->itemCount; i++)
    {
        int itemY = y + MENU_PADDING + i * MENU_ITEM_HEIGHT;
        DrawRectangle(x + MENU_PADDING, itemY + 5, 16, 16, menu->items[i].color);
        DrawText(menu->items[i].action, x + MENU_PADDING + 24, itemY + 7, 18, RAYWHITE);
    }
}