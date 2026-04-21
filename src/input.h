#ifndef INPUT_H
#define INPUT_H

#include "pet.h"

// Declare only what we need from Win32 to avoid conflicts with raylib
typedef struct
{
    long x, y;
} WPOINT;
__declspec(dllimport) int __stdcall GetCursorPos(WPOINT *lpPoint);

void DragPet(Pet *pet);

#endif