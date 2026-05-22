#ifndef INPUT_H
#define INPUT_H

#include "puppet.h"
#include "menu.h"

// =============================================================================
//  DECLARATIONS
// =============================================================================

typedef struct
{
    Vector2 mouseWinPos; // Position of the mouse in the window
    int     hoveredLimb; // Index of the hovered limb, or -1 if none
} MouseState;

// =============================================================================
//  FUNCTIONS
// =============================================================================

Vector2 GetScreenMousePos(void);             // Cursor in screen-space coords (shared by every role)
void    DragPuppet(Puppet *pup);
void    ToggleMenu(Puppet *pup, Menu *menu);

#endif