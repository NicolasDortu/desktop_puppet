#ifndef RENDERER_H
#define RENDERER_H

#include "config.h"

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

#endif