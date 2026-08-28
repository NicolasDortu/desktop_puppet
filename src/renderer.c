#include "renderer.h"

#include "raylib.h"

// =============================================================================
//  WINDOW
// =============================================================================

// Initialize a desktop-overlay window: transparent, undecorated, always on top.
void InitOverlayWindow(int width, int height)
{
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST | FLAG_WINDOW_TRANSPARENT);
    InitWindow(width, height, "Desktop Buddy");
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

// Update the window size and position to the BoundBox + Margin. Skips the
// Win32 calls when nothing changed: a settled puppet would otherwise force
// two SetWindowPos round-trips (and DWM work) per frame at idle. One window
// per process, so process-local statics are the whole cache.
void UpdateWindow(BoundBox b)
{
    int w = (int)b.w + 2 * WINDOW_MARGIN;
    int h = (int)b.h + 2 * WINDOW_MARGIN;
    int x = (int)b.x - WINDOW_MARGIN;
    int y = (int)b.y - WINDOW_MARGIN;

    static int lastW = -1, lastH = -1, lastX = -1, lastY = -1;
    if (w != lastW || h != lastH)
    {
        SetWindowSize(w, h);
        lastW = w;
        lastH = h;
    }
    if (x != lastX || y != lastY)
    {
        SetWindowPosition(x, y);
        lastX = x;
        lastY = y;
    }
}

// Load a skin from the assets/ folder next to the exe (resolved from the exe's
// own location, so it works wherever the exe is regardless of the working
// directory). Returns id == 0 if the file is missing; callers fall back to
// flat shapes.
Texture2D LoadAssetTexture(const char *file)
{
    Texture2D tex = LoadTexture(TextFormat("%sassets/%s", GetApplicationDirectory(), file));
    if (tex.id)
        SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR); // smooth when scaled to limb size
    return tex;
}

// Load a sound effect from the assets/ folder next to the exe. The caller
// must have called InitAudioDevice() first. A missing file yields an empty
// Sound; raylib's Play/Stop/IsSoundPlaying no-op safely on it.
Sound LoadAssetSound(const char *file)
{
    return LoadSound(TextFormat("%sassets/%s", GetApplicationDirectory(), file));
}