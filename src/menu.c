#include "menu.h"
#include "pet.h"
#include "input.h"

#include "raylib.h"

Menu CreateMenu()
{
    Menu menu = {
        .isOpen = false,
        .itemCount = MENU_ITEM_COUNT,
        .items = {
            {.id = MENU_ITEM_RED, .action = "RED", .color = RED},
            {.id = MENU_ITEM_GREEN, .action = "GREEN", .color = GREEN},
            {.id = MENU_ITEM_BLUE, .action = "BLUE", .color = BLUE},
        }};
    return menu;
}

void MenuActions(Menu *menu, Pet *pet)
{
    switch (GetClickedMenuItem(menu, pet))
    {
    case MENU_ITEM_RED:
        pet->color = RED;
        break;
    case MENU_ITEM_GREEN:
        pet->color = GREEN;
        break;
    case MENU_ITEM_BLUE:
        pet->color = BLUE;
        break;
    // Future items
    default:
        break;
    }
}