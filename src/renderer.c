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
    float R = pup->radius;
    float bodyR = R * 0.60f;
    float headR = R / 3.0f;
    float limbR = R / 4.0f;
    float cx = R;
    float cy = R + (float)layout.overflow;

    DrawCircleV((Vector2){cx, cy + R * 0.08f}, bodyR, YELLOW);             // body
    DrawCircleV((Vector2){cx, cy - bodyR}, headR, pup->color);             // head
    DrawCircleV((Vector2){cx - R / 1.5f, cy}, limbR, pup->color);          // left arm
    DrawCircleV((Vector2){cx + R / 1.5f, cy}, limbR, pup->color);          // right arm
    DrawCircleV((Vector2){cx - headR, cy + R / 1.33f}, limbR, pup->color); // left foot
    DrawCircleV((Vector2){cx + headR, cy + R / 1.33f}, limbR, pup->color); // right foot
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