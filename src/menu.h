#ifndef MENU_H
#define MENU_H

#include "raylib.h"
#include "puppet.h"

// --- Menu Definition ---
enum MenuItemId
{
    MENU_ITEM_RED,
    MENU_ITEM_GREEN,
    MENU_ITEM_BLUE,
    MENU_ITEM_COUNT
};

typedef struct
{
    int id;             // id is used to know which item was clicked
    const char *action; // Action of the item
    Color color;        // Color of the small rectangle
} MenuItem;

typedef struct Menu
{
    bool isOpen;
    int width;
    int itemHeight;
    int iconSize; // size of color square + font size
    int padding;
    MenuItem items[MENU_ITEM_COUNT];
    int itemCount;
} Menu;

// Cached layout values shared by renderer/input
typedef struct
{
    int pupBox;   // 2 * pup->radius
    int menuH;    // total menu height (0 if closed)
    int menuW;    // menu width (0 if closed)
    int overflow; // extra vertical space when menu is taller than puppet
    int x;        // menu top-left x within window
    int y;        // menu top-left y within window
} MenuLayout;

// --- Functions ---
Menu CreateMenu();                                               // Build the menu with its items
void MenuActions(Puppet *pup, Menu *menu);                       // Set actions in the menu
MenuLayout ComputeMenuLayout(Puppet *pup, Menu *menu);           // Compute the layout of the menu once
Rectangle GetMenuItemRect(Menu *menu, MenuLayout layout, int i); // Get the rectangle with the pos of the menu items

#endif