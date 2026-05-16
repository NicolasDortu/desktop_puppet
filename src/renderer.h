#ifndef RENDERER_H
#define RENDERER_H

#include "puppet.h"
#include "menu.h"

// -- Declarations --

// Struct to hold screen dimensions.
typedef struct
{
    int screenWidth;
    int screenHeight;
} ScreenWidthHeight;

// -- Functions --

void InitGameWindow(int size);
ScreenWidthHeight GetScreenSize(void);
void UpdateWindow(Puppet *pup);
void DrawPuppet(Puppet *pup);
void DrawMenu(Menu *menu, MenuLayout layout);
#endif