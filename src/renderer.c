#include "renderer.h"

#include "raylib.h"

// =============================================================================
//  WINDOW
// =============================================================================

// Initialize a desktop-overlay window: transparent, undecorated, always on top.
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

// Update the window size and position to the BoundBox + Margin.
void UpdateWindow(BoundBox b)
{
    SetWindowSize    ((int)b.w + 2 * WINDOW_MARGIN, (int)b.h + 2 * WINDOW_MARGIN);
    SetWindowPosition((int)b.x - WINDOW_MARGIN    , (int)b.y - WINDOW_MARGIN);
}

// Load a skin from the assets/ folder (sibling of bin/, resolved from the exe
// location so it works whatever the working directory is). Returns id == 0 if
// the file is missing; callers fall back to flat shapes.
Texture2D LoadAssetTexture(const char *file)
{
    Texture2D tex = LoadTexture(TextFormat("%s../assets/%s", GetApplicationDirectory(), file));
    if (tex.id)
        SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR); // smooth when scaled to limb size
    return tex;
}