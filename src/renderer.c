#include "renderer.h"
#include "config.h"
#include "pet.h"
#include "menu.h"

#include "raylib.h"

ScreenWidthHeight SetWindow(Pet *pet)
{
    int winSize = (int)(2 * (pet->radius + PADDING));

    // Transparent, borderless, always-on-top window sized to the ball
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST | FLAG_WINDOW_TRANSPARENT);
    InitWindow(winSize, winSize, "Desktop Toy");

    return (ScreenWidthHeight){
        .screenWidth = GetMonitorWidth(GetCurrentMonitor()),
        .screenHeight = GetMonitorHeight(GetCurrentMonitor())};
}

void RenderPet(Pet *pet, Menu *menu)
{
    int petBox = (int)(2 * (pet->radius + PADDING));
    int menuW = menu->isOpen ? MENU_WIDTH : 0;
    int menuH = menu->isOpen ? (menu->itemCount * MENU_ITEM_HEIGHT + 2 * MENU_PADDING) : 0;

    // Window grows wider when the menu is open; height = max(pet box, menu height)
    int winW = petBox + menuW;
    int winH = (menuH > petBox) ? menuH : petBox;

    SetWindowSize(winW, winH);
    SetWindowPosition(
        (int)(pet->position.x - pet->radius - PADDING),
        (int)(pet->position.y - pet->radius - PADDING));

    // Pet stays at the same window-local spot — top-left corner
    DrawCircleV((Vector2){pet->radius + PADDING, pet->radius + PADDING},
                pet->radius, pet->color);
}

void RenderMenu(Menu *menu, Pet *pet)
{
    if (!menu->isOpen)
        return;

    int petBox = (int)(2 * (pet->radius + PADDING));

    // Menu panel sits to the right of the pet box, in window-local coords
    int x = petBox;
    int y = 0;
    int h = menu->itemCount * MENU_ITEM_HEIGHT + 2 * MENU_PADDING;

    // Background
    DrawRectangle(x, y, MENU_WIDTH, h, (Color){40, 40, 40, 230});
    DrawRectangleLines(x, y, MENU_WIDTH, h, LIGHTGRAY);

    // One row per item
    for (int i = 0; i < menu->itemCount; i++)
    {
        int itemY = y + MENU_PADDING + i * MENU_ITEM_HEIGHT;
        DrawRectangle(x + MENU_PADDING, itemY + 5, 16, 16, menu->items[i].color);
        DrawText(menu->items[i].action, x + MENU_PADDING + 24, itemY + 7, 18, RAYWHITE);
    }
}