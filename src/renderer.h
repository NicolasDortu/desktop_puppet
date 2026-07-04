#ifndef RENDERER_H
#define RENDERER_H

#include "config.h"

#include "raylib.h"

// =============================================================================
// DECLARATIONS
// =============================================================================

// =============================================================================
//  FUNCTIONS
// =============================================================================

void      InitOverlayWindow(int width, int height);
BoundBox  GetScreenArea(void); // usable desktop area (screen minus taskbar)
void      UpdateWindow(BoundBox b);
Texture2D LoadAssetTexture(const char *file);

#endif