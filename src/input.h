#ifndef INPUT_H
#define INPUT_H

#include "puppet.h"
#include "menu.h"

// -- Structs --

typedef struct // TODO: verify is the function is really necessary -> See raylib 6
{
    long x, y;
} WPOINT;
__declspec(dllimport) int __stdcall GetCursorPos(WPOINT *lpPoint);

typedef struct
{
    Vector2 screenMouse; // Position of the mouse in the full screen
    bool mouseOver;      // True if the mouse is over the puppet
} MouseState;

// -- Functions --
void DragPuppet(Puppet *pup);                    // Drag the puppet with left click
void ToggleMenu(Puppet *pup, Menu *menu);        // Right-click puppet to open/close the menu
int GetClickedMenuItem(Menu *menu, Puppet *pup); // Returns clicked item id, or -1

#endif