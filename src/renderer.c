#include "renderer.h"
#include "puppet.h"
#include "menu.h"

#include "raylib.h"

// =============================================================================
//  THEME
// =============================================================================

static const Color MENU_BG_COLOR = {40, 40, 40, 230};

// =============================================================================
//  WINDOW
// =============================================================================

// Initialize the game window with the appropriate flags for a transparent, borderless, always-on-top window.
void InitGameWindow(int size)
{
    // Transparent, borderless, always-on-top window sized to the puppet
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST | FLAG_WINDOW_TRANSPARENT);
    InitWindow(size, size, "Desktop Puppet");
}

// Get the current screen size to position the puppet window and menu correctly.
ScreenWidthHeight GetScreenSize(void)
{
    return (ScreenWidthHeight){
        .screenWidth = GetMonitorWidth(GetCurrentMonitor()),
        .screenHeight = GetMonitorHeight(GetCurrentMonitor())};
}

// Update the window size and position to fit the puppet's bounds, keeping it anchored to the desktop.
void UpdateWindow(Puppet *pup)
{
    PuppetBounds b = pup->bounds;

    SetWindowSize((int)b.w, (int)b.h);
    SetWindowPosition((int)b.x, (int)b.y);
}

// =============================================================================
//  ENTITIES
// =============================================================================

// Draw the puppet limbs as circles, using the puppet's bounds to position them correctly within the window.
void DrawPuppet(Puppet *pup)
{
    PuppetBounds b = pup->bounds;
    // Window top-left in screen space (matches UpdateWindow positioning).
    float winOriginX = b.x;
    float winOriginY = b.y;

    for (int i = 0; i < LIMB_COUNT; i++)
    {
        PuppetLimb limb = pup->limbs[i];
        Vector2 local = {limb.pos.x - winOriginX, limb.pos.y - winOriginY};
        DrawCircleV(local, limb.radius, limb.color);
    }
}

// Draw the menu as a rectangle with text, positioned relative to the puppet's bounds.
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
                 RAYWHITE);
    }
}