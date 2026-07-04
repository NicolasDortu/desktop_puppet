#ifndef RENDERER_H
#define RENDERER_H

#include "config.h"

#include "raylib.h"

// =============================================================================
// DECLARATIONS
// =============================================================================

// Struct to hold screen dimensions.
typedef struct
{
    int screenWidth;
    int screenHeight;
} ScreenWidthHeight;

// =============================================================================
//  FUNCTIONS
// =============================================================================

void              InitOverlayWindow(int width, int height);
ScreenWidthHeight GetScreenSize(void);
void              UpdateWindow(BoundBox b);
Texture2D         LoadAssetTexture(const char *file);

#endif