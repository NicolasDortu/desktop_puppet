#include "renderer.h"
#include "puppet.h"

#include "raylib.h"

// =============================================================================
//  WINDOW
// =============================================================================

// Initialize a desktop-overlay window: transparent, undecorated, always on top.
// Used by every role so all three windows share the same look and title.
void InitOverlayWindow(int width, int height)
{
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST | FLAG_WINDOW_TRANSPARENT);
    InitWindow(width, height, "Desktop Puppet");
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