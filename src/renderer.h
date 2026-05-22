#ifndef RENDERER_H
#define RENDERER_H

#include "puppet.h"

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

void              InitOverlayWindow(int width, int height); // Transparent, undecorated, topmost; shared by every role
ScreenWidthHeight GetScreenSize(void);
void              UpdateWindow(Puppet *pup);
void              DrawPuppet(Puppet *pup);

#endif