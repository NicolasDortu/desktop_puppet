#include "renderer.h"
#include "puppet.h"
#include "menu.h"

#include "raylib.h"

// -- Theme --
static const Color MENU_BG_COLOR = {40, 40, 40, 230};
#define MENU_BORDER_COLOR LIGHTGRAY
#define MENU_TEXT_COLOR RAYWHITE

// -- Window --
void InitPuppetWindow(Puppet *pup)
{
    int winSize = (int)(2 * pup->radius);

    // Transparent, borderless, always-on-top window sized to the puppet
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST | FLAG_WINDOW_TRANSPARENT);
    InitWindow(winSize, winSize, "Desktop Puppet");
}

ScreenWidthHeight GetScreenSize(void)
{
    return (ScreenWidthHeight){
        .screenWidth = GetMonitorWidth(GetCurrentMonitor()),
        .screenHeight = GetMonitorHeight(GetCurrentMonitor())};
}

void UpdateWindow(Puppet *pup, Menu *menu)
{
    MenuLayout layout = ComputeMenuLayout(pup, menu);

    SetWindowSize(layout.pupBox + layout.menuW, layout.pupBox + layout.overflow);
    SetWindowPosition(
        (int)(pup->position.x - pup->radius),
        (int)(pup->position.y - pup->radius) - layout.overflow);
}

// -- Entities --
void DrawPuppet(Puppet *pup, MenuLayout layout)
{
    DrawCircleV((Vector2){pup->radius, pup->radius + layout.overflow},
                pup->radius, pup->color);
}

void DrawMenu(Menu *menu, MenuLayout layout)
{
    if (!menu->isOpen)
        return;

    DrawRectangle(layout.x, layout.y, menu->width, layout.menuH, MENU_BG_COLOR); // menu background

    for (int i = 0; i < menu->itemCount; i++) // menu
    {
        Rectangle r = GetMenuItemRect(menu, layout, i);
        int iconX = (int)r.x;
        int iconY = (int)r.y + menu->padding;
        DrawRectangle(iconX, iconY, menu->iconSize, menu->iconSize, menu->items[i].color);
        DrawText(menu->items[i].action,
                 iconX + menu->iconSize + menu->padding,
                 iconY,
                 menu->iconSize,
                 MENU_TEXT_COLOR);
    }
}