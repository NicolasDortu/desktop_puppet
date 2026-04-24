#ifndef INPUT_H
#define INPUT_H

#include "pet.h"
#include "menu.h"

// Declare only what we need from Win32 to avoid conflicts with raylib
// TODO: verify is the function is really necessary -> Is there another way to get the accelaration?
typedef struct
{
    long x, y;
} WPOINT;
__declspec(dllimport) int __stdcall GetCursorPos(WPOINT *lpPoint);

typedef struct // State of the mouse
{
    Vector2 screenMouse;
    bool mouseOver;
} MouseState;

// Functions
void DragPet(Pet *pet);                       // Drag the pet with left click
void ToggleMenu(Pet *pet, Menu *menu);        // Right-click pet to open/close
int GetClickedMenuItem(Menu *menu, Pet *pet); // Returns clicked item id, or -1

#endif