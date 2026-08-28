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

// Work-area query from GLFW, which raylib links in statically. GLFW owns the
// per-OS logic (Win32 SystemParametersInfo, X11 _NET_WORKAREA), so no platform
// code is needed here. If raylib is ever built on a non-GLFW backend, these
// symbols disappear and the build fails loudly at link time.
typedef struct GLFWmonitor GLFWmonitor;
extern GLFWmonitor *glfwGetPrimaryMonitor(void);
extern void glfwGetMonitorWorkarea(GLFWmonitor *monitor, int *x, int *y, int *w, int *h);

// Usable desktop area the puppet lives in: the primary screen minus the
// taskbar (whichever edge it is docked on). Physics treats its edges as walls.
BoundBox GetScreenArea(void)
{
    int x = 0, y = 0, w = 0, h = 0;
    glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &x, &y, &w, &h);
    if (w > 0 && h > 0)
        return (BoundBox){ .x = (float)x, .y = (float)y, .w = (float)w, .h = (float)h };

    // GLFW couldn't tell (e.g. Wayland): fall back to the full monitor.
    return (BoundBox){ .x = 0, .y = 0,
                       .w = (float)GetMonitorWidth(GetCurrentMonitor()),
                       .h = (float)GetMonitorHeight(GetCurrentMonitor()) };
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

// Load a sound effect from the assets/ folder. The caller must have called
// InitAudioDevice() first. A missing file yields an empty Sound; raylib's
// Play/Stop/IsSoundPlaying no-op safely on it.
Sound LoadAssetSound(const char *file)
{
    return LoadSound(TextFormat("%s../assets/%s", GetApplicationDirectory(), file));
}