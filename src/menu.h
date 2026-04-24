#ifndef MENU_H
#define MENU_H

#include "raylib.h"
#include "pet.h"

// --- Menu sizing ---
#define MENU_WIDTH 120
#define MENU_ITEM_HEIGHT 30
#define MENU_PADDING 5

// --- Menu Definition ---
enum MenuItemId
{
    MENU_ITEM_RED,
    MENU_ITEM_GREEN,
    MENU_ITEM_BLUE,
    MENU_ITEM_COUNT
};

typedef struct // id is used to know which item was clicked
{
    int id;
    const char *action;
    Color color;
} MenuItem;

typedef struct
{
    bool isOpen;
    MenuItem items[MENU_ITEM_COUNT];
    int itemCount;
} Menu;

// --- Functions ---
Menu CreateMenu();                      // Build the menu with its items
void MenuActions(Menu *menu, Pet *pet); // Set actions in the menu

#endif