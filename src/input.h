#ifndef INPUT_H
#define INPUT_H

#include "puppet.h"
#include "menu.h"

// -- Structs --

typedef struct // Help to get the mouse position when out of the game window and avoid conflict windows.h/raylib. // TODO: update for others OS
{
    long x, y;
} WPOINT;
__declspec(dllimport) int __stdcall GetCursorPos(WPOINT *lpPoint);

typedef struct
{
    Vector2 screenMouse; // Position of the mouse in the full screen
    int hoveredLimb;     // Index of the hovered limb, or -1 if none
} MouseState;

// -- Functions --
void DragPuppet(Puppet *pup);                    // Drag the puppet with left click
void ToggleMenu(Puppet *pup, Menu *menu);        // Right-click puppet to open/close the menu
int GetClickedMenuItem(Menu *menu, Puppet *pup); // Returns clicked item id, or -1

#endif