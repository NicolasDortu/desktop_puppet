#include "renderer.h"

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

// Update the window size and position to the BoundBox
void UpdateWindow(BoundBox b)
{
    SetWindowSize((int)b.w, (int)b.h);
    SetWindowPosition((int)b.x, (int)b.y);
}