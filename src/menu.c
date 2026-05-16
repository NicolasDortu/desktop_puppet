#include "menu.h"
#include "puppet.h"
#include "input.h"

#include "raylib.h"

// -- Menu Implementation --

// Initializes the menu with default items and settings.
Menu CreateMenu(void)
{
    Menu menu = {
        .isOpen = false,
        .width = 120,
        .itemHeight = 30,
        .iconSize = 16,
        .padding = 8,
        .itemCount = MENU_ITEM_COUNT,
        .items = {
            {.id = MENU_ITEM_RED, .action = "RED", .color = RED},
            {.id = MENU_ITEM_GREEN, .action = "GREEN", .color = GREEN},
            {.id = MENU_ITEM_BLUE, .action = "BLUE", .color = BLUE},
        }};
    return menu;
}

// Executes the action associated with the clicked menu item.
void MenuActions(Puppet *pup, Menu *menu)
{
    switch (GetClickedMenuItem(menu, pup))
    {
    case MENU_ITEM_RED:
        pup->color = RED;
        break;
    case MENU_ITEM_GREEN:
        pup->color = GREEN;
        break;
    case MENU_ITEM_BLUE:
        pup->color = BLUE;
        break;
    // case ...
    default:
        break;
    }
}

// Computes the layout of the menu based on the puppet's position and menu settings. // TODO: The menu should get it's own window
MenuLayout ComputeMenuLayout(Puppet *pup, Menu *menu)
{
    int pupBox = (int)(2 * pup->radius);
    int menuH = menu->isOpen ? (menu->itemCount * menu->itemHeight) : 0;
    int menuW = menu->isOpen ? menu->width : 0;
    int x = pupBox + menu->padding;
    int y = pupBox - menuH;

    return (MenuLayout){
        .pupBox = pupBox,
        .menuH = menuH,
        .menuW = menuW,
        .x = x,
        .y = y,
    };
}

// Returns the rectangle area of the i-th menu item, used for click detection and rendering.
Rectangle GetMenuItemRect(Menu *menu, MenuLayout layout, int i)
{
    return (Rectangle){
        (float)layout.x,
        (float)(layout.y + i * menu->itemHeight),
        (float)menu->width,
        (float)menu->itemHeight,
    };
}