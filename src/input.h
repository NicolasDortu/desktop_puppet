#ifndef INPUT_H
#define INPUT_H

#include "puppet.h"
#include "menu.h"

// =============================================================================
//  DECLARATIONS
// =============================================================================

typedef struct // Help to get the mouse pos when out of the game window and avoid conflict windows.h/raylib. // TODO: update for others OS
{
    long x, y;
} WPOINT;
__declspec(dllimport) int __stdcall GetCursorPos(WPOINT *lpPoint);

typedef struct
{
    Vector2 screenMouse; // Position of the mouse in the full screen
    int     hoveredLimb; // Index of the hovered limb, or -1 if none
} MouseState;

// =============================================================================
//  FUNCTIONS
// =============================================================================

void DragPuppet(Puppet *pup);
void ToggleMenu(Puppet *pup, Menu *menu);
int  GetClickedMenuItem(Menu *menu, Puppet *pup);

#endif