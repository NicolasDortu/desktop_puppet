#ifndef RENDERER_H
#define RENDERER_H

#include "puppet.h"
#include "menu.h"

// -- Declarations --
typedef struct
{
    int screenWidth;
    int screenHeight;
} ScreenWidthHeight;

// -- Functions --
void InitPuppetWindow(Puppet *pup);              // Create transparent borderless window
ScreenWidthHeight GetScreenSize(void);           // Query monitor dimensions
void UpdateWindow(Puppet *pup, Menu *menu);      // Update the window in case dimensions are modified
void DrawPuppet(Puppet *pup, MenuLayout layout); // Draw the puppet
void DrawMenu(Menu *menu, MenuLayout layout);    // Draw the menu
#endif