#ifndef RENDERER_H
#define RENDERER_H

#include "pet.h"
#include "menu.h"

typedef struct
{
    int screenWidth;
    int screenHeight;
} ScreenWidthHeight;

// functions
ScreenWidthHeight SetWindow(Pet *pet);
void RenderPet(Pet *pet, Menu *menu);
void RenderMenu(Menu *menu, Pet *pet);

#endif